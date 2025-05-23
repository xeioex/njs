
/*
 * Copyright (C) Dmitry Volyntsev
 * Copyright (C) hongzhidao
 * Copyright (C) Antoine Bonavita
 * Copyright (C) NGINX, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_event_connect.h>
#include "ngx_js.h"
#include "ngx_js_http.h"


static void ngx_js_http_resolve_handler(ngx_resolver_ctx_t *ctx);
static void ngx_js_http_next(ngx_js_http_t *http);
static void ngx_js_http_write_handler(ngx_event_t *wev);
static void ngx_js_http_read_handler(ngx_event_t *rev);
static void ngx_js_http_dummy_handler(ngx_event_t *ev);

static ngx_int_t ngx_js_http_process_status_line(ngx_js_http_t *http);
static ngx_int_t ngx_js_http_process_headers(ngx_js_http_t *http);
static ngx_int_t ngx_js_http_process_body(ngx_js_http_t *http);
static ngx_int_t ngx_js_http_parse_status_line(ngx_js_http_parse_t *hp,
    ngx_buf_t *b);
static ngx_int_t ngx_js_http_parse_header_line(ngx_js_http_parse_t *hp,
    ngx_buf_t *b);
static ngx_int_t ngx_js_http_parse_chunked(ngx_js_http_chunk_parse_t *hcp,
    ngx_buf_t *b, njs_chb_t *chain);

#if (NGX_SSL)
static void ngx_js_http_ssl_init_connection(ngx_js_http_t *http);
static void ngx_js_http_ssl_handshake_handler(ngx_connection_t *c);
static void ngx_js_http_ssl_handshake(ngx_js_http_t *http);
static ngx_int_t ngx_js_http_ssl_name(ngx_js_http_t *http);
#endif


static void
ngx_js_http_error(ngx_js_http_t *http, const char *fmt, ...)
{
    u_char   *p, *end;
    va_list   args;
    u_char    err[NGX_MAX_ERROR_STR];

    end = err + NGX_MAX_ERROR_STR - 1;

    va_start(args, fmt);
    p = njs_vsprintf(err, end, fmt, args);
    *p = '\0';
    va_end(args);

    http->error_handler(http, (const char *) err);
}


ngx_resolver_ctx_t *
ngx_js_http_resolve(ngx_js_http_t *http, ngx_resolver_t *r, ngx_str_t *host,
    in_port_t port, ngx_msec_t timeout)
{
    ngx_int_t            ret;
    ngx_resolver_ctx_t  *ctx;

    ctx = ngx_resolve_start(r, NULL);
    if (ctx == NULL) {
        return NULL;
    }

    if (ctx == NGX_NO_RESOLVER) {
        return ctx;
    }

    http->ctx = ctx;
    http->port = port;

    ctx->name = *host;
    ctx->handler = ngx_js_http_resolve_handler;
    ctx->data = http;
    ctx->timeout = timeout;

    ret = ngx_resolve_name(ctx);
    if (ret != NGX_OK) {
        http->ctx = NULL;
        return NULL;
    }

    return ctx;
}


static void
ngx_js_http_resolve_handler(ngx_resolver_ctx_t *ctx)
{
    u_char           *p;
    size_t            len;
    socklen_t         socklen;
    ngx_uint_t        i;
    ngx_js_http_t    *http;
    struct sockaddr  *sockaddr;

    http = ctx->data;

    if (ctx->state) {
        ngx_js_http_error(http, "\"%V\" could not be resolved (%i: %s)",
                          &ctx->name, ctx->state,
                          ngx_resolver_strerror(ctx->state));
        return;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, http->log, 0,
                   "http resolved: \"%V\"", &ctx->name);

#if (NGX_DEBUG)
    {
    u_char      text[NGX_SOCKADDR_STRLEN];
    ngx_str_t   addr;
    ngx_uint_t  i;

    addr.data = text;

    for (i = 0; i < ctx->naddrs; i++) {
        addr.len = ngx_sock_ntop(ctx->addrs[i].sockaddr, ctx->addrs[i].socklen,
                                 text, NGX_SOCKADDR_STRLEN, 0);

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, http->log, 0,
                       "name was resolved to \"%V\"", &addr);
    }
    }
#endif

    http->naddrs = ctx->naddrs;
    http->addrs = ngx_pcalloc(http->pool, http->naddrs * sizeof(ngx_addr_t));

    if (http->addrs == NULL) {
        goto failed;
    }

    for (i = 0; i < ctx->naddrs; i++) {
        socklen = ctx->addrs[i].socklen;

        sockaddr = ngx_palloc(http->pool, socklen);
        if (sockaddr == NULL) {
            goto failed;
        }

        ngx_memcpy(sockaddr, ctx->addrs[i].sockaddr, socklen);
        ngx_inet_set_port(sockaddr, http->port);

        http->addrs[i].sockaddr = sockaddr;
        http->addrs[i].socklen = socklen;

        p = ngx_pnalloc(http->pool, NGX_SOCKADDR_STRLEN);
        if (p == NULL) {
            goto failed;
        }

        len = ngx_sock_ntop(sockaddr, socklen, p, NGX_SOCKADDR_STRLEN, 1);
        http->addrs[i].name.len = len;
        http->addrs[i].name.data = p;
    }

    ngx_js_http_resolve_done(http);

    ngx_js_http_connect(http);

    return;

failed:

    ngx_js_http_error(http, "memory error");
}


static void
ngx_js_http_close_connection(ngx_connection_t *c)
{
    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "js http close connection: %d", c->fd);

#if (NGX_SSL)
    if (c->ssl) {
        c->ssl->no_wait_shutdown = 1;

        if (ngx_ssl_shutdown(c) == NGX_AGAIN) {
            c->ssl->handler = ngx_js_http_close_connection;
            return;
        }
    }
#endif

    c->destroyed = 1;

    ngx_close_connection(c);
}


void
ngx_js_http_resolve_done(ngx_js_http_t *http)
{
    if (http->ctx != NULL) {
        ngx_resolve_name_done(http->ctx);
        http->ctx = NULL;
    }
}


void
ngx_js_http_close_peer(ngx_js_http_t *http)
{
    if (http->peer.connection != NULL) {
        ngx_js_http_close_connection(http->peer.connection);
        http->peer.connection = NULL;
    }
}


void
ngx_js_http_connect(ngx_js_http_t *http)
{
    ngx_int_t    rc;
    ngx_addr_t  *addr;

    addr = &http->addrs[http->naddr];

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, http->log, 0,
                   "js http connect %ui/%ui", http->naddr, http->naddrs);

    http->peer.sockaddr = addr->sockaddr;
    http->peer.socklen = addr->socklen;
    http->peer.name = &addr->name;
    http->peer.get = ngx_event_get_peer;
    http->peer.log = http->log;
    http->peer.log_error = NGX_ERROR_ERR;

    rc = ngx_event_connect_peer(&http->peer);

    if (rc == NGX_ERROR) {
        ngx_js_http_error(http, "connect failed");
        return;
    }

    if (rc == NGX_BUSY || rc == NGX_DECLINED) {
        ngx_js_http_next(http);
        return;
    }

    http->peer.connection->data = http;
    http->peer.connection->pool = http->pool;

    http->peer.connection->write->handler = ngx_js_http_write_handler;
    http->peer.connection->read->handler = ngx_js_http_read_handler;

    http->process = ngx_js_http_process_status_line;

    ngx_add_timer(http->peer.connection->read, http->timeout);
    ngx_add_timer(http->peer.connection->write, http->timeout);

#if (NGX_SSL)
    if (http->ssl != NULL && http->peer.connection->ssl == NULL) {
        ngx_js_http_ssl_init_connection(http);
        return;
    }
#endif

    if (rc == NGX_OK) {
        ngx_js_http_write_handler(http->peer.connection->write);
    }
}


#if (NGX_SSL)

static void
ngx_js_http_ssl_init_connection(ngx_js_http_t *http)
{
    ngx_int_t          rc;
    ngx_connection_t  *c;

    c = http->peer.connection;

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, http->log, 0,
                   "js http secure connect %ui/%ui", http->naddr,
                   http->naddrs);

    if (ngx_ssl_create_connection(http->ssl, c, NGX_SSL_BUFFER|NGX_SSL_CLIENT)
        != NGX_OK)
    {
        ngx_js_http_error(http, "failed to create ssl connection");
        return;
    }

    c->sendfile = 0;

    if (ngx_js_http_ssl_name(http) != NGX_OK) {
        ngx_js_http_error(http, "failed to create ssl connection");
        return;
    }

    c->log->action = "SSL handshaking to http target";

    rc = ngx_ssl_handshake(c);

    if (rc == NGX_AGAIN) {
        c->data = http;
        c->ssl->handler = ngx_js_http_ssl_handshake_handler;
        return;
    }

    ngx_js_http_ssl_handshake(http);
}


static void
ngx_js_http_ssl_handshake_handler(ngx_connection_t *c)
{
    ngx_js_http_t  *http;

    http = c->data;

    http->peer.connection->write->handler = ngx_js_http_write_handler;
    http->peer.connection->read->handler = ngx_js_http_read_handler;

    ngx_js_http_ssl_handshake(http);
}


static void
ngx_js_http_ssl_handshake(ngx_js_http_t *http)
{
    long               rc;
    ngx_connection_t  *c;

    c = http->peer.connection;

    if (c->ssl->handshaked) {
        if (http->ssl_verify) {
            rc = SSL_get_verify_result(c->ssl->connection);

            if (rc != X509_V_OK) {
                ngx_log_error(NGX_LOG_ERR, c->log, 0,
                              "js http SSL certificate verify error: (%l:%s)",
                              rc, X509_verify_cert_error_string(rc));
                goto failed;
            }

            if (ngx_ssl_check_host(c, &http->tls_name) != NGX_OK) {
                ngx_log_error(NGX_LOG_ERR, c->log, 0,
                              "js http SSL certificate does not match \"%V\"",
                              &http->tls_name);
                goto failed;
            }
        }

        c->write->handler = ngx_js_http_write_handler;
        c->read->handler = ngx_js_http_read_handler;

        if (c->read->ready) {
            ngx_post_event(c->read, &ngx_posted_events);
        }

        http->process = ngx_js_http_process_status_line;
        ngx_js_http_write_handler(c->write);

        return;
    }

failed:

    ngx_js_http_next(http);
}


static ngx_int_t
ngx_js_http_ssl_name(ngx_js_http_t *http)
{
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
    u_char  *p;

    /* as per RFC 6066, literal IPv4 and IPv6 addresses are not permitted */
    ngx_str_t  *name = &http->tls_name;

    if (name->len == 0 || *name->data == '[') {
        goto done;
    }

    if (ngx_inet_addr(name->data, name->len) != INADDR_NONE) {
        goto done;
    }

    /*
     * SSL_set_tlsext_host_name() needs a null-terminated string,
     * hence we explicitly null-terminate name here
     */

    p = ngx_pnalloc(http->pool, name->len + 1);
    if (p == NULL) {
        return NGX_ERROR;
    }

    (void) ngx_cpystrn(p, name->data, name->len + 1);

    name->data = p;

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, http->log, 0,
                   "js http SSL server name: \"%s\"", name->data);

    if (SSL_set_tlsext_host_name(http->peer.connection->ssl->connection,
                                 (char *) name->data)
        == 0)
    {
        ngx_ssl_error(NGX_LOG_ERR, http->log, 0,
                      "SSL_set_tlsext_host_name(\"%s\") failed", name->data);
        return NGX_ERROR;
    }

#endif
done:

    return NGX_OK;
}

#endif


static void
ngx_js_http_next(ngx_js_http_t *http)
{
    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, http->log, 0, "js http next addr");

    if (++http->naddr >= http->naddrs) {
        ngx_js_http_error(http, "connect failed");
        return;
    }

    if (http->peer.connection != NULL) {
        ngx_js_http_close_connection(http->peer.connection);
        http->peer.connection = NULL;
    }

    http->buffer = NULL;

    ngx_js_http_connect(http);
}


static void
ngx_js_http_write_handler(ngx_event_t *wev)
{
    ssize_t            n, size;
    ngx_buf_t         *b;
    ngx_js_http_t     *http;
    ngx_connection_t  *c;

    c = wev->data;
    http = c->data;

    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, wev->log, 0, "js http write handler");

    if (wev->timedout) {
        ngx_js_http_error(http, "write timed out");
        return;
    }

#if (NGX_SSL)
    if (http->ssl != NULL && http->peer.connection->ssl == NULL) {
        ngx_js_http_ssl_init_connection(http);
        return;
    }
#endif

    b = http->buffer;

    if (b == NULL) {
        size = njs_chb_size(&http->chain);
        if (size < 0) {
            ngx_js_http_error(http, "memory error");
            return;
        }

        b = ngx_create_temp_buf(http->pool, size);
        if (b == NULL) {
            ngx_js_http_error(http, "memory error");
            return;
        }

        njs_chb_join_to(&http->chain, b->last);
        b->last += size;

        http->buffer = b;
    }

    size = b->last - b->pos;

    n = c->send(c, b->pos, size);

    if (n == NGX_ERROR) {
        ngx_js_http_next(http);
        return;
    }

    if (n > 0) {
        b->pos += n;

        if (n == size) {
            wev->handler = ngx_js_http_dummy_handler;

            http->buffer = NULL;

            if (wev->timer_set) {
                ngx_del_timer(wev);
            }

            if (ngx_handle_write_event(wev, 0) != NGX_OK) {
                ngx_js_http_error(http, "write failed");
            }

            return;
        }
    }

    if (!wev->timer_set) {
        ngx_add_timer(wev, http->timeout);
    }
}


static void
ngx_js_http_read_handler(ngx_event_t *rev)
{
    ssize_t            n, size;
    ngx_int_t          rc;
    ngx_buf_t         *b;
    ngx_js_http_t     *http;
    ngx_connection_t  *c;

    c = rev->data;
    http = c->data;

    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, rev->log, 0, "js http read handler");

    if (rev->timedout) {
        ngx_js_http_error(http, "read timed out");
        return;
    }

    if (http->buffer == NULL) {
        b = ngx_create_temp_buf(http->pool, http->buffer_size);
        if (b == NULL) {
            ngx_js_http_error(http, "memory error");
            return;
        }

        http->buffer = b;
    }

    for ( ;; ) {
        b = http->buffer;
        size = b->end - b->last;

        n = c->recv(c, b->last, size);

        if (n > 0) {
            b->last += n;

            rc = http->process(http);

            if (rc == NGX_ERROR) {
                return;
            }

            continue;
        }

        if (n == NGX_AGAIN) {
            if (ngx_handle_read_event(rev, 0) != NGX_OK) {
                ngx_js_http_error(http, "read failed");
            }

            return;
        }

        if (n == NGX_ERROR) {
            ngx_js_http_next(http);
            return;
        }

        break;
    }

    http->done = 1;

    rc = http->process(http);

    if (rc == NGX_DONE) {
        /* handler was called */
        return;
    }

    if (rc == NGX_AGAIN) {
        ngx_js_http_error(http, "prematurely closed connection");
    }
}


static void
ngx_js_http_dummy_handler(ngx_event_t *ev)
{
    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ev->log, 0, "js http dummy handler");
}


static ngx_int_t
ngx_js_http_process_status_line(ngx_js_http_t *http)
{
    ngx_int_t             rc;
    ngx_js_http_parse_t  *hp;

    hp = &http->http_parse;

    rc = ngx_js_http_parse_status_line(hp, http->buffer);

    if (rc == NGX_OK) {
        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, http->log, 0, "js http status %ui",
                       hp->code);

        http->response.code = hp->code;
        http->response.status_text.data = hp->status_text;
        http->response.status_text.len = hp->status_text_end - hp->status_text;
        http->process = ngx_js_http_process_headers;

        return http->process(http);
    }

    if (rc == NGX_AGAIN) {
        return NGX_AGAIN;
    }

    /* rc == NGX_ERROR */

    ngx_js_http_error(http, "invalid http status line");

    return NGX_ERROR;
}


static ngx_int_t
ngx_js_http_process_headers(ngx_js_http_t *http)
{
    size_t                len, vlen;
    ngx_int_t             rc;
    ngx_js_http_parse_t  *hp;

    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, http->log, 0,
                   "js http process headers");

    hp = &http->http_parse;

    if (http->response.headers.header_list.size == 0) {
        rc = ngx_list_init(&http->response.headers.header_list, http->pool, 4,
                           sizeof(ngx_js_tb_elt_t));
        if (rc != NGX_OK) {
            ngx_js_http_error(http, "alloc failed");
            return NGX_ERROR;
        }
    }

    for ( ;; ) {
        rc = ngx_js_http_parse_header_line(hp, http->buffer);

        if (rc == NGX_OK) {
            len = hp->header_name_end - hp->header_name_start;
            vlen = hp->header_end - hp->header_start;

            rc = http->append_headers(http, &http->response.headers,
                                       hp->header_name_start, len,
                                       hp->header_start, vlen);

            if (rc == NGX_ERROR) {
                ngx_js_http_error(http, "cannot add respose header");
                return NGX_ERROR;
            }

            ngx_log_debug4(NGX_LOG_DEBUG_EVENT, http->log, 0,
                           "js http header \"%*s: %*s\"",
                           len, hp->header_name_start, vlen, hp->header_start);

            if (len == (sizeof("Transfer-Encoding") -1)
                && vlen == (sizeof("chunked") - 1)
                && ngx_strncasecmp(hp->header_name_start,
                                   (u_char *) "Transfer-Encoding", len) == 0
                && ngx_strncasecmp(hp->header_start, (u_char *) "chunked",
                                   vlen) == 0)
            {
                hp->chunked = 1;
            }

            if (len == (sizeof("Content-Length") - 1)
                && ngx_strncasecmp(hp->header_name_start,
                                   (u_char *) "Content-Length", len) == 0)
            {
                hp->content_length_n = ngx_atoof(hp->header_start, vlen);
                if (hp->content_length_n == NGX_ERROR) {
                    ngx_js_http_error(http, "invalid http content length");
                    return NGX_ERROR;
                }

                if (!http->header_only
                    && hp->content_length_n
                       > (off_t) http->max_response_body_size)
                {
                    ngx_js_http_error(http,
                                      "http content length is too large");
                    return NGX_ERROR;
                }
            }

            continue;
        }

        if (rc == NGX_DONE) {
            http->response.headers.guard = GUARD_IMMUTABLE;
            break;
        }

        if (rc == NGX_AGAIN) {
            return NGX_AGAIN;
        }

        /* rc == NGX_ERROR */

        ngx_js_http_error(http, "invalid http header");

        return NGX_ERROR;
    }

    njs_chb_destroy(&http->chain);

    http->process = ngx_js_http_process_body;

    return http->process(http);
}


static ngx_int_t
ngx_js_http_process_body(ngx_js_http_t *http)
{
    ssize_t     size, chsize, need;
    ngx_int_t   rc;
    ngx_buf_t  *b;

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, http->log, 0,
                   "js http process body done:%ui", (ngx_uint_t) http->done);

    if (http->done) {
        size = njs_chb_size(&http->response.chain);
        if (size < 0) {
            ngx_js_http_error(http, "memory error");
            return NGX_ERROR;
        }

        if (!http->header_only
            && http->http_parse.chunked
            && http->http_parse.content_length_n == -1)
        {
            ngx_js_http_error(http, "invalid http chunked response");
            return NGX_ERROR;
        }

        if (http->header_only
            || http->http_parse.content_length_n == -1
            || size == http->http_parse.content_length_n)
        {
            http->ready_handler(http);
            return NGX_DONE;
        }

        if (size < http->http_parse.content_length_n) {
            return NGX_AGAIN;
        }

        ngx_js_http_error(http, "http trailing data");
        return NGX_ERROR;
    }

    b = http->buffer;

    if (http->http_parse.chunked) {
        rc = ngx_js_http_parse_chunked(&http->http_chunk_parse, b,
                                       &http->response.chain);
        if (rc == NGX_ERROR) {
            ngx_js_http_error(http, "invalid http chunked response");
            return NGX_ERROR;
        }

        size = njs_chb_size(&http->response.chain);

        if (rc == NGX_OK) {
            http->http_parse.content_length_n = size;
        }

        if (size > http->max_response_body_size * 10) {
            ngx_js_http_error(http, "very large http chunked response");
            return NGX_ERROR;
        }

        b->pos = http->http_chunk_parse.pos;

    } else {
        size = njs_chb_size(&http->response.chain);

        if (http->header_only) {
            need = 0;

        } else  if (http->http_parse.content_length_n == -1) {
            need = http->max_response_body_size - size;

        } else {
            need = http->http_parse.content_length_n - size;
        }

        chsize = ngx_min(need, b->last - b->pos);

        if (size + chsize > http->max_response_body_size) {
            ngx_js_http_error(http, "http response body is too large");
            return NGX_ERROR;
        }

        if (chsize > 0) {
            njs_chb_append(&http->response.chain, b->pos, chsize);
            b->pos += chsize;
        }

        rc = (need > chsize) ? NGX_AGAIN : NGX_DONE;
    }

    if (b->pos == b->end) {
        if (http->chunk == NULL) {
            b = ngx_create_temp_buf(http->pool, http->buffer_size);
            if (b == NULL) {
                ngx_js_http_error(http, "memory error");
                return NGX_ERROR;
            }

            http->buffer = b;
            http->chunk = b;

        } else {
            b->last = b->start;
            b->pos = b->start;
        }
    }

    return rc;
}


static ngx_int_t
ngx_js_http_parse_status_line(ngx_js_http_parse_t *hp, ngx_buf_t *b)
{
    u_char   ch;
    u_char  *p;
    enum {
        sw_start = 0,
        sw_H,
        sw_HT,
        sw_HTT,
        sw_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_status,
        sw_space_after_status,
        sw_status_text,
        sw_almost_done
    } state;

    state = hp->state;

    for (p = b->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {

        /* "HTTP/" */
        case sw_start:
            switch (ch) {
            case 'H':
                state = sw_H;
                break;
            default:
                return NGX_ERROR;
            }
            break;

        case sw_H:
            switch (ch) {
            case 'T':
                state = sw_HT;
                break;
            default:
                return NGX_ERROR;
            }
            break;

        case sw_HT:
            switch (ch) {
            case 'T':
                state = sw_HTT;
                break;
            default:
                return NGX_ERROR;
            }
            break;

        case sw_HTT:
            switch (ch) {
            case 'P':
                state = sw_HTTP;
                break;
            default:
                return NGX_ERROR;
            }
            break;

        case sw_HTTP:
            switch (ch) {
            case '/':
                state = sw_first_major_digit;
                break;
            default:
                return NGX_ERROR;
            }
            break;

        /* the first digit of major HTTP version */
        case sw_first_major_digit:
            if (ch < '1' || ch > '9') {
                return NGX_ERROR;
            }

            state = sw_major_digit;
            break;

        /* the major HTTP version or dot */
        case sw_major_digit:
            if (ch == '.') {
                state = sw_first_minor_digit;
                break;
            }

            if (ch < '0' || ch > '9') {
                return NGX_ERROR;
            }

            break;

        /* the first digit of minor HTTP version */
        case sw_first_minor_digit:
            if (ch < '0' || ch > '9') {
                return NGX_ERROR;
            }

            state = sw_minor_digit;
            break;

        /* the minor HTTP version or the end of the request line */
        case sw_minor_digit:
            if (ch == ' ') {
                state = sw_status;
                break;
            }

            if (ch < '0' || ch > '9') {
                return NGX_ERROR;
            }

            break;

        /* HTTP status code */
        case sw_status:
            if (ch == ' ') {
                break;
            }

            if (ch < '0' || ch > '9') {
                return NGX_ERROR;
            }

            hp->code = hp->code * 10 + (ch - '0');

            if (++hp->count == 3) {
                state = sw_space_after_status;
            }

            break;

        /* space or end of line */
        case sw_space_after_status:
            switch (ch) {
            case ' ':
                state = sw_status_text;
                break;
            case '.':                    /* IIS may send 403.1, 403.2, etc */
                state = sw_status_text;
                break;
            case CR:
                break;
            case LF:
                goto done;
            default:
                return NGX_ERROR;
            }
            break;

        /* any text until end of line */
        case sw_status_text:
            switch (ch) {
            case CR:
                hp->status_text_end = p;
                state = sw_almost_done;
                break;
            case LF:
                hp->status_text_end = p;
                goto done;
            }

            if (hp->status_text == NULL) {
                hp->status_text = p;
            }

            break;

        /* end of status line */
        case sw_almost_done:
            switch (ch) {
            case LF:
                goto done;
            default:
                return NGX_ERROR;
            }
        }
    }

    b->pos = p;
    hp->state = state;

    return NGX_AGAIN;

done:

    b->pos = p + 1;
    hp->state = sw_start;

    return NGX_OK;
}


static ngx_int_t
ngx_js_http_parse_header_line(ngx_js_http_parse_t *hp, ngx_buf_t *b)
{
    u_char  c, ch, *p;
    enum {
        sw_start = 0,
        sw_name,
        sw_space_before_value,
        sw_value,
        sw_space_after_value,
        sw_almost_done,
        sw_header_almost_done
    } state;

    state = hp->state;

    for (p = b->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {

        /* first char */
        case sw_start:

            switch (ch) {
            case CR:
                hp->header_end = p;
                state = sw_header_almost_done;
                break;
            case LF:
                hp->header_end = p;
                goto header_done;
            default:
                state = sw_name;
                hp->header_name_start = p;

                c = (u_char) (ch | 0x20);
                if (c >= 'a' && c <= 'z') {
                    break;
                }

                if (ch >= '0' && ch <= '9') {
                    break;
                }

                return NGX_ERROR;
            }
            break;

        /* header name */
        case sw_name:
            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'z') {
                break;
            }

            if (ch == ':') {
                hp->header_name_end = p;
                state = sw_space_before_value;
                break;
            }

            if (ch == '-' || ch == '_') {
                break;
            }

            if (ch >= '0' && ch <= '9') {
                break;
            }

            if (ch == CR) {
                hp->header_name_end = p;
                hp->header_start = p;
                hp->header_end = p;
                state = sw_almost_done;
                break;
            }

            if (ch == LF) {
                hp->header_name_end = p;
                hp->header_start = p;
                hp->header_end = p;
                goto done;
            }

            return NGX_ERROR;

        /* space* before header value */
        case sw_space_before_value:
            switch (ch) {
            case ' ':
                break;
            case CR:
                hp->header_start = p;
                hp->header_end = p;
                state = sw_almost_done;
                break;
            case LF:
                hp->header_start = p;
                hp->header_end = p;
                goto done;
            default:
                hp->header_start = p;
                state = sw_value;
                break;
            }
            break;

        /* header value */
        case sw_value:
            switch (ch) {
            case ' ':
                hp->header_end = p;
                state = sw_space_after_value;
                break;
            case CR:
                hp->header_end = p;
                state = sw_almost_done;
                break;
            case LF:
                hp->header_end = p;
                goto done;
            }
            break;

        /* space* before end of header line */
        case sw_space_after_value:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                state = sw_value;
                break;
            }
            break;

        /* end of header line */
        case sw_almost_done:
            switch (ch) {
            case LF:
                goto done;
            default:
                return NGX_ERROR;
            }

        /* end of header */
        case sw_header_almost_done:
            switch (ch) {
            case LF:
                goto header_done;
            default:
                return NGX_ERROR;
            }
        }
    }

    b->pos = p;
    hp->state = state;

    return NGX_AGAIN;

done:

    b->pos = p + 1;
    hp->state = sw_start;

    return NGX_OK;

header_done:

    b->pos = p + 1;
    hp->state = sw_start;

    return NGX_DONE;
}


#define                                                                       \
ngx_size_is_sufficient(cs)                                                    \
    (cs < ((__typeof__(cs)) 1 << (sizeof(cs) * 8 - 4)))


#define NGX_JS_HTTP_CHUNK_MIDDLE     0
#define NGX_JS_HTTP_CHUNK_ON_BORDER  1
#define NGX_JS_HTTP_CHUNK_END        2


static ngx_int_t
ngx_js_http_chunk_buffer(ngx_js_http_chunk_parse_t *hcp, ngx_buf_t *b,
    njs_chb_t *chain)
{
    size_t  size;

    size = b->last - hcp->pos;

    if (hcp->chunk_size < size) {
        njs_chb_append(chain, hcp->pos, hcp->chunk_size);
        hcp->pos += hcp->chunk_size;

        return NGX_JS_HTTP_CHUNK_END;
    }

    njs_chb_append(chain, hcp->pos, size);
    hcp->pos += size;

    hcp->chunk_size -= size;

    if (hcp->chunk_size == 0) {
        return NGX_JS_HTTP_CHUNK_ON_BORDER;
    }

    return NGX_JS_HTTP_CHUNK_MIDDLE;
}


static ngx_int_t
ngx_js_http_parse_chunked(ngx_js_http_chunk_parse_t *hcp,
    ngx_buf_t *b, njs_chb_t *chain)
{
    u_char     c, ch;
    ngx_int_t  rc;

    enum {
        sw_start = 0,
        sw_chunk_size,
        sw_chunk_size_linefeed,
        sw_chunk_end_newline,
        sw_chunk_end_linefeed,
        sw_chunk,
    } state;

    state = hcp->state;

    hcp->pos = b->pos;

    while (hcp->pos < b->last) {
        /*
         * The sw_chunk state is tested outside the switch
         * to preserve hcp->pos and to not touch memory.
         */
        if (state == sw_chunk) {
            rc = ngx_js_http_chunk_buffer(hcp, b, chain);
            if (rc == NGX_ERROR) {
                return rc;
            }

            if (rc == NGX_JS_HTTP_CHUNK_MIDDLE) {
                break;
            }

            state = sw_chunk_end_newline;

            if (rc == NGX_JS_HTTP_CHUNK_ON_BORDER) {
                break;
            }

            /* rc == NGX_JS_HTTP_CHUNK_END */
        }

        ch = *hcp->pos++;

        switch (state) {

        case sw_start:
            state = sw_chunk_size;

            c = ch - '0';

            if (c <= 9) {
                hcp->chunk_size = c;
                continue;
            }

            c = (ch | 0x20) - 'a';

            if (c <= 5) {
                hcp->chunk_size = 0x0A + c;
                continue;
            }

            return NGX_ERROR;

        case sw_chunk_size:

            c = ch - '0';

            if (c > 9) {
                c = (ch | 0x20) - 'a';

                if (c <= 5) {
                    c += 0x0A;

                } else if (ch == '\r') {
                    state = sw_chunk_size_linefeed;
                    continue;

                } else {
                    return NGX_ERROR;
                }
            }

            if (ngx_size_is_sufficient(hcp->chunk_size)) {
                hcp->chunk_size = (hcp->chunk_size << 4) + c;
                continue;
            }

            return NGX_ERROR;

        case sw_chunk_size_linefeed:
            if (ch == '\n') {

                if (hcp->chunk_size != 0) {
                    state = sw_chunk;
                    continue;
                }

                hcp->last = 1;
                state = sw_chunk_end_newline;
                continue;
            }

            return NGX_ERROR;

        case sw_chunk_end_newline:
            if (ch == '\r') {
                state = sw_chunk_end_linefeed;
                continue;
            }

            return NGX_ERROR;

        case sw_chunk_end_linefeed:
            if (ch == '\n') {

                if (!hcp->last) {
                    state = sw_start;
                    continue;
                }

                return NGX_OK;
            }

            return NGX_ERROR;

        case sw_chunk:
            /*
             * This state is processed before the switch.
             * It added here just to suppress a warning.
             */
            continue;
        }
    }

    hcp->state = state;

    return NGX_AGAIN;
}


static inline int
ngx_js_http_whitespace(u_char c)
{
    switch (c) {
    case 0x09:  /* <TAB>  */
    case 0x0A:  /* <LF>   */
    case 0x0D:  /* <CR>   */
    case 0x20:  /* <SP>   */
        return 1;

    default:
        return 0;
    }
}


void
ngx_js_http_trim(u_char **value, size_t *len, int trim_c0_control_or_space)
{
    u_char  *start, *end;

    start = *value;
    end = start + *len;

    for ( ;; ) {
        if (start == end) {
            break;
        }

        if (ngx_js_http_whitespace(*start)
            || (trim_c0_control_or_space && *start <= ' '))
        {
            start++;
            continue;
        }

        break;
    }

    for ( ;; ) {
        if (start == end) {
            break;
        }

        end--;

        if (ngx_js_http_whitespace(*end)
            || (trim_c0_control_or_space && *end <= ' '))
        {
            continue;
        }

        end++;
        break;
    }

    *value = start;
    *len = end - start;
}


static const uint32_t  token_map[] = {
    0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                 /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
    0x03ff6cfa,  /* 0000 0011 1111 1111  0110 1100 1111 1010 */

                 /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
    0xc7fffffe,  /* 1100 0111 1111 1111  1111 1111 1111 1110 */

                 /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
    0x57ffffff,  /* 0101 0111 1111 1111  1111 1111 1111 1111 */

    0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
};


static inline int
ngx_is_token(uint32_t byte)
{
    return ((token_map[byte >> 5] & ((uint32_t) 1 << (byte & 0x1f))) != 0);
}


ngx_int_t
ngx_js_check_header_name(u_char *name, size_t len)
{
    u_char  *p, *end;

    p = name;
    end = p + len;

    while (p < end) {
        if (!ngx_is_token(*p)) {
            return NGX_ERROR;
        }

        p++;
    }

    return NGX_OK;
}

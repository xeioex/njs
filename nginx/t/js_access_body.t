#!/usr/bin/perl

# (C) Dmitry Volyntsev
# (C) F5, Inc.

# Tests for http njs module, js_access body reading methods.

###############################################################################

use warnings;
use strict;

use Test::More;

use Socket qw/ CRLF /;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use lib 'lib';
use Test::Nginx qw/ :DEFAULT http_end /;

###############################################################################

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()->has(qw/http proxy/)
	->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    js_import test.js;

    js_var $foo;

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        location /text {
            js_access test.read_text;
            js_content test.content;
        }

        location /buffer {
            js_access test.read_buffer;
            js_content test.content;
        }

        location /text_twice {
            js_access test.read_text_twice;
            js_content test.content;
        }

        location /buffer_twice {
            js_access test.read_buffer_twice;
            js_content test.content;
        }

        location /text_then_buffer {
            js_access test.read_text_then_buffer;
            js_content test.content;
        }

        location /empty {
            js_access test.read_text;
            js_content test.content;
        }

        location /big {
            client_body_buffer_size 64k;
            js_access test.read_text_length;
            js_content test.content;
        }

        location /big_4k {
            client_body_buffer_size 4k;
            js_access test.read_text_length;
            js_content test.content;
        }

        location /slow {
            js_access test.read_text;
            js_content test.content;
        }

        location /proxy {
            js_access test.read_text;
            proxy_pass http://127.0.0.1:%%PORT_8081%%;
        }

        location /proxy_big {
            client_body_buffer_size 64k;
            js_access test.read_text;
            proxy_pass http://127.0.0.1:%%PORT_8081%%;
        }
    }

    server {
        listen       127.0.0.1:8081;

        location / {
            js_content test.echo_body;
        }
    }
}

EOF

$t->write_file('test.js', <<EOF);
    function content(r) {
        r.return(200, `var:\${r.variables.foo}`);
    }

    async function read_text(r) {
        let body = await r.readRequestText();
        r.variables.foo = body;
    }

    async function read_buffer(r) {
        let buf = await r.readRequestBuffer();
        r.variables.foo = String.fromCharCode.apply(null, buf);
    }

    async function read_text_twice(r) {
        let first = await r.readRequestText();
        let second = await r.readRequestText();
        r.variables.foo = (first === second) ? 'same' : 'different';
    }

    async function read_buffer_twice(r) {
        let first = await r.readRequestBuffer();
        let second = await r.readRequestBuffer();
        let eq = first.length === second.length
                 && first.every((v, i) => v === second[i]);
        r.variables.foo = eq ? 'same' : 'different';
    }

    async function read_text_then_buffer(r) {
        let text = await r.readRequestText();
        let buf = await r.readRequestBuffer();
        let text2 = String.fromCharCode.apply(null, buf);
        r.variables.foo = (text === text2) ? 'same' : 'different';
    }

    async function read_text_length(r) {
        let body = await r.readRequestText();
        r.variables.foo = body.length;
    }

    function echo_body(r) {
        r.return(200, 'echo:' + r.requestText);
    }

    export default { content, read_text, read_buffer, read_text_twice,
                     read_buffer_twice, read_text_then_buffer,
                     read_text_length, echo_body };

EOF

$t->try_run('no js_access')->plan(11);

###############################################################################

like(http_post('/text'), qr/var:REQ-BODY/, 'readRequestText');
like(http_post('/buffer'), qr/var:REQ-BODY/, 'readRequestBuffer');
like(http_get('/empty'), qr/var:/, 'readRequestText empty body');

like(http_post('/text_twice'), qr/var:same/,
	'readRequestText twice returns same value');
like(http_post('/buffer_twice'), qr/var:same/,
	'readRequestBuffer twice returns same value');
like(http_post('/text_then_buffer'), qr/var:same/,
	'readRequestText then readRequestBuffer same content');

like(http_post_big('/big'), qr/var:10240/,
	'readRequestText large body');
like(http_post_big('/big_4k'), qr/var:10240/,
	'readRequestText large body with small buffer');

like(http_post('/proxy'), qr/echo:REQ-BODY/,
	'body preserved for proxy_pass');
like(http_post_big('/proxy_big'), qr/echo:(1234567890){1024}/,
	'large body preserved for proxy_pass');

like(http_post_slow('/slow'), qr/var:SLOW-BODY/,
	'readRequestText with slow client');

###############################################################################

sub http_post {
	my ($url, %extra) = @_;

	my $p = "POST $url HTTP/1.0" . CRLF .
		"Host: localhost" . CRLF .
		"Content-Length: 8" . CRLF .
		CRLF .
		"REQ-BODY";

	return http($p, %extra);
}

sub http_post_big {
	my ($url, %extra) = @_;

	my $p = "POST $url HTTP/1.0" . CRLF .
		"Host: localhost" . CRLF .
		"Content-Length: 10240" . CRLF .
		CRLF .
		("1234567890" x 1024);

	return http($p, %extra);
}

sub http_post_slow {
	my ($url, %extra) = @_;

	my $header = "POST $url HTTP/1.1" . CRLF .
		"Host: localhost" . CRLF .
		"Content-Length: 9" . CRLF .
		"Connection: close" . CRLF .
		CRLF;

	my $s = http($header, start => 1);

	select undef, undef, undef, 0.1;
	print $s "SLOW";

	select undef, undef, undef, 0.1;
	print $s "-BODY";

	return http_end($s);
}

###############################################################################

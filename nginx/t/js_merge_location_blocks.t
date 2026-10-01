#!/usr/bin/perl

# (C) Dmitry Volyntsev
# (c) Nginx, Inc.

# Tests for http njs module, check for proper location blocks merging.

###############################################################################

use warnings;
use strict;

use Test::More;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use lib 'lib';
use Test::Nginx;

###############################################################################

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()->has(qw/http --with-debug/)
	->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    js_import main.js;

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        location /a {
            js_content main.engine_id;
        }

        location /b {
            js_content main.engine_id;
        }

        location /c {
            js_content main.engine_id;
        }

        location /d {
            js_content main.engine_id;
        }
    }
}

EOF

$t->write_file('main.js', <<EOF);
    function engine_id(r) {
        r.return(200, ngx.engine_id);
    }

    export default {engine_id};

EOF

$t->try_run('no njs available')->plan(5);

###############################################################################

my %ids;
for my $uri ('/a', '/b', '/c', '/d') {
	my ($id) = http_get($uri) =~ /\x0d\x0a\x0d\x0a(\d+)/ms;
	ok(defined $id, "engine ID for $uri");
	$ids{$id} = 1 if defined $id;
}

is(scalar keys %ids, 1, 'http js block imported once');

###############################################################################

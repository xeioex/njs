#!/usr/bin/perl

# (C) Dmitry Volyntsev
# (C) Nginx, Inc.

# Tests for stream njs module, process object.

###############################################################################

use warnings;
use strict;

use Test::More;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use lib 'lib';
use Test::Nginx;
use Test::Nginx::Stream qw/ stream /;

###############################################################################

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()->has(qw/stream stream_return/)
	->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

env FOO=bar;
env BAR=baz;

stream {
    %%TEST_GLOBALS_STREAM%%

    js_import test.js;

    js_set $env_foo            test.env_foo;
    js_set $env_bar            test.env_bar;
    js_set $env_keys           test.env_keys;
    js_set $argv               test.argv;

    server {
        listen  127.0.0.1:8081;
        return  $env_foo;
    }

    server {
        listen  127.0.0.1:8082;
        return  $env_bar;
    }

    server {
        listen 127.0.0.1:8083;
        return $env_keys;
    }

    server {
        listen 127.0.0.1:8084;
        return $argv;
    }
}

EOF

$t->write_file('test.js', <<EOF);
    function env(s, v) {
        var e = process.env[v];
        return e ? e : 'undefined';
    }

    function env_foo(s) {
        return env(s, 'FOO');
    }

    function env_bar(s) {
        return env(s, 'BAR');
    }

    function env_keys(s) {
        return Object.keys(process.env).join(',');
    }


    function argv(r) {
        var av = process.argv;
        return `\${Array.isArray(av)} \${av[0].indexOf('nginx') >= 0}`;
    }

    export default { env_foo, env_bar, env_keys, argv };

EOF

$t->try_run('no njs stream session object')->plan(4);

###############################################################################

is(stream('127.0.0.1:' . port(8081))->read(), 'bar', 'env_foo');
is(stream('127.0.0.1:' . port(8082))->read(), 'baz', 'env_bar');
is(stream('127.0.0.1:' . port(8083))->read(), 'FOO,BAR', 'env_keys');
is(stream('127.0.0.1:' . port(8084))->read(), 'true true', 'argv');

###############################################################################

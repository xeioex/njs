#!/usr/bin/perl

# (C) Dmitry Volyntsev
# (C) F5, Inc.

# Tests for stream njs module - Shared Array Buffer support.

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

stream {
    %%TEST_GLOBALS_STREAM%%

    js_shared_array_zone zone=test:32k;
    js_shared_array_zone zone=buffer:32k;

    js_import test.js;

    js_set $init test.init;
    js_set $size test.size;
    js_set $increment test.increment;
    js_set $keys test.keys;

    server {
        listen  127.0.0.1:8081;
        return  $init;
    }

    server {
        listen  127.0.0.1:8082;
        return  $size;
    }

    server {
        listen  127.0.0.1:8083;
        return  $increment;
    }

    server {
        listen  127.0.0.1:8084;
        return  $keys;
    }

    js_set $rw_result test.rw_result;

    server {
        listen      127.0.0.1:8085;
        js_preread  test.read_write;
        return      $rw_result;
    }
}

EOF

$t->write_file('test.js', <<EOF);
    function init(s) {
        var sab = ngx.sharedArray.test;
        var view = new Int32Array(sab);
        return "init: " + view[0];
    }

    function increment(s) {
        var sab = ngx.sharedArray.test;
        var view = new Int32Array(sab);
        var val = Atomics.add(view, 0, 1);
        return "inc: " + (val + 1).toString();
    }

    function size(s) {
        var sab = ngx.sharedArray.buffer;
        return "size: " + sab.byteLength;
    }

    function read_write(s) {
        var collect = '';

        s.on('upload', function (data, flags) {
            collect += data;

            var n = collect.search('\\n');
            if (n != -1) {
                var cmd = collect.substr(0, n);
                var parts = cmd.split(' ');
                var action = parts[0];

                var sab = ngx.sharedArray.buffer;
                var view = new Uint8Array(sab);

                if (action === 'read') {
                    var at = parseInt(parts[1]) || 0;
                    s.variables.rw_result = "read: " + view[at];

                } else if (action === 'write') {
                    var at = parseInt(parts[1]) || 0;
                    var value = parseInt(parts[2]) || 0;
                    view[at] = value;
                    s.variables.rw_result = "written";
                }

                s.off('upload');
                s.done();
            }
        });
    }

    function rw_result(s) {
        return s.variables.rw_result;
    }

    function keys(s) {
        var k = Object.keys(ngx.sharedArray);
        return JSON.stringify(k.sort());
    }

    export default { init, increment, read_write, rw_result, size, keys };
EOF

$t->try_run('no QuickJS or shared array support')->plan(11);

###############################################################################

is(stream('127.0.0.1:' . port(8081))->read(), 'init: 0',
    'initialize shared array');
is(stream('127.0.0.1:' . port(8082))->read(), 'size: 32768',
    'check buffer size');
is(stream('127.0.0.1:' . port(8083))->read(), 'inc: 1', 'first increment');
is(stream('127.0.0.1:' . port(8083))->read(), 'inc: 2', 'second increment');
is(stream('127.0.0.1:' . port(8085))->io("write 5 6\n"), "written",
    'write to buffer');
is(stream('127.0.0.1:' . port(8085))->io("read 5\n"), "read: 6",
    'read from buffer');
is(stream('127.0.0.1:' . port(8085))->io("read 32767\n"), "read: 0",
    'read from buffer end');
is(stream('127.0.0.1:' . port(8085))->io("read 32768\n"), "read: undefined",
    'read out of buffer bounds');
is(stream('127.0.0.1:' . port(8085))->io("write 32767 1\n"), "written",
    'write to buffer end');
is(stream('127.0.0.1:' . port(8085))->io("read 32767\n"), "read: 1",
    'read from buffer end again');
is(stream('127.0.0.1:' . port(8084))->read(), '["buffer","test"]',
    'enumerate zones');

###############################################################################

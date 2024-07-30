#!/usr/bin/perl

# (C) Thomas P.

# Tests for http njs module, reading location capture variables.

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

my $t = Test::Nginx->new()->has(qw/http/)
	->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    js_import test.js;

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        location /njs {
            js_content test.njs;
        }

        location ~ /(.+)/(.+) {
            js_content test.variables;
        }
    }
}

EOF

$t->write_file('test.js', <<EOF);
    function variables(r) {
        let content = "";
        for (let i=0; i<4; i++) {
            content += `\\\$\${i} is "\${r.variables[i.toString()]}"\n`; 
        }
        return r.return(200, content);
    }

    function test_njs(r) {
        r.return(200, njs.version);
    }

    export default {njs:test_njs, variables};

EOF

$t->try_run('no njs capture variables')->plan(4);

###############################################################################

TODO: {
local $TODO = 'not yet' unless has_version('0.8.6');

like(http_get('/test/hello'), qr/\$0 is "\/test\/hello"/, 'global capture var');
like(http_get('/test/hello'), qr/\$1 is "test"/, 'local capture var 1');
like(http_get('/test/hello'), qr/\$2 is "hello"/, 'local capture var 2');
like(http_get('/test/hello'), qr/\$3 is "undefined"/, 'undefined capture var');

}

###############################################################################

sub has_version {
	my $need = shift;

	http_get('/njs') =~ /^([.0-9]+)$/m;

	my @v = split(/\./, $1);
	my ($n, $v);

	for $n (split(/\./, $need)) {
		$v = shift @v || 0;
		return 0 if $n > $v;
		return 1 if $v > $n;
	}

	return 1;
}

###############################################################################
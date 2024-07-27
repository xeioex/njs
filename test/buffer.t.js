/*---
includes: [compatBuffer.js, runTsuite.js]
flags: [async]
---*/

function p(args, default_opts) {
    let params = merge({}, default_opts);
    params = merge(params, args);

    return params;
}

let alloc_tsuite = {
    name: "Buffer.alloc() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = Buffer.alloc(params.size, params.fill, params.encoding);

        if (r.toString() !== params.expected) {
            throw Error(`unexpected output "${r.toString()}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: { encoding: 'utf-8' },

    tests: [
        { size: 3, fill: 0x61, expected: 'aaa' },
        { size: 3, fill: 'A', expected: 'AAA' },
        { size: 3, fill: 'ABCD', expected: 'ABC' },
        { size: 3, fill: '414243', encoding: 'hex', expected: 'ABC' },
        { size: 4, fill: '414243', encoding: 'hex', expected: 'ABCA' },
        { size: 3, fill: 'QUJD', encoding: 'base64', expected: 'ABC' },
        { size: 3, fill: 'QUJD', encoding: 'base64url', expected: 'ABC' },
        { size: 3, fill: Buffer.from('ABCD'), encoding: 'utf-8', expected: 'ABC' },
        { size: 3, fill: Buffer.from('ABCD'), encoding: 'utf8', expected: 'ABC' },
        { size: 3, fill: 'ABCD', encoding: 'utf-128',
          exception: 'TypeError: "utf-128" encoding is not supported' },
        { size: 3, fill: Buffer.from('def'), expected: 'def' },
    ],
};


let byteLength_tsuite = {
    name: "Buffer.byteLength() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = Buffer.byteLength(params.value, params.encoding);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: { encoding: 'utf-8' },

    tests: [
        { value: 'abc', expected: 3 },
        { value: 'αβγ', expected: 6 },
        { value: 'αβγ', encoding: 'utf-8', expected: 6 },
        { value: 'αβγ', encoding: 'utf8', expected: 6 },
        { value: 'αβγ', encoding: 'utf-128', exception: 'TypeError: "utf-128" encoding is not supported' },
        { value: '414243', encoding: 'hex', expected: 3 },
        { value: 'QUJD', encoding: 'base64', expected: 3 },
        { value: 'QUJD', encoding: 'base64url', expected: 3 },
        { value: Buffer.from('αβγ'), expected: 6 },
        { value: Buffer.alloc(3).buffer, expected: 3 },
        { value: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1), expected: 3 },
    ],
};


let concat_tsuite = {
    name: "Buffer.concat() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = Buffer.concat(params.buffers, params.length);

        if (r.toString() !== params.expected) {
            throw Error(`unexpected output "${r.toString()}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},
    tests: [
        { buffers: [ Buffer.from('abc'),
                     Buffer.from(new Uint8Array([0x64, 0x65, 0x66]).buffer, 1) ],
          expected: 'abcef' },
        { buffers: [ Buffer.from('abc'), Buffer.from('def'), Buffer.from('') ],
          expected: 'abcdef' },
        { buffers: [ Buffer.from(''), Buffer.from('abc'), Buffer.from('def') ],
          length: 4, expected: 'abcd' },
    ],
};


let compare_tsuite = {
    name: "Buffer.compare() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = Buffer.compare(params.buf1, params.buf2);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},

    tests: [
        { buf1: Buffer.from('abc'), buf2: Buffer.from('abc'), expected: 0 },
        { buf1: Buffer.from('abc'),
          buf2: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1),
          expected: 0 },
        { buf1: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1),
          buf2: Buffer.from('abc'),
          expected: 0 },
        { buf1: Buffer.from('abc'), buf2: Buffer.from('def'), expected: -1 },
        { buf1: Buffer.from('def'), buf2: Buffer.from('abc'), expected: 1 },
        { buf1: Buffer.from('abc'), buf2: Buffer.from('abcd'), expected: -1 },
        { buf1: Buffer.from('abcd'), buf2: Buffer.from('abc'), expected: 1 },
        { buf1: Buffer.from('abc'), buf2: Buffer.from('ab'), expected: 1 },
        { buf1: Buffer.from('ab'), buf2: Buffer.from('abc'), expected: -1 },
        { buf1: Buffer.from('abc'), buf2: Buffer.from(''), expected: 1 },
        { buf1: Buffer.from(''), buf2: Buffer.from('abc'), expected: -1 },
        { buf1: Buffer.from(''), buf2: Buffer.from(''), expected: 0 },
    ],
};


let comparePrototype_tsuite = {
    name: "buf.compare() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = params.buf.compare(params.target, params.tStart, params.tEnd,
                                   params.sStart, params.sEnd);


        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},

    tests: [
        { buf: Buffer.from('abc'), target: Buffer.from('abc'), expected: 0 },
        { buf: Buffer.from('abc'),
          target: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1),
          expected: 0 },
        { buf: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1),
          target: Buffer.from('abc'), expected: 0 },
        { buf: Buffer.from('abc'), target: Buffer.from('def'), expected: -1 },
        { buf: Buffer.from('def'), target: Buffer.from('abc'), expected: 1 },
        { buf: Buffer.from('abc'), target: Buffer.from('abcd'), expected: -1 },
        { buf: Buffer.from('abcd'), target: Buffer.from('abc'), expected: 1 },
        { buf: Buffer.from('abc'), target: Buffer.from('ab'), expected: 1 },
        { buf: Buffer.from('ab'), target: Buffer.from('abc'), expected: -1 },
        { buf: Buffer.from('abc'), target: Buffer.from(''), expected: 1 },
        { buf: Buffer.from(''), target: Buffer.from('abc'), expected: -1 },
        { buf: Buffer.from(''), target: Buffer.from(''), expected: 0 },

        { buf: Buffer.from('abcdef'), target: Buffer.from('abc'),
          sEnd: 3, expected: 0 },
        { buf: Buffer.from('abcdef'), target: Buffer.from('def'),
          sStart: 3, expected: 0 },
        { buf: Buffer.from('abcdef'), target: Buffer.from('abc'),
          sStart: 0, sEnd: 3, expected: 0 },
        { buf: Buffer.from('abcdef'), target: Buffer.from('def'),
          sStart: 3, sEnd: 6, expected: 0 },
        { buf: Buffer.from('abcdef'), target: Buffer.from('def'),
          sStart: 3, sEnd: 5, tStart: 0, tEnd: 2, expected: 0 },
    ],
};


let copy_tsuite = {
    name: "buf.copy() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = params.buf.copy(params.target, params.tStart, params.sStart, params.sEnd);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        if (params.target.toString() !== params.expected_buf) {
            throw Error(`unexpected buf "${params.target.toString()}" != "${params.expected_buf}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},

    tests: [
        { buf: Buffer.from('abcdef'), target: Buffer.from('123456'),
          expected: 6, expected_buf: 'abcdef' },
        { buf: Buffer.from('abcdef'), target: Buffer.from('123456'),
          tStart: 0, expected: 6, expected_buf: 'abcdef' },
        { buf: Buffer.from('abcdef'), target: Buffer.from('123456'),
          tStart: 0, sStart: 0, expected: 6, expected_buf: 'abcdef' },
        { buf: Buffer.from('abcdef'), target: Buffer.from('123456'),
          tStart: 0, sStart: 0, sEnd: 3, expected: 3, expected_buf: 'abc456' },
        { buf: Buffer.from('abcdef'), target: Buffer.from('123456'),
          tStart: 2, sStart: 2, sEnd: 3, expected: 1, expected_buf: '12c456' },
        { buf: Buffer.from('abcdef'), target: Buffer.from('123456'),
          tStart: 7, exception: 'RangeError: \"targetStart\" is out of bounds' },
        { buf: Buffer.from('abcdef'), target: Buffer.from('123456'),
          sStart: 7, exception: 'RangeError: \"sourceStart\" is out of bounds' },
    ],
};


let equals_tsuite = {
    name: "buf.equals() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = params.buf1.equals(params.buf2);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},
    tests: [

        { buf1: Buffer.from('abc'), buf2: Buffer.from('abc'), expected: true },
        { buf1: Buffer.from('abc'),
          buf2: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1),
          expected: true },
        { buf1: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1),
          buf2: Buffer.from('abc'), expected: true },
        { buf1: Buffer.from('abc'), buf2: Buffer.from('def'), expected: false },
        { buf1: Buffer.from('def'), buf2: Buffer.from('abc'), expected: false },
        { buf1: Buffer.from('abc'), buf2: Buffer.from('abcd'), expected: false },
        { buf1: Buffer.from('abcd'), buf2: Buffer.from('abc'), expected: false },
        { buf1: Buffer.from('abc'), buf2: Buffer.from('ab'), expected: false },
        { buf1: Buffer.from('ab'), buf2: Buffer.from('abc'), expected: false },
        { buf1: Buffer.from('abc'), buf2: Buffer.from(''), expected: false },
        { buf1: Buffer.from(''), buf2: Buffer.from('abc'), expected: false },
        { buf1: Buffer.from(''), buf2: Buffer.from(''), expected: true },
    ],
};


let fill_tsuite = {
    name: "buf.fill() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = params.buf.fill(params.value, params.offset, params.end);

        if (r.toString() !== params.expected) {
            throw Error(`unexpected output "${r.toString()}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},
    tests: [
        { buf: Buffer.from('abc'), value: 0x61, expected: 'aaa' },
        { buf: Buffer.from('abc'), value: 0x61, expected: 'aaa', offset: 0, end: 3 },
        { buf: Buffer.from('abc'), value: 0x61, expected: 'abc', offset: 0, end: 0 },
        { buf: Buffer.from('abc'), value: 'A', expected: 'AAA' },
        { buf: Buffer.from('abc'), value: 'ABCD', expected: 'ABC' },
        { buf: Buffer.from('abc'), value: '414243', offset: 'hex', expected: 'ABC' },
        { buf: Buffer.from('abc'), value: '414243', offset: 'utf-128',
          exception: 'TypeError: "utf-128" encoding is not supported' },
        { buf: Buffer.from('abc'), value: 'ABCD', offset: 1, expected: 'aAB' },
        { buf: Buffer.from('abc'), value: Buffer.from('def'), expected: 'def' },
        { buf: Buffer.from('def'),
          value: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1),
          expected: 'abc' },
        { buf: Buffer.from(new Uint8Array([0x60, 0x61, 0x62, 0x63]).buffer, 1),
          value: Buffer.from('def'),
          expected: 'def' },
    ],
};


let from_tsuite = {
    name: "Buffer.from() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let buf = Buffer.from.apply(null, params.args);

        if (params.modify) {
            params.modify(buf);
        }

        let r = buf.toString(params.fmt);

        if (r.length !== params.expected.length) {
            throw Error(`unexpected "${r}" length ${r.length} != ${params.expected.length}`);
        }

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },
    prepare_args: p,
    opts: { fmt: 'utf-8' },

    tests: [
        { args: [[0x62, 0x75, 0x66, 0x66, 0x65, 0x72]], expected: 'buffer' },
        { args: [{length:3, 0:0x62, 1:0x75, 2:0x66}], expected: 'buf' },
        { args: [[-1, 1, 255, 22323, -Infinity, Infinity, NaN]], fmt: "hex", expected: 'ff01ff33000000' },
        { args: [{length:5, 0:'A'.charCodeAt(0), 2:'X', 3:NaN, 4:0xfd}], fmt: "hex", expected: '41000000fd' },
        { args: [[1, 2, 0.23, '5', 'A']], fmt: "hex", expected: '0102000500' },
        { args: [new Uint8Array([0xff, 0xde, 0xba])], fmt: "hex", expected: 'ffdeba' },

        { args: [(new Uint8Array([0xaa, 0xbb, 0xcc])).buffer], fmt: "hex", expected: 'aabbcc' },
        { args: [(new Uint8Array([0xaa, 0xbb, 0xcc])).buffer, 1], fmt: "hex", expected: 'bbcc' },
        { args: [(new Uint8Array([0xaa, 0xbb, 0xcc])).buffer, 1, 1], fmt: "hex", expected: 'bb' },
        { args: [(new Uint8Array([0xaa, 0xbb, 0xcc])).buffer, '1', '1'], fmt: "hex", expected: 'bb' },
        { args: [(new Uint8Array([0xaa, 0xbb, 0xcc])).buffer, 1, 0], fmt: "hex", expected: '' },
        { args: [(new Uint8Array([0xaa, 0xbb, 0xcc])).buffer, 1, -1], fmt: "hex", expected: '' },

        { args: [(new Uint8Array([0xaa, 0xbb, 0xcc])).buffer], fmt: "hex",
          modify: (buf) => { buf[1] = 0; },
          expected: 'aa00cc' },

        { args: [new Uint16Array([234, 123])], fmt: "hex", expected: 'ea7b' },
        { args: [new Uint32Array([234, 123])], fmt: "hex", expected: 'ea7b' },
        { args: [new Float32Array([234.001, 123.11])], fmt: "hex", expected: 'ea7b' },
        { args: [new Uint32Array([234, 123])], fmt: "hex", expected: 'ea7b' },
        { args: [new Float64Array([234.001, 123.11])], fmt: "hex", expected: 'ea7b' },

        { args: [(new Uint8Array(2)).buffer, -1],
          exception: 'RangeError: invalid index' },
        { args: [(new Uint8Array(2)).buffer, 3],
          exception: 'RangeError: \"offset\" is outside of buffer bounds' },
        { args: [(new Uint8Array(2)).buffer, 1, 2],
          exception: 'RangeError: \"length\" is outside of buffer bounds' },

        { args: [Buffer.from([0xaa, 0xbb, 0xcc]).toJSON()], fmt: "hex", expected: 'aabbcc' },
        { args: [{type: 'Buffer', data: [0xaa, 0xbb, 0xcc]}], fmt: "hex", expected: 'aabbcc' },
        { args: [new String('00aabbcc'), 'hex'], fmt: "hex", expected: '00aabbcc' },
        { args: [Buffer.from([0xaa, 0xbb, 0xcc]).toJSON()], fmt: "hex", expected: 'aabbcc' },

        { args: [(function() {var arr = new Array(1, 2, 3); arr.valueOf = () => arr; return arr})()],
          fmt: "hex", expected: '010203' },
        { args: [(function() {var obj = new Object(); obj.valueOf = () => obj; return obj})()],
          exception: 'TypeError: first argument is not a string or Buffer-like object' },
        { args: [(function() {var obj = new Object(); obj.valueOf = () => undefined; return obj})()],
          exception: 'TypeError: first argument is not a string or Buffer-like object' },
        { args: [(function() {var obj = new Object(); obj.valueOf = () => null; return obj})()],
          exception: 'TypeError: first argument is not a string or Buffer-like object' },
        { args: [(function() {var obj = new Object(); obj.valueOf = () => new Array(1, 2, 3); return obj})()],
          fmt: "hex", expected: '010203' },
        { args: [(function() {var a = [1,2,3,4]; a[1] = { valueOf() { a.length = 3; return 1; } }; return a})()],
          fmt: "hex", expected: '01010300' },

        { args: [{type: 'B'}],
          exception: 'TypeError: first argument is not a string or Buffer-like object' },
        { args: [{type: undefined}],
          exception: 'TypeError: first argument is not a string or Buffer-like object' },
        { args: [{type: 'Buffer'}],
          exception: 'TypeError: first argument is not a string or Buffer-like object' },
        { args: [{type: 'Buffer', data: null}],
          exception: 'TypeError: first argument is not a string or Buffer-like object' },
        { args: [{type: 'Buffer', data: {}}],
          exception: 'TypeError: first argument is not a string or Buffer-like object' },

        { args: ['', 'utf-128'], exception: 'TypeError: "utf-128" encoding is not supported' },

        { args: [''], fmt: "hex", expected: '' },
        { args: ['α'], fmt: "hex", expected: 'ceb1' },
        { args: ['α', 'utf-8'], fmt: "hex", expected: 'ceb1' },
        { args: ['α', 'utf8'], fmt: "hex", expected: 'ceb1' },
        { args: ['', 'hex'], fmt: "hex", expected: '' },
        { args: ['aa0', 'hex'], fmt: "hex", expected: 'aa' },
        { args: ['00aabbcc', 'hex'], fmt: "hex", expected: '00aabbcc' },
        { args: ['deadBEEF##', 'hex'], fmt: "hex", expected: 'deadbeef' },
        { args: ['6576696c', 'hex'], expected: 'evil' },
        { args: ['f3', 'hex'], expected: '�' },

        { args: ['', "base64"], expected: '' },
        { args: ['#', "base64"], expected: '' },
        { args: ['Q', "base64"], expected: '' },
        { args: ['QQ', "base64"], expected: 'A' },
        { args: ['QQ=', "base64"], expected: 'A' },
        { args: ['QQ==', "base64"], expected: 'A' },
        { args: ['QUI=', "base64"], expected: 'AB' },
        { args: ['QUI', "base64"], expected: 'AB' },
        { args: ['QUJD', "base64"], expected: 'ABC' },
        { args: ['QUJDRA==', "base64"], expected: 'ABCD' },

        { args: ['', "base64url"], expected: '' },
        { args: ['QQ', "base64url"], expected: 'A' },
        { args: ['QUI', "base64url"], expected: 'AB' },
        { args: ['QUJD', "base64url"], expected: 'ABC' },
        { args: ['QUJDRA', "base64url"], expected: 'ABCD' },
        { args: ['QUJDRA#', "base64url"], expected: 'ABCD' },
]};


let includes_tsuite = {
    name: "buf.includes() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = params.buf.includes(params.value, params.offset, params.encoding);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},

    tests: [
        { buf: Buffer.from('abcdef'), value: 'abc', expected: true },
        { buf: Buffer.from('abcdef'), value: 'def', expected: true },
        { buf: Buffer.from('abcdef'), value: 'abc', offset: 1, expected: false },
        { buf: Buffer.from('abcdef'), value: {},
          exception: 'TypeError: "value" argument must be of type string or an instance of Buffer' },
    ],
};


let indexOf_tsuite = {
    name: "buf.indexOf() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = params.buf.indexOf(params.value, params.offset, params.encoding);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},

    tests: [
        { buf: Buffer.from('abcdef'), value: 'abc', expected: 0 },
        { buf: Buffer.from('abcdef'), value: 'def', expected: 3 },
        { buf: Buffer.from('abcdef'), value: 'abc', offset: 1, expected: -1 },
        { buf: Buffer.from('abcdef'), value: 'def', offset: 1, expected: 3 },
        { buf: Buffer.from('abcdef'), value: 'def', offset: -3, expected: 3 },
        { buf: Buffer.from('abcdef'), value: '626364', encoding: 'hex', expected: 1 },
        { buf: Buffer.from('abcdef'), value: '626364', encoding: 'utf-128',
          exception: 'TypeError: "utf-128" encoding is not supported' },
        { buf: Buffer.from('abcdef'), value: 0x62, expected: 1 },
        { buf: Buffer.from('abcabc'), value: 0x61, offset: 1, expected: 3 },
        { buf: Buffer.from('abcdef'), value: Buffer.from('def'), expected: 3 },
        { buf: Buffer.from('abcdef'), value: Buffer.from(new Uint8Array([0x60, 0x62, 0x63]).buffer, 1), expected: 1 },
        { buf: Buffer.from('abcdef'), value: {},
          exception: 'TypeError: "value" argument must be of type string or an instance of Buffer' },
    ],
};


let isBuffer_tsuite = {
    name: "Buffer.isBuffer() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = Buffer.isBuffer(params.value);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },
    prepare_args: p,
    opts: {},

    tests: [
        { value: Buffer.from('α'), expected: true },
        { value: new Uint8Array(10), expected: false },
        { value: {}, expected: false },
        { value: 1, expected: false },
]};


let isEncoding_tsuite = {
    name: "Buffer.isEncoding() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = Buffer.isEncoding(params.value);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},

    tests: [
        { value: 'utf-8', expected: true },
        { value: 'utf8', expected: true },
        { value: 'utf-128', expected: false },
        { value: 'hex', expected: true },
        { value: 'base64', expected: true },
        { value: 'base64url', expected: true },
    ],
};


function compare_object(a, b) {
    if (a === b) {
        return true;
    }

    if (typeof a !== 'object' || typeof b !== 'object') {
        return false;
    }

    if (Object.keys(a).length !== Object.keys(b).length) {
        return false;
    }

    for (let key in a) {
        if (!compare_object(a[key], b[key])) {
            return false;
        }

    }

    return true;
}


let lastIndexOf_tsuite = {
    name: "buf.lastIndexOf() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = params.buf.lastIndexOf(params.value, params.offset, params.encoding);

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},

    tests: [
        { buf: Buffer.from('abcdef'), value: 'abc', expected: 0 },
        { buf: Buffer.from('abcabc'), value: 'abc', expected: 3 },
        { buf: Buffer.from('abcdef'), value: 'def', expected: 3 },
        { buf: Buffer.from('abcdef'), value: 'abc', offset: 1, expected: 0 },
        { buf: Buffer.from('abcdef'), value: 'def', offset: 1, expected: -1 },
        { buf: Buffer.from(Buffer.from('Zabcdef').buffer, 1), value: 'abcdef', expected: 0 },
        { buf: Buffer.from(Buffer.from('Zabcdef').buffer, 1), value: 'abcdefg', expected: -1 },
        { buf: Buffer.from('abcdef'), value: '626364', encoding: 'hex', expected: 1 },
        { buf: Buffer.from('abcdef'), value: '626364', encoding: 'utf-128',
          exception: 'TypeError: "utf-128" encoding is not supported' },
        { buf: Buffer.from('abcabc'), value: 0x61, expected: 3 },
        { buf: Buffer.from('abcabc'), value: 0x61, offset: 1, expected: 0 },
        { buf: Buffer.from('abcdef'), value: Buffer.from('def'), expected: 3 },
        { buf: Buffer.from('abcdef'), value: Buffer.from(new Uint8Array([0x60, 0x62, 0x63]).buffer, 1), expected: 1 },
        { buf: Buffer.from('abcdef'), value: {},
          exception: 'TypeError: "value" argument must be of type string or an instance of Buffer' },
    ],
};


let toJSON_tsuite = {
    name: "Buffer.toJSON() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = Buffer.from(params.value).toJSON();

        if (!compare_object(r, params.expected)) {
            throw Error(`unexpected output "${JSON.stringify(r)}" != "${JSON.stringify(params.expected)}"`);
        }

        return 'SUCCESS';
    },

    prepare_args: p,
    opts: {},
    tests: [
        { value: '', expected: { type: 'Buffer', data: [] } },
        { value: 'αβγ', expected: { type: 'Buffer', data: [0xCE, 0xB1, 0xCE, 0xB2, 0xCE, 0xB3] } },
        { value: new Uint8Array([0xff, 0xde, 0xba]), expected: { type: 'Buffer', data: [0xFF, 0xDE, 0xBA] } },
    ],
};


let toString_tsuite = {
    name: "Buffer.toString() tests",
    skip: () => (!has_buffer()),
    T: async (params) => {
        let r = Buffer.from(params.value).toString(params.fmt);

        if (r.length !== params.expected.length) {
            throw Error(`unexpected "${r}" length ${r.length} != ${params.expected.length}`);
        }

        if (r !== params.expected) {
            throw Error(`unexpected output "${r}" != "${params.expected}"`);
        }

        return 'SUCCESS';
    },
    prepare_args: p,
    opts: { fmt: 'utf-8' },

    tests: [
        { value: '💩', expected: '💩' },
        { value: String.fromCharCode(0xD83D, 0xDCA9), expected: '💩' },
        { value: String.fromCharCode(0xD83D, 0xDCA9), expected: String.fromCharCode(0xD83D, 0xDCA9) },
        { value: new Uint8Array([0xff, 0xde, 0xba]), fmt: "hex", expected: 'ffdeba' },
        { value: new Uint8Array([0xff, 0xde, 0xba]), fmt: "base64", expected: '/966' },
        { value: new Uint8Array([0xff, 0xde, 0xba]), fmt: "base64url", expected: '_966' },
        { value: '', fmt: "utf-128", exception: 'TypeError: "utf-128" encoding is not supported' },
]};

run([
    alloc_tsuite,
    byteLength_tsuite,
    concat_tsuite,
    compare_tsuite,
    comparePrototype_tsuite,
    copy_tsuite,
    equals_tsuite,
    fill_tsuite,
    from_tsuite,
    includes_tsuite,
    indexOf_tsuite,
    isBuffer_tsuite,
    isEncoding_tsuite,
    lastIndexOf_tsuite,
    toJSON_tsuite,
    toString_tsuite,
])
.then($DONE, $DONE);

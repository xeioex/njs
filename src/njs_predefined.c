
/*
 * Copyright (C) Vadim Zhestkov
 * Copyright (C) F5, Inc.
 */


#include <njs_main.h>


typedef struct {
    uint32_t          cells[NJS_PREDEFINED_HASH_MASK + 1];
    struct {
        uint32_t     hash_mask;
        uint32_t     elts_size;
        uint32_t     elts_count;
        uint32_t     elts_deleted_count;
    } descr;
    njs_flathsh_elt_t elts[NJS_PREDEFINED_SIZE];
} njs_predefined_hash_chunk_t;


njs_predefined_values_t njs_predefined = {
    .value = {

        /* Tokens. */

        njs_string_kw("arguments",  0x02, NJS_TOKEN_ARGUMENTS),
        njs_string_kw("async",      0x02, NJS_TOKEN_ASYNC),
        njs_string_kw("await",      0x03, NJS_TOKEN_AWAIT),
        njs_string_kw("break",      0x03, NJS_TOKEN_BREAK),
        njs_string_kw("case",       0x03, NJS_TOKEN_CASE),
        njs_string_kw("catch",      0x03, NJS_TOKEN_CATCH),
        njs_string_kw("class",      0x03, NJS_TOKEN_CLASS),
        njs_string_kw("const",      0x03, NJS_TOKEN_CONST),
        njs_string_kw("continue",   0x03, NJS_TOKEN_CONTINUE),
        njs_string_kw("debugger",   0x03, NJS_TOKEN_DEBUGGER),
        njs_string_kw("default",    0x03, NJS_TOKEN_DEFAULT),
        njs_string_kw("delete",     0x03, NJS_TOKEN_DELETE),
        njs_string_kw("do",         0x03, NJS_TOKEN_DO),
        njs_string_kw("else",       0x03, NJS_TOKEN_ELSE),
        njs_string_kw("enum",       0x03, NJS_TOKEN_ENUM),
        njs_string_kw("eval",       0x02, NJS_TOKEN_EVAL),
        njs_string_kw("export",     0x03, NJS_TOKEN_EXPORT),
        njs_string_kw("extends",    0x03, NJS_TOKEN_EXTENDS),
        njs_string_kw("false",      0x03, NJS_TOKEN_FALSE),
        njs_string_kw("finally",    0x03, NJS_TOKEN_FINALLY),
        njs_string_kw("for",        0x03, NJS_TOKEN_FOR),
        njs_string_kw("from",       0x02, NJS_TOKEN_FROM),
        njs_string_kw("function",   0x03, NJS_TOKEN_FUNCTION),
        njs_string_kw("if",         0x03, NJS_TOKEN_IF),
        njs_string_kw("implements", 0x03, NJS_TOKEN_IMPLEMENTS),
        njs_string_kw("import",     0x03, NJS_TOKEN_IMPORT),
        njs_string_kw("in",         0x03, NJS_TOKEN_IN),
        njs_string_kw("instanceof", 0x03, NJS_TOKEN_INSTANCEOF),
        njs_string_kw("interface",  0x03, NJS_TOKEN_INTERFACE),
        njs_string_kw("let",        0x03, NJS_TOKEN_LET),
        njs_string_kw("meta",       0x02, NJS_TOKEN_META),
        njs_string_kw("new",        0x03, NJS_TOKEN_NEW),
        njs_string_kw("null",       0x03, NJS_TOKEN_NULL),
        njs_string_kw("of",         0x02, NJS_TOKEN_OF),
        njs_string_kw("package",    0x03, NJS_TOKEN_PACKAGE),
        njs_string_kw("private",    0x03, NJS_TOKEN_PRIVATE),
        njs_string_kw("protected",  0x03, NJS_TOKEN_PROTECTED),
        njs_string_kw("public",     0x03, NJS_TOKEN_PUBLIC),
        njs_string_kw("return",     0x03, NJS_TOKEN_RETURN),
        njs_string_kw("static",     0x03, NJS_TOKEN_STATIC),
        njs_string_kw("super",      0x03, NJS_TOKEN_SUPER),
        njs_string_kw("switch",     0x03, NJS_TOKEN_SWITCH),
        njs_string_kw("target",     0x02, NJS_TOKEN_TARGET),
        njs_string_kw("this",       0x03, NJS_TOKEN_THIS),
        njs_string_kw("throw",      0x03, NJS_TOKEN_THROW),
        njs_string_kw("true",       0x03, NJS_TOKEN_TRUE),
        njs_string_kw("try",        0x03, NJS_TOKEN_TRY),
        njs_string_kw("typeof",     0x03, NJS_TOKEN_TYPEOF),
        njs_string_kw("undefined",  0x02, NJS_TOKEN_UNDEFINED),
        njs_string_kw("var",        0x03, NJS_TOKEN_VAR),
        njs_string_kw("void",       0x03, NJS_TOKEN_VOID),
        njs_string_kw("while",      0x03, NJS_TOKEN_WHILE),
        njs_string_kw("with",       0x03, NJS_TOKEN_WITH),
        njs_string_kw("yield",      0x03, NJS_TOKEN_YIELD),

        /* Other predefined strings. */

        njs_string_kw(NJS_VERSION, 0, 0),
        njs_string_kw("(?:)", 0, 0),
        njs_string_kw(",", 0, 0),
        njs_string_kw("", 0, 0),
        njs_string_kw(" ", 0, 0),
        njs_string_kw("$262", 0, 0),
        njs_string_kw("-0", 0, 0),
        njs_string_kw("-Infinity", 0, 0),
        njs_string_kw("AggregateError", 0, 0),
        njs_string_kw("All promises were rejected", 0, 0),
        njs_string_kw("Array", 0, 0),
        njs_string_kw("Array Iterator", 0, 0),
        njs_string_kw("ArrayBuffer", 0, 0),
        njs_string_kw("AsyncFunction", 0, 0),
        njs_string_kw("Boolean", 0, 0),
        njs_string_kw("Buffer", 0, 0),
        njs_string_kw("BYTES_PER_ELEMENT", 0, 0),
        njs_string_kw("DataView", 0, 0),
        njs_string_kw("Date", 0, 0),
        njs_string_kw("E", 0, 0),
        njs_string_kw("EPSILON", 0, 0),
        njs_string_kw("Error", 0, 0),
        njs_string_kw("EvalError", 0, 0),
        njs_string_kw("Float32Array", 0, 0),
        njs_string_kw("Float64Array", 0, 0),
        njs_string_kw("Function", 0, 0),
        njs_string_kw("Infinity", 0, 0),
        njs_string_kw("Int16Array", 0, 0),
        njs_string_kw("Int32Array", 0, 0),
        njs_string_kw("Int8Array", 0, 0),
        njs_string_kw("InternalError", 0, 0),
        njs_string_kw("Invalid Date", 0, 0),
        njs_string_kw("JSON", 0, 0),
        njs_string_kw("LN10", 0, 0),
        njs_string_kw("LN2", 0, 0),
        njs_string_kw("LOG10E", 0, 0),
        njs_string_kw("LOG2E", 0, 0),
        njs_string_kw("MAX_LENGTH", 0, 0),
        njs_string_kw("MAX_SAFE_INTEGER", 0, 0),
        njs_string_kw("MAX_STRING_LENGTH", 0, 0),
        njs_string_kw("MAX_VALUE", 0, 0),
        njs_string_kw("MIN_SAFE_INTEGER", 0, 0),
        njs_string_kw("MIN_VALUE", 0, 0),
        njs_string_kw("Math", 0, 0),
        njs_string_kw("MemoryError", 0, 0),
        njs_string_kw("NEGATIVE_INFINITY", 0, 0),
        njs_string_kw("NaN", 0, 0),
        njs_string_kw("Number", 0, 0),
        njs_string_kw("Object", 0, 0),
        njs_string_kw("PI", 0, 0),
        njs_string_kw("POSITIVE_INFINITY", 0, 0),
        njs_string_kw("Promise", 0, 0),
        njs_string_kw("RangeError", 0, 0),
        njs_string_kw("ReferenceError", 0, 0),
        njs_string_kw("RegExp", 0, 0),
        njs_string_kw("SQRT1_2", 0, 0),
        njs_string_kw("SQRT2", 0, 0),
        njs_string_kw("String", 0, 0),
        njs_string_kw("Symbol", 0, 0),
        njs_string_kw("Symbol.asyncIterator", 0, 0),
        njs_string_kw("Symbol.hasInstance", 0, 0),
        njs_string_kw("Symbol.isConcatSpreadable", 0, 0),
        njs_string_kw("Symbol.iterator", 0, 0),
        njs_string_kw("Symbol.matchAll", 0, 0),
        njs_string_kw("Symbol.match", 0, 0),
        njs_string_kw("Symbol.replace", 0, 0),
        njs_string_kw("Symbol.search", 0, 0),
        njs_string_kw("Symbol.species", 0, 0),
        njs_string_kw("Symbol.split", 0, 0),
        njs_string_kw("Symbol.toPrimitive", 0, 0),
        njs_string_kw("Symbol.toStringTag", 0, 0),
        njs_string_kw("Symbol.unscopables", 0, 0),
        njs_string_kw("SyntaxError", 0, 0),
        njs_string_kw("TextDecoder", 0, 0),
        njs_string_kw("TextEncoder", 0, 0),
        njs_string_kw("TypeError", 0, 0),
        njs_string_kw("TypedArray", 0, 0),
        njs_string_kw("URIError", 0, 0),
        njs_string_kw("UTC", 0, 0),
        njs_string_kw("Uint16Array", 0, 0),
        njs_string_kw("Uint32Array", 0, 0),
        njs_string_kw("Uint8Array", 0, 0),
        njs_string_kw("Uint8ClampedArray", 0, 0),
        njs_string_kw("[Getter]", 0, 0),
        njs_string_kw("[Setter]", 0, 0),
        njs_string_kw("[Getter/Setter]", 0, 0),
        njs_string_kw("[object Array]", 0, 0),
        njs_string_kw("[object Arguments]", 0, 0),
        njs_string_kw("[object Boolean]", 0, 0),
        njs_string_kw("[object Date]", 0, 0),
        njs_string_kw("[object Error]", 0, 0),
        njs_string_kw("[object Function]", 0, 0),
        njs_string_kw("[object Null]", 0, 0),
        njs_string_kw("[object Number]", 0, 0),
        njs_string_kw("[object Object]", 0, 0),
        njs_string_kw("[object RegExp]", 0, 0),
        njs_string_kw("[object String]", 0, 0),
        njs_string_kw("[object Undefined]", 0, 0),
        njs_string_kw("__proto__", 0, 0),
        njs_string_kw("abs", 0, 0),
        njs_string_kw("acos", 0, 0),
        njs_string_kw("acosh", 0, 0),
        njs_string_kw("all", 0, 0),
        njs_string_kw("alloc", 0, 0),
        njs_string_kw("allocUnsafe", 0, 0),
        njs_string_kw("allocUnsafeSlow", 0, 0),
        njs_string_kw("allSettled", 0, 0),
        njs_string_kw("anonymous", 0, 0),
        njs_string_kw("any", 0, 0),
        njs_string_kw("apply", 0, 0),
        njs_string_kw("argv", 0, 0),
        njs_string_kw("asin", 0, 0),
        njs_string_kw("asinh", 0, 0),
        njs_string_kw("assign", 0, 0),
        njs_string_kw("asyncIterator", 0, 0),
        njs_string_kw("atan", 0, 0),
        njs_string_kw("atan2", 0, 0),
        njs_string_kw("atanh", 0, 0),
        njs_string_kw("atob", 0, 0),
        njs_string_kw("bind", 0, 0),
        njs_string_kw("boolean", 0, 0),
        njs_string_kw("btoa", 0, 0),
        njs_string_kw("buffer", 0, 0),
        njs_string_kw("byteLength", 0, 0),
        njs_string_kw("byteOffset", 0, 0),
        njs_string_kw("call", 0, 0),
        njs_string_kw("callee", 0, 0),
        njs_string_kw("caller", 0, 0),
        njs_string_kw("cbrt", 0, 0),
        njs_string_kw("ceil", 0, 0),
        njs_string_kw("charAt", 0, 0),
        njs_string_kw("charCodeAt", 0, 0),
        njs_string_kw("cluster_size", 0, 0),
        njs_string_kw("clz32", 0, 0),
        njs_string_kw("codePointAt", 0, 0),
        njs_string_kw("compare", 0, 0),
        njs_string_kw("concat", 0, 0),
        njs_string_kw("configurable", 0, 0),
        njs_string_kw("constructor", 0, 0),
        njs_string_kw("copy", 0, 0),
        njs_string_kw("copyWithin", 0, 0),
        njs_string_kw("cos", 0, 0),
        njs_string_kw("cosh", 0, 0),
        njs_string_kw("create", 0, 0),
        njs_string_kw("data", 0, 0),
        njs_string_kw("decode", 0, 0),
        njs_string_kw("decodeURI", 0, 0),
        njs_string_kw("decodeURIComponent", 0, 0),
        njs_string_kw("defineProperty", 0, 0),
        njs_string_kw("defineProperties", 0, 0),
        njs_string_kw("description", 0, 0),
        njs_string_kw("done", 0, 0),
        njs_string_kw("dump", 0, 0),
        njs_string_kw("encode", 0, 0),
        njs_string_kw("encodeInto", 0, 0),
        njs_string_kw("encodeURI", 0, 0),
        njs_string_kw("encodeURIComponent", 0, 0),
        njs_string_kw("encoding", 0, 0),
        njs_string_kw("endsWith", 0, 0),
        njs_string_kw("engine", 0, 0),
        njs_string_kw("enumerable", 0, 0),
        njs_string_kw("entries", 0, 0),
        njs_string_kw("env", 0, 0),
        njs_string_kw("equals", 0, 0),
        njs_string_kw("errors", 0, 0),
        njs_string_kw("every", 0, 0),
        njs_string_kw("exec", 0, 0),
        njs_string_kw("exp", 0, 0),
        njs_string_kw("expm1", 0, 0),
        njs_string_kw("external", 0, 0),
        njs_string_kw("fatal", 0, 0),
        njs_string_kw("fileName", 0, 0),
        njs_string_kw("fill", 0, 0),
        njs_string_kw("filter", 0, 0),
        njs_string_kw("find", 0, 0),
        njs_string_kw("findIndex", 0, 0),
        njs_string_kw("flags", 0, 0),
        njs_string_kw("floor", 0, 0),
        njs_string_kw("forEach", 0, 0),
        njs_string_kw("freeze", 0, 0),
        njs_string_kw("fromCharCode", 0, 0),
        njs_string_kw("fromCodePoint", 0, 0),
        njs_string_kw("fround", 0, 0),
        njs_string_kw("fulfilled", 0, 0),
        njs_string_kw("get", 0, 0),
        njs_string_kw("getDate", 0, 0),
        njs_string_kw("getDay", 0, 0),
        njs_string_kw("getFloat32", 0, 0),
        njs_string_kw("getFloat64", 0, 0),
        njs_string_kw("getFullYear", 0, 0),
        njs_string_kw("getHours", 0, 0),
        njs_string_kw("getInt16", 0, 0),
        njs_string_kw("getInt32", 0, 0),
        njs_string_kw("getInt8", 0, 0),
        njs_string_kw("getMilliseconds", 0, 0),
        njs_string_kw("getMinutes", 0, 0),
        njs_string_kw("getOwnPropertyDescriptor", 0, 0),
        njs_string_kw("getOwnPropertyDescriptors", 0, 0),
        njs_string_kw("getOwnPropertyNames", 0, 0),
        njs_string_kw("getOwnPropertySymbols", 0, 0),
        njs_string_kw("getPrototypeOf", 0, 0),
        njs_string_kw("getSeconds", 0, 0),
        njs_string_kw("getTime", 0, 0),
        njs_string_kw("getTimezoneOffset", 0, 0),
        njs_string_kw("getUTCDate", 0, 0),
        njs_string_kw("getUTCDay", 0, 0),
        njs_string_kw("getUTCFullYear", 0, 0),
        njs_string_kw("getUTCHours", 0, 0),
        njs_string_kw("getUTCMilliseconds", 0, 0),
        njs_string_kw("getUTCMinutes", 0, 0),
        njs_string_kw("getUTCMonth", 0, 0),
        njs_string_kw("getUTCSeconds", 0, 0),
        njs_string_kw("getUint16", 0, 0),
        njs_string_kw("getUint32", 0, 0),
        njs_string_kw("getUint8", 0, 0),
        njs_string_kw("getMonth", 0, 0),
        njs_string_kw("global", 0, 0),
        njs_string_kw("globalThis", 0, 0),
        njs_string_kw("groups", 0, 0),
        njs_string_kw("hasOwnProperty", 0, 0),
        njs_string_kw("hasInstance", 0, 0),
        njs_string_kw("hypot", 0, 0),
        njs_string_kw("ignoreBOM", 0, 0),
        njs_string_kw("ignoreCase", 0, 0),
        njs_string_kw("imul", 0, 0),
        njs_string_kw("includes", 0, 0),
        njs_string_kw("index", 0, 0),
        njs_string_kw("indexOf", 0, 0),
        njs_string_kw("input", 0, 0),
        njs_string_kw("invalid", 0, 0),
        njs_string_kw("is", 0, 0),
        njs_string_kw("isArray", 0, 0),
        njs_string_kw("isBuffer", 0, 0),
        njs_string_kw("isConcatSpreadable", 0, 0),
        njs_string_kw("isEncoding", 0, 0),
        njs_string_kw("isExtensible", 0, 0),
        njs_string_kw("isFinite", 0, 0),
        njs_string_kw("isFrozen", 0, 0),
        njs_string_kw("isInteger", 0, 0),
        njs_string_kw("isNaN", 0, 0),
        njs_string_kw("isPrototypeOf", 0, 0),
        njs_string_kw("isSafeInteger", 0, 0),
        njs_string_kw("isSealed", 0, 0),
        njs_string_kw("isView", 0, 0),
        njs_string_kw("iterator", 0, 0),
        njs_string_kw("join", 0, 0),
        njs_string_kw("keyFor", 0, 0),
        njs_string_kw("keys", 0, 0),
        njs_string_kw("k", 0, 0),
        njs_string_kw("kval", 0, 0),
        njs_string_kw("k2", 0, 0),
        njs_string_kw("k2val", 0, 0),
        njs_string_kw("lastIndex", 0, 0),
        njs_string_kw("lastIndexOf", 0, 0),
        njs_string_kw("length", 0, 0),
        njs_string_kw("lineNumber", 0, 0),
        njs_string_kw("log", 0, 0),
        njs_string_kw("log10", 0, 0),
        njs_string_kw("log1p", 0, 0),
        njs_string_kw("log2", 0, 0),
        njs_string_kw("map", 0, 0),
        njs_string_kw("matchAll", 0, 0),
        njs_string_kw("match", 0, 0),
        njs_string_kw("max", 0, 0),
        njs_string_kw("min", 0, 0),
        njs_string_kw("memoryStats", 0, 0),
        njs_string_kw("message", 0, 0),
        njs_string_kw("multiline", 0, 0),
        njs_string_kw("name", 0, 0),
        njs_string_kw("nblocks", 0, 0),
        njs_string_kw("next", 0, 0),
        njs_string_kw("njs", 0, 0),
        njs_string_kw("now", 0, 0),
        njs_string_kw("number", 0, 0),
        njs_string_kw("object", 0, 0),
        njs_string_kw("on", 0, 0),
        njs_string_kw("p", 0, 0),
        njs_string_kw("pval", 0, 0),
        njs_string_kw("p2", 0, 0),
        njs_string_kw("p2val", 0, 0),
        njs_string_kw("padEnd", 0, 0),
        njs_string_kw("padStart", 0, 0),
        njs_string_kw("page_size", 0, 0),
        njs_string_kw("parse", 0, 0),
        njs_string_kw("parseFloat", 0, 0),
        njs_string_kw("parseInt", 0, 0),
        njs_string_kw("pid", 0, 0),
        njs_string_kw("pop", 0, 0),
        njs_string_kw("pow", 0, 0),
        njs_string_kw("ppid", 0, 0),
        njs_string_kw("preventExtensions", 0, 0),
        njs_string_kw("process", 0, 0),
        njs_string_kw("propertyIsEnumerable", 0, 0),
        njs_string_kw("prototype", 0, 0),
        njs_string_kw("push", 0, 0),
        njs_string_kw("q", 0, 0),
        njs_string_kw("qval", 0, 0),
        njs_string_kw("q2", 0, 0),
        njs_string_kw("q2val", 0, 0),
        njs_string_kw("r", 0, 0),
        njs_string_kw("r2", 0, 0),
        njs_string_kw("r2val", 0, 0),
        njs_string_kw("race", 0, 0),
        njs_string_kw("random", 0, 0),
        njs_string_kw("read", 0, 0),
        njs_string_kw("readDoubleBE", 0, 0),
        njs_string_kw("readDoubleLE", 0, 0),
        njs_string_kw("readFloatBE", 0, 0),
        njs_string_kw("readFloatLE", 0, 0),
        njs_string_kw("readInt16BE", 0, 0),
        njs_string_kw("readInt16LE", 0, 0),
        njs_string_kw("readInt32BE", 0, 0),
        njs_string_kw("readInt32LE", 0, 0),
        njs_string_kw("readInt8", 0, 0),
        njs_string_kw("readIntBE", 0, 0),
        njs_string_kw("readIntLE", 0, 0),
        njs_string_kw("readUInt16BE", 0, 0),
        njs_string_kw("readUInt16LE", 0, 0),
        njs_string_kw("readUInt32BE", 0, 0),
        njs_string_kw("readUInt32LE", 0, 0),
        njs_string_kw("readUInt8", 0, 0),
        njs_string_kw("readUIntBE", 0, 0),
        njs_string_kw("readUIntLE", 0, 0),
        njs_string_kw("reason", 0, 0),
        njs_string_kw("reduce", 0, 0),
        njs_string_kw("reduceRight", 0, 0),
        njs_string_kw("reject", 0, 0),
        njs_string_kw("rejected", 0, 0),
        njs_string_kw("repeat", 0, 0),
        njs_string_kw("replace", 0, 0),
        njs_string_kw("replaceAll", 0, 0),
        njs_string_kw("require", 0, 0),
        njs_string_kw("resolve", 0, 0),
        njs_string_kw("reverse", 0, 0),
        njs_string_kw("round", 0, 0),
        njs_string_kw("rval", 0, 0),
        njs_string_kw("seal", 0, 0),
        njs_string_kw("search", 0, 0),
        njs_string_kw("set", 0, 0),
        njs_string_kw("setDate", 0, 0),
        njs_string_kw("setFloat32", 0, 0),
        njs_string_kw("setFloat64", 0, 0),
        njs_string_kw("setFullYear", 0, 0),
        njs_string_kw("setHours", 0, 0),
        njs_string_kw("setInt16", 0, 0),
        njs_string_kw("setInt32", 0, 0),
        njs_string_kw("setInt8", 0, 0),
        njs_string_kw("setMilliseconds", 0, 0),
        njs_string_kw("setMinutes", 0, 0),
        njs_string_kw("setMonth", 0, 0),
        njs_string_kw("setPrototypeOf", 0, 0),
        njs_string_kw("setSeconds", 0, 0),
        njs_string_kw("setTime", 0, 0),
        njs_string_kw("setUint16", 0, 0),
        njs_string_kw("setUint32", 0, 0),
        njs_string_kw("setUint8", 0, 0),
        njs_string_kw("setUTCDate", 0, 0),
        njs_string_kw("setUTCFullYear", 0, 0),
        njs_string_kw("setUTCHours", 0, 0),
        njs_string_kw("setUTCMilliseconds", 0, 0),
        njs_string_kw("setUTCMinutes", 0, 0),
        njs_string_kw("setUTCMonth", 0, 0),
        njs_string_kw("setUTCSeconds", 0, 0),
        njs_string_kw("shift", 0, 0),
        njs_string_kw("sign", 0, 0),
        njs_string_kw("sin", 0, 0),
        njs_string_kw("sinh", 0, 0),
        njs_string_kw("size", 0, 0),
        njs_string_kw("slice", 0, 0),
        njs_string_kw("some", 0, 0),
        njs_string_kw("sort", 0, 0),
        njs_string_kw("source", 0, 0),
        njs_string_kw("species", 0, 0),
        njs_string_kw("splice", 0, 0),
        njs_string_kw("split", 0, 0),
        njs_string_kw("sqrt", 0, 0),
        njs_string_kw("stack", 0, 0),
        njs_string_kw("startsWith", 0, 0),
        njs_string_kw("status", 0, 0),
        njs_string_kw("sticky", 0, 0),
        njs_string_kw("stream", 0, 0),
        njs_string_kw("string", 0, 0),
        njs_string_kw("stringify", 0, 0),
        njs_string_kw("subarray", 0, 0),
        njs_string_kw("substr", 0, 0),
        njs_string_kw("substring", 0, 0),
        njs_string_kw("symbol", 0, 0),
        njs_string_kw("swap16", 0, 0),
        njs_string_kw("swap32", 0, 0),
        njs_string_kw("swap64", 0, 0),
        njs_string_kw("tan", 0, 0),
        njs_string_kw("tanh", 0, 0),
        njs_string_kw("test", 0, 0),
        njs_string_kw("then", 0, 0),
        njs_string_kw("times", 0, 0),
        njs_string_kw("toDateString", 0, 0),
        njs_string_kw("toExponential", 0, 0),
        njs_string_kw("toFixed", 0, 0),
        njs_string_kw("toISOString", 0, 0),
        njs_string_kw("toJSON", 0, 0),
        njs_string_kw("toLocaleDateString", 0, 0),
        njs_string_kw("toLocaleString", 0, 0),
        njs_string_kw("toLocaleTimeString", 0, 0),
        njs_string_kw("toLowerCase", 0, 0),
        njs_string_kw("toPrimitive", 0, 0),
        njs_string_kw("toPrecision", 0, 0),
        njs_string_kw("toReversed", 0, 0),
        njs_string_kw("toSorted", 0, 0),
        njs_string_kw("toSpliced", 0, 0),
        njs_string_kw("toStringTag", 0, 0),
        njs_string_kw("toString", 0, 0),
        njs_string_kw("toTimeString", 0, 0),
        njs_string_kw("toUpperCase", 0, 0),
        njs_string_kw("toUTCString", 0, 0),
        njs_string_kw("trimEnd", 0, 0),
        njs_string_kw("trimStart", 0, 0),
        njs_string_kw("trim", 0, 0),
        njs_string_kw("trunc", 0, 0),
        njs_string_kw("type", 0, 0),
        njs_string_kw("usec", 0, 0),
        njs_string_kw("unscopables", 0, 0),
        njs_string_kw("unshift", 0, 0),
        njs_string_kw("utf-8", 0, 0),
        njs_string_kw("value", 0, 0),
        njs_string_kw("valueOf", 0, 0),
        njs_string_kw("values", 0, 0),
        njs_string_kw("version", 0, 0),
        njs_string_kw("version_number", 0, 0),
        njs_string_kw("write", 0, 0),
        njs_string_kw("writable", 0, 0),
        njs_string_kw("writeDoubleBE", 0, 0),
        njs_string_kw("writeDoubleLE", 0, 0),
        njs_string_kw("writeFloatBE", 0, 0),
        njs_string_kw("writeFloatLE", 0, 0),
        njs_string_kw("writeIntBE", 0, 0),
        njs_string_kw("writeIntLE", 0, 0),
        njs_string_kw("writeInt16BE", 0, 0),
        njs_string_kw("writeInt16LE", 0, 0),
        njs_string_kw("writeInt32BE", 0, 0),
        njs_string_kw("writeInt32LE", 0, 0),
        njs_string_kw("writeInt8", 0, 0),
        njs_string_kw("writeUIntBE", 0, 0),
        njs_string_kw("writeUIntLE", 0, 0),
        njs_string_kw("writeUInt16BE", 0, 0),
        njs_string_kw("writeUInt16LE", 0, 0),
        njs_string_kw("writeUInt32BE", 0, 0),
        njs_string_kw("writeUInt32LE", 0, 0),
        njs_string_kw("writeUInt8", 0, 0),
        njs_string_kw("written", 0, 0),
    }
};


njs_predefined_hash_chunk_t njs_predefined_hash_predefined = {
    .descr = {
        .hash_mask = NJS_PREDEFINED_HASH_MASK,
        .elts_size = NJS_PREDEFINED_SIZE,
        .elts_count = 0,
        .elts_deleted_count = 0,
    }
};


njs_flathsh_t njs_predefined_hash = {
    .slot = &njs_predefined_hash_predefined.descr,
};


uint32_t  njs_predefined_atom_id = NJS_SYMBOL_KNOWN_MAX;


static njs_int_t
njs_predefined_hash_test(njs_flathsh_query_t *lhq, void *data)
{
    size_t       size;
    u_char       *start;
    njs_value_t  *name;

    name = data;

    size = name->string.data->length;

    if (lhq->key.length != size) {
        return NJS_DECLINED;
    }

    start = (u_char *) name->string.data->start;

    if (memcmp(start, lhq->key.start, lhq->key.length) == 0) {
        return NJS_OK;
    }

    return NJS_DECLINED;
}


/*
 *  Here is used statically allocated hash with correct size.
 *  This hash is only filled in, and later used as read only one.
 *  So, alloc/free are never used here.
 */

static const njs_flathsh_proto_t  njs_predefined_hash_proto
    njs_aligned(64) =
{
    0,
    njs_predefined_hash_test,
    NULL,
    NULL,
};


njs_int_t
njs_predefined_hash_init()
{
    u_char               *start;
    size_t               len;
    uint32_t             atom_id;
    njs_int_t            ret;
    njs_uint_t           n;
    njs_value_t          *value;
    njs_flathsh_query_t  lhq;

    if (njs_predefined_hash_predefined.descr.elts_count != 0) {
        *hash = njs_predefined_hash;
        return NJS_OK;
    }

    lhq.replace = 0;
    lhq.proto = &njs_predefined_hash_proto;
    lhq.pool = NULL; /* Not used. */

    atom_id = NJS_SYMBOL_KNOWN_MAX;

    for (n = 0; n < NJS_PREDEFINED_SIZE; n++) {
        value = &njs_predefined.value[n];

        value->string.atom_id = atom_id++;

        start = value->string.data->start;
        len = value->string.data->length;

        lhq.key_hash = njs_djb_hash(start, len);
        lhq.key.length = len;
        lhq.key.start = start;

        lhq.value = (void *) value;

        ret = njs_flathsh_insert(&njs_predefined_hash, &lhq);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }
    }

    *hash = njs_predefined_hash;

    return NJS_OK;
};


/*---
includes: [compatBuffer.js, compatWebcrypto.js, compareArray.js, runTsuite.js]
flags: [async]
---*/

/*
 * Algorithm members are read before the input bytes are copied, in
 * lexicographic order for each dictionary.  BufferSource members are
 * copied when read, and a detached buffer is copied as an empty byte
 * sequence.  Each case compares an operation whose getters invalidate
 * or mutate a buffer with the same operation on the equivalent bytes.
 */

function bytes(length, value) {
    return new Uint8Array(length).fill((value === undefined) ? 1 : value);
}

function empty() {
    return new Uint8Array(0);
}

function invalidate(buffer) {
    try {
        if (is_detach_available()) {
            detach(buffer);

        } else {
            buffer.transfer();
        }

    } catch (e) {
        /* Already detached. */
    }
}

function getter(object, name, value, effect) {
    Object.defineProperty(object, name, {
        get() { effect(); return value; },
        enumerable: true,
        configurable: true
    });

    return object;
}

function coercion(value, effect) {
    return {valueOf() { effect(); return value; }};
}

async function outcome(promise) {
    try {
        let value = await promise;

        if (typeof value !== 'boolean') {
            value = new Uint8Array(value);
        }

        return {ok: true, value};

    } catch (e) {
        return {ok: false, error: e};
    }
}

async function same(name, actual, expected) {
    let a = await outcome(actual());
    let e = await outcome(expected());

    assert.sameValue(a.ok, e.ok,
                     `${name}: ${a.ok ? 'resolved' : a.error} `
                     + `vs ${e.ok ? 'resolved' : e.error}`);

    if (!a.ok) {
        return;
    }

    if (typeof e.value === 'boolean') {
        assert.sameValue(a.value, e.value, name);

    } else {
        assert.compareArray(a.value, e.value, name);
    }
}

function cipherParams(name) {
    switch (name) {
    case 'AES-GCM':
        return {name, additionalData: bytes(17, 2), iv: bytes(12, 3),
                tagLength: 128};

    case 'AES-CTR':
        return {name, counter: bytes(16, 3), length: 64};

    default:
        return {name, iv: bytes(16, 3)};
    }
}

async function cipherKey(name) {
    return crypto.subtle.importKey('raw', bytes(16), name, false,
                                   ['encrypt', 'decrypt']);
}

async function cipherInput(name, key, decrypt) {
    if (!decrypt) {
        return bytes(32, 4);
    }

    return new Uint8Array(await crypto.subtle.encrypt(cipherParams(name), key,
                                                      bytes(32, 4)));
}

async function cipherData(name, member, numeric, decrypt) {
    let op = decrypt ? 'decrypt' : 'encrypt';
    let key = await cipherKey(name);
    let input = await cipherInput(name, key, decrypt);

    await same(`${op} ${name} data invalidated by ${member}`,
        () => {
            let data = input.slice();
            let params = cipherParams(name);
            let effect = () => invalidate(data.buffer);

            if (numeric) {
                params[member] = coercion(params[member], effect);

            } else {
                getter(params, member, params[member], effect);
            }

            return crypto.subtle[op](params, key, data);
        },
        () => crypto.subtle[op](cipherParams(name), key, empty()));
}

async function cipherMember(name, member, later, mutate) {
    let key = await cipherKey(name);

    await same(`encrypt ${name} ${member} ${mutate ? 'mutated' : 'invalidated'}`
               + ` by ${later}`,
        () => {
            let params = cipherParams(name);
            let buffer = params[member];

            getter(params, later, params[later], () => {
                if (mutate) {
                    buffer.fill(7);

                } else {
                    invalidate(buffer.buffer);
                }
            });

            return crypto.subtle.encrypt(params, key, bytes(32, 4));
        },
        () => {
            let params = cipherParams(name);
            let copied = (later > member);

            if (!copied) {
                params[member] = mutate ? bytes(params[member].length, 7)
                                        : empty();
            }

            return crypto.subtle.encrypt(params, key, bytes(32, 4));
        });
}

async function cipherTemporary(name, member) {
    let key = await cipherKey(name);

    await same(`encrypt ${name} temporary ${member}`,
        () => {
            let params = cipherParams(name);
            let value = params[member];

            Object.defineProperty(params, member, {
                get() { return value.slice(); },
                enumerable: true
            });

            return crypto.subtle.encrypt(params, key, bytes(32, 4));
        },
        () => crypto.subtle.encrypt(cipherParams(name), key, bytes(32, 4)));
}

async function cipherResize() {
    if (typeof ArrayBuffer.prototype.resize !== 'function') {
        return;
    }

    let key = await cipherKey('AES-CBC');

    await same('encrypt AES-CBC data resized by iv',
        () => {
            let buffer = new ArrayBuffer(32, {maxByteLength: 64});
            let params = cipherParams('AES-CBC');

            new Uint8Array(buffer).fill(4);
            getter(params, 'iv', params.iv, () => buffer.resize(0));

            return crypto.subtle.encrypt(params, key, new Uint8Array(buffer));
        },
        () => crypto.subtle.encrypt(cipherParams('AES-CBC'), key, empty()));
}

function deriveParams(name) {
    if (name === 'PBKDF2') {
        return {name, hash: 'SHA-256', iterations: 1, salt: bytes(16, 5)};
    }

    return {name, hash: 'SHA-256', info: bytes(8, 6), salt: bytes(16, 5)};
}

async function deriveMember(name, member, later, numeric) {
    let key = await crypto.subtle.importKey('raw', bytes(16), name, false,
                                           ['deriveBits']);

    await same(`deriveBits ${name} ${member} invalidated by ${later}`,
        () => {
            let params = deriveParams(name);
            let buffer = params[member];
            let effect = () => invalidate(buffer.buffer);

            if (numeric) {
                params[later] = coercion(params[later], effect);

            } else {
                getter(params, later, params[later], effect);
            }

            return crypto.subtle.deriveBits(params, key, 128);
        },
        () => {
            let params = deriveParams(name);

            if (later < member) {
                params[member] = empty();
            }

            return crypto.subtle.deriveBits(params, key, 128);
        });
}

async function deriveTemporary(name) {
    let key = await crypto.subtle.importKey('raw', bytes(16), name, false,
                                           ['deriveBits']);

    await same(`deriveBits ${name} temporary members`,
        () => {
            let params = deriveParams(name);
            let members = Object.keys(params);

            for (let i = 0; i < members.length; i++) {
                let value = params[members[i]];

                if (value instanceof Uint8Array) {
                    Object.defineProperty(params, members[i], {
                        get() { return value.slice(); },
                        enumerable: true
                    });
                }
            }

            return crypto.subtle.deriveBits(params, key, 128);
        },
        () => crypto.subtle.deriveBits(deriveParams(name), key, 128));
}

async function digestDetached() {
    await same('digest detached data',
        () => {
            let data = bytes(32, 4);

            invalidate(data.buffer);

            return crypto.subtle.digest('SHA-256', data);
        },
        () => crypto.subtle.digest('SHA-256', empty()));
}

function exported(format, promise) {
    return promise.then(key => crypto.subtle.exportKey(format, key));
}

async function importData(name, format, member) {
    let params;
    let data;

    switch (name) {
    case 'HMAC':
        params = () => ({name, hash: 'SHA-256'});
        data = bytes(16, 8);
        break;

    case 'ECDSA':
        params = () => ({name, namedCurve: 'P-256'});
        data = new Uint8Array(await crypto.subtle.exportKey(format,
            (await crypto.subtle.generateKey(params(), true,
                                             ['sign', 'verify'])).publicKey));
        break;

    default:
        params = () => ({name, hash: 'SHA-256'});
        data = new Uint8Array(await crypto.subtle.exportKey(format,
            (await crypto.subtle.generateKey({name, modulusLength: 1024,
                                  publicExponent: new Uint8Array([1, 0, 1]),
                                  hash: 'SHA-256'}, true,
                                 ['sign', 'verify'])).publicKey));
        break;
    }

    let usages = (name === 'HMAC') ? ['sign'] : ['verify'];

    await same(`importKey ${format} ${name} keyData invalidated by ${member}`,
        () => {
            let input = data.slice();
            let algorithm = params();

            getter(algorithm, member, algorithm[member],
                   () => invalidate(input.buffer));

            return exported(format, crypto.subtle.importKey(format, input,
                                                            algorithm, true,
                                                            usages));
        },
        () => exported(format, crypto.subtle.importKey(format, empty(),
                                                       params(), true,
                                                       usages)));
}

async function signKeys(name) {
    let params;

    switch (name) {
    case 'ECDSA':
        params = {name, namedCurve: 'P-256'};
        break;

    case 'RSA-PSS':
        params = {name, modulusLength: 1024,
                  publicExponent: new Uint8Array([1, 0, 1]),
                  hash: 'SHA-256'};
        break;

    default:
        return crypto.subtle.importKey('raw', bytes(16), {name,
                                       hash: 'SHA-256'}, false,
                                       ['sign', 'verify']);
    }

    return crypto.subtle.generateKey(params, false, ['sign', 'verify']);
}

function signParams(name) {
    switch (name) {
    case 'ECDSA':
        return {name, hash: 'SHA-256'};

    case 'RSA-PSS':
        return {name, saltLength: 16};

    default:
        return {name};
    }
}

async function signData(name, member, numeric) {
    let keys = await signKeys(name);
    let priv = keys.privateKey || keys;
    let pub = keys.publicKey || keys;

    await same(`sign ${name} data invalidated by ${member}`,
        async () => {
            let data = bytes(32, 4);
            let params = signParams(name);
            let effect = () => invalidate(data.buffer);

            if (numeric) {
                params[member] = coercion(params[member], effect);

            } else {
                getter(params, member, params[member], effect);
            }

            let signature = await crypto.subtle.sign(params, priv, data);

            return crypto.subtle.verify(signParams(name), pub, signature,
                                        empty());
        },
        async () => {
            let signature = await crypto.subtle.sign(signParams(name), priv,
                                                     empty());

            return crypto.subtle.verify(signParams(name), pub, signature,
                                        empty());
        });
}

async function verifyData(name, member, target, numeric) {
    let keys = await signKeys(name);
    let priv = keys.privateKey || keys;
    let pub = keys.publicKey || keys;
    let signed = new Uint8Array(await crypto.subtle.sign(signParams(name),
                                                         priv, bytes(32, 4)));

    await same(`verify ${name} ${target} invalidated by ${member}`,
        () => {
            let signature = signed.slice();
            let data = bytes(32, 4);
            let params = signParams(name);
            let buffer = (target === 'signature') ? signature : data;
            let effect = () => invalidate(buffer.buffer);

            if (numeric) {
                params[member] = coercion(params[member], effect);

            } else {
                getter(params, member, params[member], effect);
            }

            return crypto.subtle.verify(params, pub, signature, data);
        },
        () => crypto.subtle.verify(signParams(name), pub,
                                   (target === 'signature') ? empty()
                                                            : signed.slice(),
                                   (target === 'data') ? empty()
                                                       : bytes(32, 4)));
}

function unwrapParams(name) {
    return (name === 'AES-GCM') ? {name, iv: bytes(12, 3)} : {name};
}

async function unwrapData(name, target) {
    let wrapping = await crypto.subtle.importKey('raw', bytes(16), name, false,
                                                ['wrapKey', 'unwrapKey']);
    let key = await crypto.subtle.importKey('raw', bytes(16, 9), 'AES-CBC',
                                           true, ['encrypt']);
    let wrapped = new Uint8Array(await crypto.subtle.wrapKey('raw', key,
                                         wrapping, unwrapParams(name)));

    let unwrap = (data, params, algorithm, usages) =>
        crypto.subtle.unwrapKey('raw', data, wrapping, params, algorithm,
                                true, usages)
        .then(k => crypto.subtle.exportKey('raw', k));

    await same(`unwrapKey ${name} wrappedKey invalidated by ${target}`,
        () => {
            let data = wrapped.slice();
            let params = unwrapParams(name);
            let algorithm = {name: 'AES-CBC'};
            let usages = ['encrypt'];
            let effect = () => invalidate(data.buffer);

            switch (target) {
            case 'unwrapAlgorithm':
                getter(params, 'name', name, effect);
                break;

            case 'unwrappedKeyAlgorithm':
                getter(algorithm, 'name', 'AES-CBC', effect);
                break;

            default:
                getter(usages, 0, 'encrypt', effect);
                break;
            }

            return unwrap(data, params, algorithm, usages);
        },
        () => unwrap(empty(), unwrapParams(name), {name: 'AES-CBC'},
                     ['encrypt']));
}

async function ecdhPublic() {
    let alg = {name: 'ECDH', namedCurve: 'P-256'};
    let pair = await crypto.subtle.generateKey(alg, true, ['deriveBits']);
    let raw = await crypto.subtle.exportKey('raw', pair.publicKey);
    let box = {key: await crypto.subtle.importKey('raw', raw, alg, true, [])};

    /* Drop the references the async frame keeps to awaited values. */

    for (let i = 0; i < 5; i++) {
        await null;
    }

    await same('deriveBits ECDH temporary public key',
        () => {
            let params = {name: 'ECDH'};

            Object.defineProperty(params, 'public', {
                get() {
                    let k = box.key;

                    box.key = null;

                    if (typeof $262 !== 'undefined'
                        && typeof $262.gc === 'function')
                    {
                        $262.gc();
                    }

                    return k;
                }
            });

            return crypto.subtle.deriveBits(params, pair.privateKey, 256);
        },
        async () => {
            let pub = await crypto.subtle.importKey('raw', raw, alg, true, []);

            return crypto.subtle.deriveBits({name: 'ECDH', public: pub},
                                            pair.privateKey, 256);
        });
}

async function test() {
    if (!has_webcrypto()
        || (typeof njs !== 'undefined' && njs.engine !== 'QuickJS')
        || (!is_detach_available()
            && typeof ArrayBuffer.prototype.transfer !== 'function'))
    {
        return;
    }

    let tests = [];

    /*
     * Node.js reads BufferSource members lazily, so copy-on-read of
     * algorithm members is only checked for njs.
     */
    let copyOnRead = (typeof njs !== 'undefined');

    [false, true].forEach(decrypt => {
        ['AES-GCM', 'AES-CTR', 'AES-CBC'].forEach(name => {
            Object.keys(cipherParams(name)).forEach(member => {
                let numeric = (member === 'length' || member === 'tagLength');

                tests.push(() => cipherData(name, member, false, decrypt));

                if (numeric) {
                    tests.push(() => cipherData(name, member, true, decrypt));
                }
            });
        });
    });

    [false, true].forEach(mutate => {
        if (copyOnRead) {
            tests.push(() => cipherMember('AES-GCM', 'iv', 'tagLength',
                                          mutate));
            tests.push(() => cipherMember('AES-GCM', 'additionalData', 'iv',
                                          mutate));
            tests.push(() => cipherMember('AES-CTR', 'counter', 'length',
                                          mutate));
        }

        tests.push(() => cipherMember('AES-GCM', 'iv', 'additionalData',
                                      mutate));
    });

    tests.push(() => cipherTemporary('AES-GCM', 'iv'));
    tests.push(() => cipherTemporary('AES-GCM', 'additionalData'));
    tests.push(() => cipherTemporary('AES-CTR', 'counter'));
    tests.push(() => cipherTemporary('AES-CBC', 'iv'));
    tests.push(() => cipherResize());

    /* Node.js rejects a detached salt or info instead of using no bytes. */

    if (copyOnRead) {
        tests.push(() => deriveMember('PBKDF2', 'salt', 'hash', false));
        tests.push(() => deriveMember('PBKDF2', 'salt', 'iterations', false));
        tests.push(() => deriveMember('PBKDF2', 'salt', 'iterations', true));
        tests.push(() => deriveMember('HKDF', 'salt', 'info', false));
        tests.push(() => deriveMember('HKDF', 'info', 'hash', false));
        tests.push(() => deriveMember('HKDF', 'info', 'salt', false));
    }

    tests.push(() => deriveTemporary('PBKDF2'));
    tests.push(() => deriveTemporary('HKDF'));

    tests.push(() => digestDetached());

    tests.push(() => importData('HMAC', 'raw', 'hash'));
    tests.push(() => importData('ECDSA', 'raw', 'namedCurve'));
    tests.push(() => importData('ECDSA', 'spki', 'namedCurve'));
    tests.push(() => importData('RSA-PSS', 'spki', 'hash'));

    tests.push(() => signData('HMAC', 'name', false));
    tests.push(() => signData('ECDSA', 'hash', false));
    tests.push(() => signData('RSA-PSS', 'saltLength', false));
    tests.push(() => signData('RSA-PSS', 'saltLength', true));
    tests.push(() => verifyData('HMAC', 'name', 'data', false));
    tests.push(() => verifyData('HMAC', 'name', 'signature', false));
    tests.push(() => verifyData('ECDSA', 'hash', 'data', false));
    tests.push(() => verifyData('ECDSA', 'hash', 'signature', false));
    tests.push(() => verifyData('RSA-PSS', 'saltLength', 'data', true));
    tests.push(() => verifyData('RSA-PSS', 'saltLength', 'signature', true));

    ['unwrapAlgorithm', 'unwrappedKeyAlgorithm', 'usages'].forEach(target => {
        tests.push(() => unwrapData('AES-GCM', target));
    });

    if (await has_webcrypto_algorithm('AES-KW')) {
        tests.push(() => unwrapData('AES-KW', 'usages'));
    }

    tests.push(() => ecdhPublic());

    for (let i = 0; i < tests.length; i++) {
        await tests[i]();
    }
}

test().then($DONE, $DONE);

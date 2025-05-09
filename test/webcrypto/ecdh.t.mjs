/*---
includes: [compatBuffer.js, compatWebcrypto.js, webCryptoUtils.js, runTsuite.js]
flags: [async]
---*/

async function testDeriveBits(params) {
    let aliceKeyPair = await crypto.subtle.generateKey({ name: "ECDH", namedCurve: params.curve },
                                                       true, ["deriveKey", "deriveBits"]);

    let bobKeyPair = await crypto.subtle.generateKey({ name: "ECDH", namedCurve: params.curve },
                                                     true, ["deriveKey", "deriveBits"]);

    let ecdhParams = { name: "ECDH", public: bobKeyPair.publicKey };

    let result = await crypto.subtle.deriveBits(ecdhParams, aliceKeyPair.privateKey, params.length);

    if (!result || !result.byteLength) {
        throw Error(`ECDH derivation failed - no result`);
    }

    let ecdhParamsReverse = { name: "ECDH", public: aliceKeyPair.publicKey };

    let secondResult = await crypto.subtle.deriveBits(ecdhParamsReverse, bobKeyPair.privateKey, params.length);

    if (result.byteLength * 8 < params.length || secondResult.byteLength * 8 < params.length) {
        throw Error("ECDH key agreement failed - derived bits too short");
    }

    const firstBits = new Uint8Array(result);
    const secondBits = new Uint8Array(secondResult);

    if (firstBits.length !== secondBits.length) {
        throw Error(`ECDH symmetry test failed: result lengths differ ${firstBits.length} vs ${secondBits.length}`);
    }

    for (let i = 0; i < firstBits.length; i++) {
        if (firstBits[i] !== secondBits[i]) {
            throw Error(`ECDH symmetry test failed: derived bits differ at position ${i}`);
        }
    }

    return "SUCCESS";
}

let ecdh_bits_tsuite = {
    name: "ECDH-DeriveBits",
    skip: () => (!has_buffer() || !has_webcrypto()),
    T: testDeriveBits,
    opts: {
        curve: "P-256",
        length: 256
    },
    tests: [
        { curve: "P-256" },
        { curve: "P-384", length: 384 },
        { curve: "P-521", length: 528 },
    ]
};

function compareKeyData(key1, key2) {
    if (key1.length !== key2.length) {
        return false;
    }

    for (let i = 0; i < key2.length; i++) {
        if (key1[i] !== key2[i]) {
            return false;
        }
    }

    return true;
}

async function testDeriveKey(params) {
    let aliceKeyPair = await crypto.subtle.generateKey({ name: "ECDH", namedCurve: params.curve },
                                                       true, ["deriveKey", "deriveBits"]);

    let bobKeyPair = await crypto.subtle.generateKey({ name: "ECDH", namedCurve: params.curve },
                                                     true, ["deriveKey", "deriveBits"]);
    let eveKeyPair = await crypto.subtle.generateKey({ name: "ECDH", namedCurve: params.curve },
                                                     true, ["deriveKey", "deriveBits"]);

    let ecdhParamsAlice = { name: "ECDH", public: bobKeyPair.publicKey };
    let ecdhParamsBob = { name: "ECDH", public: aliceKeyPair.publicKey };
    let ecdhParamsEve = { name: "ECDH", public: eveKeyPair.publicKey };

    let derivedAlgorithm = { name: params.derivedAlgorithm.name };

    if (params.derivedAlgorithm.length) {
        derivedAlgorithm.length = params.derivedAlgorithm.length;
    }

    if (params.derivedAlgorithm.name === "HMAC") {
        if (typeof params.derivedAlgorithm.hash === 'string') {
            derivedAlgorithm.hash = { name: params.derivedAlgorithm.hash };

        } else {
            derivedAlgorithm.hash = params.derivedAlgorithm.hash;
        }
    }

    let aliceDerivedKey = await crypto.subtle.deriveKey(ecdhParamsAlice, aliceKeyPair.privateKey,
                                                        derivedAlgorithm, params.extractable, params.usage);

    if (aliceDerivedKey.extractable !== params.extractable) {
        throw Error(`ECDH extractable test failed: ${params.extractable} vs ${aliceDerivedKey.extractable}`);
    }

    if (compareUsage(aliceDerivedKey.usages, params.usage) !== true) {
        throw Error(`ECDH usage test failed: ${params.usage} vs ${aliceDerivedKey.usages}`);
    }

    let bobDerivedKey = await crypto.subtle.deriveKey(ecdhParamsBob, bobKeyPair.privateKey,
                                                      derivedAlgorithm, params.extractable, params.usage);

    let eveDerivedKey = await crypto.subtle.deriveKey(ecdhParamsEve, eveKeyPair.privateKey,
                                                      derivedAlgorithm, params.extractable, params.usage);

    if (params.extractable &&
        (params.derivedAlgorithm.name === "AES-GCM"
         || params.derivedAlgorithm.name === "AES-CBC"
         || params.derivedAlgorithm.name === "AES-CTR"
         || params.derivedAlgorithm.name === "HMAC"))
    {
        const aliceRawKey = await crypto.subtle.exportKey("raw", aliceDerivedKey);
        const bobRawKey = await crypto.subtle.exportKey("raw", bobDerivedKey);
        const eveRawKey = await crypto.subtle.exportKey("raw", eveDerivedKey);

        const aliceKeyData = new Uint8Array(aliceRawKey);
        const bobKeyData = new Uint8Array(bobRawKey);
        const eveKeyData = new Uint8Array(eveRawKey);

        if (compareKeyData(aliceKeyData, bobKeyData) !== true) {
            if (aliceKeyData.length !== bobKeyData.length) {
                throw Error(`ECDH key symmetry test failed: keys differ in length ${aliceKeyData.length} vs ${bobKeyData.length}`);
            }

            for (let i = 0; i < aliceKeyData.length; i++) {
                if (aliceKeyData[i] !== bobKeyData[i]) {
                    throw Error(`ECDH key symmetry test failed: keys differ at position ${i}`);
                }
            }
        }

        if (compareKeyData(aliceKeyData, eveKeyData) === true) {
            throw Error(`ECDH key symmetry test failed: keys are equal`);
        }
    }

    return "SUCCESS";
}

let ecdh_key_tsuite = {
    name: "ECDH-DeriveKey",
    skip: () => (!has_buffer() || !has_webcrypto()),
    T: testDeriveKey,
    opts: {
        curve: "P-256",
        extractable: true,
        derivedAlgorithm: {
            name: "AES-GCM",
            length: 256
        },
        usage: ["encrypt", "decrypt"]
    },
    tests: [
        { curve: "P-256" },
        { curve: "P-256", extractable: false },
        { curve: "P-256", derivedAlgorithm: { name: "AES-CBC", length: 256 } },
        { curve: "P-256", derivedAlgorithm: { name: "AES-CTR", length: 256 } },
        { curve: "P-256", derivedAlgorithm: { name: "HMAC", hash: "SHA-256", length: 256 },
          usage: ["sign", "verify"] },
        { curve: "P-384" },
        { curve: "P-384", extractable: false },
        { curve: "P-521" },
        { curve: "P-256", derivedAlgorithm: { name: "AES-GCM", length: 256 } },
        { curve: "P-256", derivedAlgorithm: { name: "HMAC", hash: "SHA-384", length: 256 },
          usage: ["sign", "verify"] }
    ]
};

run([
    ecdh_bits_tsuite,
    ecdh_key_tsuite,
])
.then($DONE, $DONE);

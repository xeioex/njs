/*---
includes: [compareArray.js, compatFs.js]
flags: [async]
---*/

var fname = 'build/test/fs_promises_01';

let stages = [];

Promise.resolve()
.then(() => {
    return fsp.writeFile(fname, fname);
})
.then(data => {
    stages.push('init');
    assert.sameValue(data, undefined, 'init');
})
.then(() => {
    return fsp.readFile(fname).then(fsp.readFile);
})
.then(data => {
    stages.push('short circut');
    assert.sameValue(data.toString(), fname, 'short circut');
})
.then(() => {
    var read = fsp.readFile.bind(fsp, fname, 'utf8');
    var write = fsp.writeFile.bind(fsp, fname);
    var append = fsp.appendFile.bind(fsp, fname);

    return write(fname).then(read).then(append).then(read);
})
.then(data => {
    stages.push('chain');
    assert.sameValue(data, fname + fname, 'chain');
})
.then(() => {
    stages.push('errors ok');
})
.then(() => {
    assert.compareArray(stages, ["init", "short circut", "chain", "errors ok"]);
})
.then($DONE, $DONE);
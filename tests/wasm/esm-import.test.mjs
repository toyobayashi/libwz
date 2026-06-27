import assert from 'node:assert/strict'
import * as wz from '../../dist/index.js'

assert.equal(typeof wz.init, 'function')
assert.equal(typeof wz.getWzBindingType, 'function')
assert.equal(typeof wz.WzFile, 'function')
assert.equal(typeof wz.createWzApi, 'undefined')
assert.equal(typeof wz.createWzApiSync, 'undefined')

assert.equal(wz.getWzBindingType(), 'native')
await wz.init()
assert.equal(wz.getWzBindingType(), 'native')

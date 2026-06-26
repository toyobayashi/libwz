import assert from 'node:assert/strict'
import fs from 'node:fs'
import test from 'node:test'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import * as wz from '../../dist/index.js'

const root = path.dirname(path.dirname(path.dirname(fileURLToPath(import.meta.url))))
const sample = path.join(
  root,
  'Harepacker-resurrected',
  'UnitTest_WzFile',
  'WzFiles',
  'Common',
  'TamingMob_GMS_87.wz'
)

test('package has one flattened async entry and no browser-main or sync artifacts', async () => {
  const dist = path.join(root, 'dist')
  const entry = fs.readFileSync(path.join(dist, 'index.js'), 'utf8')

  assert.doesNotMatch(entry, /createWzApiSync|sync-loader|browser-main|worker\.js/)
  assert.equal(fs.existsSync(path.join(dist, 'wasm')), false)
  assert.equal(fs.existsSync(path.join(dist, 'browser-main.js')), false)
  assert.equal(fs.existsSync(path.join(dist, 'worker.js')), false)
  assert.equal(fs.existsSync(path.join(dist, 'sync-loader.js')), false)
  assert.equal(fs.existsSync(path.join(dist, 'libwz-sync.js')), false)
  assert.equal(fs.existsSync(path.join(dist, 'libwz-sync.wasm')), false)
  assert.equal(fs.existsSync(path.join(dist, 'libwz-browser.js')), false)
  assert.equal(fs.existsSync(path.join(dist, 'libwz-browser.wasm')), false)
  assert.deepEqual(
    fs.readdirSync(dist).filter((entry) => entry !== '.DS_Store').sort(),
    ['index.d.ts', 'index.js', 'libwz.js', 'libwz.wasm']
  )

  await assert.rejects(import('../../dist/browser-main.js'), /Cannot find module|not defined/)
})

test('root API can switch from native to wasm binding', async () => {
  assert.equal(typeof wz.init, 'function')
  assert.equal(wz.getWzBindingType(), 'native')
  assert.equal(wz.MapleVersion.GMS, 0)
  assert.equal(wz.ParseStatus.SUCCESS, 1)
  await wz.init({ forceWasm: true })
  assert.equal(wz.getWzBindingType(), 'wasm')
})

test('wasm API opens path input in Node.js after async init', async () => {
  await wz.init(new URL('../../dist/libwz.wasm', import.meta.url), {
    mount: {
      hostPath: root,
      wasmPath: '/repo'
    }
  })
  using file = new wz.WzFile(
    `/repo/${path.relative(root, sample)}`,
    wz.MapleVersion.GMS
  )
  assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS)
  assert.equal(file.getWzDirectory()?.getName(), 'TamingMob_GMS_87.wz')
})

test('wasm API opens Blob input through the native read callback', async () => {
  const bytes = fs.readFileSync(sample)
  class TestBlobSlice {
    constructor (bytes, start, end) {
      this.bytes = bytes
      this.start = start
      this.end = end
    }
  }
  class TestBlob {
    constructor (bytes, start = 0, end = bytes.byteLength) {
      this.bytes = bytes
      this.start = start
      this.end = end
      this.size = end - start
    }
    slice (start = 0, end = this.size) {
      return new TestBlobSlice(
        this.bytes,
        this.start + Math.trunc(start),
        this.start + Math.min(Math.trunc(end), this.size)
      )
    }
  }
  globalThis.FileReaderSync = class {
    readAsArrayBuffer (slice) {
      return slice.bytes.buffer.slice(
        slice.bytes.byteOffset + slice.start,
        slice.bytes.byteOffset + slice.end
      )
    }
  }
  await wz.init()
  using file = wz.WzFile.fromBlob(
    'TamingMob_GMS_87.wz',
    new TestBlob(bytes),
    wz.MapleVersion.GMS
  )
  assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS)
  assert.equal(file.getWzDirectory()?.getName(), 'TamingMob_GMS_87.wz')
  delete globalThis.FileReaderSync
})

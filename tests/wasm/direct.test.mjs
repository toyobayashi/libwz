import assert from 'node:assert/strict'
import fs from 'node:fs'
import os from 'node:os'
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

const customWz = path.join(root, 'tests', 'Custom.wz')

function findFirstCanvas (props) {
  for (const prop of props) {
    if (prop instanceof wz.WzCanvasProperty) return prop
    const found = findFirstCanvas(prop.wzProperties())
    if (found !== null) return found
  }
  return null
}

function findFirstCanvasInDirectory (directory) {
  for (const image of directory.wzImages()) {
    image.parseImage()
    const canvas = findFirstCanvas(image.wzProperties())
    if (canvas !== null) return canvas
  }
  for (const child of directory.wzDirectories()) {
    const canvas = findFirstCanvasInDirectory(child)
    if (canvas !== null) return canvas
  }
  return null
}

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
  await wz.init(new URL('../../dist/libwz.wasm', import.meta.url))
  using file = new wz.WzFile(sample, wz.MapleVersion.GMS)
  assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS)
  assert.equal(file.getWzDirectory()?.getName(), 'TamingMob_GMS_87.wz')
})

test('wasm property saveToFile APIs match the supported export surface', async () => {
  await wz.init({ forceWasm: true })
  assert.equal(Object.hasOwn(wz.WzStringProperty.prototype, 'saveToFile'), false)
  assert.equal(Object.hasOwn(wz.WzPngProperty.prototype, 'saveToFile'), true)
  assert.equal(Object.hasOwn(wz.WzCanvasProperty.prototype, 'saveToFile'), true)
  assert.equal(Object.hasOwn(wz.WzLuaProperty.prototype, 'saveToFile'), false)
  assert.equal(Object.hasOwn(wz.WzBinaryProperty.prototype, 'saveToFile'), true)
  assert.equal(Object.hasOwn(wz.WzRawDataProperty.prototype, 'saveToFile'), false)
  assert.equal(Object.hasOwn(wz.WzVideoProperty.prototype, 'saveToFile'), false)
})

test('wasm canvas saveToFile exports a PNG image', async () => {
  await wz.init({ forceWasm: true })
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'libwz-wasm-canvas-save-'))
  const outPath = path.join(tmpDir, 'canvas.png')
  using file = new wz.WzFile(customWz, wz.MapleVersion.GMS)
  try {
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS)
    const canvas = findFirstCanvasInDirectory(file.getWzDirectory())
    assert.ok(canvas instanceof wz.WzCanvasProperty)
    assert.equal(canvas.saveToFile(outPath), true)
    assert.deepEqual(
      [...fs.readFileSync(outPath).subarray(0, 8)],
      [137, 80, 78, 71, 13, 10, 26, 10]
    )
  } finally {
    fs.rmSync(tmpDir, { recursive: true, force: true })
  }
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

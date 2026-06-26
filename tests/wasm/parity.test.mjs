import assert from 'node:assert/strict'
import path from 'node:path'
import test from 'node:test'
import { fileURLToPath } from 'node:url'
import * as wz from '../../dist/index.js'

const native = wz

const root = path.dirname(path.dirname(path.dirname(fileURLToPath(import.meta.url))))
const sample = path.join(
  root,
  'Harepacker-resurrected',
  'UnitTest_WzFile',
  'WzFiles',
  'Common',
  'TamingMob_GMS_87.wz'
)

function summarizeParsedRoot (file, status, parseStatus) {
  assert.equal(status, parseStatus.SUCCESS)
  const rootDirectory = file.getWzDirectory()
  assert.notEqual(rootDirectory, null)
  const images = rootDirectory.wzImages()
  return {
    status,
    rootName: rootDirectory.getName(),
    imageCount: images.length,
    firstImageName: images[0]?.getName() ?? null
  }
}

test('native and wasm expose matching root summaries', async () => {
  const expected = (() => {
    using file = new native.WzFile(sample, native.MapleVersion.GMS)
    return summarizeParsedRoot(file, file.parseWzFile(), native.ParseStatus)
  })()

  await wz.init(undefined, {
    mount: {
      hostPath: root,
      wasmPath: '/repo'
    }
  })
  const direct = (() => {
    using file = new wz.WzFile(
      `/repo/${path.relative(root, sample)}`,
      wz.MapleVersion.GMS
    )
    return summarizeParsedRoot(file, file.parseWzFile(), wz.ParseStatus)
  })()

  assert.deepEqual(direct, expected)
})

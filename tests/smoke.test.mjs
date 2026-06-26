import assert from 'node:assert/strict'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import test from 'node:test'
import { fileURLToPath } from 'node:url'
import * as wz from '../dist/index.js'

const __dirname = path.dirname(fileURLToPath(import.meta.url))

const sampleWz = path.join(
  __dirname,
  '..',
  'Harepacker-resurrected',
  'UnitTest_WzFile',
  'WzFiles',
  'Common',
  'TamingMob_GMS_87.wz'
)

const commonWzDir = path.join(
  __dirname,
  '..',
  'Harepacker-resurrected',
  'UnitTest_WzFile',
  'WzFiles',
  'Common'
)

const legacyWzFiles = [
  ['TamingMob_000_KMS_359.wz', wz.MapleVersion.BMS],
  ['TamingMob_000_GMS_237.wz', wz.MapleVersion.BMS],
  ['TamingMob_GMS_146.wz', wz.MapleVersion.BMS],
  ['TamingMob_GMS_176.wz', wz.MapleVersion.BMS],
  ['TamingMob_GMS_230.wz', wz.MapleVersion.BMS],
  ['TamingMob_GMS_75.wz', wz.MapleVersion.GMS],
  ['TamingMob_GMS_87.wz', wz.MapleVersion.GMS],
  ['TamingMob_GMS_95.wz', wz.MapleVersion.GMS],
  ['TamingMob_SEA_135.wz', wz.MapleVersion.BMS],
  ['TamingMob_SEA_160.wz', wz.MapleVersion.BMS],
  ['TamingMob_SEA_211.wz', wz.MapleVersion.BMS],
  ['TamingMob_SEA_212.wz', wz.MapleVersion.BMS],
  ['TamingMob_000_SEA218.wz', wz.MapleVersion.BMS],
  ['TamingMob_ThaiMS_3.wz', wz.MapleVersion.BMS],
  ['TamingMob_TMS_113.wz', wz.MapleVersion.EMS],
  ['TMS_113_Item.wz', wz.MapleVersion.EMS]
]

test('exports Java-parallel enums and classes', () => {
  assert.equal(wz.MapleVersion.GMS, 0)
  assert.equal(wz.ParseStatus.SUCCESS, 1)
  assert.equal(wz.PropertyType.RAW, 12)
  assert.equal(typeof wz.WzFile, 'function')
  assert.equal(Symbol.dispose in wz.WzFile.prototype, true)
})

test('opens, parses, and wraps borrowed objects without owning them', () => {
  const file = new wz.WzFile(sampleWz, wz.MapleVersion.GMS)
  try {
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS)
    assert.equal(file.getName(), 'TamingMob_GMS_87.wz')

    const root = file.getWzDirectory()
    assert.ok(root instanceof wz.WzDirectory)
    assert.equal(root.getName(), 'TamingMob_GMS_87.wz')
    assert.equal(typeof root.getOffset(), 'bigint')
    assert.ok(root.countImages() > 0)

    const images = root.wzImages()
    assert.ok(images.length > 0)
    assert.ok(images[0] instanceof wz.WzImage)

    const image = root.getImage(0)
    assert.ok(image instanceof wz.WzImage)
    assert.equal(typeof image.getOffset(), 'bigint')
    image.close()
    assert.ok(root.getImage(0) instanceof wz.WzImage)
  } finally {
    file[Symbol.dispose]()
  }

  assert.throws(() => file.getName(), /disposed/i)
})

test('opens a WZ file from owned bytes', () => {
  const bytes = new Uint8Array(fs.readFileSync(sampleWz))
  const file = wz.WzFile.fromBytes(
    'TamingMob_GMS_87.wz',
    bytes,
    wz.MapleVersion.GMS
  )
  try {
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS)
    assert.equal(file.getName(), 'TamingMob_GMS_87.wz')
  } finally {
    file.close()
  }
})

test('property factory returns concrete property classes', () => {
  const file = new wz.WzFile(sampleWz, wz.MapleVersion.GMS)
  try {
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS)
    const image = file.getWzDirectory().getImage(0)
    image.parseImage()
    const props = image.wzProperties()
    assert.ok(props.length > 0)
    assert.equal(props.get(0), props[0])
    assert.ok(props[0] instanceof wz.WzImageProperty)
    assert.notEqual(props[0].constructor, wz.WzImageProperty)
  } finally {
    file.close()
  }
})

test('WzFile participates in explicit resource management', () => {
  let fileRef
  {
    using file = new wz.WzFile(sampleWz, wz.MapleVersion.GMS)
    fileRef = file
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS)
  }
  assert.throws(() => fileRef.getName(), /disposed/i)
})

test('detectMapleVersion returns maple version and file version', () => {
  const detected = wz.WzTool.detectMapleVersion(sampleWz)
  assert.equal(typeof detected, 'object')
  assert.equal(detected.mapleVersion, wz.MapleVersion.GMS)
  assert.equal(detected.version, 87)
})

test('parses the same legacy WZ files covered by UnitTest_WzFile', () => {
  for (const [fileName, mapleVersion] of legacyWzFiles) {
    const file = new wz.WzFile(path.join(commonWzDir, fileName), mapleVersion)
    try {
      assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS, fileName)
      assert.equal(file.getName(), fileName)
      assert.ok(file.getWzDirectory() instanceof wz.WzDirectory, fileName)
    } finally {
      file.close()
    }
  }
})

test('creates, edits, saves, and reopens a WZ file', () => {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'libwz-node-'))
  const outPath = path.join(tmpDir, 'Task8.wz')
  const file = wz.WzFile.create(87, wz.MapleVersion.GMS)
  try {
    const root = file.getWzDirectory()
    assert.ok(root instanceof wz.WzDirectory)

    const image = root.createImage('Edit.img')
    assert.ok(image instanceof wz.WzImage)
    const value = wz.WzProperty.createInt('answer', 42)
    image.addProperty(value)
    value.setValue(84)

    assert.equal(image.wzProperties().length, 1)
    assert.equal(image.getFromPath('answer').getValue(), 84)
    const duplicate = wz.WzProperty.createInt('answer', 1)
    assert.throws(() => { image.addProperty(duplicate) })
    duplicate.close()

    const detached = wz.WzProperty.createString('detached', 'free me')
    detached.close()

    file.saveToDisk(outPath)
  } finally {
    file.close()
  }

  const reopened = new wz.WzFile(outPath, wz.MapleVersion.GMS)
  try {
    assert.equal(reopened.parseWzFile(), wz.ParseStatus.SUCCESS)
    const image = reopened.getWzDirectory().getImageByName('Edit.img')
    assert.ok(image instanceof wz.WzImage)
    image.parseImage()
    const value = image.getFromPath('answer')
    assert.ok(value instanceof wz.WzIntProperty)
    assert.equal(value.getValue(), 84)
  } finally {
    reopened.close()
    fs.rmSync(tmpDir, { recursive: true, force: true })
  }
})

test('property removal invalidates aliases for the same native handle', () => {
  const file = wz.WzFile.create(87, wz.MapleVersion.GMS)
  try {
    const image = file.getWzDirectory().createImage('Aliases.img')
    const prop = wz.WzProperty.createInt('value', 1)
    image.addProperty(prop)
    const alias = image.getFromPath('value')
    assert.ok(alias instanceof wz.WzIntProperty)

    image.removeProperty(prop)

    assert.throws(() => prop.nativePtr(), /disposed/i)
    assert.throws(() => alias.nativePtr(), /disposed/i)
  } finally {
    file.close()
  }
})

test('clearProperties invalidates known child wrappers', () => {
  const file = wz.WzFile.create(87, wz.MapleVersion.GMS)
  try {
    const image = file.getWzDirectory().createImage('Clear.img')
    const parent = wz.WzProperty.createSub('parent')
    const child = wz.WzProperty.createInt('child', 7)
    parent.addProperty(child)
    image.addProperty(parent)
    const parentAlias = image.getFromPath('parent')
    const childAlias = parentAlias.getChildByName('child')
    assert.ok(childAlias instanceof wz.WzIntProperty)

    image.clearProperties()

    assert.throws(() => parent.nativePtr(), /disposed/i)
    assert.throws(() => parentAlias.nativePtr(), /disposed/i)
    assert.throws(() => child.nativePtr(), /disposed/i)
    assert.throws(() => childAlias.nativePtr(), /disposed/i)
  } finally {
    file.close()
  }
})

test('long property bigints must fit in int64', () => {
  const tooLarge = 1n << 80n
  assert.throws(() => wz.WzProperty.createLong('tooLarge', tooLarge), RangeError)

  const prop = wz.WzProperty.createLong('ok', 1n)
  try {
    assert.throws(() => { prop.setValue(tooLarge) }, RangeError)
  } finally {
    prop.close()
  }
})

test('closing a WZ file invalidates known wrappers in its tree', () => {
  const file = wz.WzFile.create(87, wz.MapleVersion.GMS)
  const root = file.getWzDirectory()
  const image = root.createImage('Close.img')
  const prop = wz.WzProperty.createInt('value', 1)
  image.addProperty(prop)
  const propAlias = image.getFromPath('value')
  assert.ok(propAlias instanceof wz.WzIntProperty)

  file.close()

  assert.throws(() => root.getName(), /disposed/i)
  assert.throws(() => image.getName(), /disposed/i)
  assert.throws(() => prop.getValue(), /disposed/i)
  assert.throws(() => propAlias.getValue(), /disposed/i)
})

test('removeDirectory invalidates known descendant wrappers', () => {
  const file = wz.WzFile.create(87, wz.MapleVersion.GMS)
  try {
    const root = file.getWzDirectory()
    const dir = root.createDirectory('RemoveDir')
    const image = dir.createImage('Nested.img')
    const prop = wz.WzProperty.createInt('value', 2)
    image.addProperty(prop)
    const imageAlias = dir.getImageByName('Nested.img')
    const propAlias = imageAlias.getFromPath('value')
    assert.ok(propAlias instanceof wz.WzIntProperty)

    root.removeDirectory(dir)

    assert.throws(() => dir.getName(), /disposed/i)
    assert.throws(() => image.getName(), /disposed/i)
    assert.throws(() => imageAlias.getName(), /disposed/i)
    assert.throws(() => prop.getValue(), /disposed/i)
    assert.throws(() => propAlias.getValue(), /disposed/i)
  } finally {
    file.close()
  }
})

test('removeImage invalidates known property wrappers', () => {
  const file = wz.WzFile.create(87, wz.MapleVersion.GMS)
  try {
    const root = file.getWzDirectory()
    const image = root.createImage('RemoveImage.img')
    const prop = wz.WzProperty.createInt('value', 3)
    image.addProperty(prop)
    const propAlias = image.getFromPath('value')
    assert.ok(propAlias instanceof wz.WzIntProperty)

    root.removeImage(image)

    assert.throws(() => image.getName(), /disposed/i)
    assert.throws(() => prop.getValue(), /disposed/i)
    assert.throws(() => propAlias.getValue(), /disposed/i)
  } finally {
    file.close()
  }
})

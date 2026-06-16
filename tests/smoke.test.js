const assert = require("node:assert/strict");
const path = require("node:path");
const test = require("node:test");
const wz = require("..");

const sampleWz = path.join(
  __dirname,
  "..",
  "Harepacker-resurrected",
  "UnitTest_WzFile",
  "WzFiles",
  "Common",
  "TamingMob_GMS_87.wz"
);

const commonWzDir = path.join(
  __dirname,
  "..",
  "Harepacker-resurrected",
  "UnitTest_WzFile",
  "WzFiles",
  "Common"
);

const legacyWzFiles = [
  ["TamingMob_000_KMS_359.wz", wz.MapleVersion.BMS],
  ["TamingMob_000_GMS_237.wz", wz.MapleVersion.BMS],
  ["TamingMob_GMS_146.wz", wz.MapleVersion.BMS],
  ["TamingMob_GMS_176.wz", wz.MapleVersion.BMS],
  ["TamingMob_GMS_230.wz", wz.MapleVersion.BMS],
  ["TamingMob_GMS_75.wz", wz.MapleVersion.GMS],
  ["TamingMob_GMS_87.wz", wz.MapleVersion.GMS],
  ["TamingMob_GMS_95.wz", wz.MapleVersion.GMS],
  ["TamingMob_SEA_135.wz", wz.MapleVersion.BMS],
  ["TamingMob_SEA_160.wz", wz.MapleVersion.BMS],
  ["TamingMob_SEA_211.wz", wz.MapleVersion.BMS],
  ["TamingMob_SEA_212.wz", wz.MapleVersion.BMS],
  ["TamingMob_000_SEA218.wz", wz.MapleVersion.BMS],
  ["TamingMob_ThaiMS_3.wz", wz.MapleVersion.BMS],
  ["TamingMob_TMS_113.wz", wz.MapleVersion.EMS],
  ["TMS_113_Item.wz", wz.MapleVersion.EMS],
];

test("exports Java-parallel enums and classes", () => {
  assert.equal(wz.MapleVersion.GMS, 0);
  assert.equal(wz.ParseStatus.SUCCESS, 1);
  assert.equal(wz.PropertyType.RAW, 12);
  assert.equal(typeof wz.WzFile, "function");
  assert.equal(Symbol.dispose in wz.WzFile.prototype, true);
});

test("opens, parses, and wraps borrowed objects without owning them", () => {
  const file = new wz.WzFile(sampleWz, wz.MapleVersion.GMS);
  try {
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS);
    assert.equal(file.getName(), "TamingMob_GMS_87.wz");

    const root = file.getWzDirectory();
    assert.ok(root instanceof wz.WzDirectory);
    assert.equal(root.getName(), "TamingMob_GMS_87.wz");
    assert.ok(root.countImages() > 0);

    const images = root.wzImages();
    assert.ok(images.length > 0);
    assert.ok(images[0] instanceof wz.WzImage);

    const image = root.getImage(0);
    assert.ok(image instanceof wz.WzImage);
    image.close();
    assert.ok(root.getImage(0) instanceof wz.WzImage);
  } finally {
    file[Symbol.dispose]();
  }

  assert.throws(() => file.getName(), /disposed/i);
});

test("property factory returns concrete property classes", () => {
  const file = new wz.WzFile(sampleWz, wz.MapleVersion.GMS);
  try {
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS);
    const image = file.getWzDirectory().getImage(0);
    image.parseImage();
    const props = image.wzProperties();
    assert.ok(props.length > 0);
    assert.equal(props.get(0), props[0]);
    assert.ok(props[0] instanceof wz.WzImageProperty);
    assert.notEqual(props[0].constructor, wz.WzImageProperty);
  } finally {
    file.close();
  }
});

test("WzFile participates in explicit resource management", () => {
  let fileRef;
  {
    using file = new wz.WzFile(sampleWz, wz.MapleVersion.GMS);
    fileRef = file;
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS);
  }
  assert.throws(() => fileRef.getName(), /disposed/i);
});

test("parses the same legacy WZ files covered by UnitTest_WzFile", () => {
  for (const [fileName, mapleVersion] of legacyWzFiles) {
    const file = new wz.WzFile(path.join(commonWzDir, fileName), mapleVersion);
    try {
      assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS, fileName);
      assert.equal(file.getName(), fileName);
      assert.ok(file.getWzDirectory() instanceof wz.WzDirectory, fileName);
    } finally {
      file.close();
    }
  }
});

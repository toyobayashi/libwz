# WZ Editing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement MapleLib-compatible WZ node editing and `.wz` repacking in C++, then expose it through C API, JNI, and NodeAPI.

**Architecture:** Build the save pipeline from the bottom up: binary writer primitives, property serialization, image serialization, directory/file repacking, then public editing APIs. Each C++ save/edit function must map to a named MapleLib method, with short source-reference comments such as `// MapleLib: WzFile.SaveToDisk` where useful.

**Tech Stack:** C++23, CMake, Google Test, C ABI, JNI, NodeAPI, cpplint, existing `Result<T, Error>` error model.

---

## File Map

- Create `include/wz/Util/WzBinaryWriter.h`: C++ writer counterpart for MapleLib `Util/WzBinaryWriter.cs`.
- Create `src/Util/WzBinaryWriter.cpp`: encrypted string writing, compressed integers, cached string/object writing, offset encoding.
- Modify `CMakeLists.txt`: add writer source to `wz`.
- Modify `include/wz/WzImageProperty.h`: add virtual `WriteValue(WzBinaryWriter*)` and static `WritePropertyList`.
- Modify `src/WzImageProperty.cpp`: implement property list writing dispatch.
- Modify every file in `include/wz/Properties/*.h` and `src/Properties/*.cpp`: implement `WriteValue` and scalar setters where missing.
- Modify `include/wz/WzImage.h` and `src/WzImage.cpp`: add `SaveImage`, checksum calculation, temporary block metadata, changed propagation.
- Modify `include/wz/WzDirectory.h` and `src/WzDirectory.cpp`: add `GenerateDataFile`, `SaveDirectory`, `SaveImages`, offset helpers, safer edit methods.
- Modify `include/wz/WzFile.h` and `src/WzFile.cpp`: add `SaveToDisk`.
- Modify `include/wz/wz_api.h` and `src/capi/wz_api.cpp`: expose edit/save constructors and mutations.
- Modify Java wrappers under `java/src/main/java/io/github/toyobayashi/libwz/` and `src/jni/wz_jni.cpp`: expose same edit/save surface.
- Modify `src/node/binding.cpp` and `index.ts`: expose NodeAPI editing.
- Add tests:
  - `tests/TestWzBinaryWriter.cpp`
  - `tests/TestEditing.cpp`
  - `tests/TestRepack.cpp`
  - extend `tests/smoke.test.js`
  - extend Java tests if native library loading is available.

## MapleLib Traceability Rule

For each implementation task, inspect the referenced C# file immediately before writing code:

```powershell
Get-Content Harepacker-resurrected\MapleLib\MapleLib\WzLib\Util\WzBinaryWriter.cs
Get-Content Harepacker-resurrected\MapleLib\MapleLib\WzLib\WzImage.cs
Get-Content Harepacker-resurrected\MapleLib\MapleLib\WzLib\WzDirectory.cs
Get-Content Harepacker-resurrected\MapleLib\MapleLib\WzLib\WzFile.cs
```

Do not copy C# code text. Reimplement the algorithm in C++ and include compact source-reference comments only where the algorithm is non-obvious.

### Task 1: Add WzBinaryWriter Primitives

**C# Reference:**
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/Util/WzBinaryWriter.cs`
- `Harepacker-resurrected/MapleLib/MapleLib/Helpers/ByteUtils.cs`

**Files:**
- Create: `include/wz/Util/WzBinaryWriter.h`
- Create: `src/Util/WzBinaryWriter.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/TestWzBinaryWriter.cpp`

- [ ] **Step 1: Write failing writer primitive tests**

Create `tests/TestWzBinaryWriter.cpp`:

```cpp
#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzAESConstant.h"
#include "wz/WzHeader.h"

namespace {

std::vector<uint8_t> Bytes(const std::string& s) {
  return std::vector<uint8_t>(s.begin(), s.end());
}

}  // namespace

TEST(WzBinaryWriter, WritesCompressedIntLikeMapleLib) {
  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  wz::WzBinaryWriter writer(stream, wz::WzAESConstant::GMSIv);

  writer.WriteCompressedInt(0x7f);
  writer.WriteCompressedInt(0x80);
  writer.WriteCompressedInt(-128);
  writer.WriteCompressedInt(-129);

  const std::string out = stream.str();
  std::vector<uint8_t> actual(out.begin(), out.end());
  std::vector<uint8_t> expected = {
      0x7f,
      0x80, 0x80, 0x00, 0x00, 0x00,
      0x80, 0x80, 0xff, 0xff, 0xff,
      0x80, 0x7f, 0xff, 0xff, 0xff,
  };
  EXPECT_EQ(actual, expected);
}

TEST(WzBinaryWriter, WritesCompressedLongLikeMapleLib) {
  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  wz::WzBinaryWriter writer(stream, wz::WzAESConstant::GMSIv);

  writer.WriteCompressedLong(0x7f);
  writer.WriteCompressedLong(0x80);

  const std::string out = stream.str();
  std::vector<uint8_t> actual(out.begin(), out.end());
  std::vector<uint8_t> expected = {
      0x7f,
      0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  EXPECT_EQ(actual, expected);
}

TEST(WzBinaryWriter, StringRoundTripsThroughReader) {
  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  wz::WzBinaryWriter writer(stream, wz::WzAESConstant::GMSIv);
  writer.WriteString("Property");

  stream.seekg(0);
  wz::WzBinaryReader reader(stream, wz::WzAESConstant::GMSIv);
  EXPECT_EQ(reader.ReadString(), "Property");
}

TEST(WzBinaryWriter, WriteStringValueUsesCachedOffsetForLongStrings) {
  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  wz::WzBinaryWriter writer(stream, wz::WzAESConstant::GMSIv);

  writer.WriteStringValue("Property", 0x73, 0x1b);
  const size_t first_size = stream.str().size();
  writer.WriteStringValue("Property", 0x73, 0x1b);

  const std::string out = stream.str();
  ASSERT_GT(out.size(), first_size + 1);
  EXPECT_EQ(static_cast<uint8_t>(out[first_size]), 0x1b);
}

TEST(WzBinaryWriter, WriteWzObjectValueUsesCachedOffsetWithHeaderStart) {
  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  wz::WzHeader header = wz::WzHeader::GetDefault();
  header.SetFStart(60);
  wz::WzBinaryWriter writer(stream, wz::WzAESConstant::GMSIv);
  writer.SetHeader(&header);

  writer.WriteWzObjectValue("LongImageName.img", wz::WzDirectoryType::WzImage_4);
  const size_t first_size = stream.str().size();
  writer.WriteWzObjectValue("LongImageName.img", wz::WzDirectoryType::WzImage_4);

  const std::string out = stream.str();
  ASSERT_GT(out.size(), first_size + 1);
  EXPECT_EQ(static_cast<uint8_t>(out[first_size]),
            static_cast<uint8_t>(wz::WzDirectoryType::RetrieveStringFromOffset_2));
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=ON
cmake --build build
ctest --test-dir build -R WzBinaryWriter --output-on-failure
```

Expected: build fails because `wz/Util/WzBinaryWriter.h` does not exist.

- [ ] **Step 3: Add writer header**

Create `include/wz/Util/WzBinaryWriter.h`:

```cpp
#ifndef WZ_UTIL_WZBINARYWRITER_H_
#define WZ_UTIL_WZBINARYWRITER_H_

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "wz/Result.h"
#include "wz/Util/Defines.h"
#include "wz/Util/WzMutableKey.h"
#include "wz/WzEnums.h"
#include "wz/WzHeader.h"

namespace wz {

class WzBinaryWriter {
 public:
  WzBinaryWriter(std::ostream& output, const std::array<uint8_t, 4>& wzIv);
  WZ_DISALLOW_COPY_AND_MOVE(WzBinaryWriter)

  uint32_t Hash() const { return hash_; }
  void SetHash(uint32_t hash) { hash_ = hash; }
  WzHeader* Header() const { return header_; }
  void SetHeader(WzHeader* header) { header_ = header; }
  int64_t Position() const;
  bool Good() const { return output_.good(); }
  void ClearStringCache() { stringCache_.clear(); }

  void WriteByte(uint8_t value);
  void WriteInt16(int16_t value);
  void WriteUInt16(uint16_t value);
  void WriteInt32(int32_t value);
  void WriteUInt32(uint32_t value);
  void WriteInt64(int64_t value);
  void WriteUInt64(uint64_t value);
  void WriteFloat(float value);
  void WriteDouble(double value);
  void WriteBytes(const std::vector<uint8_t>& bytes);
  void WriteBytes(const uint8_t* bytes, size_t size);
  void WriteString(const std::string& value);
  void WriteNullTerminatedString(const std::string& value);
  void WriteStringValue(const std::string& value,
                        uint8_t withoutOffset,
                        uint8_t withOffset);
  bool WriteWzObjectValue(const std::string& value, WzDirectoryType type);
  void WriteCompressedInt(int32_t value);
  void WriteCompressedLong(int64_t value);
  Result<void> WriteOffset(int64_t value);

 private:
  void WriteAsciiString(const std::string& value);
  void WriteUnicodeString(const std::string& value);
  static uint32_t RotateLeft(uint32_t value, uint8_t count);

  std::ostream& output_;
  WzMutableKey wzKey_;
  uint32_t hash_ = 0;
  WzHeader* header_ = nullptr;
  std::unordered_map<std::string, int32_t> stringCache_;
};

}  // namespace wz

#endif  // WZ_UTIL_WZBINARYWRITER_H_
```

- [ ] **Step 4: Add writer implementation**

Create `src/Util/WzBinaryWriter.cpp`:

```cpp
#include "wz/Util/WzBinaryWriter.h"
#include <cstring>
#include "wz/Util/WzKeyGenerator.h"
#include "wz/WzAESConstant.h"

namespace wz {

WzBinaryWriter::WzBinaryWriter(std::ostream& output,
                               const std::array<uint8_t, 4>& wzIv)
    : output_(output), wzKey_(WzKeyGenerator::GenerateWzKey(wzIv)) {}

int64_t WzBinaryWriter::Position() const {
  return static_cast<int64_t>(output_.tellp());
}

void WzBinaryWriter::WriteByte(uint8_t value) {
  output_.put(static_cast<char>(value));
}

void WzBinaryWriter::WriteInt16(int16_t value) {
  WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void WzBinaryWriter::WriteUInt16(uint16_t value) {
  WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void WzBinaryWriter::WriteInt32(int32_t value) {
  WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void WzBinaryWriter::WriteUInt32(uint32_t value) {
  WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void WzBinaryWriter::WriteInt64(int64_t value) {
  WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void WzBinaryWriter::WriteUInt64(uint64_t value) {
  WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void WzBinaryWriter::WriteFloat(float value) {
  WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void WzBinaryWriter::WriteDouble(double value) {
  WriteBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void WzBinaryWriter::WriteBytes(const std::vector<uint8_t>& bytes) {
  WriteBytes(bytes.data(), bytes.size());
}

void WzBinaryWriter::WriteBytes(const uint8_t* bytes, size_t size) {
  if (size == 0) return;
  output_.write(reinterpret_cast<const char*>(bytes),
                static_cast<std::streamsize>(size));
}

void WzBinaryWriter::WriteString(const std::string& value) {
  if (value.empty()) {
    WriteByte(0);
    return;
  }
  bool unicode = false;
  for (unsigned char ch : value) {
    if (ch > 0x7f) {
      unicode = true;
      break;
    }
  }
  if (unicode) {
    WriteUnicodeString(value);
  } else {
    WriteAsciiString(value);
  }
}

void WzBinaryWriter::WriteAsciiString(const std::string& value) {
  if (value.size() > 0x7f) {
    WriteByte(0x80);
    WriteInt32(static_cast<int32_t>(value.size()));
  } else {
    WriteByte(static_cast<uint8_t>(-static_cast<int8_t>(value.size())));
  }
  uint8_t mask = 0xaa;
  for (size_t i = 0; i < value.size(); i++) {
    uint8_t encrypted = static_cast<uint8_t>(value[i]);
    encrypted ^= wzKey_[i];
    encrypted ^= mask;
    mask++;
    WriteByte(encrypted);
  }
}

void WzBinaryWriter::WriteUnicodeString(const std::string& value) {
  std::u16string utf16;
  for (unsigned char ch : value) utf16.push_back(static_cast<char16_t>(ch));
  if (utf16.size() >= 0x7f) {
    WriteByte(0x7f);
    WriteInt32(static_cast<int32_t>(utf16.size()));
  } else {
    WriteByte(static_cast<uint8_t>(utf16.size()));
  }
  uint16_t mask = 0xaaaa;
  for (size_t i = 0; i < utf16.size(); i++) {
    uint16_t encrypted = static_cast<uint16_t>(utf16[i]);
    encrypted ^= static_cast<uint16_t>((wzKey_[i * 2 + 1] << 8) + wzKey_[i * 2]);
    encrypted ^= mask;
    mask++;
    WriteUInt16(encrypted);
  }
}

void WzBinaryWriter::WriteNullTerminatedString(const std::string& value) {
  output_.write(value.data(), static_cast<std::streamsize>(value.size()));
  WriteByte(0);
}

void WzBinaryWriter::WriteStringValue(const std::string& value,
                                      uint8_t withoutOffset,
                                      uint8_t withOffset) {
  auto found = stringCache_.find(value);
  if (value.size() > 4 && found != stringCache_.end()) {
    WriteByte(withOffset);
    WriteInt32(found->second);
    return;
  }
  WriteByte(withoutOffset);
  int32_t offset = static_cast<int32_t>(Position());
  WriteString(value);
  stringCache_.emplace(value, offset);
}

bool WzBinaryWriter::WriteWzObjectValue(const std::string& value,
                                        WzDirectoryType type) {
  std::string key = std::to_string(static_cast<uint8_t>(type)) + "_" + value;
  auto found = stringCache_.find(key);
  if (value.size() > 4 && found != stringCache_.end()) {
    WriteByte(static_cast<uint8_t>(WzDirectoryType::RetrieveStringFromOffset_2));
    WriteInt32(found->second);
    return true;
  }
  int64_t fstart = header_ ? static_cast<int64_t>(header_->FStart()) : 0;
  int32_t offset = static_cast<int32_t>(Position() - fstart);
  WriteByte(static_cast<uint8_t>(type));
  WriteString(value);
  stringCache_.emplace(std::move(key), offset);
  return false;
}

void WzBinaryWriter::WriteCompressedInt(int32_t value) {
  if (value > 0x7f || value <= -0x80) {
    WriteByte(0x80);
    WriteInt32(value);
  } else {
    WriteByte(static_cast<uint8_t>(static_cast<int8_t>(value)));
  }
}

void WzBinaryWriter::WriteCompressedLong(int64_t value) {
  if (value > 0x7f || value <= -0x80) {
    WriteByte(0x80);
    WriteInt64(value);
  } else {
    WriteByte(static_cast<uint8_t>(static_cast<int8_t>(value)));
  }
}

uint32_t WzBinaryWriter::RotateLeft(uint32_t value, uint8_t count) {
  count &= 31;
  return static_cast<uint32_t>((value << count) | (value >> (32 - count)));
}

Result<void> WzBinaryWriter::WriteOffset(int64_t value) {
  if (!header_) {
    return std::unexpected(Error::InvalidArgument(
        "WzBinaryWriter::WriteOffset requires a WzHeader"));
  }
  uint32_t encOffset = static_cast<uint32_t>(Position());
  encOffset = (encOffset - header_->FStart()) ^ 0xffffffffU;
  encOffset *= hash_;
  encOffset -= WzAESConstant::WZ_OffsetConstant;
  encOffset = RotateLeft(encOffset, static_cast<uint8_t>(encOffset & 0x1f));
  uint32_t writeOffset =
      encOffset ^ (static_cast<uint32_t>(value) - (header_->FStart() * 2));
  WriteUInt32(writeOffset);
  return {};
}

}  // namespace wz
```

- [ ] **Step 5: Add source to CMake**

Modify the main library source list in `CMakeLists.txt` to include:

```cmake
src/Util/WzBinaryWriter.cpp
```

- [ ] **Step 6: Run writer tests**

Run:

```powershell
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=ON
cmake --build build
ctest --test-dir build -R WzBinaryWriter --output-on-failure
```

Expected: writer tests pass. If the Unicode round-trip fails, change only
`WriteUnicodeString` so it emits the UTF-16 byte order consumed by
`WzBinaryReader::ReadString`, then rerun this exact command until it passes.

- [ ] **Step 7: Format, lint, commit**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
git add include\wz\Util\WzBinaryWriter.h src\Util\WzBinaryWriter.cpp CMakeLists.txt tests\TestWzBinaryWriter.cpp
git commit -m "feat: add wz binary writer"
```

Expected: format and lint pass with zero errors; commit succeeds.

### Task 2: Add Property WriteValue Serialization

**C# Reference:**
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/WzImageProperty.cs`
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/WzProperties/*.cs`

**Files:**
- Modify: `include/wz/WzImageProperty.h`
- Modify: `src/WzImageProperty.cpp`
- Modify: `include/wz/Properties/*.h`
- Modify: `src/Properties/*.cpp`
- Test: `tests/TestPropertyWriteValue.cpp`

- [ ] **Step 1: Write failing property serialization tests**

Create `tests/TestPropertyWriteValue.cpp`:

```cpp
#include <sstream>
#include "gtest/gtest.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzAESConstant.h"
#include "wz/WzImageProperty.h"

TEST(PropertyWriteValue, WritesIntPropertyListEntry) {
  wz::WzPropertyCollection props;
  props.Add(std::make_unique<wz::WzIntProperty>("count", 42));

  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  wz::WzBinaryWriter writer(stream, wz::WzAESConstant::GMSIv);
  wz::WzImageProperty::WritePropertyList(&writer, props);

  stream.seekg(0);
  wz::WzBinaryReader reader(stream, wz::WzAESConstant::GMSIv);
  ASSERT_EQ(reader.ReadUInt16(), 0);
  ASSERT_EQ(reader.ReadCompressedInt(), 1);
  ASSERT_EQ(reader.ReadByte(), 0);
  ASSERT_EQ(reader.ReadString(), "count");
  ASSERT_EQ(reader.ReadByte(), 0x03);
  ASSERT_EQ(reader.ReadCompressedInt(), 42);
}

TEST(PropertyWriteValue, WritesSubPropertyHeaderAndChildren) {
  auto sub = std::make_unique<wz::WzSubProperty>("info");
  sub->AddProperty(std::make_unique<wz::WzStringProperty>("name", "abc"));

  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  wz::WzBinaryWriter writer(stream, wz::WzAESConstant::GMSIv);
  sub->WriteValue(&writer);

  stream.seekg(0);
  wz::WzBinaryReader reader(stream, wz::WzAESConstant::GMSIv);
  ASSERT_EQ(reader.ReadByte(), wz::WzImage::WzImageHeaderByte_WithoutOffset);
  ASSERT_EQ(reader.ReadString(), "Property");
  ASSERT_EQ(reader.ReadUInt16(), 0);
  ASSERT_EQ(reader.ReadCompressedInt(), 1);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
cmake --build build
ctest --test-dir build -R PropertyWriteValue --output-on-failure
```

Expected: build fails because `WriteValue` and `WritePropertyList` do not exist.

- [ ] **Step 3: Add base virtual methods**

Modify `include/wz/WzImageProperty.h`:

```cpp
class WzBinaryWriter;

class WzImageProperty : public WzObject {
 public:
  // existing declarations...
  virtual Result<void> WriteValue(WzBinaryWriter* writer) const;
  static Result<void> WritePropertyList(WzBinaryWriter* writer,
                                        const WzPropertyCollection& properties);
};
```

- [ ] **Step 4: Implement property list writing**

Modify `src/WzImageProperty.cpp`:

```cpp
Result<void> WzImageProperty::WriteValue(WzBinaryWriter*) const {
  return std::unexpected(Error::NotImplemented(
      "WriteValue is not implemented for property type " + Name()));
}

Result<void> WzImageProperty::WritePropertyList(
    WzBinaryWriter* writer, const WzPropertyCollection& properties) {
  if (!writer) {
    return std::unexpected(Error::InvalidArgument("writer is null"));
  }
  writer->WriteUInt16(0);
  writer->WriteCompressedInt(static_cast<int32_t>(properties.size()));
  for (auto* prop : properties) {
    writer->WriteStringValue(prop->Name(), 0x00, 0x01);
    auto result = prop->WriteValue(writer);
    if (!result.has_value()) return std::unexpected(result.error());
  }
  return {};
}
```

- [ ] **Step 5: Implement scalar WriteValue methods**

For each scalar header, add `Result<void> WriteValue(WzBinaryWriter* writer) const override;`.

Implement in the matching `.cpp` files using MapleLib type bytes:

```cpp
Result<void> WzIntProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x03);
  writer->WriteCompressedInt(value_);
  return {};
}

Result<void> WzShortProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x02);
  writer->WriteInt16(value_);
  return {};
}

Result<void> WzLongProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x14);
  writer->WriteCompressedLong(value_);
  return {};
}

Result<void> WzFloatProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x04);
  writer->WriteByte(0x80);
  writer->WriteFloat(value_);
  return {};
}

Result<void> WzDoubleProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x05);
  writer->WriteDouble(value_);
  return {};
}

Result<void> WzStringProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x08);
  writer->WriteStringValue(value_, 0x00, 0x01);
  return {};
}

Result<void> WzNullProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x00);
  return {};
}
```

Use each class's actual private value member name from its header.

- [ ] **Step 6: Implement container and extended WriteValue methods**

Add these methods following MapleLib:

```cpp
Result<void> WzSubProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x09);
  writer->WriteStringValue("Property",
                           WzImage::WzImageHeaderByte_WithoutOffset,
                           WzImage::WzImageHeaderByte_WithOffset);
  auto* children = const_cast<WzSubProperty*>(this)->WzProperties();
  return WzImageProperty::WritePropertyList(writer, *children);
}

Result<void> WzVectorProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x09);
  writer->WriteStringValue("Shape2D#Vector2D",
                           WzImage::WzImageHeaderByte_WithoutOffset,
                           WzImage::WzImageHeaderByte_WithOffset);
  writer->WriteCompressedInt(X());
  writer->WriteCompressedInt(Y());
  return {};
}

Result<void> WzUOLProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x09);
  writer->WriteStringValue("UOL",
                           WzImage::WzImageHeaderByte_WithoutOffset,
                           WzImage::WzImageHeaderByte_WithOffset);
  writer->WriteByte(0);
  writer->WriteStringValue(Value(), 0x00, 0x01);
  return {};
}
```

For `WzCanvasProperty`, `WzConvexProperty`, `WzRawDataProperty`,
`WzBinaryProperty`, `WzLuaProperty`, and `WzVideoProperty`, inspect their C++
fields first and implement the MapleLib byte order exactly. If a property does
not currently retain enough parsed bytes to write faithfully, return
`Error::NotImplemented` with the exact property type and add a failing test in
Task 3 before expanding the data model.

- [ ] **Step 7: Run property serialization tests**

Run:

```powershell
cmake --build build
ctest --test-dir build -R PropertyWriteValue --output-on-failure
```

Expected: tests pass for scalar/sub property writing.

- [ ] **Step 8: Format, lint, commit**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
git add include\wz\WzImageProperty.h src\WzImageProperty.cpp include\wz\Properties src\Properties tests\TestPropertyWriteValue.cpp
git commit -m "feat: serialize wz properties"
```

Expected: format and lint pass with zero errors; commit succeeds.

### Task 3: Implement Image Save and Changed Propagation

**C# Reference:**
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/WzImage.cs`

**Files:**
- Modify: `include/wz/WzImage.h`
- Modify: `src/WzImage.cpp`
- Modify: `src/WzPropertyCollection.cpp`
- Test: `tests/TestEditing.cpp`

- [ ] **Step 1: Write failing edit behavior tests**

Create `tests/TestEditing.cpp`:

```cpp
#include <memory>
#include <sstream>
#include "gtest/gtest.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzAESConstant.h"
#include "wz/WzImage.h"

TEST(Editing, AddPropertySetsParentAndMarksImageChanged) {
  wz::WzImage image("test.img");
  image.SetChanged(false);

  auto prop = std::make_unique<wz::WzIntProperty>("x", 7);
  wz::WzImageProperty* raw = prop.get();
  image.AddProperty(std::move(prop));

  EXPECT_EQ(raw->Parent(), &image);
  EXPECT_TRUE(image.Changed());
}

TEST(Editing, RemovePropertyClearsParentAndMarksImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("x", 7);
  wz::WzImageProperty* raw = prop.get();
  image.AddProperty(std::move(prop));
  image.SetChanged(false);

  image.RemoveProperty(raw);

  EXPECT_EQ(raw->Parent(), nullptr);
  EXPECT_TRUE(image.Changed());
}

TEST(Editing, SaveChangedImageWritesPropertyImage) {
  wz::WzImage image("test.img");
  image.AddProperty(std::make_unique<wz::WzStringProperty>("name", "abc"));

  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  wz::WzBinaryWriter writer(stream, wz::WzAESConstant::GMSIv);
  ASSERT_TRUE(image.SaveImage(&writer).has_value());
  EXPECT_GT(image.BlockSize(), 0);

  stream.seekg(0);
  wz::WzBinaryReader reader(stream, wz::WzAESConstant::GMSIv);
  EXPECT_EQ(reader.ReadByte(), wz::WzImage::WzImageHeaderByte_WithoutOffset);
  EXPECT_EQ(reader.ReadString(), "Property");
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
cmake --build build
ctest --test-dir build -R Editing --output-on-failure
```

Expected: build fails because `SaveImage` does not exist and removal does not
clear parent while preserving a testable pointer.

- [ ] **Step 3: Add image temporary block metadata and save declarations**

Modify `include/wz/WzImage.h`:

```cpp
Result<void> SaveImage(WzBinaryWriter* writer,
                       bool isDefaultUserKey = true,
                       bool forceReadFromData = false);
void CalculateAndSetImageChecksum(const std::vector<uint8_t>& bytes);
void UnparseImage();

int64_t TempFileStart() const { return tempFileStart_; }
void SetTempFileStart(int64_t v) { tempFileStart_ = v; }
int64_t TempFileEnd() const { return tempFileEnd_; }
void SetTempFileEnd(int64_t v) { tempFileEnd_ = v; }

int64_t tempFileStart_ = 0;
int64_t tempFileEnd_ = 0;
```

- [ ] **Step 4: Implement SaveImage**

Modify `src/WzImage.cpp`:

```cpp
Result<void> WzImage::SaveImage(WzBinaryWriter* writer,
                                bool isDefaultUserKey,
                                bool forceReadFromData) {
  if (!writer) {
    return std::unexpected(Error::InvalidArgument("writer is null"));
  }
  if (Changed() || !isDefaultUserKey || forceReadFromData) {
    if (reader_ && !parsed_) {
      parseEverything_ = true;
      auto parsed = ParseImage();
      if (!parsed.has_value()) return std::unexpected(parsed.error());
    }

    int64_t start = writer->Position();
    writer->WriteStringValue("Property",
                             WzImageHeaderByte_WithoutOffset,
                             WzImageHeaderByte_WithOffset);
    auto result = WzImageProperty::WritePropertyList(writer, properties_);
    if (!result.has_value()) return std::unexpected(result.error());
    size_ = static_cast<int>(writer->Position() - start);
    return {};
  }

  if (!reader_) {
    return std::unexpected(Error::InvalidArgument(
        "Cannot save unchanged image without original reader"));
  }
  std::lock_guard<std::recursive_mutex> lock(reader_->Mutex());
  int64_t original = reader_->Position();
  reader_->SetPosition(offset_);
  auto bytes = reader_->ReadBytes(size_);
  writer->WriteBytes(bytes);
  reader_->SetPosition(original);
  return {};
}

void WzImage::CalculateAndSetImageChecksum(const std::vector<uint8_t>& bytes) {
  checksum_ = 0;
  for (uint8_t b : bytes) checksum_ += b;
}

void WzImage::UnparseImage() {
  parsed_ = false;
  properties_.clear();
  properties_ = WzPropertyCollection(this);
}
```

- [ ] **Step 5: Clear parent before property removal**

Modify `src/WzImage.cpp` `RemoveProperty(WzImageProperty* prop)`:

```cpp
prop->SetParent(nullptr);
properties_.erase(it);
SetChanged(true);
```

Modify `WzPropertyCollection::erase` and `erase_at` if necessary so parent
clearing happens before the `unique_ptr` is destroyed. If tests cannot safely
observe a removed raw pointer after destruction, change the test to assert via a
non-owning detached remove API added in this task:

```cpp
std::unique_ptr<WzImageProperty> WzImage::TakeProperty(WzImageProperty* prop);
```

and expose destructive remove separately in later C API tasks.

- [ ] **Step 6: Run editing tests**

Run:

```powershell
cmake --build build
ctest --test-dir build -R Editing --output-on-failure
```

Expected: tests pass. If the remove pointer test is unsafe under `unique_ptr`,
replace it with `TakeProperty` as described and assert the detached object's
parent is null.

- [ ] **Step 7: Format, lint, commit**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
git add include\wz\WzImage.h src\WzImage.cpp src\WzPropertyCollection.cpp tests\TestEditing.cpp
git commit -m "feat: save changed wz images"
```

Expected: format and lint pass with zero errors; commit succeeds.

### Task 4: Implement Directory Metadata and File Repack

**C# Reference:**
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/WzDirectory.cs`
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/WzFile.cs`

**Files:**
- Modify: `include/wz/WzDirectory.h`
- Modify: `src/WzDirectory.cpp`
- Modify: `include/wz/WzFile.h`
- Modify: `src/WzFile.cpp`
- Test: `tests/TestRepack.cpp`

- [ ] **Step 1: Write failing repack round-trip test**

Create `tests/TestRepack.cpp`:

```cpp
#include <filesystem>
#include <memory>
#include "gtest/gtest.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/WzFile.h"

TEST(Repack, SavesAndReopensEditedWzFile) {
  const std::filesystem::path input =
      "Harepacker-resurrected/UnitTest_WzFile/WzFiles/Common/TamingMob_GMS_95.wz";
  if (!std::filesystem::exists(input)) {
    GTEST_SKIP() << "sample wz file is not available";
  }

  wz::WzFile file(input.string(), 95, wz::WzMapleVersion::GMS);
  ASSERT_EQ(file.ParseWzFile(), wz::WzFileParseStatus::Success);
  auto* root = file.GetWzDirectory();
  ASSERT_NE(root, nullptr);

  auto* image = root->WzImages().empty() ? nullptr : root->WzImages()[0];
  ASSERT_NE(image, nullptr);
  ASSERT_TRUE(image->ParseImage().has_value());
  image->AddProperty(std::make_unique<wz::WzIntProperty>("libwz_edit_test", 1234));

  std::filesystem::path output =
      std::filesystem::temp_directory_path() / "libwz_repack_test.wz";
  std::filesystem::remove(output);
  ASSERT_TRUE(file.SaveToDisk(output.string()).has_value());
  ASSERT_TRUE(std::filesystem::exists(output));

  wz::WzFile reopened(output.string(), 95, wz::WzMapleVersion::GMS);
  ASSERT_EQ(reopened.ParseWzFile(), wz::WzFileParseStatus::Success);
  auto* reopenedImage = reopened.GetWzDirectory()->GetImageByName(image->Name());
  ASSERT_NE(reopenedImage, nullptr);
  ASSERT_TRUE(reopenedImage->ParseImage().has_value());
  auto* prop = reopenedImage->GetPropertyByName("libwz_edit_test").value();
  ASSERT_NE(prop, nullptr);
  EXPECT_EQ(prop->GetInt(), 1234);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
cmake --build build
ctest --test-dir build -R Repack --output-on-failure
```

Expected: build fails because `SaveToDisk` does not exist.

- [ ] **Step 3: Add directory save declarations**

Modify `include/wz/WzDirectory.h`:

```cpp
Result<int> GenerateDataFile(const std::array<uint8_t, 4>* useIv,
                             bool isDefaultUserKey,
                             std::ostream* tempStream);
Result<void> SaveDirectory(WzBinaryWriter* writer);
Result<void> SaveImages(WzBinaryWriter* writer, std::istream* tempStream);
uint32_t GetOffsets(uint32_t currentOffset);
uint32_t GetImgOffsets(uint32_t currentOffset);
int OffsetSize() const { return offsetSize_; }

int offsetSize_ = 0;
```

- [ ] **Step 4: Implement directory data generation**

Modify `src/WzDirectory.cpp` with MapleLib order:

```cpp
Result<int> WzDirectory::GenerateDataFile(const std::array<uint8_t, 4>* useIv,
                                          bool isDefaultUserKey,
                                          std::ostream* tempStream) {
  bool useCustomIv = useIv != nullptr;
  size_ = 0;
  int entryCount = static_cast<int>(subDirs_.size() + images_.size());
  if (entryCount == 0) {
    offsetSize_ = 1;
    return size_ = 0;
  }

  size_ = WzTool::GetCompressedIntLength(entryCount);
  offsetSize_ = WzTool::GetCompressedIntLength(entryCount);

  for (auto& img : images_) {
    img->SetChanged(img->Changed() || useCustomIv || !isDefaultUserKey);
    if (img->Changed()) {
      std::stringstream mem(std::ios::in | std::ios::out | std::ios::binary);
      WzBinaryWriter imgWriter(mem, useCustomIv ? *useIv : wz_iv_);
      auto save = img->SaveImage(&imgWriter, isDefaultUserKey, useCustomIv);
      if (!save.has_value()) return std::unexpected(save.error());
      std::string bytesString = mem.str();
      std::vector<uint8_t> bytes(bytesString.begin(), bytesString.end());
      img->CalculateAndSetImageChecksum(bytes);
      img->SetTempFileStart(static_cast<int64_t>(tempStream->tellp()));
      tempStream->write(bytesString.data(),
                        static_cast<std::streamsize>(bytesString.size()));
      img->SetTempFileEnd(static_cast<int64_t>(tempStream->tellp()));
    } else {
      img->SetTempFileStart(img->Offset());
      img->SetTempFileEnd(img->Offset() + img->BlockSize());
    }
    img->UnparseImage();

    int nameLen = WzTool::GetWzObjectValueLength(img->Name(), 4);
    int imgLen = img->BlockSize();
    size_ += nameLen + WzTool::GetCompressedIntLength(imgLen) + imgLen +
             WzTool::GetCompressedIntLength(img->Checksum()) + 4;
    offsetSize_ += nameLen + WzTool::GetCompressedIntLength(imgLen) +
                   WzTool::GetCompressedIntLength(img->Checksum()) + 4;
  }

  for (auto& dir : subDirs_) {
    int nameLen = WzTool::GetWzObjectValueLength(dir->Name(), 3);
    auto childSize = dir->GenerateDataFile(useIv, isDefaultUserKey, tempStream);
    if (!childSize.has_value()) return std::unexpected(childSize.error());
    size_ += nameLen + childSize.value() +
             WzTool::GetCompressedIntLength(dir->BlockSize()) +
             WzTool::GetCompressedIntLength(dir->Checksum()) + 4;
    offsetSize_ += nameLen + WzTool::GetCompressedIntLength(dir->BlockSize()) +
                   WzTool::GetCompressedIntLength(dir->Checksum()) + 4;
  }
  return size_;
}
```

- [ ] **Step 4a: Add Wz object value length helper**

Modify `include/wz/Util/WzTool.h`:

```cpp
static int GetWzObjectValueLength(const std::string& s, int type);
```

Modify `src/Util/WzTool.cpp`:

```cpp
int WzTool::GetWzObjectValueLength(const std::string& s, int type) {
  return 1 + GetEncodedStringLength(s);
}
```

This matches the non-offset first write size used by MapleLib for directory
object values. Offset reuse is handled by `WzBinaryWriter::WriteWzObjectValue`.

- [ ] **Step 5: Implement offset and directory writing**

Implement in `src/WzDirectory.cpp`:

```cpp
uint32_t WzDirectory::GetOffsets(uint32_t currentOffset) {
  offset_ = currentOffset;
  currentOffset += static_cast<uint32_t>(offsetSize_);
  for (auto& dir : subDirs_) currentOffset = dir->GetOffsets(currentOffset);
  return currentOffset;
}

uint32_t WzDirectory::GetImgOffsets(uint32_t currentOffset) {
  for (auto& img : images_) {
    img->SetOffset(currentOffset);
    currentOffset += static_cast<uint32_t>(img->BlockSize());
  }
  for (auto& dir : subDirs_) currentOffset = dir->GetImgOffsets(currentOffset);
  return currentOffset;
}

Result<void> WzDirectory::SaveDirectory(WzBinaryWriter* writer) {
  offset_ = writer->Position();
  int entryCount = static_cast<int>(subDirs_.size() + images_.size());
  if (entryCount == 0) {
    size_ = 0;
    writer->WriteByte(0);
    return {};
  }
  writer->WriteCompressedInt(entryCount);
  for (auto& img : images_) {
    writer->WriteWzObjectValue(img->Name(), WzDirectoryType::WzImage_4);
    writer->WriteCompressedInt(img->BlockSize());
    writer->WriteCompressedInt(img->Checksum());
    auto offsetResult = writer->WriteOffset(img->Offset());
    if (!offsetResult.has_value()) return std::unexpected(offsetResult.error());
  }
  for (auto& dir : subDirs_) {
    writer->WriteWzObjectValue(dir->Name(), WzDirectoryType::WzDirectory_3);
    writer->WriteCompressedInt(dir->BlockSize());
    writer->WriteCompressedInt(dir->Checksum());
    auto offsetResult = writer->WriteOffset(dir->Offset());
    if (!offsetResult.has_value()) return std::unexpected(offsetResult.error());
  }
  for (auto& dir : subDirs_) {
    if (dir->BlockSize() > 0) {
      auto result = dir->SaveDirectory(writer);
      if (!result.has_value()) return std::unexpected(result.error());
    } else {
      writer->WriteByte(0);
    }
  }
  return {};
}
```

- [ ] **Step 6: Implement image block writing**

Implement in `src/WzDirectory.cpp`:

```cpp
Result<void> WzDirectory::SaveImages(WzBinaryWriter* writer,
                                     std::istream* tempStream) {
  for (auto& img : images_) {
    if (img->Changed()) {
      tempStream->seekg(img->TempFileStart());
      std::vector<uint8_t> buffer(static_cast<size_t>(img->BlockSize()));
      tempStream->read(reinterpret_cast<char*>(buffer.data()),
                       static_cast<std::streamsize>(buffer.size()));
      writer->WriteBytes(buffer);
    } else {
      auto* r = img->Reader();
      if (!r) {
        return std::unexpected(Error::InvalidArgument(
            "Cannot save unchanged image without original reader"));
      }
      std::lock_guard<std::recursive_mutex> lock(r->Mutex());
      r->SetPosition(img->TempFileStart());
      writer->WriteBytes(r->ReadBytes(img->TempFileEnd() - img->TempFileStart()));
    }
  }
  for (auto& dir : subDirs_) {
    auto result = dir->SaveImages(writer, tempStream);
    if (!result.has_value()) return std::unexpected(result.error());
  }
  return {};
}
```

- [ ] **Step 7: Add WzFile::SaveToDisk**

Modify `include/wz/WzFile.h`:

```cpp
Result<void> SaveToDisk(const std::string& path,
                        std::optional<bool> saveAs64Bit = std::nullopt,
                        WzMapleVersion preferredVersion = WzMapleVersion::UNKNOWN);
```

Modify `src/WzFile.cpp`:

```cpp
Result<void> WzFile::SaveToDisk(const std::string& outputPath,
                                std::optional<bool> saveAs64Bit,
                                WzMapleVersion preferredVersion) {
  if (!wzDir_) {
    return std::unexpected(Error::InvalidArgument("WzFile has no directory"));
  }
  std::array<uint8_t, 4> outputIv =
      preferredVersion == WzMapleVersion::UNKNOWN
          ? WzTool::GetIvByMapleVersion(maplepLocalVersion_)
          : WzTool::GetIvByMapleVersion(preferredVersion);
  bool sameIv = outputIv == wzDir_->WzIv();
  bool isDefaultUserKey = true;
  bool save64 = saveAs64Bit.has_value() ? saveAs64Bit.value()
                                        : !wz_withEncryptVersionHeader_;

  uint32_t newHash =
      CheckAndGetVersionHash(save64 ? wzVersionHeader64bit_start
                                    : wzVersionHeader_,
                             mapleStoryPatchVersion_);
  if (newHash == 0) newHash = versionHash_;
  versionHash_ = newHash;
  wzDir_->SetHash(versionHash_);

  std::stringstream tempData(std::ios::in | std::ios::out | std::ios::binary);
  const std::array<uint8_t, 4>* customIv = sameIv ? nullptr : &outputIv;
  auto dataResult = wzDir_->GenerateDataFile(customIv, isDefaultUserKey, &tempData);
  if (!dataResult.has_value()) return std::unexpected(dataResult.error());

  uint32_t dirStart = header_.FStart() + (save64 ? 0U : 2U);
  uint32_t totalLen = wzDir_->GetImgOffsets(wzDir_->GetOffsets(dirStart));
  header_.SetFSize(totalLen - header_.FStart());

  std::ofstream output(wz::to_path(outputPath), std::ios::binary | std::ios::trunc);
  if (!output.is_open()) {
    return std::unexpected(Error::IoError("Failed to open output WZ file"));
  }

  WzBinaryWriter writer(output, outputIv);
  writer.SetHash(versionHash_);
  writer.WriteBytes(reinterpret_cast<const uint8_t*>(header_.Ident().data()), 4);
  writer.WriteUInt64(header_.FSize());
  writer.WriteUInt32(header_.FStart());
  writer.WriteNullTerminatedString(header_.Copyright());
  while (writer.Position() < static_cast<int64_t>(header_.FStart())) {
    writer.WriteByte(0);
  }
  if (!save64) writer.WriteUInt16(wzVersionHeader_);
  writer.SetHeader(&header_);

  auto dirResult = wzDir_->SaveDirectory(&writer);
  if (!dirResult.has_value()) return std::unexpected(dirResult.error());
  writer.ClearStringCache();
  tempData.seekg(0);
  auto imageResult = wzDir_->SaveImages(&writer, &tempData);
  if (!imageResult.has_value()) return std::unexpected(imageResult.error());
  return {};
}
```

Use the existing `WzHeader::Ident()`, `FSize()`, `FStart()`, and `Copyright()`
accessors, and the existing `wz::to_path` helper already used by `WzFile.cpp`.

- [ ] **Step 8: Run repack test**

Run:

```powershell
cmake --build build
ctest --test-dir build -R Repack --output-on-failure
```

Expected: test passes. If the test reports `WriteValue is not implemented` for
a specific property class, stop this task and insert a focused subtask before
Step 8 that implements that class's writer from its MapleLib `WriteValue`
method and adds one property-specific serialization test.

- [ ] **Step 9: Format, lint, commit**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
git add include\wz\WzDirectory.h src\WzDirectory.cpp include\wz\WzFile.h src\WzFile.cpp include\wz\Util\WzTool.h src\Util\WzTool.cpp tests\TestRepack.cpp
git commit -m "feat: repack wz files"
```

Expected: format and lint pass with zero errors; commit succeeds.

### Task 5: Harden Core Editing API

**C# Reference:**
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/WzDirectory.cs`
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/WzImage.cs`
- `Harepacker-resurrected/MapleLib/MapleLib/WzLib/IPropertyContainer.cs`

**Files:**
- Modify: `include/wz/WzObject.h`
- Modify: `src/WzObject.cpp`
- Modify: `include/wz/WzDirectory.h`
- Modify: `src/WzDirectory.cpp`
- Modify: `include/wz/WzImageProperty.h`
- Modify: property container classes
- Test: `tests/TestEditing.cpp`

- [ ] **Step 1: Add failing tests for duplicate names and nested changed propagation**

Append to `tests/TestEditing.cpp`:

```cpp
TEST(Editing, DuplicatePropertyNameReturnsError) {
  wz::WzImage image("test.img");
  ASSERT_TRUE(image.TryAddProperty(std::make_unique<wz::WzIntProperty>("x", 1))
                  .has_value());
  auto result = image.TryAddProperty(std::make_unique<wz::WzIntProperty>("x", 2));
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
}

TEST(Editing, ChangingNestedScalarMarksParentImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("x", 1);
  wz::WzIntProperty* raw = prop.get();
  image.AddProperty(std::move(prop));
  image.SetChanged(false);

  raw->SetValue(2);

  EXPECT_TRUE(image.Changed());
  EXPECT_EQ(raw->Value(), 2);
}
```

- [ ] **Step 2: Add fallible edit methods**

Add alongside existing void APIs:

```cpp
Result<void> WzObject::Rename(const std::string& name);
Result<void> WzImage::TryAddProperty(std::unique_ptr<WzImageProperty> prop);
Result<std::unique_ptr<WzImageProperty>> WzImage::TakeProperty(WzImageProperty* prop);
Result<WzDirectory*> WzDirectory::CreateDirectory(const std::string& name);
Result<WzImage*> WzDirectory::CreateImage(const std::string& name);
Result<void> WzDirectory::TryAddDirectory(std::unique_ptr<WzDirectory> dir);
Result<void> WzDirectory::TryAddImage(std::unique_ptr<WzImage> img);
```

Keep existing `AddProperty(WzImageProperty*)` and `AddImage(WzImage*)` as
compatibility wrappers that call the fallible methods and ignore the result.

- [ ] **Step 3: Add MarkParentImageChanged helper**

Add to `WzImageProperty`:

```cpp
void MarkParentImageChanged() {
  if (auto* image = ParentImage()) image->SetChanged(true);
}
```

Update every `SetValue` override in scalar/data properties:

```cpp
void WzIntProperty::SetValue(int32_t value) {
  value_ = value;
  MarkParentImageChanged();
}
```

- [ ] **Step 4: Run editing tests**

Run:

```powershell
cmake --build build
ctest --test-dir build -R Editing --output-on-failure
```

Expected: all editing tests pass.

- [ ] **Step 5: Format, lint, commit**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
git add include\wz src tests\TestEditing.cpp
git commit -m "feat: harden wz editing operations"
```

Expected: format and lint pass with zero errors; commit succeeds.

### Task 6: Extend C API

**C# Reference:**
- Use C++ methods from Tasks 3-5; API names should mirror Java lower-camel concepts while preserving C snake-case.

**Files:**
- Modify: `include/wz/wz_api.h`
- Modify: `src/capi/wz_api.cpp`
- Test: add C API coverage to `tests/TestEditing.cpp` or create `tests/TestCapiEditing.cpp`

- [ ] **Step 1: Write failing C API tests**

Create `tests/TestCapiEditing.cpp`:

```cpp
#include <filesystem>
#include "gtest/gtest.h"
#include "wz/wz_api.h"

TEST(CapiEditing, CreateImagePropertySetAndRemove) {
  wz_file file = nullptr;
  ASSERT_EQ(wz_create_file(95, WZ_GMS, &file), WZ_ERROR_NONE);
  wz_dir root = nullptr;
  ASSERT_EQ(wz_file_get_wz_directory(file, &root), WZ_ERROR_NONE);

  wz_image image = nullptr;
  ASSERT_EQ(wz_dir_create_image(root, "test.img", &image), WZ_ERROR_NONE);

  wz_property prop = nullptr;
  ASSERT_EQ(wz_property_create_int("count", 7, &prop), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(image, prop), WZ_ERROR_NONE);
  ASSERT_EQ(wz_int_set_value(prop, 8), WZ_ERROR_NONE);

  int32_t value = 0;
  ASSERT_EQ(wz_int_get_value(prop, &value), WZ_ERROR_NONE);
  EXPECT_EQ(value, 8);

  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(prop)), WZ_ERROR_NONE);
  wz_close_file(file);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
cmake --build build
ctest --test-dir build -R CapiEditing --output-on-failure
```

Expected: build fails because new C API functions are missing.

- [ ] **Step 3: Add C API declarations**

Modify `include/wz/wz_api.h`:

```c
wz_error_code wz_create_file(short game_version,
                             wz_maple_version version,
                             wz_file* out_file);
wz_error_code wz_file_save_to_disk(wz_file file, const char* file_path);
wz_error_code wz_file_save_to_disk_ex(wz_file file,
                                      const char* file_path,
                                      int has_save_as_64bit,
                                      int save_as_64bit,
                                      wz_maple_version version);
wz_error_code wz_object_set_name(wz_object obj, const char* name);
wz_error_code wz_object_remove(wz_object obj);
wz_error_code wz_dir_create_directory(wz_dir dir,
                                      const char* name,
                                      wz_dir* out_dir);
wz_error_code wz_dir_create_image(wz_dir dir,
                                  const char* name,
                                  wz_image* out_image);
wz_error_code wz_dir_remove_directory(wz_dir dir, wz_dir child);
wz_error_code wz_dir_remove_image(wz_dir dir, wz_image child);
wz_error_code wz_property_create_null(const char* name, wz_property* out_prop);
wz_error_code wz_property_create_int(const char* name,
                                     int32_t value,
                                     wz_property* out_prop);
wz_error_code wz_property_create_string(const char* name,
                                        const char* value,
                                        wz_property* out_prop);
wz_error_code wz_image_add_property(wz_image image, wz_property prop);
wz_error_code wz_property_add_child(wz_property parent, wz_property child);
wz_error_code wz_image_remove_property(wz_image image, wz_property prop);
wz_error_code wz_property_remove_child(wz_property parent, wz_property child);
wz_error_code wz_image_clear_properties(wz_image image);
wz_error_code wz_property_clear_children(wz_property parent);
```

Add these declarations too:

```c
wz_error_code wz_property_create_short(const char* name,
                                       int16_t value,
                                       wz_property* out_prop);
wz_error_code wz_property_create_long(const char* name,
                                      int64_t value,
                                      wz_property* out_prop);
wz_error_code wz_property_create_float(const char* name,
                                       float value,
                                       wz_property* out_prop);
wz_error_code wz_property_create_double(const char* name,
                                        double value,
                                        wz_property* out_prop);
wz_error_code wz_property_create_sub(const char* name, wz_property* out_prop);
wz_error_code wz_property_create_vector(const char* name,
                                        int32_t x,
                                        int32_t y,
                                        wz_property* out_prop);
wz_error_code wz_property_create_uol(const char* name,
                                     const char* value,
                                     wz_property* out_prop);
wz_error_code wz_short_set_value(wz_property prop, int16_t value);
wz_error_code wz_long_set_value(wz_property prop, int64_t value);
wz_error_code wz_float_set_value(wz_property prop, float value);
wz_error_code wz_double_set_value(wz_property prop, double value);
wz_error_code wz_string_set_value(wz_property prop, const char* value);
wz_error_code wz_uol_set_value(wz_property prop, const char* value);
```

- [ ] **Step 4: Implement C API functions**

Modify `src/capi/wz_api.cpp` using existing validation helpers. Constructor
pattern:

```cpp
wz_error_code wz_property_create_int(const char* name,
                                     int32_t value,
                                     wz_property* out_prop) {
  if (!out_prop) return set_error(WZ_ERROR_INVALID_ARGUMENT, "out_prop is null");
  *out_prop = nullptr;
  if (!name) return set_error(WZ_ERROR_INVALID_ARGUMENT, "name is null");
  auto* prop = new wz::WzIntProperty(name, value);
  *out_prop = reinterpret_cast<wz_property>(prop);
  return clear_error();
}
```

Ownership transfer pattern:

```cpp
wz_error_code wz_image_add_property(wz_image image, wz_property prop) {
  if (!image || !prop) return set_error(WZ_ERROR_NULL_HANDLE, "null handle");
  auto* img = reinterpret_cast<wz::WzImage*>(image);
  auto* property = reinterpret_cast<wz::WzImageProperty*>(prop);
  auto result = img->TryAddProperty(std::unique_ptr<wz::WzImageProperty>(property));
  if (!result.has_value()) return result_void(result);
  return clear_error();
}
```

After ownership transfer, callers must not separately free the child property.
Document this in comments above the C API declarations.

- [ ] **Step 5: Run C API tests**

Run:

```powershell
cmake --build build
ctest --test-dir build -R CapiEditing --output-on-failure
```

Expected: tests pass.

- [ ] **Step 6: Format, lint, commit**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
git add include\wz\wz_api.h src\capi\wz_api.cpp tests\TestCapiEditing.cpp
git commit -m "feat: expose wz editing in c api"
```

Expected: format and lint pass with zero errors; commit succeeds.

### Task 7: Extend JNI and Java Wrappers

**Files:**
- Modify: `src/jni/wz_jni.cpp`
- Modify: Java wrappers in `java/src/main/java/io/github/toyobayashi/libwz/`
- Test: `java/src/test/java/io/github/toyobayashi/libwz/EditingTest.java`

- [ ] **Step 1: Write failing Java test**

Create `java/src/test/java/io/github/toyobayashi/libwz/EditingTest.java`:

```java
package io.github.toyobayashi.libwz;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import org.junit.Test;

public class EditingTest {
  @Test
  public void createAndEditImageProperty() {
    WzFile file = WzFile.create((short) 95, WzEnums.WzMapleVersion.GMS);
    WzDirectory root = file.getWzDirectory();
    WzImage image = root.addImage("test.img");
    WzIntProperty prop = WzPropertyFactory.createInt("count", 7);
    image.addProperty(prop);
    prop.setValue(8);
    assertEquals(8, prop.getValue());
    assertNotEquals(0, image.getNativePtr());
    file.dispose();
  }
}
```

- [ ] **Step 2: Add Java native declarations and wrappers**

Add methods using existing naming style:

```java
public native static long nativeCreate(short gameVersion, int version);
public void saveToDisk(String path) { nativeSaveToDisk(nativePtr, path); }
public void setName(String name) { WzObject.nativeSetName(nativePtr, name); }
public void remove() { WzObject.nativeRemove(nativePtr); }
```

For `WzDirectory`:

```java
public WzImage addImage(String name) {
  long ptr = nativeAddImage(nativePtr, name);
  return new WzImage(ptr);
}
```

For `WzImage` and container properties:

```java
public void addProperty(WzImageProperty prop) {
  nativeAddProperty(nativePtr, prop.getNativePtr());
}
```

- [ ] **Step 3: Implement JNI by calling C API**

Add to `src/jni/wz_jni.cpp`:

```cpp
JNIEXPORT void JNICALL JNI_FUNC(WzFile, nativeSaveToDisk)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  JniUtfString p(env, path);
  WZ_API_CALL(env, wz_file_save_to_disk(reinterpret_cast<wz_file>(ptr), p))
}

JNIEXPORT void JNICALL JNI_FUNC(WzObject, nativeSetName)(
    JNIEnv* env, jclass, jlong ptr, jstring name) {
  JniUtfString n(env, name);
  WZ_API_CALL(env, wz_object_set_name(reinterpret_cast<wz_object>(ptr), n))
}

JNIEXPORT void JNICALL JNI_FUNC(WzObject, nativeRemove)(
    JNIEnv* env, jclass, jlong ptr) {
  WZ_API_CALL(env, wz_object_remove(reinterpret_cast<wz_object>(ptr)))
}
```

Use the same pattern for directory/image/property creation and add/remove.

- [ ] **Step 4: Run Java/JNI tests**

Run:

```powershell
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=ON
cmake --build build
cd java
.\mvnw test
cd ..
```

Expected: Java tests pass if native library loading is configured. If native
loading is not configured in the local environment, record the exact failure and
continue after C++/C API tests pass.

- [ ] **Step 5: Format, lint, commit**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
git add src\jni\wz_jni.cpp java\src\main\java java\src\test\java
git commit -m "feat: expose wz editing in jni"
```

Expected: format and lint pass with zero errors; commit succeeds.

### Task 8: Extend NodeAPI and TypeScript Surface

**Files:**
- Modify: `src/node/binding.cpp`
- Modify: `index.ts`
- Modify: `tests/smoke.test.js`

- [ ] **Step 1: Write failing Node smoke test**

Append to `tests/smoke.test.js`:

```js
test('editing api creates and mutates properties', () => {
  const file = libwz.createFile(95, libwz.WzMapleVersion.GMS);
  const root = libwz.fileGetWzDirectory(file);
  const image = libwz.dirCreateImage(root, 'test.img');
  const prop = libwz.propertyCreateInt('count', 7);
  libwz.imageAddProperty(image, prop);
  libwz.intSetValue(prop, 8);
  expect(libwz.intGetValue(prop)).toBe(8);
  libwz.closeFile(file);
});
```

- [ ] **Step 2: Add Node native functions**

Modify `src/node/binding.cpp`:

```cpp
Napi::Value CreateFile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int gameVersion = info[0].As<Napi::Number>().Int32Value();
  int version = info[1].As<Napi::Number>().Int32Value();
  wz_file file = nullptr;
  WZ_NODE_API_CALL(env, wz_create_file(static_cast<short>(gameVersion),
                                      static_cast<wz_maple_version>(version),
                                      &file));
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value FileSaveToDisk(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[1].As<Napi::String>().Utf8Value();
  WZ_NODE_API_CALL(env, wz_file_save_to_disk(FromHandle<wz_file>(info[0]),
                                            path.c_str()));
  return env.Undefined();
}

Napi::Value PropertyCreateInt(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  int32_t value = info[1].As<Napi::Number>().Int32Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_property_create_int(name.c_str(), value, &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}
```

Add wrappers for directory/image add/remove, object rename/remove, scalar
setters, and save.

- [ ] **Step 3: Register Node exports**

In the module init function, add:

```cpp
exports.Set("createFile", Napi::Function::New(env, CreateFile));
exports.Set("fileSaveToDisk", Napi::Function::New(env, FileSaveToDisk));
exports.Set("propertyCreateInt", Napi::Function::New(env, PropertyCreateInt));
exports.Set("imageAddProperty", Napi::Function::New(env, ImageAddProperty));
exports.Set("dirCreateImage", Napi::Function::New(env, DirCreateImage));
exports.Set("objectSetName", Napi::Function::New(env, ObjectSetName));
exports.Set("objectRemove", Napi::Function::New(env, ObjectRemove));
exports.Set("intSetValue", Napi::Function::New(env, IntSetValue));
```

- [ ] **Step 4: Update TypeScript exports**

Modify `index.ts` so the TypeScript surface declares the new functions:

```ts
export function createFile(gameVersion: number, version: WzMapleVersion): bigint;
export function fileSaveToDisk(file: bigint, path: string): void;
export function dirCreateImage(dir: bigint, name: string): bigint;
export function propertyCreateInt(name: string, value: number): bigint;
export function imageAddProperty(image: bigint, property: bigint): void;
export function objectSetName(object: bigint, name: string): void;
export function objectRemove(object: bigint): void;
export function intSetValue(property: bigint, value: number): void;
```

Use the existing `index.ts` style if it is generated or manually wraps native
exports.

- [ ] **Step 5: Run Node smoke tests**

Run:

```powershell
npm test
```

Expected: existing smoke tests and new editing smoke test pass.

- [ ] **Step 6: Format, lint, commit**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
git add src\node\binding.cpp index.ts tests\smoke.test.js package.json
git commit -m "feat: expose wz editing in node api"
```

Expected: format and lint pass with zero errors; commit succeeds.

### Task 9: Full Verification and API Consistency Audit

**Files:**
- Modify only files needed to fix audit findings.
- Test: all test suites.

- [ ] **Step 1: Build a C# reference matrix**

Create a scratch note in the final PR description, not in code, with rows:

```text
MapleLib WzFile.SaveToDisk -> wz::WzFile::SaveToDisk -> wz_file_save_to_disk -> WzFile.saveToDisk -> fileSaveToDisk
MapleLib WzDirectory.AddImage -> wz::WzDirectory::TryAddImage -> wz_dir_create_image -> WzDirectory.addImage -> dirCreateImage
MapleLib WzImage.AddProperty -> wz::WzImage::TryAddProperty -> wz_image_add_property -> WzImage.addProperty -> imageAddProperty
MapleLib WzImageProperty.WritePropertyList -> wz::WzImageProperty::WritePropertyList
MapleLib WzBinaryWriter.WriteOffset -> wz::WzBinaryWriter::WriteOffset
```

- [ ] **Step 2: Run full formatting and lint**

Run:

```powershell
.\format.bat
cpplint --recursive --config .cpplint src include
```

Expected: zero lint errors.

- [ ] **Step 3: Run full C++ build and tests**

Run:

```powershell
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all C++ tests pass.

- [ ] **Step 4: Run Node tests**

Run:

```powershell
npm test
```

Expected: all Node tests pass.

- [ ] **Step 5: Run Java tests**

Run:

```powershell
cd java
.\mvnw test
cd ..
```

Expected: Java tests pass. If native library loading fails, capture the exact
error output in the final implementation notes and keep the C++/C API/NodeAPI
verification results as the required local proof.

- [ ] **Step 6: Inspect git status**

Run:

```powershell
git status --short
```

Expected: only intended files changed; `.vscode/settings.json` remains
untracked and unstaged unless the user explicitly asks otherwise.

- [ ] **Step 7: Final commit**

If there are audit fixes:

```powershell
git add <only files changed by this task>
git commit -m "test: verify wz editing support"
```

Expected: commit succeeds. If there are no audit fixes, record "no audit-fix
commit needed" in the final implementation notes.

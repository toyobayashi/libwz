#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzTool.h"
#include "wz/WzAESConstant.h"
#include "wz/WzEnums.h"
#include "wz/WzHeader.h"

namespace {

std::filesystem::path TempPath(const std::string& name) {
  return std::filesystem::temp_directory_path() / name;
}

std::vector<uint8_t> ReadAll(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(in),
                              std::istreambuf_iterator<char>());
}

}  // namespace

TEST(WzBinaryWriterTest, WritesCompressedIntegersLikeReaderExpects) {
  const auto path = TempPath("libwz_binary_writer_compressed_int.bin");
  {
    std::ofstream out(path, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);

    writer.WriteCompressedInt(127);
    writer.WriteCompressedInt(128);
    writer.WriteCompressedInt(-128);
    writer.WriteCompressedLong(127);
    writer.WriteCompressedLong(128);
    writer.WriteCompressedLong(-128);
  }

  std::ifstream in(path, std::ios::binary);
  wz::WzBinaryReader reader(in, wz::WzAESConstant::WZ_GMSIV);

  EXPECT_EQ(reader.ReadCompressedInt(), 127);
  EXPECT_EQ(reader.ReadCompressedInt(), 128);
  EXPECT_EQ(reader.ReadCompressedInt(), -128);
  EXPECT_EQ(reader.ReadLong(), 127);
  EXPECT_EQ(reader.ReadLong(), 128);
  EXPECT_EQ(reader.ReadLong(), -128);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(WzBinaryWriterTest, WrittenStringsRoundTripThroughReader) {
  const auto path = TempPath("libwz_binary_writer_strings.bin");
  const std::string unicode = "Name_\xE6\xB5\x8B\xE8\xAF\x95";
  {
    std::ofstream out(path, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);

    writer.WriteString("");
    writer.WriteString("Property");
    writer.WriteString(unicode);
    writer.WriteNullTerminatedString("raw");
  }

  std::ifstream in(path, std::ios::binary);
  wz::WzBinaryReader reader(in, wz::WzAESConstant::WZ_GMSIV);

  EXPECT_EQ(reader.ReadString(), "");
  EXPECT_EQ(reader.ReadString(), "Property");
  EXPECT_EQ(reader.ReadString(), unicode);
  EXPECT_EQ(reader.ReadNullTerminatedString(), "raw");

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(WzBinaryWriterTest, WriteStringValueReusesCachedAbsoluteOffset) {
  const auto path = TempPath("libwz_binary_writer_string_value.bin");
  {
    std::ofstream out(path, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);

    writer.WriteStringValue("LongName", 0x73, 0x1B);
    writer.WriteStringValue("LongName", 0x73, 0x1B);
  }

  std::ifstream in(path, std::ios::binary);
  wz::WzBinaryReader reader(in, wz::WzAESConstant::WZ_GMSIV);

  EXPECT_EQ(reader.ReadByte(), 0x73);
  const int32_t cached_offset = static_cast<int32_t>(reader.Position());
  EXPECT_EQ(reader.ReadString(), "LongName");
  EXPECT_EQ(reader.ReadByte(), 0x1B);
  EXPECT_EQ(reader.ReadInt32(), cached_offset);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(WzBinaryWriterTest, WriteWzObjectValueReusesHeaderRelativeOffset) {
  const auto path = TempPath("libwz_binary_writer_object_value.bin");
  wz::WzHeader header = wz::WzHeader::GetDefault();
  header.SetFStart(4);

  {
    std::ofstream out(path, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
    writer.SetHeader(header);

    writer.WriteUInt32(0);
    EXPECT_FALSE(writer.WriteWzObjectValue("LongImageName",
                                           wz::WzDirectoryType::WzImage_4));
    EXPECT_TRUE(writer.WriteWzObjectValue("LongImageName",
                                          wz::WzDirectoryType::WzImage_4));
  }

  std::ifstream in(path, std::ios::binary);
  wz::WzBinaryReader reader(in, wz::WzAESConstant::WZ_GMSIV);
  reader.SetHeader(&header);

  reader.ReadUInt32();
  EXPECT_EQ(reader.ReadByte(),
            static_cast<uint8_t>(wz::WzDirectoryType::WzImage_4));
  EXPECT_EQ(reader.ReadString(), "LongImageName");
  EXPECT_EQ(
      reader.ReadByte(),
      static_cast<uint8_t>(wz::WzDirectoryType::RetrieveStringFromOffset_2));
  const int32_t stored_offset = reader.ReadInt32();
  EXPECT_EQ(stored_offset, 0);

  const int64_t remember_pos = reader.Position();
  reader.SetPosition(header.FStart() + stored_offset);
  EXPECT_EQ(reader.ReadByte(),
            static_cast<uint8_t>(wz::WzDirectoryType::WzImage_4));
  EXPECT_EQ(reader.ReadString(), "LongImageName");
  reader.SetPosition(remember_pos);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(WzBinaryWriterTest, WriteOffsetRoundTripsThroughReader) {
  const auto path = TempPath("libwz_binary_writer_offset.bin");
  wz::WzHeader header = wz::WzHeader::GetDefault();
  header.SetFStart(4);
  constexpr uint32_t kHash = 123456789;
  constexpr int64_t kTargetOffset = 4096;

  {
    std::ofstream out(path, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
    writer.SetHeader(header);
    writer.SetHash(kHash);

    writer.WriteUInt32(0);
    writer.WriteOffset(kTargetOffset);
  }

  std::ifstream in(path, std::ios::binary);
  wz::WzBinaryReader reader(in, wz::WzAESConstant::WZ_GMSIV);
  reader.SetHeader(&header);
  reader.SetHash(kHash);

  reader.ReadUInt32();
  EXPECT_EQ(reader.ReadOffset(), kTargetOffset);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(WzBinaryWriterTest, WriteOffsetUsesIdentityRotationForCountZero) {
  const auto path = TempPath("libwz_binary_writer_offset_rotate_zero.bin");
  wz::WzHeader header = wz::WzHeader::GetDefault();
  header.SetFStart(4);
  constexpr uint32_t kHash = 13;
  constexpr uint32_t kTargetOffset = 4096;

  {
    std::ofstream out(path, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
    writer.SetHeader(header);
    writer.SetHash(kHash);

    for (int i = 0; i < 34; ++i) {
      writer.WriteByte(0);
    }
    writer.WriteOffset(kTargetOffset);
  }

  const uint32_t pre_rotate =
      ((((34u - header.FStart()) ^ 0xFFFFFFFFu) * kHash) -
       wz::WzAESConstant::WZ_OffsetConstant);
  ASSERT_EQ(pre_rotate & 0x1Fu, 0u);
  const uint32_t expected =
      pre_rotate ^ (kTargetOffset - (header.FStart() * 2));

  const std::vector<uint8_t> bytes = ReadAll(path);
  ASSERT_EQ(bytes.size(), 38u);
  EXPECT_EQ(bytes[34], static_cast<uint8_t>(expected & 0xFFu));
  EXPECT_EQ(bytes[35], static_cast<uint8_t>((expected >> 8) & 0xFFu));
  EXPECT_EQ(bytes[36], static_cast<uint8_t>((expected >> 16) & 0xFFu));
  EXPECT_EQ(bytes[37], static_cast<uint8_t>((expected >> 24) & 0xFFu));

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

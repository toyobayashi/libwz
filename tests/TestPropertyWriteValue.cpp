#include <gtest/gtest.h>

#include <zlib.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "TestStreams.h"
#include "wz/Properties/WzCanvasProperty.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzPngProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Properties/WzUOLProperty.h"
#include "wz/Properties/WzVectorProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzStream.h"
#include "wz/WzAESConstant.h"
#include "wz/WzImageProperty.h"

namespace {

wz::WzBinaryReader MakeReader(const wz::WzMemoryStream& stream) {
  return wz::WzBinaryReader(test::MemorySource(stream.Buffer()),
                            wz::WzAESConstant::WZ_GMSIV);
}

std::vector<uint8_t> CompressZlib(const std::vector<uint8_t>& data) {
  uLongf compressedSize = compressBound(static_cast<uLong>(data.size()));
  std::vector<uint8_t> compressed(compressedSize);
  EXPECT_EQ(compress2(compressed.data(),
                      &compressedSize,
                      data.data(),
                      static_cast<uLong>(data.size()),
                      Z_DEFAULT_COMPRESSION),
            Z_OK);
  compressed.resize(compressedSize);
  return compressed;
}

std::vector<uint8_t> InflateZlib(const std::vector<uint8_t>& data,
                                 size_t expectedSize) {
  std::vector<uint8_t> inflated(expectedSize);
  z_stream stream = {};
  stream.avail_in = static_cast<uInt>(data.size());
  stream.next_in = const_cast<Bytef*>(data.data());
  stream.avail_out = static_cast<uInt>(inflated.size());
  stream.next_out = inflated.data();

  EXPECT_EQ(inflateInit(&stream), Z_OK);
  int result = inflate(&stream, Z_FINISH);
  inflateEnd(&stream);
  EXPECT_EQ(result, Z_STREAM_END);
  return inflated;
}

}  // namespace

TEST(PropertyWriteValueTest, WritesIntPropertyListEntryReadableByReader) {
  wz::WzMemoryStream out;
  wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
  wz::WzPropertyCollection properties;
  properties.Add(std::make_unique<wz::WzIntProperty>("count", 300));

  auto result = wz::WzImageProperty::WritePropertyList(&writer, properties);

  ASSERT_TRUE(result.has_value()) << result.error().message();
  auto reader = MakeReader(out);

  EXPECT_EQ(reader.ReadUInt16(), 0);
  EXPECT_EQ(reader.ReadCompressedInt(), 1);
  EXPECT_EQ(reader.ReadByte(), 0x00);
  EXPECT_EQ(reader.ReadString(), "count");
  EXPECT_EQ(reader.ReadByte(), 3);
  EXPECT_EQ(reader.ReadCompressedInt(), 300);
}

TEST(PropertyWriteValueTest, SubPropertyWritesPropertyHeaderAndChildCount) {
  wz::WzMemoryStream out;
  wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
  wz::WzSubProperty sub("node");
  sub.AddProperty(std::make_unique<wz::WzIntProperty>("x", 7));

  auto result = sub.WriteValue(&writer);

  ASSERT_TRUE(result.has_value()) << result.error().message();
  auto reader = MakeReader(out);

  EXPECT_EQ(reader.ReadByte(), 0x73);
  EXPECT_EQ(reader.ReadString(), "Property");
  EXPECT_EQ(reader.ReadUInt16(), 0);
  EXPECT_EQ(reader.ReadCompressedInt(), 1);
}

TEST(PropertyWriteValueTest, WritesStringUolAndVectorExtendedValues) {
  wz::WzMemoryStream out;
  wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
  wz::WzStringProperty stringProp("label", "hello");
  wz::WzUOLProperty uolProp("link", "../label");
  wz::WzVectorProperty vectorProp("origin", 12, -5);

  ASSERT_TRUE(stringProp.WriteValue(&writer).has_value());
  ASSERT_TRUE(uolProp.WriteValue(&writer).has_value());
  ASSERT_TRUE(vectorProp.WriteValue(&writer).has_value());

  auto reader = MakeReader(out);

  EXPECT_EQ(reader.ReadByte(), 8);
  EXPECT_EQ(reader.ReadByte(), 0x00);
  EXPECT_EQ(reader.ReadString(), "hello");

  EXPECT_EQ(reader.ReadByte(), 0x73);
  EXPECT_EQ(reader.ReadString(), "UOL");
  EXPECT_EQ(reader.ReadByte(), 0);
  EXPECT_EQ(reader.ReadByte(), 0x00);
  EXPECT_EQ(reader.ReadString(), "../label");

  EXPECT_EQ(reader.ReadByte(), 0x73);
  EXPECT_EQ(reader.ReadString(), "Shape2D#Vector2D");
  EXPECT_EQ(reader.ReadCompressedInt(), 12);
  EXPECT_EQ(reader.ReadCompressedInt(), -5);
}

TEST(PropertyWriteValueTest, CanvasWithoutPngReturnsPreciseUnsupportedError) {
  wz::WzMemoryStream out;
  wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
  wz::WzCanvasProperty canvas("frame");

  auto result = canvas.WriteValue(&writer);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::NotImplemented);
  EXPECT_EQ(result.error().message(),
            "Cannot write Canvas property without a PNG property");
}

TEST(PropertyWriteValueTest, ListWzPngExtractionReturnsSingleZlibStream) {
  const std::vector<uint8_t> rawPixels = {1, 2, 3, 4};
  const std::vector<uint8_t> compressed = CompressZlib(rawPixels);

  wz::WzMemoryStream pngData;
  wz::WzBinaryWriter writer(pngData, wz::WzAESConstant::WZ_GMSIV);
  writer.WriteCompressedInt(1);
  writer.WriteCompressedInt(1);
  writer.WriteCompressedInt(2);
  writer.WriteCompressedInt(0);
  writer.WriteInt32(0);

  auto readerForKey = MakeReader(pngData);
  auto& key = readerForKey.GetWzKey();
  std::vector<uint8_t> listWzBytes;
  const int32_t firstBlockSize = 2;
  listWzBytes.insert(listWzBytes.end(),
                     reinterpret_cast<const uint8_t*>(&firstBlockSize),
                     reinterpret_cast<const uint8_t*>(&firstBlockSize) + 4);
  for (int i = 0; i < firstBlockSize; ++i) {
    listWzBytes.push_back(compressed[i] ^ key[i]);
  }
  const int32_t secondBlockSize =
      static_cast<int32_t>(compressed.size() - firstBlockSize);
  listWzBytes.insert(listWzBytes.end(),
                     reinterpret_cast<const uint8_t*>(&secondBlockSize),
                     reinterpret_cast<const uint8_t*>(&secondBlockSize) + 4);
  for (int i = 0; i < secondBlockSize; ++i) {
    listWzBytes.push_back(compressed[firstBlockSize + i] ^ key[i]);
  }

  writer.WriteInt32(static_cast<int32_t>(listWzBytes.size()) + 1);
  writer.WriteByte(0);
  ASSERT_TRUE(pngData.Write(listWzBytes.data(), listWzBytes.size()));

  auto reader = MakeReader(pngData);
  wz::WzPngProperty png(&reader, false);

  auto converted = png.GetCompressedBytesForExtraction(false);

  ASSERT_TRUE(converted.has_value()) << converted.error().message();
  EXPECT_EQ(converted.value()[0], 0x78);
  EXPECT_NE(converted.value()[2], 0x78);
  EXPECT_EQ(InflateZlib(converted.value(), rawPixels.size()), rawPixels);
}

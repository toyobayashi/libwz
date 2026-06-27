#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

#include "TestStreams.h"
#include "wz/Properties/WzCanvasProperty.h"
#include "wz/Properties/WzIntProperty.h"
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

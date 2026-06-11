#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "wz/PngUtility.h"
#include "wz/Properties/WzCanvasProperty.h"
#include "wz/Properties/WzConvexProperty.h"
#include "wz/Properties/WzDoubleProperty.h"
#include "wz/Properties/WzFloatProperty.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzLongProperty.h"
#include "wz/Properties/WzNullProperty.h"
#include "wz/Properties/WzShortProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Properties/WzUOLProperty.h"
#include "wz/Properties/WzVectorProperty.h"
#include "wz/WzImageProperty.h"

// --- Property Types ---
TEST(PropertyType, AllTypes) {
  wz::WzNullProperty nullProp("n");
  EXPECT_EQ(nullProp.PropertyType(), wz::WzPropertyType::Null);
  wz::WzShortProperty s("s", 42);
  EXPECT_EQ(s.PropertyType(), wz::WzPropertyType::Short);
  EXPECT_EQ(s.Value(), 42);
  EXPECT_EQ(s.GetInt(), 42);
  EXPECT_EQ(s.GetShort(), 42);
  EXPECT_EQ(s.GetLong(), 42);
  EXPECT_FLOAT_EQ(s.GetFloat(), 42.f);
  EXPECT_DOUBLE_EQ(s.GetDouble(), 42.0);
  wz::WzIntProperty i("i", 12345);
  EXPECT_EQ(i.PropertyType(), wz::WzPropertyType::Int);
  EXPECT_EQ(i.Value(), 12345);
  EXPECT_EQ(i.GetInt(), 12345);
  EXPECT_EQ(i.GetShort(), (int16_t)12345);
  EXPECT_EQ(i.GetLong(), 12345);
  EXPECT_FLOAT_EQ(i.GetFloat(), 12345.f);
  wz::WzLongProperty l("l", 1234567890LL);
  EXPECT_EQ(l.PropertyType(), wz::WzPropertyType::Long);
  EXPECT_EQ(l.GetLong(), 1234567890LL);
  EXPECT_EQ(l.GetInt(), (int32_t)1234567890LL);
  EXPECT_EQ(l.GetShort(), (int16_t)1234567890LL);
  EXPECT_FLOAT_EQ(l.GetFloat(), (float)1234567890LL);
  EXPECT_DOUBLE_EQ(l.GetDouble(), (double)1234567890LL);
  wz::WzFloatProperty f("f", 3.14f);
  EXPECT_EQ(f.PropertyType(), wz::WzPropertyType::Float);
  EXPECT_FLOAT_EQ(f.GetFloat(), 3.14f);
  EXPECT_EQ(f.GetInt(), 3);
  EXPECT_EQ(f.GetShort(), 3);
  EXPECT_EQ(f.GetLong(), 3LL);
  wz::WzDoubleProperty d("d", 2.71828);
  EXPECT_EQ(d.PropertyType(), wz::WzPropertyType::Double);
  EXPECT_DOUBLE_EQ(d.GetDouble(), 2.71828);
  EXPECT_EQ(d.GetInt(), 2);
  EXPECT_EQ(d.GetShort(), 2);
  EXPECT_EQ(d.GetLong(), 2LL);
  wz::WzStringProperty str("str", "hello");
  EXPECT_EQ(str.PropertyType(), wz::WzPropertyType::String);
  EXPECT_EQ(str.Value(), "hello");
  wz::WzSubProperty sub("sub");
  EXPECT_EQ(sub.PropertyType(), wz::WzPropertyType::SubProperty);
  sub.AddProperty(new wz::WzIntProperty("c", 100));
  EXPECT_EQ(sub.WzProperties()->size(), 1u);
  wz::WzVectorProperty vec("vec");
  vec.X = new wz::WzIntProperty("X", 10);
  vec.Y = new wz::WzIntProperty("Y", 20);
  EXPECT_EQ(vec.PropertyType(), wz::WzPropertyType::Vector);
  EXPECT_EQ(vec.X->Value(), 10);
  EXPECT_EQ(vec.Y->Value(), 20);
  wz::WzVectorProperty vec2("v2", 100, 200);
  EXPECT_EQ(vec2.X->Value(), 100);
  EXPECT_EQ(vec2.Y->Value(), 200);
  wz::WzVectorProperty vec3("v3", 1.5f, 2.5f);
  EXPECT_EQ(vec3.X->Value(), 1);
  EXPECT_EQ(vec3.Y->Value(), 2);
  wz::WzCanvasProperty canvas("c");
  EXPECT_EQ(canvas.PropertyType(), wz::WzPropertyType::Canvas);
  EXPECT_FALSE(canvas.ContainsInlinkProperty());
  EXPECT_FALSE(canvas.ContainsOutlinkProperty());
  wz::WzConvexProperty convex("cv");
  EXPECT_EQ(convex.PropertyType(), wz::WzPropertyType::Convex);
  wz::WzUOLProperty uol("u", "../../s/0");
  EXPECT_EQ(uol.PropertyType(), wz::WzPropertyType::UOL);
  EXPECT_EQ(uol.Value(), "../../s/0");
}

// --- RGB565 ---
TEST(PngUtility, RGB565ToColor) {
  auto c = wz::PngUtility::RGB565ToColor(0xF800);
  EXPECT_EQ(c.R, 255);
  EXPECT_EQ(c.G, 0);
  EXPECT_EQ(c.B, 0);
  c = wz::PngUtility::RGB565ToColor(0x07E0);
  EXPECT_EQ(c.R, 0);
  EXPECT_EQ(c.G, 255);
  EXPECT_EQ(c.B, 0);
  c = wz::PngUtility::RGB565ToColor(0x001F);
  EXPECT_EQ(c.R, 0);
  EXPECT_EQ(c.G, 0);
  EXPECT_EQ(c.B, 255);
  c = wz::PngUtility::RGB565ToColor(0xFFFF);
  EXPECT_EQ(c.R, 255);
  EXPECT_EQ(c.G, 255);
  EXPECT_EQ(c.B, 255);
  c = wz::PngUtility::RGB565ToColor(0x0000);
  EXPECT_EQ(c.R, 0);
  EXPECT_EQ(c.G, 0);
  EXPECT_EQ(c.B, 0);
  c = wz::PngUtility::RGB565ToColor(0x7BEF);
  EXPECT_LE(std::abs(c.R - 123), 2);
  EXPECT_LE(std::abs(c.G - 125), 2);
  EXPECT_LE(std::abs(c.B - 123), 2);
  c = wz::PngUtility::RGB565ToColor(0xFFE0);
  EXPECT_EQ(c.R, 255);
  EXPECT_EQ(c.G, 255);
  EXPECT_EQ(c.B, 0);
  c = wz::PngUtility::RGB565ToColor(0x07FF);
  EXPECT_EQ(c.R, 0);
  EXPECT_EQ(c.G, 255);
  EXPECT_EQ(c.B, 255);
  c = wz::PngUtility::RGB565ToColor(0xF81F);
  EXPECT_EQ(c.R, 255);
  EXPECT_EQ(c.G, 0);
  EXPECT_EQ(c.B, 255);
}

// --- BGRA4444 ---
TEST(PngUtility, BGRA4444NibbleExpansion) {
  std::vector<uint8_t> raw(8, 0xFF), out;
  auto r = wz::PngUtility::DecompressImagePixelDataBgra4444(raw, 2, 2, out);
  EXPECT_TRUE(r.is_ok());
  EXPECT_EQ(out.size(), 16u);
  for (auto b : out) EXPECT_EQ(b, 0xFF);
}
TEST(PngUtility, BGRA4444InsufficientDataThrows) {
  std::vector<uint8_t> raw(64), out;
  auto r = wz::PngUtility::DecompressImagePixelDataBgra4444(raw, 8, 8, out);
  EXPECT_TRUE(r.is_err());
}

// --- DXT3 ---
TEST(PngUtility, DXT3SolidColor) {
  std::vector<uint8_t> raw(16, 0), out;
  for (int i = 0; i < 8; i++) raw[i] = 0xFF;
  raw[8] = 0x00;
  raw[9] = 0xF8;
  raw[10] = 0x00;
  raw[11] = 0xF8;
  auto r = wz::PngUtility::DecompressImageDXT3(raw, 4, 4, out);
  EXPECT_TRUE(r.is_ok());
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(out[i * 4 + 3], 255);
    EXPECT_EQ(out[i * 4 + 2], 255);
    EXPECT_EQ(out[i * 4 + 1], 0);
    EXPECT_EQ(out[i * 4], 0);
  }
}
TEST(PngUtility, DXT3InsufficientData) {
  std::vector<uint8_t> raw(32), out;
  auto r = wz::PngUtility::DecompressImageDXT3(raw, 8, 8, out);
  EXPECT_TRUE(r.is_err());
}

// --- DXT5 ---
TEST(PngUtility, DXT5Basic) {
  std::vector<uint8_t> raw(16, 0), out;
  raw[0] = 255;
  raw[1] = 255;
  raw[8] = 0x00;
  raw[9] = 0xF8;
  raw[10] = 0x00;
  raw[11] = 0xF8;
  auto r = wz::PngUtility::DecompressImageDXT5(raw, 4, 4, out);
  EXPECT_TRUE(r.is_ok());
  for (int i = 0; i < 16; i++) EXPECT_EQ(out[i * 4 + 3], 255);
}
TEST(PngUtility, DXT5InsufficientData) {
  std::vector<uint8_t> raw(32), out;
  auto r = wz::PngUtility::DecompressImageDXT5(raw, 8, 8, out);
  EXPECT_TRUE(r.is_err());
}
TEST(PngUtility, DXT5InvalidDimensions) {
  std::vector<uint8_t> raw(64), out;
  auto r = wz::PngUtility::DecompressImageDXT5(raw, 0, 0, out);
  EXPECT_TRUE(r.is_err());
}

// --- Format517 ---
TEST(PngUtility, Format517) {
  std::vector<uint8_t> raw(8, 0), out;
  raw[0] = 0x00;
  raw[1] = 0xF8;
  raw[2] = 0xE0;
  raw[3] = 0x07;
  raw[4] = 0x1F;
  raw[5] = 0x00;
  raw[6] = 0xFF;
  raw[7] = 0xFF;
  wz::PngUtility::DecompressImagePixelDataForm517(raw, 32, 32, out);
  EXPECT_EQ(out.size(), 2048u);
}

// --- Color table ---
TEST(PngUtility, ExpandColorTable) {
  wz::RGBColor c[4];
  wz::PngUtility::ExpandColorTable(c, 0xF800, 0x001F);
  EXPECT_EQ(c[0].R, 255);
  EXPECT_EQ(c[0].G, 0);
  EXPECT_EQ(c[0].B, 0);
  EXPECT_EQ(c[1].R, 0);
  EXPECT_EQ(c[1].G, 0);
  EXPECT_EQ(c[1].B, 255);
  EXPECT_NEAR(c[2].R, 170, 1);
  EXPECT_EQ(c[2].G, 0);
  EXPECT_NEAR(c[2].B, 85, 1);
  wz::PngUtility::ExpandColorTable(c, 0, 0xF800);
  EXPECT_EQ(c[3].R, 0);
  EXPECT_EQ(c[3].G, 0);
  EXPECT_EQ(c[3].B, 0);
}

// --- Alpha table ---
TEST(PngUtility, ExpandAlphaDXT3) {
  uint8_t a[16];
  uint8_t raw[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  wz::PngUtility::ExpandAlphaTableDXT3(a, raw, 0);
  for (int i = 0; i < 16; i++) EXPECT_EQ(a[i], 255);
}
TEST(PngUtility, ExpandAlphaDXT5) {
  uint8_t a[8];
  wz::PngUtility::ExpandAlphaTableDXT5(a, 200, 100);
  EXPECT_EQ(a[0], 200);
  EXPECT_EQ(a[1], 100);
  wz::PngUtility::ExpandAlphaTableDXT5(a, 100, 200);
  EXPECT_EQ(a[6], 0);
  EXPECT_EQ(a[7], 255);
}

// --- Format sizes ---
TEST(PngUtility, Format1Size) {
  EXPECT_EQ(wz::PngUtility::GetUncompressedSizeByFormat(8, 8, 1), 128);
}
TEST(PngUtility, Format2Size) {
  EXPECT_EQ(wz::PngUtility::GetUncompressedSizeByFormat(8, 8, 2), 256);
}
TEST(PngUtility, Format517Min) {
  EXPECT_EQ(wz::PngUtility::GetUncompressedSizeByFormat(16, 16, 517), 2);
}
TEST(PngUtility, Format2050Size) {
  EXPECT_EQ(wz::PngUtility::GetUncompressedSizeByFormat(8, 8, 2050), 64);
}

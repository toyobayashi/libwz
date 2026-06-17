#include <gtest/gtest.h>
#include "wz/PortalType.h"

TEST(PortalType, ValidateAll) {
  auto types = {wz::PortalType::StartPoint,
                wz::PortalType::Invisible,
                wz::PortalType::Visible,
                wz::PortalType::Default,
                wz::PortalType::Collision,
                wz::PortalType::Changeable,
                wz::PortalType::ChangeableInvisible,
                wz::PortalType::TownPortalPoint,
                wz::PortalType::Script,
                wz::PortalType::ScriptInvisible,
                wz::PortalType::CollisionScript,
                wz::PortalType::Hidden,
                wz::PortalType::ScriptHidden,
                wz::PortalType::CollisionVerticalJump,
                wz::PortalType::CollisionCustomImpact,
                wz::PortalType::CollisionCustomImpact2,
                wz::PortalType::CollisionUnknownPcig,
                wz::PortalType::ScriptHiddenUng,
                wz::PortalType::UNKNOWN_PCC,
                wz::PortalType::UNKNOWN_PCIR};
  for (auto t : types) {
    EXPECT_TRUE(wz::PortalTypeExtensions::ToCode(t).has_value());
    EXPECT_TRUE(wz::PortalTypeExtensions::GetFriendlyName(t).has_value());
  }
}
TEST(PortalType, ToCode) {
  auto r1 = wz::PortalTypeExtensions::ToCode(wz::PortalType::StartPoint);
  ASSERT_TRUE(r1.has_value());
  EXPECT_EQ(r1.value(), "sp");
  auto r2 = wz::PortalTypeExtensions::ToCode(wz::PortalType::Invisible);
  ASSERT_TRUE(r2.has_value());
  EXPECT_EQ(r2.value(), "pi");
  auto r3 = wz::PortalTypeExtensions::ToCode(wz::PortalType::Visible);
  ASSERT_TRUE(r3.has_value());
  EXPECT_EQ(r3.value(), "pv");
  auto r4 = wz::PortalTypeExtensions::ToCode(wz::PortalType::Default);
  ASSERT_TRUE(r4.has_value());
  EXPECT_EQ(r4.value(), "default");
  auto r5 = wz::PortalTypeExtensions::ToCode(wz::PortalType::UNKNOWN_PCC);
  ASSERT_TRUE(r5.has_value());
  EXPECT_EQ(r5.value(), "pcc");
}
TEST(PortalType, FromCode) {
  auto r1 = wz::PortalTypeExtensions::FromCode("sp");
  ASSERT_TRUE(r1.has_value());
  EXPECT_EQ(r1.value(), wz::PortalType::StartPoint);
  auto r2 = wz::PortalTypeExtensions::FromCode("pi");
  ASSERT_TRUE(r2.has_value());
  EXPECT_EQ(r2.value(), wz::PortalType::Invisible);
  auto r3 = wz::PortalTypeExtensions::FromCode("pv");
  ASSERT_TRUE(r3.has_value());
  EXPECT_EQ(r3.value(), wz::PortalType::Visible);
  auto r4 = wz::PortalTypeExtensions::FromCode("default");
  ASSERT_TRUE(r4.has_value());
  EXPECT_EQ(r4.value(), wz::PortalType::Default);
  auto r5 = wz::PortalTypeExtensions::FromCode("pcc");
  ASSERT_TRUE(r5.has_value());
  EXPECT_EQ(r5.value(), wz::PortalType::UNKNOWN_PCC);
}
TEST(PortalType, FromCodeCaseInsensitive) {
  auto r1 = wz::PortalTypeExtensions::FromCode("SP");
  ASSERT_TRUE(r1.has_value());
  EXPECT_EQ(r1.value(), wz::PortalType::StartPoint);
  auto r2 = wz::PortalTypeExtensions::FromCode("Pi");
  ASSERT_TRUE(r2.has_value());
  EXPECT_EQ(r2.value(), wz::PortalType::Invisible);
  auto r3 = wz::PortalTypeExtensions::FromCode("pV");
  ASSERT_TRUE(r3.has_value());
  EXPECT_EQ(r3.value(), wz::PortalType::Visible);
}
TEST(PortalType, FromCodeNullThrows) {
  auto r = wz::PortalTypeExtensions::FromCode("");
  EXPECT_FALSE(r.has_value());
}
TEST(PortalType, FromCodeInvalidThrows) {
  auto r = wz::PortalTypeExtensions::FromCode("invalid");
  EXPECT_FALSE(r.has_value());
}
TEST(PortalType, RoundTrip) {
  auto types = {wz::PortalType::StartPoint,
                wz::PortalType::Invisible,
                wz::PortalType::Visible,
                wz::PortalType::Default,
                wz::PortalType::Collision,
                wz::PortalType::Changeable,
                wz::PortalType::ChangeableInvisible,
                wz::PortalType::TownPortalPoint,
                wz::PortalType::Script,
                wz::PortalType::ScriptInvisible,
                wz::PortalType::CollisionScript,
                wz::PortalType::Hidden,
                wz::PortalType::ScriptHidden,
                wz::PortalType::CollisionVerticalJump,
                wz::PortalType::CollisionCustomImpact,
                wz::PortalType::CollisionCustomImpact2,
                wz::PortalType::CollisionUnknownPcig,
                wz::PortalType::ScriptHiddenUng,
                wz::PortalType::UNKNOWN_PCC,
                wz::PortalType::UNKNOWN_PCIR};
  for (auto t : types) {
    auto codeR = wz::PortalTypeExtensions::ToCode(t);
    ASSERT_TRUE(codeR.has_value());
    auto ptR = wz::PortalTypeExtensions::FromCode(codeR.value());
    ASSERT_TRUE(ptR.has_value());
    EXPECT_EQ(ptR.value(), t);
  }
}

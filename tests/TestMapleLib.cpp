#include <gtest/gtest.h>
#include <filesystem>
#include "wz/CRC32.h"
#include "wz/Util/WzTool.h"
#include "wz/WzDirectory.h"
#include "wz/WzEnums.h"
#include "wz/WzFile.h"
#include "wz/WzFileManager.h"

TEST(UnitTest_MapleLib, TestCrcCalculation) {
  uint32_t crc_firstRun = wz::CCrc32::GetCrc32(200, 0, false, false);
  EXPECT_EQ(crc_firstRun, 2384409922u);
  uint32_t crc = wz::CWvsPhysicalSpace2D::GetConstantCRC(200);
  EXPECT_EQ(crc, 1696968404u);
}

TEST(WzToolTest, RotateLeft) {
  EXPECT_EQ(wz::WzTool::RotateLeft(0x12345678, 8), 0x34567812u);
}

TEST(WzToolTest, GetIvByMapleVersion) {
  auto ivGMS = wz::WzTool::GetIvByMapleVersion(wz::WzMapleVersion::GMS);
  EXPECT_EQ(ivGMS[0], 0x4D);
  EXPECT_EQ(ivGMS[1], 0x23);
  EXPECT_EQ(ivGMS[2], 0xC7);
  EXPECT_EQ(ivGMS[3], 0x2B);
  auto ivBMS = wz::WzTool::GetIvByMapleVersion(wz::WzMapleVersion::BMS);
  EXPECT_EQ(ivBMS[0], 0);
  auto ivEMS = wz::WzTool::GetIvByMapleVersion(wz::WzMapleVersion::EMS);
  EXPECT_EQ(ivEMS[0], 0xB9);
}

TEST(WzFileManagerTest, LoadWzFile) {
  std::string testDir;
  for (auto& c : {"../Harepacker-resurrected/UnitTest_WzFile/WzFiles/Common",
                  "Harepacker-resurrected/UnitTest_WzFile/WzFiles/Common"}) {
    if (std::filesystem::exists(c)) {
      testDir = c;
      break;
    }
  }
  if (testDir.empty()) GTEST_SKIP() << "Test WZ files not found";
  wz::WzFileManager mgr("", true);
  auto* wzf =
      mgr.LoadWzFile(testDir + "/TamingMob_GMS_87.wz", wz::WzMapleVersion::GMS);
  ASSERT_NE(wzf, nullptr);
  ASSERT_NE(wzf->GetWzDirectory(), nullptr);
  EXPECT_GT(wzf->GetWzDirectory()->WzImages().size(), 0u);
  mgr.UnloadWzFile(wzf);
}

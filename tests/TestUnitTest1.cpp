#include <gtest/gtest.h>
#include <filesystem>
#include <functional>
#include <string>
#include "wz/Properties/WzCanvasProperty.h"
#include "wz/Properties/WzPngProperty.h"
#include "wz/WzDirectory.h"
#include "wz/WzEnums.h"
#include "wz/WzFile.h"
#include "wz/WzFileManager.h"
#include "wz/WzImage.h"
#include "wz/WzImageProperty.h"

namespace fs = std::filesystem;

static std::string FindWzTestDir() {
  for (auto& c : {"../Harepacker-resurrected/UnitTest_WzFile/WzFiles/Common",
                  "../../Harepacker-resurrected/UnitTest_WzFile/WzFiles/Common",
                  "Harepacker-resurrected/UnitTest_WzFile/WzFiles/Common"}) {
    if (fs::exists(c)) return c;
  }
  return "";
}

static std::string FindHotfixTestDir() {
  for (auto& c : {"../Harepacker-resurrected/UnitTest_WzFile/WzFiles/Hotfix",
                  "../../Harepacker-resurrected/UnitTest_WzFile/WzFiles/Hotfix",
                  "Harepacker-resurrected/UnitTest_WzFile/WzFiles/Hotfix"}) {
    if (fs::exists(c)) return c;
  }
  return "";
}

TEST(UnitTest1, Enums) {
  EXPECT_EQ(static_cast<int>(wz::WzFileParseStatus::Path_Is_Null), -1);
  EXPECT_EQ(static_cast<int>(wz::WzFileParseStatus::Error_Game_Ver_Hash), -2);
  EXPECT_EQ(static_cast<int>(wz::WzFileParseStatus::Failed_Unknown), 0);
  EXPECT_EQ(static_cast<int>(wz::WzFileParseStatus::Success), 1);
  EXPECT_EQ(wz::GetErrorDescription(wz::WzFileParseStatus::Success), "Success");
  EXPECT_EQ(wz::GetErrorDescription(wz::WzFileParseStatus::Path_Is_Null),
            "Path is null");
}

TEST(UnitTest1, TestOpeningAndSavingHotfixWzFile) {
  std::string hotfixDir = FindHotfixTestDir();
  if (hotfixDir.empty()) GTEST_SKIP() << "Hotfix WZ files not found";
  std::string filePath = hotfixDir + "/Data.wz";
  ASSERT_TRUE(fs::exists(filePath));
  wz::WzFileManager mgr("", true);
  auto* img = mgr.LoadDataWzHotfixFile(filePath, wz::WzMapleVersion::BMS);
  ASSERT_NE(img, nullptr) << "Hotfix Data.wz loading failed";
  EXPECT_TRUE(img->Parsed());
  EXPECT_GT(img->WzProperties()->size(), 0u);
}

class WzFileTest : public ::testing::TestWithParam<
                       std::tuple<std::string, wz::WzMapleVersion>> {};
TEST_P(WzFileTest, TestOlderWzFiles) {
  auto [fileName, version] = GetParam();
  std::string testDir = FindWzTestDir();
  if (testDir.empty()) GTEST_SKIP() << "Test WZ files not found";
  std::string filePath = testDir + "/" + fileName;
  wz::WzFile f(filePath, static_cast<short>(-1), version);
  auto status = f.ParseWzFile();
  EXPECT_EQ(status, wz::WzFileParseStatus::Success)
      << "Failed: " << fileName << ": " << wz::GetErrorDescription(status);
  ASSERT_NE(f.GetWzDirectory(), nullptr);
}
INSTANTIATE_TEST_SUITE_P(
    OlderWzFiles,
    WzFileTest,
    ::testing::Values(
        std::make_tuple("TamingMob_000_KMS_359.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_000_GMS_237.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_GMS_146.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_GMS_176.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_GMS_230.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_GMS_75.wz", wz::WzMapleVersion::GMS),
        std::make_tuple("TamingMob_GMS_87.wz", wz::WzMapleVersion::GMS),
        std::make_tuple("TamingMob_GMS_95.wz", wz::WzMapleVersion::GMS),
        std::make_tuple("TamingMob_SEA_135.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_SEA_160.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_SEA_211.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_SEA_212.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_000_SEA218.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_ThaiMS_3.wz", wz::WzMapleVersion::BMS),
        std::make_tuple("TamingMob_TMS_113.wz", wz::WzMapleVersion::EMS),
        std::make_tuple("TMS_113_Item.wz", wz::WzMapleVersion::EMS)));

static std::string FindMapleStoryDir() {
#ifdef _WIN32
  const char* home = std::getenv("USERPROFILE");
#else
  const char* home = std::getenv("HOME");
#endif
  if (!home) return "";
  for (auto& c : {std::string(home) + "/kinoko/MapleStory",
                  std::string(home) + "/MapleStory"}) {
    if (fs::exists(c)) return c;
  }
  return "";
}

TEST(UnitTest1, MobWz_1210100_stand_0_PngParse) {
  std::string msDir = FindMapleStoryDir();
  if (msDir.empty()) GTEST_SKIP() << "MapleStory directory not found";

  std::string mobPath = msDir + "/Mob.wz";
  if (!fs::exists(mobPath)) GTEST_SKIP() << "Mob.wz not found at " << mobPath;

  wz::WzFile f(mobPath, static_cast<short>(-1), wz::WzMapleVersion::GMS);
  auto status = f.ParseWzFile();
  ASSERT_EQ(status, wz::WzFileParseStatus::Success)
      << "Failed to parse Mob.wz: " << wz::GetErrorDescription(status);

  auto* root = f.GetWzDirectory();
  ASSERT_NE(root, nullptr) << "Root directory is null";

  wz::WzImage* img = root->GetImageByName("1210100.img");
  ASSERT_NE(img, nullptr) << "1210100.img not found in Mob.wz";
  auto parseResult = img->ParseImage();
  ASSERT_TRUE(parseResult.is_ok())
      << "Failed to parse 1210100.img: " << parseResult.err().message();
  ASSERT_TRUE(parseResult.ok()) << "Failed to parse 1210100.img";

  auto* prop = img->GetFromPath("stand/0");
  ASSERT_NE(prop, nullptr) << "stand/0 not found in 1210100.img";

  ASSERT_EQ(prop->PropertyType(), wz::WzPropertyType::Canvas)
      << "stand/0 is not a Canvas property";

  auto* canvas = static_cast<wz::WzCanvasProperty*>(prop);
  auto* pngProp = canvas->PngProperty();
  ASSERT_NE(pngProp, nullptr) << "Canvas has no PNG property";

  auto result = pngProp->GetImage(false);
  EXPECT_TRUE(result.is_ok())
      << "GetImage() failed: " << result.err().message();
}

TEST(UnitTest1, TestLegacyExtractionStyleSavePath_DoesNotThrow) {
  std::string testDir = FindWzTestDir();
  if (testDir.empty()) GTEST_SKIP() << "Test WZ files not found";
  struct LF {
    std::string n;
    wz::WzMapleVersion v;
  };
  std::vector<LF> legacy = {{"TamingMob_GMS_75.wz", wz::WzMapleVersion::GMS},
                            {"TamingMob_GMS_87.wz", wz::WzMapleVersion::GMS},
                            {"TamingMob_GMS_95.wz", wz::WzMapleVersion::GMS}};
  for (auto& lf : legacy) {
    std::string fp = testDir + "/" + lf.n;
    wz::WzFile f(fp, static_cast<short>(-1), lf.v);
    EXPECT_EQ(f.ParseWzFile(), wz::WzFileParseStatus::Success);
    int count = 0;
    std::function<void(wz::WzDirectory*)> parseAll = [&](wz::WzDirectory* d) {
      for (auto* img : d->WzImages()) {
        if (count >= 150) return;
        count++;
        auto parseResult = img->ParseImage();
        if (parseResult.is_err()) {
          ADD_FAILURE() << "Failed to parse image: " << img->Name() << ": "
                        << parseResult.err().message();
          continue;
        }
        if (!parseResult.ok()) {
          continue;
        }
        for (auto* p : *img->WzProperties()) {
          if (p && p->PropertyType() == wz::WzPropertyType::Canvas) {
            auto* c = static_cast<wz::WzCanvasProperty*>(p);
            if (c->PngProperty()) {
              if (c->PngProperty()->GetImage(false).is_err()) {
                ADD_FAILURE() << "Failed to get image bytes for: " << lf.n
                              << ":" << img->Name();
                break;
              }
            }
          }
        }
      }
      for (auto* sd : d->WzDirectories()) {
        if (count >= 150) return;
        parseAll(sd);
      }
    };
    parseAll(f.GetWzDirectory());
  }
}

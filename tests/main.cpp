#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include "wz/wz.h"

#define EXPECT_OK(exp, fmt, ...)                                               \
  do {                                                                         \
    auto res = (exp);                                                          \
    if (!res) {                                                                \
      fprintf(stderr, "Test failed: " fmt "\n", ##__VA_ARGS__);                \
    }                                                                          \
  } while (0)

int main() {
#ifdef _WIN32
  char* HOME = std::getenv("USERPROFILE");
#else
  char* HOME = std::getenv("HOME");
#endif

  std::filesystem::path BASE_MAPLE_DIR =
      std::filesystem::path(HOME ? HOME : "") / "kinoko" / "MapleStory";
  wz::WzMapleVersion version = wz::WzMapleVersion::GMS;

  bool isStandalone =
      false;  // Set to true if loading single file without directory structure
  wz::WzFileManager* manager =
      new wz::WzFileManager(BASE_MAPLE_DIR.string(), isStandalone);
  manager->BuildWzFileList();  // Scans and builds list of available .wz files
  const auto& map = manager->GetWzFilesList();
  for (const auto& [baseName, fileList] : map) {
    std::cout << "Base: " << baseName << "\n";
    for (const auto& file : fileList) {
      std::cout << "  File: " << file << "\n";
    }
  }

  auto filePath = (BASE_MAPLE_DIR / "UI.wz").string();
  auto wzFileResult = manager->LoadWzFile("UI.wz", version);
  EXPECT_OK(wzFileResult.has_value(),
            "Failed to load WzFile: %s",
            filePath.c_str());
  // wz::WzFile* wzFile = new wz::WzFile(filePath, version);
  wz::WzFile* wzFile = wzFileResult.value();
  EXPECT_OK(wzFile->ParseWzFile() == wz::WzFileParseStatus::Success,
            "Failed to parse .wz file: %s",
            filePath.c_str());
  // Access the root directory
  wz::WzDirectory* root = wzFile->GetWzDirectory();
  EXPECT_OK(
      root != nullptr, "Root directory is null for file: %s", filePath.c_str());

  wz::WzImage* img = root->GetImageByName("StatusBar.img");
  EXPECT_OK(
      img, "Failed to find image 'StatusBar' in file: %s", filePath.c_str());

  wz::WzCanvasProperty* c =
      static_cast<wz::WzCanvasProperty*>(img->GetFromPath("gauge/hpFlash/0"));
  EXPECT_OK(c->SaveToFile("tmp/hpFlash0.png"),
            "Failed to save canvas property to file");

  const auto* properties = img->WzProperties();
  for (auto* prop : *properties) {
    std::cout << "Property: " << prop->Name()
              << ", Type: " << static_cast<int>(prop->PropertyType())
              << std::endl;
    if (prop->Name() == "gauge") {
      for (auto* subProp :
           *static_cast<wz::WzSubProperty*>(prop)->WzProperties()) {
        std::cout << "subProp: " << subProp->Name()
                  << ", Type: " << static_cast<int>(subProp->PropertyType())
                  << std::endl;
        if (subProp->Name() == "graduation") {
          EXPECT_OK(static_cast<wz::WzCanvasProperty*>(subProp)
                        ->PngProperty()
                        ->SaveToFile("tmp/graduation.png"),
                    "Failed to save graduation png");
        }
      }
      break;
    }
  }

  manager->UnloadWzFile(wzFile);

  filePath = (BASE_MAPLE_DIR / "Sound.wz").string();
  wzFile = new wz::WzFile(filePath, version);
  EXPECT_OK(wzFile->ParseWzFile() == wz::WzFileParseStatus::Success,
            "Failed to parse .wz file: %s",
            filePath.c_str());

  img = wzFile->GetWzDirectory()->GetImageByName("Bgm00.img");
  EXPECT_OK(img, "Failed to find image 'Bgm00' in file: %s", filePath.c_str());

  wz::WzBinaryProperty* sound =
      static_cast<wz::WzBinaryProperty*>(img->GetFromPath("FloralLife"));
  EXPECT_OK(sound->SaveToFile("tmp/FloralLife2.mp3"),
            "Failed to save FloralLife2.mp3");

  for (auto* prop : *img->WzProperties()) {
    std::cout << "Property: " << prop->Name()
              << ", Type: " << static_cast<int>(prop->PropertyType())
              << std::endl;
    if (prop->PropertyType() == wz::WzPropertyType::Sound) {
      wz::WzBinaryProperty* soundProp =
          static_cast<wz::WzBinaryProperty*>(prop);
      EXPECT_OK(soundProp->SaveToFile("tmp/" + prop->Name() + ".mp3"),
                "Failed to save sound property: %s",
                prop->Name().c_str());
    }
  }
  manager->UnloadWzFile(wzFile);

  // --- Export Mob.wz canvas images ---
  std::filesystem::create_directories("tmp");
  auto exportMobCanvas = [&](const std::string& imgName,
                             const std::string& path,
                             const std::string& outName) {
    std::string mobPath = (BASE_MAPLE_DIR / "Mob.wz").string();
    wz::WzFile mobFile(mobPath, version);
    EXPECT_OK(mobFile.ParseWzFile() == wz::WzFileParseStatus::Success,
              "Failed to parse Mob.wz: %s",
              mobPath.c_str());

    auto* mobImg = mobFile.GetWzDirectory()->GetImageByName(imgName);
    EXPECT_OK(mobImg, "%s not found in Mob.wz", imgName.c_str());
    auto parseResult = mobImg->ParseImage();
    EXPECT_OK(parseResult.has_value() && parseResult.value(),
              "Failed to parse %s",
              imgName.c_str());

    auto* prop = mobImg->GetFromPath(path);
    EXPECT_OK(prop, "%s not found in %s", path.c_str(), imgName.c_str());
    auto* canvas = static_cast<wz::WzCanvasProperty*>(prop);
    EXPECT_OK(
        canvas->SaveToFile(outName), "Failed to export %s", outName.c_str());
    printf("Exported: %s\n", outName.c_str());
  };

  exportMobCanvas("1210100.img", "stand/0", "tmp/1210100_stand_0.png");
  exportMobCanvas("1210101.img", "stand/0", "tmp/1210101_stand_0.png");
  exportMobCanvas("8220004.img", "stand/0", "tmp/8220004_stand_0.png");

  delete wzFile;
  delete manager;
  return 0;
}

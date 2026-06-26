#include "WzFileManager.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include "wz/Util/ListFileParser.h"
#include "wz/Util/WzPath.h"
#include "wz/Util/WzTool.h"
#include "wz/WzDirectory.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace fs = std::filesystem;

namespace wz {

WzFileManager* WzFileManager::fileManager = nullptr;

const std::vector<std::string>& WzFileManager::ExcludedDirectories() {
  static const std::vector<std::string> dirs = {"bak",
                                                "backup",
                                                "original",
                                                "xml",
                                                "hshield",
                                                "blackcipher",
                                                "harepacker",
                                                "hacreator"};
  return dirs;
}

static std::string ToLower(const std::string& s) {
  std::string result = s;
  std::transform(
      result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
      });
  return result;
}

static bool IsExcluded(const std::string& pathLower) {
  for (const auto& excl : WzFileManager::ExcludedDirectories()) {
    if (pathLower.find(excl) != std::string::npos) return true;
  }
  return false;
}

static int SafeStoi(const std::string& s) {
  char* end = nullptr;
  errno = 0;
  auto v = std::strtol(s.c_str(), &end, 10);
  if (errno != 0 || end == s.c_str() || *end != '\0' || v < 0) return -1;
  return static_cast<int>(v);
}

WzFileManager::WzFileManager()
    : baseDir_(),
      is_standalone_wz_file_(false),
      init_as_64bit_(false),
      is_pre_bb_data_wz_format_(false) {
  fileManager = this;
}

WzFileManager::WzFileManager(const std::string& directory,
                             bool is_standalone_wz_file)
    : baseDir_(directory), is_standalone_wz_file_(is_standalone_wz_file) {
  if (is_standalone_wz_file_) {
    init_as_64bit_ = false;
    is_pre_bb_data_wz_format_ = false;
  } else {
    init_as_64bit_ = Detect64BitDirectoryWzFileFormat(baseDir_);
    is_pre_bb_data_wz_format_ = DetectIsPreBBDataWZFileFormat(baseDir_);
  }
  if (init_as_64bit_) {
    baseDirWithData_ = (wz::to_path(baseDir_) / "Data").string();
  } else {
    baseDirWithData_ = baseDir_;
  }
  fileManager = this;
}

bool WzFileManager::Detect64BitDirectoryWzFileFormat(
    const std::string& baseDirectoryPath) {
  std::error_code ec;
  fs::path dataDir = wz::to_path(baseDirectoryPath) / "Data";
  if (!fs::is_directory(dataDir, ec) || ec) return false;

  int nWzFiles = 0;
  for (auto it = fs::recursive_directory_iterator(dataDir, ec);
       it != fs::end(it);
       it.increment(ec)) {
    if (ec) {
      ec.clear();
      continue;
    }
    bool isFile = it->is_regular_file(ec);
    if (ec) {
      ec.clear();
      continue;
    }
    if (isFile && it->path().extension() == ".wz") {
      nWzFiles++;
      if (nWzFiles > 40) return true;
    }
  }
  return false;
}

bool WzFileManager::DetectIsPreBBDataWZFileFormat(
    const std::string& baseDirectoryPath) {
  std::error_code ec;
  if (!fs::is_directory(baseDirectoryPath, ec) || ec) return false;

  fs::path listWzPath = wz::to_path(baseDirectoryPath) / "List.wz";
  if (fs::is_regular_file(listWzPath, ec) && !ec &&
      WzTool::IsListFile(listWzPath.string())) {
    return true;
  }
  ec.clear();

  fs::path dataWzPath = wz::to_path(baseDirectoryPath) / "Data.wz";
  if (fs::is_regular_file(dataWzPath, ec) && !ec) {
    bool skillExists =
        fs::is_regular_file(wz::to_path(baseDirectoryPath) / "Skill.wz", ec);
    ec.clear();
    bool stringExists =
        fs::is_regular_file(wz::to_path(baseDirectoryPath) / "String.wz", ec);
    ec.clear();
    bool characterExists = fs::is_regular_file(
        wz::to_path(baseDirectoryPath) / "Character.wz", ec);
    ec.clear();

    if (!skillExists && !stringExists && !characterExists) {
      bool skillDir = fs::is_directory(
          wz::to_path(baseDirectoryPath) / "Data" / "Skill", ec);
      ec.clear();
      bool stringDir = fs::is_directory(
          wz::to_path(baseDirectoryPath) / "Data" / "String", ec);
      ec.clear();
      bool characterDir = fs::is_directory(
          wz::to_path(baseDirectoryPath) / "Data" / "Character", ec);
      ec.clear();

      if (!skillDir && !stringDir && !characterDir) return true;
    }
  }
  return false;
}

bool WzFileManager::DetectIsPreBBDataWZFileFormat(
    const std::string& baseDirectoryPath, WzMapleVersion encryption) {
  bool result = DetectIsPreBBDataWZFileFormat(baseDirectoryPath);
  if (result) return true;

  std::string uiWzFilePath =
      (wz::to_path(baseDirectoryPath) / "UI.wz").string();
  std::error_code ec;
  if (!fs::is_regular_file(uiWzFilePath, ec) || ec) return false;

  WzFile uiWzFile(uiWzFilePath, encryption);
  WzFileParseStatus parseStatus = uiWzFile.ParseWzFile();
  if (parseStatus != WzFileParseStatus::Success) return false;

  WzDirectory* wzDir = uiWzFile.GetWzDirectory();
  if (!wzDir) return true;

  WzImage* uiWindow2 = wzDir->GetImageByName("UIWindow2.img");
  if (!uiWindow2) return true;

  auto parseImageResult = uiWindow2->ParseImage();
  if (!parseImageResult.has_value()) return false;
  if (!(*uiWindow2)[BIG_BANG_MARKER]) return true;

  return false;
}

bool WzFileManager::IsBigBangUpdate(WzImage* uiWindow2Image) {
  if (!uiWindow2Image) return false;
  return (*uiWindow2Image)[BIG_BANG_MARKER] != nullptr;
}

bool WzFileManager::IsBigBang2Update(WzImage* uiWindow2Image) {
  if (!uiWindow2Image) return false;
  return (*uiWindow2Image)[BIG_BANG_2_MARKER] != nullptr;
}

bool WzFileManager::DetectBigBang2Format(const std::string& mapleStoryPath,
                                         WzMapleVersion encryption,
                                         bool is64Bit) {
  if (DetectBetaDataWzFormat(mapleStoryPath)) return false;

  std::error_code ec;
  std::string uiWzPath;

  if (is64Bit) {
    fs::path uiDir = wz::to_path(mapleStoryPath) / "Data" / "UI";
    if (fs::is_directory(uiDir, ec) && !ec) {
      for (const auto& entry : fs::directory_iterator(uiDir, ec)) {
        if (ec) {
          ec.clear();
          continue;
        }
        if (entry.is_regular_file(ec) && entry.path().extension() == ".wz") {
          uiWzPath = entry.path().string();
          break;
        }
        ec.clear();
      }
    }
  } else {
    uiWzPath = (wz::to_path(mapleStoryPath) / "UI.wz").string();
  }

  if (uiWzPath.empty() || !fs::is_regular_file(uiWzPath, ec) || ec)
    return false;

  WzFile wzFile(uiWzPath, encryption);
  if (wzFile.ParseWzFile() != WzFileParseStatus::Success) return false;

  WzDirectory* wzDir = wzFile.GetWzDirectory();
  if (!wzDir) return false;

  WzImage* uiWindow2 = wzDir->GetImageByName("UIWindow2.img");
  if (!uiWindow2) return false;

  auto parseImageResult = uiWindow2->ParseImage();
  if (!parseImageResult.has_value()) return false;
  bool result = IsBigBang2Update(uiWindow2);

  return result;
}

bool WzFileManager::DetectBetaDataWzFormat(
    const std::string& baseDirectoryPath) {
  std::error_code ec;
  if (!fs::is_directory(baseDirectoryPath, ec) || ec) return false;

  fs::path dataWzPath = wz::to_path(baseDirectoryPath) / "Data.wz";
  if (!fs::is_regular_file(dataWzPath, ec) || ec) return false;
  ec.clear();

  if (fs::is_regular_file(wz::to_path(baseDirectoryPath) / "Skill.wz", ec) &&
      !ec)
    return false;
  ec.clear();
  if (fs::is_regular_file(wz::to_path(baseDirectoryPath) / "String.wz", ec) &&
      !ec)
    return false;
  ec.clear();
  if (fs::is_regular_file(wz::to_path(baseDirectoryPath) / "Character.wz",
                          ec) &&
      !ec)
    return false;
  ec.clear();

  fs::path dataDir = wz::to_path(baseDirectoryPath) / "Data";
  if (fs::is_directory(dataDir, ec) && !ec) {
    if (fs::is_directory(dataDir / "Skill", ec) && !ec) return false;
    ec.clear();
    if (fs::is_directory(dataDir / "String", ec) && !ec) return false;
    ec.clear();
    if (fs::is_directory(dataDir / "Character", ec) && !ec) return false;
    ec.clear();
  }

  return true;
}

void WzFileManager::BuildWzFileList() {
  if (built_wz_file_directory_list_) return;
  built_wz_file_directory_list_ = true;

  const std::string& searchBase = WzBaseDirectory();
  std::error_code ec;

  if (init_as_64bit_) {
    if (!fs::is_directory(searchBase, ec) || ec) return;

    for (auto it = fs::recursive_directory_iterator(searchBase, ec);
         it != fs::end(it);
         it.increment(ec)) {
      if (ec) {
        ec.clear();
        continue;
      }

      bool isDir = it->is_directory(ec);
      if (ec) {
        ec.clear();
        continue;
      }
      if (!isDir) continue;

      fs::path dirPath = it->path();
      std::string dirName = dirPath.filename().string();
      if (dirName == "Packs") continue;

      std::string dirStr = dirPath.string();
      if (IsExcluded(ToLower(dirStr))) continue;

      std::vector<std::string> iniFiles;
      for (auto fit = fs::directory_iterator(dirPath, ec); fit != fs::end(fit);
           fit.increment(ec)) {
        if (ec) {
          ec.clear();
          continue;
        }
        bool isReg = fit->is_regular_file(ec);
        if (ec) {
          ec.clear();
          continue;
        }
        if (isReg && fit->path().extension() == ".ini") {
          iniFiles.push_back(fit->path().string());
        }
      }
      if (iniFiles.size() != 1) continue;

      std::string iniFile = iniFiles[0];
      int wzFileIndex = -1;
      {
        std::ifstream iniStream(wz::to_path(iniFile));
        std::string line;
        while (std::getline(iniStream, line)) {
          auto pipePos = line.find('|');
          if (pipePos != std::string::npos &&
              line.substr(0, pipePos) == "LastWzIndex") {
            wzFileIndex = SafeStoi(line.substr(pipePos + 1));
            break;
          }
        }
      }
      if (wzFileIndex < 0) continue;

      std::string iniStem = wz::to_path(iniFile).stem().string();

      for (int i = 0; i <= wzFileIndex; i++) {
        char pad[64];
        snprintf(pad, sizeof(pad), "_%03d", i);
        std::string partialFileName = iniStem + std::string(pad) + ".wz";
        fs::path fullPath = dirPath / partialFileName;
        if (!fs::is_regular_file(fullPath, ec) || ec) {
          ec.clear();
          continue;
        }

        std::string fileNameNoExt = fullPath.stem().string();
        if (IsExcluded(ToLower(fileNameNoExt))) continue;

        std::string wzDirName = dirStr;
        std::string baseDirLower = ToLower(searchBase);
        if (wzDirName.size() > baseDirLower.size() &&
            ToLower(wzDirName.substr(0, baseDirLower.size())) == baseDirLower) {
          wzDirName = ToLower(wzDirName.substr(baseDirLower.size() + 1));
        } else {
          wzDirName = ToLower(dirName);
        }
        std::replace(wzDirName.begin(), wzDirName.end(), '\\', '/');

        wzFilesList_[wzDirName].push_back(ToLower(fileNameNoExt));

        bool bIsCanvas = ContainsCanvasDirectory(fullPath.string());
        if (!bIsCanvas) {
          wzFilesDirectoryList_[ToLower(fileNameNoExt)] = dirPath.string();
        } else {
          std::string canvasKey = wzDirName + "/" + ToLower(fileNameNoExt);
          wzFilesDirectoryList_[canvasKey] = dirPath.string();
        }
      }
    }
  } else {
    if (!fs::is_directory(searchBase, ec) || ec) return;

    for (auto it = fs::recursive_directory_iterator(searchBase, ec);
         it != fs::end(it);
         it.increment(ec)) {
      if (ec) {
        ec.clear();
        continue;
      }

      bool isReg = it->is_regular_file(ec);
      if (ec) {
        ec.clear();
        continue;
      }
      if (!isReg) continue;
      if (it->path().extension() != ".wz") continue;

      std::string wzPath = it->path().string();
      std::string dirStr = it->path().parent_path().string();

      if (IsExcluded(ToLower(wzPath))) continue;

      std::string fileName = it->path().filename().string();
      std::string fileNameNoExt = fileName.substr(0, fileName.size() - 3);

      std::string baseName = ToLower(fileNameNoExt);
      baseName.erase(
          std::remove_if(baseName.begin(),
                         baseName.end(),
                         [](unsigned char c) { return !std::isalpha(c); }),
          baseName.end());

      auto& vec = wzFilesList_[baseName];
      vec.push_back(ToLower(fileNameNoExt));

      if (wzFilesDirectoryList_.find(ToLower(fileNameNoExt)) ==
          wzFilesDirectoryList_.end()) {
        wzFilesDirectoryList_[ToLower(fileNameNoExt)] = dirStr;
      }
    }
  }
}

Result<WzFile*> WzFileManager::LoadWzFile(const std::string& baseName,
                                          WzMapleVersion encVersion) {
  std::string filePath = GetWzFilePath(baseName);
  if (filePath.empty()) return nullptr;

  auto wzf =
      std::make_unique<WzFile>(filePath, static_cast<int16_t>(-1), encVersion);

  WzFileParseStatus status = wzf->ParseWzFile();
  if (status != WzFileParseStatus::Success) {
    return std::unexpected(
        Error::ParseError("Error parsing " + baseName + ".wz (" +
                          GetErrorDescription(status) + ")"));
  }

  auto loadResult = LoadWzFile(baseName, std::move(wzf));
  if (!loadResult.has_value()) return nullptr;
  return loadResult.value();
}

Result<WzFile*> WzFileManager::LoadWzFile(const std::string& baseName,
                                          std::unique_ptr<WzFile> wzf) {
  if (!wzf) {
    return std::unexpected(Error::InvalidArgument("wzf is null"));
  }

  std::string key = ToLower(baseName);
  auto dotWz = key.find(".wz");
  if (dotWz != std::string::npos && dotWz == key.size() - 3) {
    key = key.substr(0, key.size() - 3);
  }

  if (wzFiles_.find(key) != wzFiles_.end()) {
    return std::unexpected(
        Error::InvalidArgument("WZ file has already been loaded: " + key));
  }

  WzFile* result = wzf.get();
  wzFiles_[key] = std::move(wzf);
  if (result->GetWzDirectory()) {
    wzDirs_[key] = result->GetWzDirectory();
  }

  return result;
}

bool WzFileManager::LoadLegacyDataWzFile(const std::string& baseName,
                                         WzMapleVersion encVersion) {
  std::string filePath = GetWzFilePath(baseName);
  if (filePath.empty()) return false;

  auto wzf =
      std::make_unique<WzFile>(filePath, static_cast<int16_t>(-1), encVersion);
  WzFileParseStatus status = wzf->ParseWzFile();
  if (status != WzFileParseStatus::Success) {
    return false;
  }

  std::string key = ToLower(baseName);
  auto dotWz = key.find(".wz");
  if (dotWz != std::string::npos && dotWz == key.size() - 3) {
    key = key.substr(0, key.size() - 3);
  }

  if (wzFiles_.find(key) != wzFiles_.end()) {
    return false;
  }

  auto* wzd = wzf->GetWzDirectory();
  if (wzd) {
    wzDirs_[key] = wzd;
    for (auto* subDir : wzd->WzDirectories()) {
      wzDirs_[ToLower(subDir->Name())] = subDir;
    }
  }
  wzFiles_[key] = std::move(wzf);
  return true;
}

Result<WzImage*> WzFileManager::LoadDataWzHotfixFile(
    const std::string& basePath, WzMapleVersion encVersion) {
  std::string filePath = basePath;
  if (!is_standalone_wz_file_ && !init_as_64bit_ && !baseDir_.empty()) {
    filePath = (wz::to_path(baseDir_) / basePath).string();
  }
  std::error_code ec;
  if (!fs::is_regular_file(filePath, ec) || ec)
    return std::unexpected(Error::IoError("File does not exist: " + filePath));

  std::ifstream fstream(wz::to_path(filePath), std::ios::binary);
  if (!fstream.is_open()) {
    return std::unexpected(Error::IoError("Failed to open file: " + filePath));
  }

  std::string key = ToLower(basePath);
  auto existing = wzImages_.find(key);
  if (existing != wzImages_.end()) {
    return existing->second.get();
  }

  auto fname = wz::to_path(filePath).filename().string();
  auto img = std::make_unique<WzImage>(fname, std::move(fstream), encVersion);
  auto parseResult = img->ParseImage();
  if (!parseResult.has_value()) {
    return std::unexpected(parseResult.error());
  }

  auto* result = img.get();
  wzImages_[key] = std::move(img);
  return result;
}

Result<void> WzFileManager::LoadCanvasSection(const std::string& canvasFolder,
                                              WzMapleVersion encVersion) {
  if (wzCanvasSectionLoaded_.find(canvasFolder) != wzCanvasSectionLoaded_.end())
    return {};

  const std::string& searchBase = WzBaseDirectory();
  fs::path canvasDir =
      wz::to_path(searchBase) / canvasFolder / CANVAS_DIRECTORY_NAME;
  std::error_code ec;
  if (!fs::is_directory(canvasDir, ec) || ec) return {};

  std::string iniFile;
  int wzFileIndex = -1;
  for (auto fit = fs::directory_iterator(canvasDir, ec); fit != fs::end(fit);
       fit.increment(ec)) {
    if (ec) {
      ec.clear();
      continue;
    }
    bool isReg = fit->is_regular_file(ec);
    if (ec) {
      ec.clear();
      continue;
    }
    if (isReg && fit->path().extension() == ".ini") {
      iniFile = fit->path().string();
      break;
    }
  }
  if (iniFile.empty()) return {};

  {
    std::ifstream iniStream(wz::to_path(iniFile));
    std::string line;
    while (std::getline(iniStream, line)) {
      auto pipePos = line.find('|');
      if (pipePos != std::string::npos &&
          line.substr(0, pipePos) == "LastWzIndex") {
        wzFileIndex = SafeStoi(line.substr(pipePos + 1));
        break;
      }
    }
  }
  if (wzFileIndex < 0) return {};

  std::string canvasLower = ToLower(CANVAS_DIRECTORY_NAME);
  std::string canvasBase =
      canvasFolder + "/" + canvasLower + "/" + canvasLower + "_0";
  for (int i = 0; i <= wzFileIndex; i++) {
    char pad[64];
    snprintf(pad, sizeof(pad), "%02d", i);
    std::string canvasKey = canvasBase + std::string(pad);
    if (!IsWzFileLoaded(canvasKey)) {
      auto result = LoadWzFile(canvasKey, encVersion);
      if (!result.has_value()) {
        return std::unexpected(result.error());
      }
    }
  }

  wzCanvasSectionLoaded_.insert(canvasFolder);
  return {};
}

bool WzFileManager::LoadListWzFile(WzMapleVersion fileVersion) {
  if (Is64Bit()) return false;
  if (!listWzPaths_.empty()) return true;

  const std::string listWzBaseName = "List";
  std::string filePath = GetWzFilePath(listWzBaseName);
  if (filePath.empty()) return false;

  auto entries = ListFileParser::ParseListFile(filePath, fileVersion);
  if (entries.empty()) return false;

  listWzPaths_.insert(listWzPaths_.end(), entries.begin(), entries.end());
  return true;
}

void WzFileManager::UnloadWzFile(WzFile* wzFile) {
  if (!wzFile) return;

  std::string keyFound;
  for (auto& pair : wzFiles_) {
    if (pair.second.get() == wzFile) {
      keyFound = pair.first;
      break;
    }
  }
  if (!keyFound.empty()) {
    wzFiles_.erase(keyFound);
    wzDirs_.erase(keyFound);
  }
  for (auto it = wzDirs_.begin(); it != wzDirs_.end();) {
    if (it->second && it->second->WzFileParent() == wzFile) {
      it = wzDirs_.erase(it);
    } else {
      ++it;
    }
  }
}

void WzFileManager::UnloadWzImgFile(WzImage* wzImage) {
  if (!wzImage) return;

  std::string keyFound;
  for (auto& pair : wzImages_) {
    if (pair.second.get() == wzImage) {
      keyFound = pair.first;
      break;
    }
  }
  if (!keyFound.empty()) {
    wzImages_.erase(keyFound);
  }
}

WzFileManager::~WzFileManager() {
  Clear();
  if (fileManager == this) {
    fileManager = nullptr;
  }
}

void WzFileManager::Clear() {
  wzFiles_.clear();
  wzImages_.clear();
  wzDirs_.clear();
  wzFilesList_.clear();
  wzFilesDirectoryList_.clear();
  wzCanvasSectionLoaded_.clear();
  listWzPaths_.clear();
  built_wz_file_directory_list_ = false;
}

bool WzFileManager::IsWzFileLoaded(const std::string& baseName) const {
  std::string key = ToLower(baseName);
  auto dotWz = key.find(".wz");
  if (dotWz != std::string::npos && dotWz == key.size() - 3) {
    key = key.substr(0, key.size() - 3);
  }
  return wzFiles_.find(key) != wzFiles_.end();
}

std::vector<std::string> WzFileManager::GetWzFileNameListFromBase(
    const std::string& baseName) const {
  if (is_pre_bb_data_wz_format_) {
    auto it = wzFilesList_.find("data");
    if (it != wzFilesList_.end()) return it->second;
    return {};
  }
  auto it = wzFilesList_.find(baseName);
  if (it != wzFilesList_.end()) return it->second;
  return {};
}

std::vector<WzDirectory*> WzFileManager::GetWzDirectoriesFromBase(
    const std::string& baseName, bool isCanvas) {
  auto fileNames = GetWzFileNameListFromBase(baseName);
  std::vector<WzDirectory*> result;

  if (is_pre_bb_data_wz_format_) {
    for (const auto& fn : fileNames) {
      (void)fn;
      auto* dir = GetDirectoryByName("data");
      if (dir) {
        auto* subDir = dir->GetDirectoryByName(baseName);
        if (subDir) result.push_back(subDir);
      }
    }
    return result;
  }

  if (isCanvas) {
    std::string canvasDir = baseName;
    std::replace(canvasDir.begin(), canvasDir.end(), '\\', '/');
    canvasDir += '/';
    for (const auto& name : fileNames) {
      auto* dir = GetDirectoryByName(canvasDir + name);
      if (dir) result.push_back(dir);
    }
  } else {
    for (const auto& name : fileNames) {
      auto* dir = GetDirectoryByName(name);
      if (dir) result.push_back(dir);
    }
  }
  return result;
}

WzObject* WzFileManager::FindWzImageByName(const std::string& baseWzName,
                                           const std::string& imageName) {
  auto dirs = GetWzDirectoriesFromBase(ToLower(baseWzName));
  for (auto* dir : dirs) {
    if (!dir) continue;
    if (imageName.empty()) return dir;
    auto* img = dynamic_cast<WzObject*>(dir->GetImageByName(imageName));
    if (img) return img;
  }
  return nullptr;
}

std::vector<WzObject*> WzFileManager::FindWzImagesByName(
    const std::string& baseWzName, const std::string& imageName) {
  std::vector<WzObject*> result;
  auto dirs = GetWzDirectoriesFromBase(ToLower(baseWzName));
  for (auto* dir : dirs) {
    if (!dir) continue;
    if (imageName.empty()) {
      result.push_back(dir);
    } else {
      auto* img = dir->GetImageByName(imageName);
      if (img) result.push_back(img);
    }
  }
  return result;
}

WzDirectory* WzFileManager::GetDirectoryByName(const std::string& name) const {
  auto it = wzDirs_.find(ToLower(name));
  if (it != wzDirs_.end()) return it->second;
  return nullptr;
}

bool WzFileManager::ContainsCanvasDirectory(const std::string& path) {
  std::string pathLower = ToLower(path);
  std::string canvasLower = ToLower(CANVAS_DIRECTORY_NAME);
  fs::path p(pathLower);
  for (const auto& part : p) {
    if (part.string() == canvasLower) return true;
  }
  return false;
}

std::string WzFileManager::NormaliseWzCanvasDirectory(
    const std::string& filePathOrBaseFileName) {
  std::string pathLower = ToLower(filePathOrBaseFileName);
  std::string canvasMarker = "/" + ToLower(CANVAS_DIRECTORY_NAME);
  auto pos = pathLower.find(canvasMarker);
  if (pos != std::string::npos) {
    return pathLower.substr(0, pos);
  }
  return {};
}

std::string WzFileManager::GetWzFilePath(
    const std::string& filePathOrBaseFileName) const {
  if (!built_wz_file_directory_list_ || is_standalone_wz_file_) {
    std::error_code ec;
    if (fs::is_regular_file(filePathOrBaseFileName, ec) && !ec)
      return filePathOrBaseFileName;
    return {};
  }

  std::string key = ToLower(filePathOrBaseFileName);
  auto dotWz = key.find(".wz");
  if (dotWz != std::string::npos && dotWz == key.size() - 3) {
    key = key.substr(0, key.size() - 3);
  }

  auto it = wzFilesDirectoryList_.find(key);
  if (it == wzFilesDirectoryList_.end()) {
    std::error_code ec;
    if (fs::is_regular_file(filePathOrBaseFileName, ec) && !ec)
      return filePathOrBaseFileName;
    return {};
  }

  std::error_code ec;
  if (!ContainsCanvasDirectory(key)) {
    std::string fileName = key + ".wz";
    if (!fileName.empty()) {
      fileName[0] = static_cast<char>(
          std::toupper(static_cast<unsigned char>(fileName[0])));
    }
    fs::path filePath = wz::to_path(it->second) / fileName;
    if (fs::is_regular_file(filePath, ec) && !ec) return filePath.string();
  } else {
    fs::path filePath = wz::to_path(it->second) / (key + ".wz");
    if (fs::is_regular_file(filePath, ec) && !ec) return filePath.string();
  }
  return {};
}

}  // namespace wz

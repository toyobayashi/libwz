#include "wz/WzFile.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include "wz/Properties/WzCanvasProperty.h"
#include "wz/Properties/WzConvexProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Properties/WzVectorProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzPath.h"
#include "wz/Util/WzTool.h"
#include "wz/WzAESConstant.h"
#include "wz/WzDirectory.h"
#include "wz/WzFileManager.h"
#include "wz/WzImage.h"

namespace wz {

WzFile::WzFile(int16_t gameVersion, WzMapleVersion version)
    : mapleStoryPatchVersion_(gameVersion), maplepLocalVersion_(version) {
  header_ = WzHeader::GetDefault();
  wz_iv_ = WzTool::GetIvByMapleVersion(version);
  wzDir_ = new WzDirectory(nullptr, "", 0, wz_iv_, this);
}

WzFile::WzFile(const std::string& filePath, WzMapleVersion version)
    : WzFile(filePath, -1, version) {}

WzFile::WzFile(const std::string& filePath,
               int16_t gameVersion,
               WzMapleVersion version)
    : path_(filePath),
      mapleStoryPatchVersion_(gameVersion),
      maplepLocalVersion_(version) {
  name_ = wz::to_path(filePath).filename().string();
  wz_iv_ = WzTool::GetIvByMapleVersion(version);
}

WzFile::WzFile(const std::string& filePath, const std::array<uint8_t, 4>& wzIv)
    : path_(filePath), wz_iv_(wzIv) {
  name_ = wz::to_path(filePath).filename().string();
  mapleStoryPatchVersion_ = -1;
  maplepLocalVersion_ = WzMapleVersion::CUSTOM;
}

WzFile::~WzFile() {
  isUnloaded_ = true;
  if (wzDir_) {
    delete wzDir_;
    wzDir_ = nullptr;
  }
  if (fileReader_.has_value()) {
    fileReader_->Close();
    fileReader_.reset();
  }
  if (fileStream_.is_open()) fileStream_.close();
  path_.clear();
  name_.clear();
}

uint32_t WzFile::CheckAndGetVersionHash(int wzVersionHeader,
                                        int maplestoryPatchVersion) {
  std::string verStr = std::to_string(maplestoryPatchVersion);
  int32_t versionHash = 0;
  for (char ch : verStr) {
    versionHash = (versionHash * 32) + static_cast<uint8_t>(ch) + 1;
  }

  if (wzVersionHeader == 770) return static_cast<uint32_t>(versionHash);

  int decryptedVersionNumber = static_cast<uint8_t>(
      ~(((versionHash >> 24) & 0xFF) ^ ((versionHash >> 16) & 0xFF) ^
        ((versionHash >> 8) & 0xFF) ^ (versionHash & 0xFF)));

  if (wzVersionHeader == decryptedVersionNumber)
    return static_cast<uint32_t>(versionHash);
  return 0;
}

void WzFile::Check64BitClient(WzBinaryReader* reader) {
  if (header_.FSize() >= 2) {
    reader->SetPosition(header_.FStart());
    int encver = reader->ReadUInt16();
    if (encver > 0xff) {
      wz_withEncryptVersionHeader_ = false;
    } else if (encver == 0x80) {
      if (header_.FSize() >= 5) {
        reader->SetPosition(header_.FStart());
        int propCount = reader->ReadInt32();
        if (propCount > 0 && (propCount & 0xff) == 0 && propCount <= 0xffff) {
          wz_withEncryptVersionHeader_ = false;
        }
      }
    }
    reader->SetPosition(header_.FStart());
  } else {
    wz_withEncryptVersionHeader_ = false;
  }
}

bool WzFile::TryDecodeWithWZVersionNumber(WzBinaryReader* reader,
                                          int useWzVersionHeader,
                                          int useMapleStoryPatchVersion) {
  mapleStoryPatchVersion_ = static_cast<int16_t>(useMapleStoryPatchVersion);
  versionHash_ =
      CheckAndGetVersionHash(useWzVersionHeader, mapleStoryPatchVersion_);
  if (versionHash_ == 0) return false;

  reader->SetHash(versionHash_);
  int64_t fallbackOffsetPosition = reader->Position();

  std::unique_ptr<WzDirectory> testDirectory =
      std::make_unique<WzDirectory>(reader, name_, versionHash_, wz_iv_, this);
  auto parseResult = testDirectory->ParseDirectory();
  if (!parseResult.has_value()) {
    reader->SetPosition(fallbackOffsetPosition);
    return false;
  }

  // Check first image to validate
  if (!testDirectory->WzImages().empty()) {
    auto* testImage = testDirectory->WzImages()[0];
    reader->SetPosition(testImage->Offset());
    uint8_t checkByte = reader->ReadByte();
    reader->SetPosition(fallbackOffsetPosition);

    if (checkByte == 0x73 || checkByte == 0x1b) {
      delete wzDir_;
      wzDir_ = testDirectory.release();
      return true;
    }
    // C# returns true for any checkByte (0x30, 0x6C, 0xBC, etc.)
    reader->SetPosition(fallbackOffsetPosition);
    delete wzDir_;
    wzDir_ = testDirectory.release();
    return true;
  } else {
    // No images - might be Base.wz
    if (Is64BitWzFile() && mapleStoryPatchVersion_ == 113) {
      reader->SetPosition(fallbackOffsetPosition);
      return false;
    }
    delete wzDir_;
    wzDir_ = testDirectory.release();
    return true;
  }
}

WzFileParseStatus WzFile::ParseMainWzDirectory() {
  if (path_.empty()) {
    return WzFileParseStatus::Path_Is_Null;
  }

  if (wzDir_) {
    delete wzDir_;
    wzDir_ = nullptr;
  }
  fileReader_.reset();
  if (fileStream_.is_open()) fileStream_.close();
  fileStream_ = std::ifstream(wz::to_path(path_), std::ios::binary);
  if (!fileStream_.is_open()) {
    return WzFileParseStatus::Failed_Unknown;
  }

  fileReader_.emplace(fileStream_, wz_iv_);
  WzBinaryReader& fileReader = *fileReader_;

  header_.SetIdent(fileReader.ReadString(4));
  header_.SetFSize(fileReader.ReadUInt64());
  header_.SetFStart(fileReader.ReadUInt32());
  header_.SetCopyright(
      fileReader.ReadString(static_cast<size_t>(header_.FStart() - 17U)));

  fileReader.ReadByte();  // unk1
  fileReader.ReadBytes(static_cast<int>(
      header_.FStart() - static_cast<uint64_t>(fileReader.Position())));
  fileReader.SetHeader(&header_);

  Check64BitClient(&fileReader);
  wzVersionHeader_ = wz_withEncryptVersionHeader_ ? fileReader.ReadUInt16()
                                                  : wzVersionHeader64bit_start;

  if (mapleStoryPatchVersion_ == -1) {
    // Try version range
    if (!wz_withEncryptVersionHeader_) {
      for (uint16_t v = wzVersionHeader64bit_start;
           v < wzVersionHeader64bit_start + 10;
           v++) {
        if (TryDecodeWithWZVersionNumber(&fileReader, wzVersionHeader_, v)) {
          return WzFileParseStatus::Success;
        }
      }
    }
    // Brute-force versions
    const int16_t MAX_PATCH_VERSION = 2000;
    for (int j = 0; j < MAX_PATCH_VERSION; j++) {
      if (TryDecodeWithWZVersionNumber(&fileReader, wzVersionHeader_, j)) {
        return WzFileParseStatus::Success;
      }
    }
    return WzFileParseStatus::Error_Game_Ver_Hash;
  } else {
    versionHash_ =
        CheckAndGetVersionHash(wzVersionHeader_, mapleStoryPatchVersion_);
    fileReader.SetHash(versionHash_);

    wzDir_ = new WzDirectory(&fileReader, name_, versionHash_, wz_iv_, this);
    auto parseResult = wzDir_->ParseDirectory();
    if (!parseResult.has_value()) {
      delete wzDir_;
      wzDir_ = nullptr;
      return WzFileParseStatus::Failed_Unknown;
    }
  }

  return WzFileParseStatus::Success;
}

WzFileParseStatus WzFile::ParseWzFile() {
  return ParseMainWzDirectory();
}

WzObject* WzFile::GetObjectFromPath(const std::string& path,
                                    bool checkFirstDirectoryName) {
  if (!wzDir_) return nullptr;

  if (!checkFirstDirectoryName) {
    // C# checkFirstDirectoryName=false: start from own WzDirectory
    WzObject* curObj = wzDir_;
    std::string remaining = path;
    while (!remaining.empty()) {
      size_t pos = remaining.find('/');
      std::string component;
      if (pos == std::string::npos) {
        component = remaining;
        remaining.clear();
      } else {
        component = remaining.substr(0, pos);
        remaining = remaining.substr(pos + 1);
      }
      if (component.empty()) continue;
      curObj = TraverseComponent(curObj, component);
      if (!curObj) return nullptr;
    }
    return curObj;
  }

  // C# checkFirstDirectoryName=true: cross-file lookup
  auto* mgr = WzFileManager::fileManager;
  if (!mgr) return nullptr;

  size_t firstSep = path.find('/');
  if (firstSep == std::string::npos) return wzDir_;

  std::string firstComponent = path.substr(0, firstSep);
  std::string remaining = path.substr(firstSep + 1);

  WzObject* curObj = nullptr;
  int pathIndex = 0;

  bool bIsCanvasDir = WzFileManager::ContainsCanvasDirectory(path);
  if (bIsCanvasDir) {
    std::string beforeCanvasPath =
        WzFileManager::NormaliseWzCanvasDirectory(path);
    std::replace(beforeCanvasPath.begin(), beforeCanvasPath.end(), '\\', '/');
    auto wzDir = mgr->GetWzDirectoriesFromBase(beforeCanvasPath, true);

    std::string canvasMarker =
        std::string("/") + WzFileManager::CANVAS_DIRECTORY_NAME + "/";
    size_t canvasPos = path.find(canvasMarker);
    std::string itemDirectoryPath =
        (canvasPos != std::string::npos)
            ? path.substr(canvasPos + canvasMarker.size())
            : path;

    size_t subPos = itemDirectoryPath.find('/');
    std::string firstItem = (subPos != std::string::npos)
                                ? itemDirectoryPath.substr(0, subPos)
                                : itemDirectoryPath;

    size_t count = 1;
    for (size_t p = itemDirectoryPath.find('/'); p != std::string::npos;
         p = itemDirectoryPath.find('/', p + 1)) {
      count++;
    }

    bool found = false;
    for (auto* dir : wzDir) {
      WzObject* inner = (*dir)[firstItem];
      if (inner) {
        curObj = inner;
        int totalSegments = 1;
        for (size_t p = path.find('/'); p != std::string::npos;
             p = path.find('/', p + 1)) {
          totalSegments++;
        }
        pathIndex = totalSegments - static_cast<int>(count) + 1;
        found = true;
        break;
      }
    }
    if (!found) return nullptr;
  } else {
    // Match C#: look up WzDirectory by first component name directly
    // from the manager's loaded directories. Bypass GetWzDirectoriesFromBase
    // because its pre-BB redirect may not apply here.
    std::string lookup = firstComponent;
    for (auto& c : lookup) c = static_cast<char>(std::tolower(c));

    WzDirectory* wzInnerDir = nullptr;
    // Try lookup as-is, then strip ".wz"
    for (int attempt = 0; attempt < 2 && !wzInnerDir; ++attempt) {
      std::string key = (attempt == 1 && lookup.size() > 3 &&
                         lookup.substr(lookup.size() - 3) == ".wz")
                            ? lookup.substr(0, lookup.size() - 3)
                            : lookup;
      for (const auto& pair : mgr->GetWzFiles()) {
        WzDirectory* dir = pair.second->GetWzDirectory();
        if (!dir) continue;
        std::string name = dir->Name();
        bool match = name.size() == firstComponent.size() &&
                     std::equal(name.begin(),
                                name.end(),
                                firstComponent.begin(),
                                [](char a, char b) {
                                  return std::tolower(a) == std::tolower(b);
                                });
        bool suffixMatch =
            (name.size() > 3 && name.size() - 3 == firstComponent.size() &&
             std::equal(name.begin(),
                        name.end() - 3,
                        firstComponent.begin(),
                        [](char a, char b) {
                          return std::tolower(a) == std::tolower(b);
                        }));
        if (match || suffixMatch) {
          wzInnerDir = dir;
          break;
        }
      }
    }

    if (wzInnerDir) {
      curObj = wzInnerDir;
      pathIndex = 1;
    } else {
      // Count total segments
      int totalSegments = 1;
      for (size_t p = path.find('/'); p != std::string::npos;
           p = path.find('/', p + 1)) {
        totalSegments++;
      }

      if (totalSegments >= 2) {
        size_t s2 = remaining.find('/');
        std::string seg2 =
            (s2 != std::string::npos) ? remaining.substr(0, s2) : remaining;
        std::string segRem =
            (s2 != std::string::npos) ? remaining.substr(s2 + 1) : "";

        curObj = mgr->FindWzImageByName(lookup, seg2);
        if (curObj) {
          pathIndex = 2;
        } else if (totalSegments >= 3) {
          std::string combinedBase = lookup + "/" + seg2;
          size_t s3 = segRem.find('/');
          std::string seg3 =
              (s3 != std::string::npos) ? segRem.substr(0, s3) : segRem;

          curObj = mgr->FindWzImageByName(combinedBase, seg3);
          if (curObj) {
            pathIndex = 3;
          } else {
            return nullptr;
          }
        } else {
          return nullptr;
        }
      } else {
        return nullptr;
      }
    }
  }

  if (!curObj) return nullptr;

  // Iterate remaining path segments from pathIndex
  int currentIdx = 0;
  size_t segStart = 0;
  for (size_t p = 0; p <= path.size(); ++p) {
    if (p == path.size() || path[p] == '/') {
      if (p > segStart) {
        if (currentIdx >= pathIndex) {
          std::string seg = path.substr(segStart, p - segStart);
          curObj = TraverseComponent(curObj, seg);
          if (!curObj) return nullptr;
        }
        currentIdx++;
      }
      segStart = p + 1;
    }
  }

  return curObj;
}

WzObject* WzFile::TraverseComponent(WzObject* curObj,
                                    const std::string& component) {
  if (!curObj) return nullptr;
  WzObjectType type = curObj->ObjectType();
  if (type == WzObjectType::Directory) {
    return (*static_cast<WzDirectory*>(curObj))[component];
  } else if (type == WzObjectType::Image) {
    return (*static_cast<WzImage*>(curObj))[component];
  } else if (type == WzObjectType::Property) {
    auto* prop = static_cast<WzImageProperty*>(curObj);
    auto pt = prop->PropertyType();
    if (pt == WzPropertyType::Canvas) {
      return (*static_cast<WzCanvasProperty*>(prop))[component];
    } else if (pt == WzPropertyType::Convex) {
      return (*static_cast<WzConvexProperty*>(prop))[component];
    } else if (pt == WzPropertyType::SubProperty) {
      return (*static_cast<WzSubProperty*>(prop))[component];
    } else if (pt == WzPropertyType::Vector) {
      // C# boxes int to WzObject; in C++ we cannot return int as WzObject*.
      // Use direct getter: static_cast<WzVectorProperty*>(prop)->X() / Y()
      return nullptr;
    }
    return nullptr;
  }
  return nullptr;
}

}  // namespace wz

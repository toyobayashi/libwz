#ifndef WZ_WZFILE_H_
#define WZ_WZFILE_H_
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "wz/Util/Defines.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzDataSource.h"
#include "wz/Util/WzStream.h"
#include "wz/WzDirectory.h"
#include "wz/WzEnums.h"
#include "wz/WzHeader.h"
#include "wz/WzObject.h"

namespace wz {

class IPropertyContainer;

class WzFile final : public WzObject {
 public:
  WzFile(short gameVersion, WzMapleVersion version);
  WzFile(const std::string& filePath, WzMapleVersion version);
  WzFile(const std::string& filePath,
         short gameVersion,
         WzMapleVersion version);
  WzFile(const std::string& fileName,
         std::shared_ptr<WzDataSource> source,
         short gameVersion,
         WzMapleVersion version);
  WzFile(const std::string& filePath, const std::array<uint8_t, 4>& wzIv);
  WzFile(const std::string& fileName,
         std::shared_ptr<WzDataSource> source,
         const std::array<uint8_t, 4>& wzIv);
  ~WzFile() override;

  WZ_DISALLOW_COPY_AND_MOVE(WzFile)

  WzObjectType ObjectType() const override { return WzObjectType::File; }
  WzObject* Parent() const override { return nullptr; }
  void SetParent(WzObject*) override {}
  WzFile* WzFileParent() const override { return const_cast<WzFile*>(this); }

  WzDirectory* GetWzDirectory() const { return wzDir_.get(); }
  void SetWzDirectory(std::unique_ptr<WzDirectory> dir) {
    wzDir_ = std::move(dir);
  }

  WzHeader* Header() { return &header_; }
  short Version() const { return mapleStoryPatchVersion_; }
  std::string FilePath() const { return path_; }
  WzMapleVersion MapleVersion() const { return maplepLocalVersion_; }
  bool Is64BitWzFile() const { return !wz_withEncryptVersionHeader_; }
  bool IsUnloaded() const { return isUnloaded_; }

  const std::array<uint8_t, 4>& WzIv() const { return wz_iv_; }
  uint32_t VersionHash() const { return versionHash_; }

  WzFileParseStatus ParseWzFile();
  WzObject* GetObjectFromPath(const std::string& path,
                              bool checkFirstDirectoryName = true);
  Result<void> SaveToDisk(
      const std::string& path,
      std::optional<bool> saveAs64Bit = std::nullopt,
      WzMapleVersion preferredVersion = WzMapleVersion::UNKNOWN);

 private:
  static WzObject* TraverseComponent(WzObject* curObj,
                                     const std::string& component);
  static constexpr uint16_t wzVersionHeader64bit_start = 770;
  void CreateWZVersionHash();

  std::string path_;
  std::shared_ptr<WzDataSource> source_;
  std::unique_ptr<WzDirectory> wzDir_;
  WzFileStream fileStream_;
  std::optional<WzBinaryReader> fileReader_;
  WzHeader header_;
  uint16_t wzVersionHeader_ = 0;
  uint32_t versionHash_ = 0;
  short mapleStoryPatchVersion_ = 0;
  WzMapleVersion maplepLocalVersion_ = WzMapleVersion::UNKNOWN;
  bool wz_withEncryptVersionHeader_ = true;
  std::array<uint8_t, 4> wz_iv_ = {};
  bool isUnloaded_ = false;

  WzFileParseStatus ParseMainWzDirectory();
  void Check64BitClient(WzBinaryReader* reader);
  bool TryDecodeWithWZVersionNumber(WzBinaryReader* reader,
                                    int useWzVersionHeader,
                                    int useMapleStoryPatchVersion);
  static uint32_t CheckAndGetVersionHash(int wzVersionHeader,
                                         int maplestoryPatchVersion);
};

}  // namespace wz
#endif  // WZ_WZFILE_H_

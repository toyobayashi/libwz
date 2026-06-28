#ifndef WZ_WZDIRECTORY_H_
#define WZ_WZDIRECTORY_H_
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "wz/Result.h"
#include "wz/Util/Defines.h"
#include "wz/Util/WzStream.h"
#include "wz/WzEnums.h"
#include "wz/WzObject.h"

namespace wz {

class WzBinaryReader;
class WzBinaryWriter;
class WzFile;
class WzImage;
class WzImageProperty;

class WzDirectory final : public WzObject {
 public:
  WzDirectory();
  explicit WzDirectory(const std::string& dirName);
  WzDirectory(WzBinaryReader* reader,
              const std::string& dirName,
              uint32_t verHash,
              const std::array<uint8_t, 4>& wzIv,
              WzFile* wzFile);
  ~WzDirectory() override;

  WZ_DISALLOW_COPY_AND_MOVE(WzDirectory)

  WzObjectType ObjectType() const override { return WzObjectType::Directory; }
  WzFile* WzFileParent() const override { return wzFile_; }
  Result<std::unique_ptr<WzObject>> Remove() override;

  int BlockSize() const { return size_; }
  void SetBlockSize(int v) { size_ = v; }
  int Checksum() const { return checksum_; }
  void SetChecksum(int v) { checksum_ = v; }
  int64_t Offset() const { return offset_; }
  void SetOffset(int64_t v) { offset_ = v; }

  std::vector<WzImage*> WzImages() const;
  std::vector<WzDirectory*> WzDirectories() const;

  WzBinaryReader* Reader() const { return reader_; }
  void SetWzFile(WzFile* f) { wzFile_ = f; }

  Result<void> ParseDirectory();
  Result<void> ParseImages();

  Result<void> AddImage(WzImage* img);
  Result<void> AddImage(std::unique_ptr<WzImage> img);
  Result<void> AddDirectory(WzDirectory* dir);
  Result<void> AddDirectory(std::unique_ptr<WzDirectory> dir);
  Result<WzDirectory*> CreateDirectory(const std::string& name);
  Result<WzImage*> CreateImage(const std::string& name);
  Result<std::unique_ptr<WzImage>> RemoveImage(WzImage* image);
  Result<std::unique_ptr<WzDirectory>> RemoveDirectory(WzDirectory* dir);
  void ClearImages();
  void ClearDirectories();
  WzImage* GetImageByName(const std::string& name);
  WzDirectory* GetDirectoryByName(const std::string& name);
  int CountImages() const;

  WzObject* operator[](const std::string& name) const;

  const std::array<uint8_t, 4>& WzIv() const { return wz_iv_; }
  void SetWzIv(const std::array<uint8_t, 4>& iv);
  uint32_t Hash() const { return hash_; }
  void SetHash(uint32_t h);

  Result<int> GenerateDataFile(const std::array<uint8_t, 4>* useIv,
                               bool isDefaultUserKey,
                               WzMemoryStream* tempStream);
  Result<void> SaveDirectory(WzBinaryWriter* writer);
  Result<void> SaveImages(WzBinaryWriter* writer, WzMemoryStream* tempStream);
  Result<uint32_t> GetOffsets(uint32_t currentOffset);
  Result<uint32_t> GetImgOffsets(uint32_t currentOffset);

 private:
  std::vector<std::unique_ptr<WzImage>> images_;
  std::vector<std::unique_ptr<WzDirectory>> subDirs_;
  WzBinaryReader* reader_ = nullptr;
  int64_t offset_ = 0;
  uint32_t hash_ = 0;
  int size_ = 0;
  int offsetSize_ = 0;
  int checksum_ = 0;
  std::array<uint8_t, 4> wz_iv_ = {};
  WzFile* wzFile_ = nullptr;
};

}  // namespace wz
#endif  // WZ_WZDIRECTORY_H_

#ifndef WZ_WZDIRECTORY_H_
#define WZ_WZDIRECTORY_H_
#include <array>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include "wz/Result.h"
#include "wz/Util/Defines.h"
#include "wz/WzEnums.h"
#include "wz/WzObject.h"

namespace wz {

class WzBinaryReader;
class WzBinaryWriter;
class WzFile;
class WzImage;

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
  void Remove() override;

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

  void AddImage(WzImage* img);
  void AddDirectory(WzDirectory* dir);
  void ClearImages();
  void ClearDirectories();
  WzImage* GetImageByName(const std::string& name);
  WzDirectory* GetDirectoryByName(const std::string& name);
  void RemoveImage(WzImage* image);
  void RemoveDirectory(WzDirectory* dir);
  int CountImages() const;

  WzObject* operator[](const std::string& name) const;

  const std::array<uint8_t, 4>& WzIv() const { return wz_iv_; }
  void SetWzIv(const std::array<uint8_t, 4>& iv);
  uint32_t Hash() const { return hash_; }
  void SetHash(uint32_t h);

  Result<int> GenerateDataFile(const std::array<uint8_t, 4>* useIv,
                               bool isDefaultUserKey,
                               std::ostream* tempStream);
  Result<void> SaveDirectory(WzBinaryWriter* writer);
  Result<void> SaveImages(WzBinaryWriter* writer, std::istream* tempStream);
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

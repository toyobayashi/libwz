#ifndef WZ_WZIMAGE_H_
#define WZ_WZIMAGE_H_
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "wz/Util/Defines.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/WzImageProperty.h"
#include "wz/WzObject.h"
#include "wz/WzPropertyCollection.h"

namespace wz {

class WzImage final : public WzObject, public IPropertyContainer {
 public:
  static constexpr int WzImageHeaderByte_WithoutOffset = 0x73;
  static constexpr int WzImageHeaderByte_WithOffset = 0x1B;

  WzImage();
  explicit WzImage(const std::string& name);
  WzImage(const std::string& name, WzBinaryReader& reader, int checksum);
  WzImage(const std::string& name,
          std::ifstream&& dataStream,
          WzMapleVersion mapleVersion);
  ~WzImage() override;

  WZ_DISALLOW_COPY_AND_MOVE(WzImage)

  WzObjectType ObjectType() const override;
  WzFile* WzFileParent() const override;
  void Remove() override;

  bool Parsed() const { return parsed_; }
  void SetParsed(bool v) { parsed_ = v; }
  bool Changed() const { return is_image_changed_; }
  void SetChanged(bool v) { is_image_changed_ = v; }
  int BlockSize() const { return size_; }
  void SetBlockSize(int v) { size_ = v; }
  int Checksum() const { return checksum_; }
  int64_t Offset() const { return offset_; }
  void SetOffset(int64_t v) { offset_ = v; }
  int BlockStart() const { return blockStart_; }
  int64_t TempFileStart() const { return tempFileStart_; }
  void SetTempFileStart(int64_t v) { tempFileStart_ = v; }
  int64_t TempFileEnd() const { return tempFileEnd_; }
  void SetTempFileEnd(int64_t v) { tempFileEnd_ = v; }
  bool ParseEverything() const { return parseEverything_; }
  void SetParseEverything(bool v) { parseEverything_ = v; }
  bool IsLuaWzImage() const;

  using IPropertyContainer::AddProperty;
  WzPropertyCollection* WzProperties() override;
  Result<WzPropertyCollection*> WzPropertiesResult();
  void AddProperty(WzImageProperty* prop) override;
  void AddProperty(std::unique_ptr<WzImageProperty> prop) override;
  void RemoveProperty(const std::string& propertyName) override;
  void RemoveProperty(WzImageProperty* prop) override;
  std::unique_ptr<WzImageProperty> TakeProperty(WzImageProperty* prop);
  void ClearProperties() override;

  WzImageProperty* GetFromPath(const std::string& path);
  Result<WzImageProperty*> GetFromPathResult(const std::string& path);
  Result<bool> ParseImage(bool forceReadFromData = false);

  WzImageProperty* operator[](const std::string& name);
  Result<WzImageProperty*> GetPropertyByName(const std::string& name);
  Result<void> SaveImage(WzBinaryWriter* writer,
                         bool isDefaultUserKey = true,
                         bool forceReadFromData = false);
  void CalculateAndSetImageChecksum(const std::vector<uint8_t>& bytes);
  void UnparseImage();

  WzBinaryReader* Reader() const { return reader_; }

 private:
  Result<void> EnsureParsed();

  bool parsed_ = false;
  bool is_image_changed_ = false;
  int size_ = 0;
  int checksum_ = 0;
  int64_t offset_ = 0;
  int64_t tempFileStart_ = 0;
  int64_t tempFileEnd_ = 0;
  int blockStart_ = 0;
  bool parseEverything_ = false;
  std::ifstream dataStream_;
  std::optional<WzBinaryReader> ownedReader_;
  WzBinaryReader* reader_ = nullptr;
  WzPropertyCollection properties_;
};

}  // namespace wz
#endif  // WZ_WZIMAGE_H_

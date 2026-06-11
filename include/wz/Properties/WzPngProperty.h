#ifndef WZ_PROPERTIES_WZPNGPROPERTY_H_
#define WZ_PROPERTIES_WZPNGPROPERTY_H_
#include <cstdint>
#include <vector>
#include "wz/Result.h"
#include "wz/WzEnums.h"
#include "wz/WzImageProperty.h"

namespace wz {
class WzBinaryReader;
class WzPngProperty : public WzImageProperty {
 public:
  WzPngProperty();
  WzPngProperty(WzBinaryReader* reader, bool parseNow);
  WZ_DISALLOW_COPY_AND_MOVE(WzPngProperty)

  WzPropertyType PropertyType() const override { return WzPropertyType::PNG; }
  std::string Name() const override { return "PNG"; }
  void SetName(const std::string&) override {}

  int Width() const { return width_; }
  int Height() const { return height_; }
  WzPngFormat Format() const { return format_; }
  bool ListWzUsed() const { return listWzUsed_; }

  Result<std::vector<uint8_t>> GetImage(bool saveInMemory);
  Result<std::vector<uint8_t>> GetCompressedBytes(bool saveInMemory);
  Result<std::vector<uint8_t>> GetCompressedBytesForExtraction(
      bool saveInMemory);
  Result<std::vector<uint8_t>> GetRawImage(bool saveInMemory);
  void ParsePng(bool saveInMemory);

  bool SaveToFile(const std::string& filePath);

  WzBinaryReader* Reader() const { return wzReader_; }

 private:
  int width_ = 0, height_ = 0;
  WzPngFormat format_ = WzPngFormat::Format1;
  std::vector<uint8_t> compressedImageBytes_;
  std::vector<uint8_t> pngData_;
  bool listWzUsed_ = false;
  WzBinaryReader* wzReader_ = nullptr;
  int64_t offs_ = 0;

  int GetUncompressedSize() const;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZPNGPROPERTY_H_

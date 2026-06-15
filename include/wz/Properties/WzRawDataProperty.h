#ifndef WZ_PROPERTIES_WZRAWDATAPROPERTY_H_
#define WZ_PROPERTIES_WZRAWDATAPROPERTY_H_
#include <cstdint>
#include <vector>
#include "wz/Result.h"
#include "wz/WzImageProperty.h"
#include "wz/WzPropertyCollection.h"

namespace wz {
class WzBinaryReader;
class WzRawDataProperty : public WzImageProperty, public IPropertyContainer {
 public:
  static constexpr const char* RAW_DATA_HEADER = "RawData";
  WzRawDataProperty(const std::string& name,
                    WzBinaryReader* reader,
                    uint8_t type);
  ~WzRawDataProperty() override;
  WZ_DISALLOW_COPY_AND_MOVE(WzRawDataProperty)

  WzPropertyType PropertyType() const override { return WzPropertyType::Raw; }
  bool IsRawDataProperty() const override { return true; }
  WzPropertyCollection* WzProperties() override { return &properties_; }
  void AddProperty(WzImageProperty* prop) override;
  void RemoveProperty(const std::string& propertyName) override;
  void RemoveProperty(WzImageProperty* prop) override;
  void ClearProperties() override;
  Result<std::vector<uint8_t>> GetBytes() override;
  Result<std::vector<uint8_t>> GetBytes(bool saveInMemory);
  void Parse(bool parseNow);
  uint8_t Type() const { return type_; }
  bool SaveToFile(const std::string& filePath);

 private:
  WzBinaryReader* wzReader_ = nullptr;
  uint8_t type_ = 0;
  int64_t rawDataOffset_ = 0;
  int length_ = 0;
  std::vector<uint8_t> bytes_;
  WzPropertyCollection properties_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZRAWDATAPROPERTY_H_

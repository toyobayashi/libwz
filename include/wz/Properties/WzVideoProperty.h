#ifndef WZ_PROPERTIES_WZVIDEOPROPERTY_H_
#define WZ_PROPERTIES_WZVIDEOPROPERTY_H_
#include <cstdint>
#include <vector>
#include "wz/Result.h"
#include "wz/WzImageProperty.h"
#include "wz/WzPropertyCollection.h"

namespace wz {
class WzBinaryReader;
class WzVideoProperty : public WzImageProperty, public IPropertyContainer {
 public:
  static constexpr const char* CANVAS_VIDEO_HEADER = "Canvas#Video";
  WzVideoProperty(const std::string& name, WzBinaryReader* reader);
  ~WzVideoProperty() override;
  WZ_DISALLOW_COPY_AND_MOVE(WzVideoProperty)

  WzPropertyType PropertyType() const override { return WzPropertyType::Raw; }
  bool IsVideoProperty() const override { return true; }
  Result<void> WriteValue(WzBinaryWriter* writer) const override;
  WzPropertyCollection* WzProperties() override { return &properties_; }
  using IPropertyContainer::AddProperty;
  Result<void> AddProperty(WzImageProperty* prop) override;
  Result<void> AddProperty(std::unique_ptr<WzImageProperty> prop) override;
  Result<std::unique_ptr<WzImageProperty>> RemoveProperty(
      const std::string& propertyName) override;
  Result<std::unique_ptr<WzImageProperty>> RemoveProperty(
      WzImageProperty* prop) override;
  Result<void> ClearProperties() override;
  Result<std::vector<uint8_t>> GetBytes() override;
  Result<std::vector<uint8_t>> GetBytes(bool saveInMemory);
  void Parse(bool parseNow);

 private:
  WzBinaryReader* wzReader_ = nullptr;
  int64_t offset_ = 0;
  int length_ = 0;
  int type_ = 0;
  std::vector<uint8_t> bytes_;
  WzPropertyCollection properties_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZVIDEOPROPERTY_H_

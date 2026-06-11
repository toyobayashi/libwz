#ifndef WZ_PROPERTIES_WZSTRINGPROPERTY_H_
#define WZ_PROPERTIES_WZSTRINGPROPERTY_H_
#include "wz/WzImageProperty.h"
namespace wz {
class WzStringProperty : public WzImageProperty {
 public:
  WzStringProperty() = default;
  WzStringProperty(const std::string& name, const std::string& value)
      : value_(value) {
    SetName(name);
  }
  WZ_DISALLOW_COPY_AND_MOVE(WzStringProperty)
  WzPropertyType PropertyType() const override {
    return WzPropertyType::String;
  }
  const std::string& Value() const { return value_; }
  void SetValue(const std::string& value) override { value_ = value; }
  std::string GetString() const override { return value_; }
  int32_t GetInt() const override;
  int16_t GetShort() const override;
  int64_t GetLong() const override;
  bool SaveToFile(const std::string& filePath);

 private:
  std::string value_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZSTRINGPROPERTY_H_

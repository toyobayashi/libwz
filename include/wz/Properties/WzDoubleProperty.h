#ifndef WZ_PROPERTIES_WZDOUBLEPROPERTY_H_
#define WZ_PROPERTIES_WZDOUBLEPROPERTY_H_
#include "wz/WzImageProperty.h"
namespace wz {
class WzDoubleProperty : public WzImageProperty {
 public:
  WzDoubleProperty() = default;
  WzDoubleProperty(const std::string& name, double value) : value_(value) {
    SetName(name);
  }
  WZ_DISALLOW_COPY_AND_MOVE(WzDoubleProperty)
  WzPropertyType PropertyType() const override {
    return WzPropertyType::Double;
  }
  double Value() const { return value_; }
  Result<void> WriteValue(WzBinaryWriter* writer) const override;
  void SetValue(double value) override {
    if (value_ == value) return;
    value_ = value;
    MarkParentImageChanged();
  }
  double GetDouble() const override { return value_; }
  float GetFloat() const override { return static_cast<float>(value_); }
  int32_t GetInt() const override { return static_cast<int32_t>(value_); }
  int16_t GetShort() const override { return static_cast<int16_t>(value_); }
  int64_t GetLong() const override { return static_cast<int64_t>(value_); }

 private:
  double value_ = 0.0;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZDOUBLEPROPERTY_H_

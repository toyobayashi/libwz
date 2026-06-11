#ifndef WZ_PROPERTIES_WZFLOATPROPERTY_H_
#define WZ_PROPERTIES_WZFLOATPROPERTY_H_
#include "wz/WzImageProperty.h"
namespace wz {
class WzFloatProperty : public WzImageProperty {
 public:
  WzFloatProperty() = default;
  WzFloatProperty(const std::string& name, float value) : value_(value) {
    SetName(name);
  }
  WZ_DISALLOW_COPY_AND_MOVE(WzFloatProperty)
  WzPropertyType PropertyType() const override { return WzPropertyType::Float; }
  float Value() const { return value_; }
  void SetValue(float value) override { value_ = value; }
  float GetFloat() const override { return value_; }
  double GetDouble() const override { return value_; }
  int32_t GetInt() const override { return static_cast<int32_t>(value_); }
  int16_t GetShort() const override { return static_cast<int16_t>(value_); }
  int64_t GetLong() const override { return static_cast<int64_t>(value_); }

 private:
  float value_ = 0.0f;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZFLOATPROPERTY_H_

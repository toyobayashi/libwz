#ifndef WZ_PROPERTIES_WZLONGPROPERTY_H_
#define WZ_PROPERTIES_WZLONGPROPERTY_H_
#include "wz/WzImageProperty.h"
namespace wz {
class WzLongProperty : public WzImageProperty {
 public:
  WzLongProperty() = default;
  WzLongProperty(const std::string& name, int64_t value) : value_(value) {
    SetName(name);
  }
  WZ_DISALLOW_COPY_AND_MOVE(WzLongProperty)
  WzPropertyType PropertyType() const override { return WzPropertyType::Long; }
  int64_t Value() const { return value_; }
  void SetValue(int64_t value) override { value_ = value; }
  int32_t GetInt() const override { return static_cast<int32_t>(value_); }
  int16_t GetShort() const override { return static_cast<int16_t>(value_); }
  int64_t GetLong() const override { return value_; }
  float GetFloat() const override { return static_cast<float>(value_); }
  double GetDouble() const override { return static_cast<double>(value_); }

 private:
  int64_t value_ = 0;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZLONGPROPERTY_H_

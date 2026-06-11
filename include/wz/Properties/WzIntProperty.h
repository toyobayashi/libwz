#ifndef WZ_PROPERTIES_WZINTPROPERTY_H_
#define WZ_PROPERTIES_WZINTPROPERTY_H_
#include "wz/WzImageProperty.h"
namespace wz {
class WzIntProperty : public WzImageProperty {
 public:
  WzIntProperty() = default;
  WzIntProperty(const std::string& name, int32_t value) : value_(value) {
    SetName(name);
  }
  WZ_DISALLOW_COPY_AND_MOVE(WzIntProperty)
  WzPropertyType PropertyType() const override { return WzPropertyType::Int; }
  int32_t Value() const { return value_; }
  void SetValue(int32_t value) override { value_ = value; }
  int32_t GetInt() const override { return value_; }
  int16_t GetShort() const override { return static_cast<int16_t>(value_); }
  int64_t GetLong() const override { return value_; }
  float GetFloat() const override { return static_cast<float>(value_); }
  double GetDouble() const override { return static_cast<double>(value_); }

 private:
  int32_t value_ = 0;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZINTPROPERTY_H_

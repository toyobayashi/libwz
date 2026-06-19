#ifndef WZ_PROPERTIES_WZSHORTPROPERTY_H_
#define WZ_PROPERTIES_WZSHORTPROPERTY_H_
#include "wz/WzImageProperty.h"
namespace wz {
class WzShortProperty : public WzImageProperty {
 public:
  WzShortProperty() = default;
  WzShortProperty(const std::string& name, int16_t value) : value_(value) {
    SetName(name);
  }
  WZ_DISALLOW_COPY_AND_MOVE(WzShortProperty)
  WzPropertyType PropertyType() const override { return WzPropertyType::Short; }
  int16_t Value() const { return value_; }
  Result<void> WriteValue(WzBinaryWriter* writer) const override;
  void SetValue(int32_t value) override {
    auto next = static_cast<int16_t>(value);
    if (value_ == next) return;
    value_ = next;
    MarkParentImageChanged();
  }
  int32_t GetInt() const override { return value_; }
  int16_t GetShort() const override { return value_; }
  int64_t GetLong() const override { return value_; }
  float GetFloat() const override { return static_cast<float>(value_); }
  double GetDouble() const override { return static_cast<double>(value_); }

 private:
  int16_t value_ = 0;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZSHORTPROPERTY_H_

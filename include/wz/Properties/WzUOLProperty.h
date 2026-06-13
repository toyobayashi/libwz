#ifndef WZ_PROPERTIES_WZUOLPROPERTY_H_
#define WZ_PROPERTIES_WZUOLPROPERTY_H_
#include <string>
#include <vector>
#include "wz/Result.h"
#include "wz/WzImageProperty.h"

namespace wz {
class WzUOLProperty : public WzImageProperty {
 public:
  WzUOLProperty() = default;
  WzUOLProperty(const std::string& name, const std::string& value);
  WZ_DISALLOW_COPY_AND_MOVE(WzUOLProperty)
  WzPropertyType PropertyType() const override { return WzPropertyType::UOL; }
  const std::string& Value() const { return val_; }
  void SetValue(const std::string& value) override { val_ = value; }

  // All accessors delegate to the resolved LinkValue (matches C# UOLRES path)
  WzPropertyCollection* WzProperties() override;
  WzImageProperty* operator[](const std::string& name) override;
  WzImageProperty* GetFromPath(const std::string& path) override;

  int32_t GetInt() const override;
  int16_t GetShort() const override;
  int64_t GetLong() const override;
  float GetFloat() const override;
  double GetDouble() const override;
  std::string GetString() const override;
  Result<std::vector<uint8_t>> GetBytes() override;

  WzObject* LinkValue() const;

 private:
  std::string val_;
  mutable WzObject* linkVal_ = nullptr;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZUOLPROPERTY_H_

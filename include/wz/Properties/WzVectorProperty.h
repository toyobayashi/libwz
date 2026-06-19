#ifndef WZ_PROPERTIES_WZVECTORPROPERTY_H_
#define WZ_PROPERTIES_WZVECTORPROPERTY_H_
#include <memory>
#include "wz/Properties/WzIntProperty.h"
#include "wz/WzImageProperty.h"

namespace wz {
class WzVectorProperty : public WzImageProperty {
 public:
  WzVectorProperty() = default;
  ~WzVectorProperty() override;
  WZ_DISALLOW_COPY_AND_MOVE(WzVectorProperty)
  explicit WzVectorProperty(const std::string& name) { SetName(name); }
  WzVectorProperty(const std::string& name,
                   std::unique_ptr<WzIntProperty> x,
                   std::unique_ptr<WzIntProperty> y)
      : X(std::move(x)), Y(std::move(y)) {
    SetName(name);
  }
  WzVectorProperty(const std::string& name, int32_t x, int32_t y)
      : X(std::make_unique<WzIntProperty>("", x)),
        Y(std::make_unique<WzIntProperty>("", y)) {
    SetName(name);
  }
  WzVectorProperty(const std::string& name, float x, float y)
      : X(std::make_unique<WzIntProperty>("", static_cast<int32_t>(x))),
        Y(std::make_unique<WzIntProperty>("", static_cast<int32_t>(y))) {
    SetName(name);
  }
  WzPropertyType PropertyType() const override {
    return WzPropertyType::Vector;
  }
  Result<void> WriteValue(WzBinaryWriter* writer) const override;
  std::unique_ptr<WzIntProperty> X;
  std::unique_ptr<WzIntProperty> Y;

 private:
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZVECTORPROPERTY_H_

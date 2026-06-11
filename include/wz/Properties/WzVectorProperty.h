#ifndef WZ_PROPERTIES_WZVECTORPROPERTY_H_
#define WZ_PROPERTIES_WZVECTORPROPERTY_H_
#include "wz/Properties/WzIntProperty.h"
#include "wz/WzImageProperty.h"

namespace wz {
class WzVectorProperty : public WzImageProperty {
 public:
  WzVectorProperty() = default;
  ~WzVectorProperty() override;
  WZ_DISALLOW_COPY_AND_MOVE(WzVectorProperty)
  explicit WzVectorProperty(const std::string& name) { SetName(name); }
  WzVectorProperty(const std::string& name, WzIntProperty* x, WzIntProperty* y)
      : X(x), Y(y) {
    SetName(name);
  }
  WzVectorProperty(const std::string& name, int32_t x, int32_t y)
      : X(new WzIntProperty("", x)), Y(new WzIntProperty("", y)) {
    SetName(name);
  }
  WzVectorProperty(const std::string& name, float x, float y)
      : X(new WzIntProperty("", static_cast<int32_t>(x))),
        Y(new WzIntProperty("", static_cast<int32_t>(y))) {
    SetName(name);
  }
  WzPropertyType PropertyType() const override {
    return WzPropertyType::Vector;
  }
  WzIntProperty* X = nullptr;
  WzIntProperty* Y = nullptr;

 private:
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZVECTORPROPERTY_H_

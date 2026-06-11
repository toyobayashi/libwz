#ifndef WZ_PROPERTIES_WZUOLPROPERTY_H_
#define WZ_PROPERTIES_WZUOLPROPERTY_H_
#include <string>
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
  std::string GetString() const override { return val_; }
  WzObject* ResolveLinkValue();

 private:
  std::string val_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZUOLPROPERTY_H_

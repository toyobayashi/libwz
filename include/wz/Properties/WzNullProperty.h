#ifndef WZ_PROPERTIES_WZNULLPROPERTY_H_
#define WZ_PROPERTIES_WZNULLPROPERTY_H_
#include "wz/WzImageProperty.h"

namespace wz {
class WzNullProperty : public WzImageProperty {
 public:
  WzNullProperty() = default;
  explicit WzNullProperty(const std::string& name) { SetName(name); }
  WZ_DISALLOW_COPY_AND_MOVE(WzNullProperty)
  WzPropertyType PropertyType() const override { return WzPropertyType::Null; }
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZNULLPROPERTY_H_

#ifndef WZ_PROPERTIES_WZCONVEXPROPERTY_H_
#define WZ_PROPERTIES_WZCONVEXPROPERTY_H_
#include "wz/WzImageProperty.h"
#include "wz/WzPropertyCollection.h"

namespace wz {
class WzConvexProperty : public WzImageProperty, public IPropertyContainer {
 public:
  WzConvexProperty();
  explicit WzConvexProperty(const std::string& name);
  ~WzConvexProperty() override;
  WZ_DISALLOW_COPY_AND_MOVE(WzConvexProperty)

  WzPropertyType PropertyType() const override {
    return WzPropertyType::Convex;
  }
  WzPropertyCollection* WzProperties() override { return &properties_; }
  void AddProperty(WzImageProperty* prop) override;
  void RemoveProperty(const std::string& propertyName) override;
  void RemoveProperty(WzImageProperty* prop) override;
  void ClearProperties() override;
  WzImageProperty* operator[](const std::string& name) override;

 private:
  WzPropertyCollection properties_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZCONVEXPROPERTY_H_

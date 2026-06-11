#ifndef WZ_PROPERTIES_WZSUBPROPERTY_H_
#define WZ_PROPERTIES_WZSUBPROPERTY_H_
#include <string>
#include "wz/WzImageProperty.h"
#include "wz/WzPropertyCollection.h"

namespace wz {
class WzSubProperty : public WzImageProperty, public IPropertyContainer {
 public:
  WzSubProperty();
  explicit WzSubProperty(const std::string& name);
  ~WzSubProperty() override;
  WZ_DISALLOW_COPY_AND_MOVE(WzSubProperty)

  WzPropertyType PropertyType() const override {
    return WzPropertyType::SubProperty;
  }
  WzPropertyCollection* WzProperties() override { return &properties_; }
  void AddProperty(WzImageProperty* prop) override;
  void RemoveProperty(const std::string& propertyName) override;
  void RemoveProperty(WzImageProperty* prop) override;
  void ClearProperties() override;
  WzImageProperty* operator[](const std::string& name) override;
  WzImageProperty* GetFromPath(const std::string& path) override;

 private:
  WzPropertyCollection properties_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZSUBPROPERTY_H_

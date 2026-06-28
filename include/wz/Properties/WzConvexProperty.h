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
  Result<void> WriteValue(WzBinaryWriter* writer) const override;
  WzPropertyCollection* WzProperties() override { return &properties_; }
  using IPropertyContainer::AddProperty;
  Result<void> AddProperty(WzImageProperty* prop) override;
  Result<void> AddProperty(std::unique_ptr<WzImageProperty> prop) override;
  Result<std::unique_ptr<WzImageProperty>> RemoveProperty(
      const std::string& propertyName) override;
  Result<std::unique_ptr<WzImageProperty>> RemoveProperty(
      WzImageProperty* prop) override;
  Result<void> ClearProperties() override;
  WzImageProperty* operator[](const std::string& name) override;
  WzImageProperty* GetFromPath(const std::string& path) override;

 private:
  WzPropertyCollection properties_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZCONVEXPROPERTY_H_

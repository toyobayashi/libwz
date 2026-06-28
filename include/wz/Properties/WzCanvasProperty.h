#ifndef WZ_PROPERTIES_WZCANVASPROPERTY_H_
#define WZ_PROPERTIES_WZCANVASPROPERTY_H_
#include <memory>
#include <string>
#include "wz/Properties/WzPngProperty.h"
#include "wz/WzImageProperty.h"
#include "wz/WzPropertyCollection.h"

namespace wz {
class WzCanvasProperty : public WzImageProperty, public IPropertyContainer {
 public:
  static constexpr const char* InlinkPropertyName = "_inlink";
  static constexpr const char* OutlinkPropertyName = "_outlink";
  static constexpr const char* OriginPropertyName = "origin";
  static constexpr const char* HeadPropertyName = "head";
  static constexpr const char* LtPropertyName = "lt";
  static constexpr const char* AnimationDelayPropertyName = "delay";

  WzCanvasProperty();
  explicit WzCanvasProperty(const std::string& name);
  ~WzCanvasProperty() override;
  WZ_DISALLOW_COPY_AND_MOVE(WzCanvasProperty)

  WzPropertyType PropertyType() const override {
    return WzPropertyType::Canvas;
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

  WzPngProperty* PngProperty() const { return imageProp_.get(); }
  void SetPngProperty(std::unique_ptr<WzPngProperty> prop) {
    imageProp_ = std::move(prop);
  }
  bool ContainsInlinkProperty() const;
  bool ContainsOutlinkProperty() const;
  Result<void> SaveToFile(const std::string& filePath);
  Result<WzImageProperty*> GetLinkedWzImageProperty();

 private:
  WzPropertyCollection properties_;
  std::unique_ptr<WzPngProperty> imageProp_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZCANVASPROPERTY_H_

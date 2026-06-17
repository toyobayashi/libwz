#ifndef WZ_PROPERTIES_WZCANVASPROPERTY_H_
#define WZ_PROPERTIES_WZCANVASPROPERTY_H_
#include <string>
#include "wz/WzImageProperty.h"
#include "wz/WzPropertyCollection.h"

namespace wz {
class WzPngProperty;
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
  WzPropertyCollection* WzProperties() override { return &properties_; }

  void AddProperty(WzImageProperty* prop) override;
  void RemoveProperty(const std::string& propertyName) override;
  void RemoveProperty(WzImageProperty* prop) override;
  void ClearProperties() override;
  WzImageProperty* operator[](const std::string& name) override;
  WzImageProperty* GetFromPath(const std::string& path) override;

  WzPngProperty* PngProperty() const { return imageProp_; }
  void SetPngProperty(WzPngProperty* prop) { imageProp_ = prop; }
  bool ContainsInlinkProperty() const;
  bool ContainsOutlinkProperty() const;
  Result<void> SaveToFile(const std::string& filePath);
  Result<WzImageProperty*> GetLinkedWzImageProperty();

 private:
  WzPropertyCollection properties_;
  WzPngProperty* imageProp_ = nullptr;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZCANVASPROPERTY_H_

#include "wz/Properties/WzCanvasProperty.h"
#include <algorithm>
#include <cctype>
#include "wz/Properties/WzPngProperty.h"
#include "wz/Properties/WzStringProperty.h"

namespace wz {

WzCanvasProperty::WzCanvasProperty() : properties_(this) {}
WzCanvasProperty::WzCanvasProperty(const std::string& name)
    : properties_(this) {
  SetName(name);
}

WzCanvasProperty::~WzCanvasProperty() {
  delete imageProp_;
  imageProp_ = nullptr;
  for (auto* prop : properties_) {
    delete prop;
  }
  properties_.clear();
}

void WzCanvasProperty::AddProperty(WzImageProperty* prop) {
  prop->SetParent(this);
  properties_.Add(prop);
}

void WzCanvasProperty::RemoveProperty(const std::string& propertyName) {
  for (size_t i = 0; i < properties_.size(); i++) {
    if (properties_[i]->Name() == propertyName) {
      properties_[i]->SetParent(nullptr);
      properties_.erase(properties_.begin() + i);
      return;
    }
  }
}

void WzCanvasProperty::RemoveProperty(WzImageProperty* prop) {
  prop->SetParent(nullptr);
  auto it = std::find(properties_.begin(), properties_.end(), prop);
  if (it != properties_.end()) properties_.erase(it);
}

void WzCanvasProperty::ClearProperties() {
  for (auto* prop : properties_) prop->SetParent(nullptr);
  properties_.clear();
}

bool WzCanvasProperty::ContainsInlinkProperty() const {
  for (auto* p : properties_) {
    if (p->Name() == InlinkPropertyName) return true;
  }
  return false;
}

bool WzCanvasProperty::ContainsOutlinkProperty() const {
  for (auto* p : properties_) {
    if (p->Name() == OutlinkPropertyName) return true;
  }
  return false;
}

WzImageProperty* WzCanvasProperty::operator[](const std::string& name) {
  if (name == "PNG") return imageProp_;

  std::string lower = name;
  std::transform(
      lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
      });
  for (auto* prop : properties_) {
    std::string n = prop->Name();
    std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (n == lower) return prop;
  }
  return nullptr;
}

bool WzCanvasProperty::SaveToFile(const std::string& filePath) {
  if (!imageProp_) return false;
  return imageProp_->SaveToFile(filePath);
}

}  // namespace wz

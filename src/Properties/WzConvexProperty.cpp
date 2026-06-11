#include "wz/Properties/WzConvexProperty.h"
#include <algorithm>
#include <cctype>

namespace wz {

WzConvexProperty::WzConvexProperty() : properties_(this) {}
WzConvexProperty::WzConvexProperty(const std::string& name)
    : properties_(this) {
  SetName(name);
}

WzConvexProperty::~WzConvexProperty() {
  for (auto* prop : properties_) {
    delete prop;
  }
  properties_.clear();
}

void WzConvexProperty::AddProperty(WzImageProperty* prop) {
  prop->SetParent(this);
  properties_.Add(prop);
}

void WzConvexProperty::RemoveProperty(const std::string& propertyName) {
  for (size_t i = 0; i < properties_.size(); i++) {
    if (properties_[i]->Name() == propertyName) {
      properties_[i]->SetParent(nullptr);
      properties_.erase(properties_.begin() + i);
      return;
    }
  }
}

void WzConvexProperty::RemoveProperty(WzImageProperty* prop) {
  prop->SetParent(nullptr);
  auto it = std::find(properties_.begin(), properties_.end(), prop);
  if (it != properties_.end()) properties_.erase(it);
}

void WzConvexProperty::ClearProperties() {
  for (auto* prop : properties_) prop->SetParent(nullptr);
  properties_.clear();
}

WzImageProperty* WzConvexProperty::operator[](const std::string& name) {
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

}  // namespace wz

#include "wz/Properties/WzSubProperty.h"
#include <algorithm>
#include <cctype>
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzImage.h"

namespace wz {

WzSubProperty::WzSubProperty() : properties_(this) {}
WzSubProperty::WzSubProperty(const std::string& name) : properties_(this) {
  SetName(name);
}

WzSubProperty::~WzSubProperty() = default;

Result<void> WzSubProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteStringValue("Property",
                           WzImage::WzImageHeaderByte_WithoutOffset,
                           WzImage::WzImageHeaderByte_WithOffset);
  return WzImageProperty::WritePropertyList(writer, properties_);
}

void WzSubProperty::AddProperty(WzImageProperty* prop) {
  AddProperty(std::unique_ptr<WzImageProperty>(prop));
}

void WzSubProperty::AddProperty(std::unique_ptr<WzImageProperty> prop) {
  prop->SetParent(this);
  properties_.Add(std::move(prop));
}

void WzSubProperty::RemoveProperty(const std::string& propertyName) {
  for (size_t i = 0; i < properties_.size(); i++) {
    if (properties_[i]->Name() == propertyName) {
      properties_.erase_at(i);
      return;
    }
  }
}

void WzSubProperty::RemoveProperty(WzImageProperty* prop) {
  auto it = std::find(properties_.begin(), properties_.end(), prop);
  if (it != properties_.end()) {
    properties_.erase(it);
  }
}

void WzSubProperty::ClearProperties() {
  properties_.clear();
}

WzImageProperty* WzSubProperty::operator[](const std::string& name) {
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

WzImageProperty* WzSubProperty::GetFromPath(const std::string& path) {
  std::vector<std::string> segments;
  size_t start = 0;
  while (true) {
    size_t end = path.find('/', start);
    if (end == std::string::npos) break;
    if (end > start) segments.push_back(path.substr(start, end - start));
    start = end + 1;
  }
  segments.push_back(path.substr(start));

  if (segments.empty()) return nullptr;

  if (segments[0] == "..") {
    auto* parent = Parent();
    if (!parent) return nullptr;

    std::string remaining;
    for (size_t i = 1; i < segments.size(); i++) {
      if (i > 1) remaining += '/';
      remaining += segments[i];
    }

    auto parentType = parent->ObjectType();
    if (parentType == WzObjectType::Image)
      return static_cast<WzImage*>(parent)->GetFromPath(remaining);
    if (parentType == WzObjectType::Property)
      return static_cast<WzImageProperty*>(parent)->GetFromPath(remaining);
    return nullptr;
  }

  WzImageProperty* ret = this;
  for (const auto& seg : segments) {
    if (!ret || !ret->WzProperties()) return nullptr;
    WzImageProperty* found = nullptr;
    for (auto* p : *ret->WzProperties()) {
      if (p && p->Name() == seg) {
        found = p;
        break;
      }
    }
    if (!found) return nullptr;
    ret = found;
  }
  return ret;
}

}  // namespace wz

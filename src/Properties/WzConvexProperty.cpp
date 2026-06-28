#include "wz/Properties/WzConvexProperty.h"
#include <algorithm>
#include <cctype>
#include <vector>
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzImage.h"

namespace wz {

namespace {

bool IsConvexChildProperty(const WzImageProperty* prop) {
  if (!prop) return false;
  switch (prop->PropertyType()) {
    case WzPropertyType::SubProperty:
    case WzPropertyType::Canvas:
    case WzPropertyType::Vector:
    case WzPropertyType::Convex:
    case WzPropertyType::Sound:
    case WzPropertyType::UOL:
      return true;
    case WzPropertyType::Raw:
      return prop->IsRawDataProperty() || prop->IsVideoProperty();
    default:
      return false;
  }
}

}  // namespace

WzConvexProperty::WzConvexProperty() : properties_(this) {}
WzConvexProperty::WzConvexProperty(const std::string& name)
    : properties_(this) {
  SetName(name);
}

WzConvexProperty::~WzConvexProperty() = default;

Result<void> WzConvexProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteStringValue("Shape2D#Convex2D",
                           WzImage::WzImageHeaderByte_WithoutOffset,
                           WzImage::WzImageHeaderByte_WithOffset);
  writer->WriteCompressedInt(static_cast<int32_t>(properties_.size()));
  for (auto* prop : properties_) {
    if (!IsConvexChildProperty(prop)) {
      return std::unexpected(Error::DataError(
          "Cannot write Convex property with non-extended child"));
    }
    auto result = prop->WriteValue(writer);
    if (!result.has_value()) return result;
  }
  return {};
}

Result<void> WzConvexProperty::AddProperty(WzImageProperty* prop) {
  return AddProperty(std::unique_ptr<WzImageProperty>(prop));
}

Result<void> WzConvexProperty::AddProperty(
    std::unique_ptr<WzImageProperty> prop) {
  if (!prop) return {};
  prop->SetParent(this);
  properties_.Add(std::move(prop));
  MarkParentImageChanged();
  return {};
}

Result<std::unique_ptr<WzImageProperty>> WzConvexProperty::RemoveProperty(
    const std::string& propertyName) {
  for (size_t i = 0; i < properties_.size(); i++) {
    if (properties_[i]->Name() == propertyName) {
      auto removed = properties_.Remove(properties_[i]);
      MarkParentImageChanged();
      return removed;
    }
  }
  return std::unique_ptr<WzImageProperty>();
}

Result<std::unique_ptr<WzImageProperty>> WzConvexProperty::RemoveProperty(
    WzImageProperty* prop) {
  auto removed = properties_.Remove(prop);
  if (removed) MarkParentImageChanged();
  return removed;
}

Result<void> WzConvexProperty::ClearProperties() {
  if (properties_.size() == 0) return {};
  properties_.clear();
  MarkParentImageChanged();
  return {};
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

WzImageProperty* WzConvexProperty::GetFromPath(const std::string& path) {
  std::vector<std::string> segments;
  size_t start = 0;
  while (true) {
    size_t end = path.find('/', start);
    if (end == std::string::npos) break;
    if (end > start) segments.push_back(path.substr(start, end - start));
    start = end + 1;
  }
  std::string last = path.substr(start);
  if (!last.empty()) segments.push_back(last);

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

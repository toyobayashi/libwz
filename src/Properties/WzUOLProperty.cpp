#include "wz/Properties/WzUOLProperty.h"
#include <algorithm>
#include "wz/WzDirectory.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace wz {
WzUOLProperty::WzUOLProperty(const std::string& name, const std::string& value)
    : val_(value) {
  SetName(name);
}

WzObject* WzUOLProperty::LinkValue() const {
  if (linkVal_) return linkVal_;

  auto* self = const_cast<WzUOLProperty*>(this);

  std::vector<std::string> paths;
  size_t start = 0, end;
  while ((end = val_.find('/', start)) != std::string::npos) {
    paths.push_back(val_.substr(start, end - start));
    start = end + 1;
  }
  paths.push_back(val_.substr(start));
  if (paths.empty()) return nullptr;

  WzObject* linkVal;
  size_t i = 0;
  if (paths[0] != "..") {
    linkVal = self->GetTopMostWzImage();
  } else {
    linkVal = self->Parent();
    i = 1;
  }

  for (; i < paths.size() && linkVal; i++) {
    const std::string& path = paths[i];
    if (path == "..") {
      linkVal = linkVal->Parent();
      continue;
    }

    if (linkVal->ObjectType() == WzObjectType::Property) {
      auto* prop = static_cast<WzImageProperty*>(linkVal);
      linkVal = (*prop)[path];
    } else if (linkVal->ObjectType() == WzObjectType::Image) {
      auto* img = static_cast<WzImage*>(linkVal);
      linkVal = (*img)[path];
    } else if (linkVal->ObjectType() == WzObjectType::Directory) {
      auto* dir = static_cast<WzDirectory*>(linkVal);
      if (path.size() >= 4 &&
          std::equal(path.end() - 4, path.end(), ".img", [](char a, char b) {
            return std::tolower(a) == b;
          })) {
        linkVal = (*dir)[path];
      } else {
        linkVal = (*dir)[path + ".img"];
      }
    }
  }
  linkVal_ = linkVal;
  return linkVal;
}

WzPropertyCollection* WzUOLProperty::WzProperties() {
  WzObject* lv = LinkValue();
  if (lv && lv->ObjectType() == WzObjectType::Property)
    return static_cast<WzImageProperty*>(lv)->WzProperties();
  return nullptr;
}

WzImageProperty* WzUOLProperty::operator[](const std::string& name) {
  WzObject* lv = LinkValue();
  if (!lv) return nullptr;
  if (lv->ObjectType() == WzObjectType::Property)
    return (*static_cast<WzImageProperty*>(lv))[name];
  if (lv->ObjectType() == WzObjectType::Image)
    return (*static_cast<WzImage*>(lv))[name];
  return nullptr;
}

WzImageProperty* WzUOLProperty::GetFromPath(const std::string& path) {
  WzObject* lv = LinkValue();
  if (!lv) return nullptr;
  if (lv->ObjectType() == WzObjectType::Property)
    return static_cast<WzImageProperty*>(lv)->GetFromPath(path);
  if (lv->ObjectType() == WzObjectType::Image)
    return static_cast<WzImage*>(lv)->GetFromPath(path);
  return nullptr;
}

int32_t WzUOLProperty::GetInt() const {
  WzObject* lv = LinkValue();
  return lv ? lv->GetInt() : 0;
}

int16_t WzUOLProperty::GetShort() const {
  WzObject* lv = LinkValue();
  return lv ? lv->GetShort() : 0;
}

int64_t WzUOLProperty::GetLong() const {
  WzObject* lv = LinkValue();
  return lv ? lv->GetLong() : 0;
}

float WzUOLProperty::GetFloat() const {
  WzObject* lv = LinkValue();
  return lv ? lv->GetFloat() : 0.0f;
}

double WzUOLProperty::GetDouble() const {
  WzObject* lv = LinkValue();
  return lv ? lv->GetDouble() : 0.0;
}

std::string WzUOLProperty::GetString() const {
  WzObject* lv = LinkValue();
  return lv ? lv->GetString() : val_;
}

Result<std::vector<uint8_t>> WzUOLProperty::GetBytes() {
  WzObject* lv = LinkValue();
  return lv ? lv->GetBytes()
            : Result<std::vector<uint8_t>>(
                  Error::DataError("UOL link resolution failed"));
}
}  // namespace wz

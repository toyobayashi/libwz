#include "wz/Properties/WzCanvasProperty.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include "wz/Properties/WzPngProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/WzFile.h"
#include "wz/WzFileManager.h"
#include "wz/WzImage.h"

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

WzImageProperty* WzCanvasProperty::GetLinkedWzImageProperty() {
  auto* inlinkProp = (*this)[InlinkPropertyName];
  std::string inlink = (inlinkProp && inlinkProp->PropertyType() ==
                                        WzPropertyType::String)
                           ? static_cast<WzStringProperty*>(inlinkProp)->Value()
                           : "";
  auto* outlinkProp = (*this)[OutlinkPropertyName];
  std::string outlink =
      (outlinkProp && outlinkProp->PropertyType() == WzPropertyType::String)
          ? static_cast<WzStringProperty*>(outlinkProp)->Value()
          : "";

  if (!inlink.empty()) {
    auto* current = Parent();
    while (current) {
      if (current->ObjectType() == WzObjectType::Image) {
        auto* prop =
            static_cast<WzImage*>(current)->GetFromPath(inlink);
        if (prop) return prop;
        break;
      }
      current = current->Parent();
    }
  } else if (!outlink.empty()) {
    auto* current = Parent();
    WzFile* wzFileParent = nullptr;
    while (current) {
      if (current->ObjectType() == WzObjectType::Directory) {
        wzFileParent = current->WzFileParent();
        break;
      }
      current = current->Parent();
    }

    if (wzFileParent) {
      // Regex: match prefix letters before digits in wz file name
      // e.g. "Mob001.wz" → "Mob/"
      std::string fname = wzFileParent->Name();
      std::string prefixWz;
      for (char c : fname) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
          prefixWz += c;
        else
          break;
      }
      prefixWz += "/";

      WzObject* foundProperty = nullptr;
      if (outlink.size() >= prefixWz.size() &&
          outlink.compare(0, prefixWz.size(), prefixWz) == 0) {
        std::string realpath =
            outlink.substr(prefixWz.size());
        std::string wzfName = wzFileParent->Name();
        auto dotPos = wzfName.rfind(".wz");
        if (dotPos != std::string::npos) wzfName = wzfName.substr(0, dotPos);
        realpath = wzfName + "/" + realpath;
        foundProperty = wzFileParent->GetObjectFromPath(realpath);
      } else {
        if (WzFileManager::fileManager &&
            WzFileManager::fileManager->Is64Bit()) {
          if (WzFileManager::ContainsCanvasDirectory(outlink)) {
            std::string canvasFolderBase =
                WzFileManager::NormaliseWzCanvasDirectory(outlink);
            WzFileManager::fileManager->LoadCanvasSection(
                canvasFolderBase, wzFileParent->MapleVersion());
          }
        }
        foundProperty = wzFileParent->GetObjectFromPath(outlink);
      }
      if (foundProperty &&
          foundProperty->ObjectType() == WzObjectType::Property)
        return static_cast<WzImageProperty*>(foundProperty);
    }
  }
  return this;
}

}  // namespace wz

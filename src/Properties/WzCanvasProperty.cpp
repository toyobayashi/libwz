#include "wz/Properties/WzCanvasProperty.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include <vector>
#include "wz/Properties/WzPngProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace wz {

WzCanvasProperty::WzCanvasProperty() : properties_(this) {}
WzCanvasProperty::WzCanvasProperty(const std::string& name)
    : properties_(this) {
  SetName(name);
}

WzCanvasProperty::~WzCanvasProperty() = default;

Result<void> WzCanvasProperty::WriteValue(WzBinaryWriter* writer) const {
  if (!imageProp_) {
    return std::unexpected(Error::NotImplemented(
        "Cannot write Canvas property without a PNG property"));
  }
  auto bytes = imageProp_->GetCompressedBytesForExtraction(false);
  if (!bytes.has_value()) return std::unexpected(bytes.error());

  writer->WriteStringValue("Canvas",
                           WzImage::WzImageHeaderByte_WithoutOffset,
                           WzImage::WzImageHeaderByte_WithOffset);
  writer->WriteByte(0);
  if (properties_.size() > 0) {
    writer->WriteByte(1);
    auto result = WzImageProperty::WritePropertyList(writer, properties_);
    if (!result.has_value()) return result;
  } else {
    writer->WriteByte(0);
  }

  writer->WriteCompressedInt(imageProp_->Width());
  writer->WriteCompressedInt(imageProp_->Height());
  const int formatValue = static_cast<int>(imageProp_->Format());
  writer->WriteCompressedInt(formatValue & 0xFF);
  writer->WriteCompressedInt(formatValue >> 8);
  writer->WriteInt32(0);
  writer->WriteInt32(static_cast<int32_t>(bytes.value().size()) + 1);
  writer->WriteByte(0);
  writer->BaseStream().write(
      reinterpret_cast<const char*>(bytes.value().data()),
      static_cast<std::streamsize>(bytes.value().size()));
  return {};
}

void WzCanvasProperty::AddProperty(WzImageProperty* prop) {
  AddProperty(std::unique_ptr<WzImageProperty>(prop));
}

void WzCanvasProperty::AddProperty(std::unique_ptr<WzImageProperty> prop) {
  if (!prop) return;
  prop->SetParent(this);
  properties_.Add(std::move(prop));
  MarkParentImageChanged();
}

void WzCanvasProperty::RemoveProperty(const std::string& propertyName) {
  for (size_t i = 0; i < properties_.size(); i++) {
    if (properties_[i]->Name() == propertyName) {
      properties_.erase_at(i);
      MarkParentImageChanged();
      return;
    }
  }
}

void WzCanvasProperty::RemoveProperty(WzImageProperty* prop) {
  auto it = std::find(properties_.begin(), properties_.end(), prop);
  if (it != properties_.end()) {
    properties_.erase(it);
    MarkParentImageChanged();
  }
}

void WzCanvasProperty::ClearProperties() {
  if (properties_.size() == 0) return;
  properties_.clear();
  MarkParentImageChanged();
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
  if (name == "PNG") return imageProp_.get();

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

WzImageProperty* WzCanvasProperty::GetFromPath(const std::string& path) {
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
    if (seg == "PNG") return imageProp_.get();
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

Result<void> WzCanvasProperty::SaveToFile(const std::string& filePath) {
  if (!imageProp_)
    return std::unexpected(Error::DataError("Canvas has no PNG property"));
  return imageProp_->SaveToFile(filePath);
}

Result<WzImageProperty*> WzCanvasProperty::GetLinkedWzImageProperty() {
  auto* inlinkProp = (*this)[InlinkPropertyName];
  std::string inlink =
      (inlinkProp && inlinkProp->PropertyType() == WzPropertyType::String)
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
        auto* prop = static_cast<WzImage*>(current)->GetFromPath(inlink);
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
        std::string realpath = outlink.substr(prefixWz.size());
        std::string wzfName = wzFileParent->Name();
        auto dotPos = wzfName.rfind(".wz");
        if (dotPos != std::string::npos) wzfName = wzfName.substr(0, dotPos);
        realpath = wzfName + "/" + realpath;
        foundProperty = wzFileParent->GetObjectFromPath(realpath);
      } else {
        // if (WzFileManager::fileManager &&
        //     WzFileManager::fileManager->Is64Bit()) {
        //   if (WzFileManager::ContainsCanvasDirectory(outlink)) {
        //     std::string canvasFolderBase =
        //         WzFileManager::NormaliseWzCanvasDirectory(outlink);
        //     auto result = WzFileManager::fileManager->LoadCanvasSection(
        //         canvasFolderBase, wzFileParent->MapleVersion());
        //     if (!result.has_value()) {
        //       return std::unexpected(result.error());
        //     }
        //   }
        // }
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

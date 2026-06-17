#include "wz/WzImage.h"
#include <algorithm>
#include <utility>
#include "wz/Properties/WzLuaProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzTool.h"
#include "wz/WzDirectory.h"
#include "wz/WzFile.h"

namespace wz {

WzImage::WzImage() : properties_(this) {}
WzImage::WzImage(const std::string& name) : properties_(this) {
  SetName(name);
  SetChanged(true);
}
WzImage::WzImage(const std::string& name, WzBinaryReader& reader, int checksum)
    : checksum_(checksum), reader_(&reader), properties_(this) {
  SetName(name);
  blockStart_ = static_cast<int>(reader.Position());
}

WzImage::WzImage(const std::string& name,
                 std::ifstream&& dataStream,
                 WzMapleVersion mapleVersion)
    : dataStream_(std::move(dataStream)), properties_(this) {
  SetName(name);
  auto iv = WzTool::GetIvByMapleVersion(mapleVersion);
  ownedReader_.emplace(dataStream_, iv);
  reader_ = &ownedReader_.value();
}

WzImage::~WzImage() {
  name_.clear();
  for (auto* prop : properties_) {
    delete prop;
  }
  properties_.clear();
  ownedReader_.reset();
  reader_ = nullptr;
}

WzFile* WzImage::WzFileParent() const {
  return Parent() ? Parent()->WzFileParent() : nullptr;
}

WzObjectType WzImage::ObjectType() const {
  return WzObjectType::Image;
}

void WzImage::Remove() {
  if (Parent()) {
    static_cast<WzDirectory*>(Parent())->RemoveImage(this);
  }
}

bool WzImage::IsLuaWzImage() const {
  const auto& n = Name();
  return n.size() >= 4 && n.substr(n.size() - 4) == ".lua";
}

WzPropertyCollection* WzImage::WzProperties() {
  auto result = WzPropertiesResult();
  if (!result.has_value()) return nullptr;
  return result.value();
}

Result<WzPropertyCollection*> WzImage::WzPropertiesResult() {
  auto parseResult = EnsureParsed();
  if (!parseResult.has_value()) return std::unexpected(parseResult.error());
  return &properties_;
}

void WzImage::AddProperty(WzImageProperty* prop) {
  // Check for duplicate name (case-insensitive) matching C#
  auto existing = GetPropertyByName(prop->Name());
  if (existing.has_value() && existing.value() != nullptr) {
    return;
  }
  auto parseResult = EnsureParsed();
  if (!parseResult.has_value()) return;
  properties_.Add(prop);
  SetChanged(true);
}

void WzImage::RemoveProperty(const std::string& propertyName) {
  // C#: first lookup by name (case-insensitive),
  // then delegate to RemoveProperty(prop)
  WzImageProperty* prop = (*this)[propertyName];
  if (prop) {
    RemoveProperty(prop);
  }
}

void WzImage::RemoveProperty(WzImageProperty* prop) {
  // C#: check containment BEFORE parsing
  auto it = std::find(properties_.begin(), properties_.end(), prop);
  if (it == properties_.end()) return;

  auto parseResult = EnsureParsed();
  if (!parseResult.has_value()) return;
  prop->SetParent(nullptr);
  properties_.erase(it);
  SetChanged(true);
}

void WzImage::ClearProperties() {
  for (auto* prop : properties_) prop->SetParent(nullptr);
  properties_.clear();
  SetChanged(true);
}

WzImageProperty* WzImage::GetFromPath(const std::string& path) {
  auto result = GetFromPathResult(path);
  if (!result.has_value()) return nullptr;
  return result.value();
}

Result<WzImageProperty*> WzImage::GetFromPathResult(const std::string& path) {
  auto parseResult = EnsureParsed();
  if (!parseResult.has_value()) return std::unexpected(parseResult.error());

  // Split by '/', skip empty segments (C# uses
  // StringSplitOptions.RemoveEmptyEntries)
  std::vector<std::string> segments;
  size_t start = 0, end;
  while ((end = path.find('/', start)) != std::string::npos) {
    std::string seg = path.substr(start, end - start);
    if (!seg.empty()) segments.push_back(std::move(seg));
    start = end + 1;
  }
  {
    std::string last = path.substr(start);
    if (!last.empty()) segments.push_back(std::move(last));
  }

  if (segments.empty() || segments[0] == "..") return nullptr;

  WzImageProperty* ret = nullptr;
  for (const auto& seg : segments) {
    WzPropertyCollection* wps = ret ? ret->WzProperties() : nullptr;
    if (ret != nullptr && wps == nullptr) return nullptr;
    auto& props = (ret == nullptr ? properties_ : *wps);
    bool found = false;
    for (auto* p : props) {
      if (p && p->Name() == seg) {
        ret = p;
        found = true;
        break;
      }
    }
    if (!found) return nullptr;
  }
  return ret;
}

Result<bool> WzImage::ParseImage() {
  if (Parsed()) return true;
  if (Changed()) {
    SetParsed(true);
    return true;
  }
  if (!reader_) return false;

  // int64_t originalPos = reader_->Position();
  reader_->SetPosition(offset_);

  uint8_t b = reader_->ReadByte();
  switch (b) {
    case 0x1:
      if (IsLuaWzImage()) {
        int length = reader_->ReadCompressedInt();
        auto rawEncBytes = reader_->ReadBytes(length);
        auto* lua = new WzLuaProperty("Script", rawEncBytes);
        lua->SetParent(this);
        properties_.Add(lua);
        parsed_ = true;
        return true;
      }
      return false;
    case WzImageHeaderByte_WithoutOffset: {
      std::string prop = reader_->ReadString();
      uint16_t val = reader_->ReadUInt16();
      if (prop != "Property" || val != 0) return false;
    } break;
    default:
      return false;
  }

  auto r = WzImageProperty::ParsePropertyList(offset_, reader_, this, this);
  if (!r.has_value()) return std::unexpected(r.error());
  for (auto* p : r.value()) {
    properties_.Add(p);
  }
  parsed_ = true;
  return true;
}

Result<void> WzImage::EnsureParsed() {
  if (!reader_ || parsed_) return {};
  auto parseResult = ParseImage();
  if (!parseResult.has_value()) return std::unexpected(parseResult.error());
  return {};
}

static std::string ToLower(const std::string& s) {
  std::string r = s;
  std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return r;
}

WzImageProperty* WzImage::operator[](const std::string& name) {
  auto result = GetPropertyByName(name);
  if (!result.has_value()) return nullptr;
  return result.value();
}

Result<WzImageProperty*> WzImage::GetPropertyByName(const std::string& name) {
  auto parseResult = EnsureParsed();
  if (!parseResult.has_value()) return std::unexpected(parseResult.error());
  std::string lower = ToLower(name);
  for (auto* prop : properties_) {
    if (ToLower(prop->Name()) == lower) return prop;
  }
  return nullptr;
}

}  // namespace wz

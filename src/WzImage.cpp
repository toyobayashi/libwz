#include "wz/WzImage.h"
#include <algorithm>
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
WzImage::WzImage(const std::string& name, WzBinaryReader* reader, int checksum)
    : checksum_(checksum), reader_(reader), properties_(this) {
  SetName(name);
  blockStart_ = static_cast<int>(reader->Position());
}

WzImage::WzImage(const std::string& name,
                 std::ifstream* dataStream,
                 WzMapleVersion mapleVersion)
    : dataStream_(dataStream), properties_(this) {
  SetName(name);
  auto iv = WzTool::GetIvByMapleVersion(mapleVersion);
  reader_ = new WzBinaryReader(dataStream, iv);
}

WzImage::~WzImage() {
  name_.clear();
  for (auto* prop : properties_) {
    delete prop;
  }
  properties_.clear();
  if (reader_ && dataStream_) {
    reader_->Close();
    delete reader_;
    reader_ = nullptr;
    delete dataStream_;
    dataStream_ = nullptr;
  }
  reader_ = nullptr;
}

WzFile* WzImage::WzFileParent() const {
  return Parent() ? Parent()->WzFileParent() : nullptr;
}

WzObjectType WzImage::ObjectType() const {
  if (reader_ && !parsed_) const_cast<WzImage*>(this)->ParseImage();
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
  if (reader_ && !parsed_) ParseImage();
  return &properties_;
}

void WzImage::AddProperty(WzImageProperty* prop) {
  // Check for duplicate name (case-insensitive) matching C#
  if ((*this)[prop->Name()] != nullptr) {
    return;
  }
  if (reader_ && !parsed_) ParseImage();
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

  if (reader_ && !parsed_) ParseImage();
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
  if (reader_ && !parsed_) ParseImage();

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
  if (r.is_err()) return r.err();
  for (auto* p : r.ok()) {
    properties_.Add(p);
  }
  parsed_ = true;
  return true;
}

static std::string ToLower(const std::string& s) {
  std::string r = s;
  std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return r;
}

WzImageProperty* WzImage::operator[](const std::string& name) {
  if (reader_ && !parsed_) ParseImage();
  std::string lower = ToLower(name);
  for (auto* prop : properties_) {
    if (ToLower(prop->Name()) == lower) return prop;
  }
  return nullptr;
}

}  // namespace wz

#include "wz/Properties/WzRawDataProperty.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzPath.h"

namespace wz {

WzRawDataProperty::WzRawDataProperty(const std::string& name,
                                     WzBinaryReader* reader,
                                     uint8_t type)
    : wzReader_(reader), type_(type), properties_(this) {
  SetName(name);
}

WzRawDataProperty::~WzRawDataProperty() {
  for (auto* prop : properties_) {
    delete prop;
  }
  properties_.clear();
}

void WzRawDataProperty::AddProperty(WzImageProperty* prop) {
  prop->SetParent(this);
  properties_.Add(prop);
}

void WzRawDataProperty::RemoveProperty(const std::string& propertyName) {
  for (size_t i = 0; i < properties_.size(); i++) {
    if (properties_[i]->Name() == propertyName) {
      properties_[i]->SetParent(nullptr);
      properties_.erase(properties_.begin() + i);
      return;
    }
  }
}

void WzRawDataProperty::RemoveProperty(WzImageProperty* prop) {
  prop->SetParent(nullptr);
  auto it = std::find(properties_.begin(), properties_.end(), prop);
  if (it != properties_.end()) properties_.erase(it);
}

void WzRawDataProperty::ClearProperties() {
  for (auto* prop : properties_) prop->SetParent(nullptr);
  properties_.clear();
}

void WzRawDataProperty::Parse(bool parseNow) {
  length_ = wzReader_->ReadCompressedInt();
  rawDataOffset_ = wzReader_->Position();
  if (parseNow)
    GetBytes(true);
  else
    wzReader_->SetPosition(rawDataOffset_ + length_);
}

Result<std::vector<uint8_t>> WzRawDataProperty::GetBytes() {
  return GetBytes(false);
}

Result<std::vector<uint8_t>> WzRawDataProperty::GetBytes(bool saveInMemory) {
  if (!bytes_.empty()) return bytes_;
  if (!wzReader_) return Error::DataError("No reader for raw data property");

  auto currentPos = wzReader_->Position();
  wzReader_->SetPosition(rawDataOffset_);
  bytes_ = wzReader_->ReadBytes(length_);
  wzReader_->SetPosition(currentPos);

  if (saveInMemory) return bytes_;
  auto result = bytes_;
  bytes_.clear();
  return result;
}

bool WzRawDataProperty::SaveToFile(const std::string& filePath) {
  auto result = GetBytes(false);
  if (!result.is_ok()) return false;
  auto& bytes = result.ok();
  std::error_code ec;
  std::filesystem::create_directories(wz::to_path(filePath).parent_path(), ec);
  if (ec) return false;
  std::ofstream out(wz::to_path(filePath), std::ios::binary);
  if (!out) return false;
  out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  return true;
}

}  // namespace wz

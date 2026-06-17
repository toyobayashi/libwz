#include "wz/Properties/WzVideoProperty.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzPath.h"

namespace wz {

WzVideoProperty::WzVideoProperty(const std::string& name,
                                 WzBinaryReader* reader)
    : wzReader_(reader), properties_(this) {
  SetName(name);
}

WzVideoProperty::~WzVideoProperty() {
  for (auto* prop : properties_) {
    delete prop;
  }
  properties_.clear();
}

void WzVideoProperty::AddProperty(WzImageProperty* prop) {
  prop->SetParent(this);
  properties_.Add(prop);
}

void WzVideoProperty::RemoveProperty(const std::string& propertyName) {
  for (size_t i = 0; i < properties_.size(); i++) {
    if (properties_[i]->Name() == propertyName) {
      delete properties_[i];
      properties_.erase(properties_.begin() + i);
      return;
    }
  }
}

void WzVideoProperty::RemoveProperty(WzImageProperty* prop) {
  auto it = std::find(properties_.begin(), properties_.end(), prop);
  if (it != properties_.end()) {
    properties_.erase(it);
    delete prop;
  }
}

void WzVideoProperty::ClearProperties() {
  for (auto* prop : properties_) {
    delete prop;
  }
  properties_.clear();
}

void WzVideoProperty::Parse(bool parseNow) {
  type_ = wzReader_->ReadByte();
  length_ = wzReader_->ReadCompressedInt();
  offset_ = wzReader_->Position();
  if (parseNow)
    (void)GetBytes(true);
  else
    wzReader_->SetPosition(offset_ + length_);
}

Result<std::vector<uint8_t>> WzVideoProperty::GetBytes() {
  return GetBytes(false);
}

Result<std::vector<uint8_t>> WzVideoProperty::GetBytes(bool saveInMemory) {
  if (!bytes_.empty()) return bytes_;
  if (!wzReader_)
    return std::unexpected(Error::DataError("No reader for video property"));

  auto currentPos = wzReader_->Position();
  wzReader_->SetPosition(offset_);
  bytes_ = wzReader_->ReadBytes(length_);
  wzReader_->SetPosition(currentPos);

  if (saveInMemory) return bytes_;
  auto result = bytes_;
  bytes_.clear();
  return result;
}

Result<void> WzVideoProperty::SaveToFile(const std::string& filePath) {
  auto result = GetBytes(false);
  if (!result.has_value()) return std::unexpected(result.error());
  auto& bytes = result.value();
  auto outPath = wz::to_path(filePath);
  auto parentPath = outPath.parent_path();
  std::error_code ec;
  if (!parentPath.empty()) {
    std::filesystem::create_directories(parentPath, ec);
    if (ec) return std::unexpected(Error::IoError(ec.message()));
  }
  std::ofstream out(outPath, std::ios::binary);
  if (!out)
    return std::unexpected(Error::IoError("Failed to open file for writing"));
  out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  return {};
}

}  // namespace wz

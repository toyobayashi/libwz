#include "wz/Properties/WzRawDataProperty.h"
#include <algorithm>
#include <filesystem>
#include <mutex>
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzPath.h"
#include "wz/Util/WzStream.h"
#include "wz/WzImage.h"

namespace wz {

WzRawDataProperty::WzRawDataProperty(const std::string& name,
                                     WzBinaryReader* reader,
                                     uint8_t type)
    : wzReader_(reader), type_(type), properties_(this) {
  SetName(name);
}

WzRawDataProperty::~WzRawDataProperty() = default;

Result<void> WzRawDataProperty::WriteValue(WzBinaryWriter* writer) const {
  auto* self = const_cast<WzRawDataProperty*>(this);
  auto data = self->GetBytes(false);
  if (!data.has_value()) return std::unexpected(data.error());

  writer->WriteStringValue(RAW_DATA_HEADER,
                           WzImage::WzImageHeaderByte_WithoutOffset,
                           WzImage::WzImageHeaderByte_WithOffset);
  writer->WriteByte(type_);
  if (type_ == 1) {
    if (properties_.size() > 0) {
      writer->WriteByte(1);
      auto result = WzImageProperty::WritePropertyList(writer, properties_);
      if (!result.has_value()) return result;
    } else {
      writer->WriteByte(0);
    }
  }
  writer->WriteCompressedInt(static_cast<int32_t>(data.value().size()));
  if (!writer->BaseStream().Write(data.value().data(), data.value().size())) {
    return std::unexpected(Error::IoError("Failed to write raw data property"));
  }
  return {};
}

void WzRawDataProperty::AddProperty(WzImageProperty* prop) {
  AddProperty(std::unique_ptr<WzImageProperty>(prop));
}

void WzRawDataProperty::AddProperty(std::unique_ptr<WzImageProperty> prop) {
  if (!prop) return;
  prop->SetParent(this);
  properties_.Add(std::move(prop));
  MarkParentImageChanged();
}

void WzRawDataProperty::RemoveProperty(const std::string& propertyName) {
  for (size_t i = 0; i < properties_.size(); i++) {
    if (properties_[i]->Name() == propertyName) {
      properties_.erase_at(i);
      MarkParentImageChanged();
      return;
    }
  }
}

void WzRawDataProperty::RemoveProperty(WzImageProperty* prop) {
  auto it = std::find(properties_.begin(), properties_.end(), prop);
  if (it != properties_.end()) {
    properties_.erase(it);
    MarkParentImageChanged();
  }
}

void WzRawDataProperty::ClearProperties() {
  if (properties_.size() == 0) return;
  properties_.clear();
  MarkParentImageChanged();
}

void WzRawDataProperty::Parse(bool parseNow) {
  length_ = wzReader_->ReadCompressedInt();
  rawDataOffset_ = wzReader_->Position();
  if (parseNow)
    (void)GetBytes(true);
  else
    wzReader_->SetPosition(rawDataOffset_ + length_);
}

Result<std::vector<uint8_t>> WzRawDataProperty::GetBytes() {
  return GetBytes(false);
}

Result<std::vector<uint8_t>> WzRawDataProperty::GetBytes(bool saveInMemory) {
  std::unique_lock<std::recursive_mutex> lock;
  if (wzReader_) {
    lock = std::unique_lock<std::recursive_mutex>(wzReader_->Mutex());
  }
  if (!bytes_.empty()) return bytes_;
  if (!wzReader_)
    return std::unexpected(Error::DataError("No reader for raw data property"));

  auto currentPos = wzReader_->Position();
  wzReader_->SetPosition(rawDataOffset_);
  bytes_ = wzReader_->ReadBytes(length_);
  wzReader_->SetPosition(currentPos);

  if (saveInMemory) return bytes_;
  auto result = bytes_;
  bytes_.clear();
  return result;
}

Result<void> WzRawDataProperty::SaveToFile(const std::string& filePath) {
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
  WzFileStream out;
  if (!out.Open(outPath, "wb"))
    return std::unexpected(Error::IoError("Failed to open file for writing"));
  if (!out.Write(bytes.data(), bytes.size()))
    return std::unexpected(Error::IoError("Failed to write file"));
  return {};
}

}  // namespace wz

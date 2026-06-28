#include "wz/Properties/WzRawDataProperty.h"
#include <algorithm>
#include <mutex>
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
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

Result<void> WzRawDataProperty::AddProperty(WzImageProperty* prop) {
  return AddProperty(std::unique_ptr<WzImageProperty>(prop));
}

Result<void> WzRawDataProperty::AddProperty(
    std::unique_ptr<WzImageProperty> prop) {
  if (!prop) return {};
  prop->SetParent(this);
  properties_.Add(std::move(prop));
  MarkParentImageChanged();
  return {};
}

Result<std::unique_ptr<WzImageProperty>> WzRawDataProperty::RemoveProperty(
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

Result<std::unique_ptr<WzImageProperty>> WzRawDataProperty::RemoveProperty(
    WzImageProperty* prop) {
  auto removed = properties_.Remove(prop);
  if (removed) MarkParentImageChanged();
  return removed;
}

Result<void> WzRawDataProperty::ClearProperties() {
  if (properties_.size() == 0) return {};
  properties_.clear();
  MarkParentImageChanged();
  return {};
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

}  // namespace wz

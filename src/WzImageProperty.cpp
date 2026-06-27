#include "wz/WzImageProperty.h"
#include <algorithm>
#include <cctype>
#include <memory>
#include <vector>
#include "wz/Properties/WzBinaryProperty.h"
#include "wz/Properties/WzCanvasProperty.h"
#include "wz/Properties/WzConvexProperty.h"
#include "wz/Properties/WzDoubleProperty.h"
#include "wz/Properties/WzFloatProperty.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzLongProperty.h"
#include "wz/Properties/WzLuaProperty.h"
#include "wz/Properties/WzNullProperty.h"
#include "wz/Properties/WzPngProperty.h"
#include "wz/Properties/WzRawDataProperty.h"
#include "wz/Properties/WzShortProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Properties/WzUOLProperty.h"
#include "wz/Properties/WzVectorProperty.h"
#include "wz/Properties/WzVideoProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace wz {

namespace {

std::string ToLower(const std::string& s) {
  std::string r = s;
  std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return r;
}

bool IsExtendedProperty(const WzImageProperty* prop) {
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

Result<void> WriteExtendedValue(WzBinaryWriter* writer,
                                const WzImageProperty* prop) {
  writer->WriteByte(9);

  const int64_t beforePos = writer->Position();
  writer->WriteInt32(0);
  auto result = prop->WriteValue(writer);
  if (!result.has_value()) return result;

  const int64_t newPos = writer->Position();
  const int32_t len = static_cast<int32_t>(newPos - beforePos);
  if (!writer->BaseStream().Seek(beforePos, WzSeekOrigin::Begin)) {
    return std::unexpected(
        Error::IoError("Failed to seek WZ extended property length"));
  }
  writer->WriteInt32(len - 4);
  if (!writer->BaseStream().Seek(newPos, WzSeekOrigin::Begin)) {
    return std::unexpected(
        Error::IoError("Failed to restore WZ extended property position"));
  }
  return {};
}

}  // namespace

WzFile* WzImageProperty::WzFileParent() const {
  auto* img = ParentImage();
  return img ? img->WzFileParent() : nullptr;
}

WzImage* WzImageProperty::ParentImage() const {
  auto* p = Parent();
  while (p) {
    if (p->ObjectType() == WzObjectType::Image) return static_cast<WzImage*>(p);
    p = p->Parent();
  }
  return nullptr;
}

void WzImageProperty::MarkParentImageChanged() const {
  auto* img = ParentImage();
  if (img) img->SetChanged(true);
}

WzImageProperty* WzImageProperty::GetLinkedWzImageProperty() {
  WzImageProperty* current = this;
  while (current->PropertyType() == WzPropertyType::UOL) {
    auto* uol = static_cast<WzUOLProperty*>(current);
    WzObject* linkValue = uol->LinkValue();
    if (linkValue && linkValue->ObjectType() == WzObjectType::Property) {
      current = static_cast<WzImageProperty*>(linkValue);
    } else {
      return current;  // broken link, return last valid
    }
  }
  return current;
}

WzImageProperty* WzImageProperty::GetFromPath(const std::string& path) {
  (void)path;
  return nullptr;
}

WzImageProperty* WzImageProperty::operator[](const std::string& name) {
  (void)name;
  return nullptr;
}

void WzImageProperty::SetValue(const std::string& value) {
  (void)value;
}
void WzImageProperty::SetValue(int32_t value) {
  (void)value;
}
void WzImageProperty::SetValue(int64_t value) {
  (void)value;
}
void WzImageProperty::SetValue(float value) {
  (void)value;
}
void WzImageProperty::SetValue(double value) {
  (void)value;
}
void WzImageProperty::SetValue(const std::vector<uint8_t>& value) {
  (void)value;
}

Result<void> WzImageProperty::WriteValue(WzBinaryWriter* writer) const {
  (void)writer;
  return std::unexpected(Error::NotImplemented(
      "WriteValue is not implemented for this property type"));
}

void IPropertyContainer::AddProperties(WzPropertyCollection& props) {
  for (auto& prop : props.TakeItems()) {
    AddProperty(std::move(prop));
  }
}

Result<void> WzImageProperty::TryAddChildProperty(WzImageProperty* prop) {
  if (!prop) {
    return std::unexpected(
        Error::InvalidArgument("Cannot add a null WZ image property"));
  }
  if (prop->Name().empty()) {
    return std::unexpected(
        Error::InvalidArgument("WZ image property name cannot be empty"));
  }
  if (prop->Parent()) {
    return std::unexpected(
        Error::InvalidArgument("WZ image property already has a parent"));
  }
  auto* properties = WzProperties();
  if (!properties) {
    return std::unexpected(
        Error::InvalidArgument("WZ object is not a property container"));
  }
  const std::string lower = ToLower(prop->Name());
  for (auto* existing : *properties) {
    if (existing && ToLower(existing->Name()) == lower) {
      return std::unexpected(Error::InvalidArgument(
          "Duplicate WZ image property name: " + prop->Name()));
    }
  }
  properties->Add(prop);
  MarkParentImageChanged();
  return {};
}

Result<void> WzImageProperty::TryAddChildProperty(
    std::unique_ptr<WzImageProperty> prop) {
  auto* raw = prop.get();
  auto result = TryAddChildProperty(raw);
  if (result.has_value()) {
    (void)prop.release();
  }
  return result;
}

Result<void> WzImageProperty::TryRemoveChildProperty(WzImageProperty* prop) {
  if (!prop) {
    return std::unexpected(
        Error::InvalidArgument("Cannot remove a null WZ image property"));
  }
  auto* properties = WzProperties();
  if (!properties) {
    return std::unexpected(
        Error::InvalidArgument("WZ object is not a property container"));
  }
  auto removed = properties->Take(prop);
  if (!removed) {
    return std::unexpected(
        Error::NotFound("WZ image property is not owned by this property"));
  }
  if (removed) MarkParentImageChanged();
  return {};
}

Result<void> WzImageProperty::TryClearChildProperties() {
  auto* properties = WzProperties();
  if (!properties) {
    return std::unexpected(
        Error::InvalidArgument("WZ object is not a property container"));
  }
  if (properties->size() == 0) return {};
  properties->clear();
  MarkParentImageChanged();
  return {};
}

Result<void> WzImageProperty::TryRemove() {
  auto* parent = Parent();
  if (!parent) {
    delete this;
    return {};
  }
  if (parent->ObjectType() == WzObjectType::Image) {
    return static_cast<WzImage*>(parent)->TryRemoveProperty(this);
  } else if (parent->ObjectType() == WzObjectType::Property) {
    return static_cast<WzImageProperty*>(parent)->TryRemoveChildProperty(this);
  }
  return std::unexpected(
      Error::NotFound("WZ image property has no removable parent"));
}

void WzImageProperty::Remove() {
  (void)TryRemove();
}

Result<WzPropertyCollection> WzImageProperty::ParsePropertyList(
    int64_t offset,
    WzBinaryReader* reader,
    WzObject* parent,
    WzImage* parentImg) {
  int entryCount = reader->ReadCompressedInt();
  WzPropertyCollection properties(parent);

  auto addOwnedProperty = [&](std::unique_ptr<WzImageProperty> prop) {
    prop->SetParent(parent);
    properties.Add(std::move(prop));
  };

  for (int i = 0; i < entryCount; i++) {
    std::string name = reader->ReadStringBlock(offset);
    uint8_t ptype = reader->ReadByte();
    switch (ptype) {
      case 0: {
        addOwnedProperty(std::make_unique<WzNullProperty>(name));
        break;
      }
      case 11:
      case 2: {
        addOwnedProperty(
            std::make_unique<WzShortProperty>(name, reader->ReadInt16()));
        break;
      }
      case 3:
      case 19: {
        addOwnedProperty(
            std::make_unique<WzIntProperty>(name, reader->ReadCompressedInt()));
        break;
      }
      case 20: {
        addOwnedProperty(
            std::make_unique<WzLongProperty>(name, reader->ReadLong()));
        break;
      }
      case 4: {
        uint8_t type = reader->ReadByte();
        float val = (type == 0x80) ? reader->ReadSingle() : 0.0f;
        addOwnedProperty(std::make_unique<WzFloatProperty>(name, val));
        break;
      }
      case 5: {
        addOwnedProperty(
            std::make_unique<WzDoubleProperty>(name, reader->ReadDouble()));
        break;
      }
      case 8: {
        addOwnedProperty(std::make_unique<WzStringProperty>(
            name, reader->ReadStringBlock(offset)));
        break;
      }
      case 9: {
        int64_t eob =
            static_cast<int64_t>(reader->ReadUInt32()) + reader->Position();
        auto exProp =
            ParseExtendedProp(offset, reader, name, parent, parentImg);
        if (!exProp.has_value()) return std::unexpected(exProp.error());
        addOwnedProperty(std::move(exProp.value()));
        if (reader->Position() != eob) {
          reader->SetPosition(eob);
        }
        break;
      }
      default:
        return std::unexpected(Error::ParseError(
            "Unknown property type at ParsePropertyList, ptype = " +
            std::to_string(ptype)));
    }
  }
  return properties;
}

Result<void> WzImageProperty::WritePropertyList(
    WzBinaryWriter* writer, const WzPropertyCollection& properties) {
  if (properties.size() == 1 &&
      properties[0]->PropertyType() == WzPropertyType::Lua) {
    return properties[0]->WriteValue(writer);
  }

  writer->WriteUInt16(0);
  writer->WriteCompressedInt(static_cast<int32_t>(properties.size()));
  for (auto* prop : properties) {
    writer->WriteStringValue(prop->Name(), 0x00, 0x01);
    auto result = IsExtendedProperty(prop) ? WriteExtendedValue(writer, prop)
                                           : prop->WriteValue(writer);
    if (!result.has_value()) return result;
  }
  return {};
}

Result<std::unique_ptr<WzImageProperty>> WzImageProperty::ParseExtendedProp(
    int64_t offset,
    WzBinaryReader* reader,
    const std::string& name,
    WzObject* parent,
    WzImage* parentImg) {
  uint8_t extType = reader->ReadByte();
  std::string iname;
  switch (extType) {
    case 0x01:
    case WzImage::WzImageHeaderByte_WithOffset:
      iname = reader->ReadStringAtOffset(offset + reader->ReadInt32());
      break;
    case 0x00:
    case WzImage::WzImageHeaderByte_WithoutOffset:
      iname = reader->ReadString();
      break;
    default:
      return std::unexpected(
          Error::ParseError("Invalid byte read at ParseExtendedProp"));
  }

  if (iname == "Property") {
    auto subProp = std::make_unique<WzSubProperty>(name);
    subProp->SetParent(parent);
    reader->SetPosition(reader->Position() + 2);
    auto r = ParsePropertyList(offset, reader, subProp.get(), parentImg);
    if (!r.has_value()) return std::unexpected(r.error());
    for (auto& child : r.value().TakeItems()) {
      subProp->WzProperties()->Add(std::move(child));
    }
    return subProp;
  } else if (iname == "Canvas") {
    auto canvasProp = std::make_unique<WzCanvasProperty>(name);
    canvasProp->SetParent(parent);
    reader->ReadByte();
    if (reader->ReadByte() == 1) {
      reader->SetPosition(reader->Position() + 2);
      auto r = ParsePropertyList(offset, reader, canvasProp.get(), parentImg);
      if (!r.has_value()) return std::unexpected(r.error());
      for (auto& child : r.value().TakeItems()) {
        canvasProp->WzProperties()->Add(std::move(child));
      }
    }
    auto png =
        std::make_unique<WzPngProperty>(reader, parentImg->ParseEverything());
    png->SetParent(canvasProp.get());
    canvasProp->SetPngProperty(std::move(png));
    return canvasProp;
  } else if (iname == "Shape2D#Vector2D") {
    auto vecProp = std::make_unique<WzVectorProperty>(name);
    vecProp->SetParent(parent);
    vecProp->X =
        std::make_unique<WzIntProperty>("X", reader->ReadCompressedInt());
    vecProp->X->SetParent(vecProp.get());
    vecProp->Y =
        std::make_unique<WzIntProperty>("Y", reader->ReadCompressedInt());
    vecProp->Y->SetParent(vecProp.get());
    return vecProp;
  } else if (iname == "Shape2D#Convex2D") {
    auto convexProp = std::make_unique<WzConvexProperty>(name);
    convexProp->SetParent(parent);
    int convexEntryCount = reader->ReadCompressedInt();
    for (int j = 0; j < convexEntryCount; j++) {
      auto entryProp =
          ParseExtendedProp(offset, reader, name, convexProp.get(), parentImg);
      if (!entryProp.has_value()) {
        return std::unexpected(entryProp.error());
      }
      convexProp->WzProperties()->Add(std::move(entryProp.value()));
    }
    return convexProp;
  } else if (iname == "Sound_DX8") {
    auto soundProp = std::make_unique<WzBinaryProperty>(
        name, reader, parentImg->ParseEverything());
    soundProp->SetParent(parent);
    return soundProp;
  } else if (iname == "UOL") {
    reader->ReadByte();
    uint8_t uolType = reader->ReadByte();
    if (uolType == 0) {
      auto uolProp =
          std::make_unique<WzUOLProperty>(name, reader->ReadString());
      uolProp->SetParent(parent);
      return uolProp;
    } else if (uolType == 1) {
      auto uolProp = std::make_unique<WzUOLProperty>(
          name, reader->ReadStringAtOffset(offset + reader->ReadInt32()));
      uolProp->SetParent(parent);
      return uolProp;
    }
    return std::unexpected(Error::ParseError("Unsupported UOL type"));
  } else if (iname == WzRawDataProperty::RAW_DATA_HEADER) {
    uint8_t rawType = reader->ReadByte();
    auto rawProp = std::make_unique<WzRawDataProperty>(name, reader, rawType);
    rawProp->SetParent(parent);
    if (rawType == 1) {
      if (reader->ReadByte() == 1) {
        reader->SetPosition(reader->Position() + 2);
        auto r = ParsePropertyList(offset, reader, rawProp.get(), parentImg);
        if (!r.has_value()) return std::unexpected(r.error());
        for (auto& child : r.value().TakeItems()) {
          rawProp->WzProperties()->Add(std::move(child));
        }
      }
    }
    rawProp->Parse(parentImg->ParseEverything());
    return rawProp;
  } else if (iname == WzVideoProperty::CANVAS_VIDEO_HEADER) {
    auto videoProp = std::make_unique<WzVideoProperty>(name, reader);
    videoProp->SetParent(parent);
    reader->ReadByte();
    if (reader->ReadByte() == 1) {
      reader->SetPosition(reader->Position() + 2);
      auto r = ParsePropertyList(offset, reader, videoProp.get(), parentImg);
      if (!r.has_value()) return std::unexpected(r.error());
      for (auto& child : r.value().TakeItems()) {
        videoProp->WzProperties()->Add(std::move(child));
      }
    }
    videoProp->Parse(parentImg->ParseEverything());
    return videoProp;
  } else {
    return std::unexpected(Error::ParseError("Unknown iname: " + iname));
  }
}

}  // namespace wz

#include "wz/WzImageProperty.h"
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
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace wz {

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

void IPropertyContainer::AddProperties(WzPropertyCollection& props) {
  for (auto* prop : props) {
    AddProperty(prop);
  }
}

Result<WzPropertyCollection> WzImageProperty::ParsePropertyList(
    int64_t offset,
    WzBinaryReader* reader,
    WzObject* parent,
    WzImage* parentImg) {
  int entryCount = reader->ReadCompressedInt();
  WzPropertyCollection properties(parent);
  std::vector<std::unique_ptr<WzImageProperty>> ownedProperties;

  auto addOwnedProperty = [&](std::unique_ptr<WzImageProperty> prop) {
    prop->SetParent(parent);
    properties.Add(prop.get());
    ownedProperties.push_back(std::move(prop));
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
        addOwnedProperty(std::unique_ptr<WzImageProperty>(exProp.value()));
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
  for (auto& prop : ownedProperties) {
    prop.release();
  }
  return properties;
}

Result<WzImageProperty*> WzImageProperty::ParseExtendedProp(
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
    for (auto* p : r.value()) subProp->AddProperty(p);
    return subProp.release();
  } else if (iname == "Canvas") {
    auto canvasProp = std::make_unique<WzCanvasProperty>(name);
    canvasProp->SetParent(parent);
    reader->ReadByte();
    if (reader->ReadByte() == 1) {
      reader->SetPosition(reader->Position() + 2);
      auto r = ParsePropertyList(offset, reader, canvasProp.get(), parentImg);
      if (!r.has_value()) return std::unexpected(r.error());
      for (auto* p : r.value()) canvasProp->AddProperty(p);
    }
    auto png =
        std::make_unique<WzPngProperty>(reader, parentImg->ParseEverything());
    png->SetParent(canvasProp.get());
    canvasProp->SetPngProperty(png.release());
    return canvasProp.release();
  } else if (iname == "Shape2D#Vector2D") {
    auto vecProp = std::make_unique<WzVectorProperty>(name);
    vecProp->SetParent(parent);
    vecProp->X = new WzIntProperty("X", reader->ReadCompressedInt());
    vecProp->X->SetParent(vecProp.get());
    vecProp->Y = new WzIntProperty("Y", reader->ReadCompressedInt());
    vecProp->Y->SetParent(vecProp.get());
    return vecProp.release();
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
      convexProp->AddProperty(entryProp.value());
    }
    return convexProp.release();
  } else if (iname == "Sound_DX8") {
    auto soundProp = std::make_unique<WzBinaryProperty>(
        name, reader, parentImg->ParseEverything());
    soundProp->SetParent(parent);
    return soundProp.release();
  } else if (iname == "UOL") {
    reader->ReadByte();
    uint8_t uolType = reader->ReadByte();
    if (uolType == 0) {
      auto uolProp =
          std::make_unique<WzUOLProperty>(name, reader->ReadString());
      uolProp->SetParent(parent);
      return uolProp.release();
    } else if (uolType == 1) {
      auto uolProp = std::make_unique<WzUOLProperty>(
          name, reader->ReadStringAtOffset(offset + reader->ReadInt32()));
      uolProp->SetParent(parent);
      return uolProp.release();
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
        for (auto* p : r.value()) rawProp->AddProperty(p);
      }
    }
    rawProp->Parse(parentImg->ParseEverything());
    return rawProp.release();
  } else if (iname == WzVideoProperty::CANVAS_VIDEO_HEADER) {
    auto videoProp = std::make_unique<WzVideoProperty>(name, reader);
    videoProp->SetParent(parent);
    reader->ReadByte();
    if (reader->ReadByte() == 1) {
      reader->SetPosition(reader->Position() + 2);
      auto r = ParsePropertyList(offset, reader, videoProp.get(), parentImg);
      if (!r.has_value()) return std::unexpected(r.error());
      for (auto* p : r.value()) videoProp->AddProperty(p);
    }
    videoProp->Parse(parentImg->ParseEverything());
    return videoProp.release();
  } else {
    return std::unexpected(Error::ParseError("Unknown iname: " + iname));
  }
}

}  // namespace wz

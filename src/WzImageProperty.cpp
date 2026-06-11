#include "wz/WzImageProperty.h"
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
    WzObject* linkValue = uol->ResolveLinkValue();
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
  for (int i = 0; i < entryCount; i++) {
    std::string name = reader->ReadStringBlock(offset);
    uint8_t ptype = reader->ReadByte();
    switch (ptype) {
      case 0: {
        auto* prop = new WzNullProperty(name);
        prop->SetParent(parent);
        properties.Add(prop);
        break;
      }
      case 11:
      case 2: {
        auto* prop = new WzShortProperty(name, reader->ReadInt16());
        prop->SetParent(parent);
        properties.Add(prop);
        break;
      }
      case 3:
      case 19: {
        auto* prop = new WzIntProperty(name, reader->ReadCompressedInt());
        prop->SetParent(parent);
        properties.Add(prop);
        break;
      }
      case 20: {
        auto* prop = new WzLongProperty(name, reader->ReadLong());
        prop->SetParent(parent);
        properties.Add(prop);
        break;
      }
      case 4: {
        uint8_t type = reader->ReadByte();
        float val = (type == 0x80) ? reader->ReadSingle() : 0.0f;
        auto* prop = new WzFloatProperty(name, val);
        prop->SetParent(parent);
        properties.Add(prop);
        break;
      }
      case 5: {
        auto* prop = new WzDoubleProperty(name, reader->ReadDouble());
        prop->SetParent(parent);
        properties.Add(prop);
        break;
      }
      case 8: {
        auto* prop =
            new WzStringProperty(name, reader->ReadStringBlock(offset));
        prop->SetParent(parent);
        properties.Add(prop);
        break;
      }
      case 9: {
        int64_t eob =
            static_cast<int64_t>(reader->ReadUInt32()) + reader->Position();
        auto exProp =
            ParseExtendedProp(offset, reader, name, parent, parentImg);
        if (exProp.is_err()) return exProp.err();
        properties.Add(exProp.ok());
        if (reader->Position() != eob) {
          reader->SetPosition(eob);
        }
        break;
      }
      default:
        return Error::ParseError(
            "Unknown property type at ParsePropertyList, ptype = " +
            std::to_string(ptype));
    }
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
      return Error::ParseError("Invalid byte read at ParseExtendedProp");
  }

  WzImageProperty* exProp = nullptr;

  if (iname == "Property") {
    auto* subProp = new WzSubProperty(name);
    subProp->SetParent(parent);
    reader->SetPosition(reader->Position() + 2);
    auto r = ParsePropertyList(offset, reader, subProp, parentImg);
    if (r.is_err()) return r.err();
    for (auto* p : r.ok()) subProp->AddProperty(p);
    exProp = subProp;
  } else if (iname == "Canvas") {
    auto* canvasProp = new WzCanvasProperty(name);
    canvasProp->SetParent(parent);
    reader->ReadByte();
    if (reader->ReadByte() == 1) {
      reader->SetPosition(reader->Position() + 2);
      auto r = ParsePropertyList(offset, reader, canvasProp, parentImg);
      if (r.is_err()) return r.err();
      for (auto* p : r.ok()) canvasProp->AddProperty(p);
    }
    auto* png = new WzPngProperty(reader, parentImg->ParseEverything());
    png->SetParent(canvasProp);
    canvasProp->SetPngProperty(png);
    exProp = canvasProp;
  } else if (iname == "Shape2D#Vector2D") {
    auto* vecProp = new WzVectorProperty(name);
    vecProp->SetParent(parent);
    vecProp->X = new WzIntProperty("X", reader->ReadCompressedInt());
    vecProp->X->SetParent(vecProp);
    vecProp->Y = new WzIntProperty("Y", reader->ReadCompressedInt());
    vecProp->Y->SetParent(vecProp);
    exProp = vecProp;
  } else if (iname == "Shape2D#Convex2D") {
    auto* convexProp = new WzConvexProperty(name);
    convexProp->SetParent(parent);
    int convexEntryCount = reader->ReadCompressedInt();
    for (int j = 0; j < convexEntryCount; j++) {
      auto entryProp =
          ParseExtendedProp(offset, reader, name, convexProp, parentImg);
      if (entryProp.is_err()) {
        delete convexProp;
        return entryProp.err();
      }
      convexProp->AddProperty(entryProp.ok());
    }
    exProp = convexProp;
  } else if (iname == "Sound_DX8") {
    auto* soundProp =
        new WzBinaryProperty(name, reader, parentImg->ParseEverything());
    soundProp->SetParent(parent);
    exProp = soundProp;
  } else if (iname == "UOL") {
    reader->ReadByte();
    uint8_t uolType = reader->ReadByte();
    if (uolType == 0) {
      exProp = new WzUOLProperty(name, reader->ReadString());
    } else if (uolType == 1) {
      exProp = new WzUOLProperty(
          name, reader->ReadStringAtOffset(offset + reader->ReadInt32()));
    }
    if (exProp) exProp->SetParent(parent);
  } else if (iname == WzRawDataProperty::RAW_DATA_HEADER) {
    uint8_t rawType = reader->ReadByte();
    auto* rawProp = new WzRawDataProperty(name, reader, rawType);
    rawProp->SetParent(parent);
    if (rawType == 1) {
      if (reader->ReadByte() == 1) {
        reader->SetPosition(reader->Position() + 2);
        auto r = ParsePropertyList(offset, reader, rawProp, parentImg);
        if (r.is_err()) return r.err();
        for (auto* p : r.ok()) rawProp->AddProperty(p);
      }
    }
    rawProp->Parse(parentImg->ParseEverything());
    exProp = rawProp;
  } else if (iname == WzVideoProperty::CANVAS_VIDEO_HEADER) {
    auto* videoProp = new WzVideoProperty(name, reader);
    videoProp->SetParent(parent);
    reader->ReadByte();
    if (reader->ReadByte() == 1) {
      reader->SetPosition(reader->Position() + 2);
      auto r = ParsePropertyList(offset, reader, videoProp, parentImg);
      if (r.is_err()) return r.err();
      for (auto* p : r.ok()) videoProp->AddProperty(p);
    }
    videoProp->Parse(parentImg->ParseEverything());
    exProp = videoProp;
  } else {
    return Error::ParseError("Unknown iname: " + iname);
  }

  return exProp;
}

}  // namespace wz

#ifndef WZ_WZIMAGEPROPERTY_H_
#define WZ_WZIMAGEPROPERTY_H_
#include <concepts>
#include <memory>
#include <string>
#include <vector>
#include "wz/Result.h"
#include "wz/Util/Defines.h"
#include "wz/WzEnums.h"
#include "wz/WzObject.h"
#include "wz/WzPropertyCollection.h"

namespace wz {

class WzBinaryReader;
class WzBinaryWriter;
class WzImage;
class WzFile;
class WzImageProperty;

class IPropertyContainer {
 public:
  virtual ~IPropertyContainer() = default;
  virtual Result<void> AddProperty(WzImageProperty* prop) = 0;
  virtual Result<void> AddProperty(
      std::unique_ptr<WzImageProperty> prop) = 0;
  template <typename T>
    requires std::derived_from<T, WzImageProperty>
  Result<void> AddProperty(std::unique_ptr<T> prop) {
    return AddProperty(std::unique_ptr<WzImageProperty>(std::move(prop)));
  }
  virtual Result<void> AddProperties(WzPropertyCollection& props);
  virtual Result<std::unique_ptr<WzImageProperty>> RemoveProperty(
      const std::string& propertyName) = 0;
  virtual Result<std::unique_ptr<WzImageProperty>> RemoveProperty(
      WzImageProperty* prop) = 0;
  virtual Result<void> ClearProperties() = 0;
  virtual WzPropertyCollection* WzProperties() = 0;
};

class WzImageProperty : public WzObject {
 public:
  WzImageProperty() = default;
  WZ_DISALLOW_COPY_AND_MOVE(WzImageProperty)

  WzObjectType ObjectType() const override { return WzObjectType::Property; }
  WzFile* WzFileParent() const override;
  Result<std::unique_ptr<WzObject>> Remove() override;

  virtual WzPropertyType PropertyType() const = 0;
  virtual bool IsRawDataProperty() const { return false; }
  virtual bool IsVideoProperty() const { return false; }
  virtual WzPropertyCollection* WzProperties() { return nullptr; }
  Result<void> AddProperty(WzImageProperty* prop);
  Result<void> AddProperty(std::unique_ptr<WzImageProperty> prop);
  // Detaches prop from this container and returns ownership to the caller.
  Result<std::unique_ptr<WzImageProperty>> RemoveProperty(
      WzImageProperty* prop);
  Result<void> ClearProperties();
  virtual Result<void> WriteValue(WzBinaryWriter* writer) const;

  virtual WzImageProperty* operator[](const std::string& name);

  virtual WzImageProperty* GetFromPath(const std::string& path);
  WzImageProperty* GetLinkedWzImageProperty();

  WzImage* ParentImage() const;
  void MarkParentImageChanged() const;

  virtual void SetValue(const std::string& value);
  virtual void SetValue(int32_t value);
  virtual void SetValue(int64_t value);
  virtual void SetValue(float value);
  virtual void SetValue(double value);
  virtual void SetValue(const std::vector<uint8_t>& value);

  static Result<WzPropertyCollection> ParsePropertyList(int64_t offset,
                                                        WzBinaryReader* reader,
                                                        WzObject* parent,
                                                        WzImage* parentImg);
  static Result<void> WritePropertyList(WzBinaryWriter* writer,
                                        const WzPropertyCollection& properties);
  static Result<std::unique_ptr<WzImageProperty>> ParseExtendedProp(
      int64_t offset,
      WzBinaryReader* reader,
      const std::string& name,
      WzObject* parent,
      WzImage* parentImg);
};

}  // namespace wz
#endif  // WZ_WZIMAGEPROPERTY_H_

#ifndef WZ_WZOBJECT_H_
#define WZ_WZOBJECT_H_
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "wz/Result.h"
#include "wz/Util/Defines.h"
#include "wz/WzEnums.h"

namespace wz {

class WzFile;
class WzDirectory;
class WzImage;
class WzImageProperty;
class WzPropertyCollection;

class WzObject {
 public:
  WzObject() noexcept = default;
  WZ_DISALLOW_COPY_AND_MOVE(WzObject)

  virtual ~WzObject() = default;
  virtual std::string Name() const { return name_; }
  virtual void SetName(const std::string& name) { name_ = name; }
  virtual WzObjectType ObjectType() const = 0;
  virtual WzObject* Parent() const { return parent_; }
  virtual void SetParent(WzObject* p) { parent_ = p; }
  virtual WzFile* WzFileParent() const;
  virtual void Remove() {}

  std::string FullPath() const;
  WzObject* GetTopMostWzDirectory();
  WzObject* GetTopMostWzImage();

  // Cast methods
  virtual int32_t GetInt() const;
  virtual int16_t GetShort() const;
  virtual int64_t GetLong() const;
  virtual float GetFloat() const;
  virtual double GetDouble() const;
  virtual std::string GetString() const;
  virtual Result<std::vector<uint8_t>> GetBytes();

  WzObject* operator[](const std::string& name);

 protected:
  std::string name_;
  WzObject* parent_ = nullptr;
};

}  // namespace wz
#endif  // WZ_WZOBJECT_H_

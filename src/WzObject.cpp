#include "wz/WzObject.h"
#include "wz/WzDirectory.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"
#include "wz/WzImageProperty.h"

namespace wz {

WzFile* WzObject::WzFileParent() const {
  if (parent_) return parent_->WzFileParent();
  return nullptr;
}

Result<void> WzObject::Rename(const std::string& name) {
  if (name.empty()) {
    return std::unexpected(
        Error::InvalidArgument("WZ object name cannot be empty"));
  }
  if (name_ == name) return {};
  if (parent_) {
    switch (ObjectType()) {
      case WzObjectType::Directory: {
        if (parent_->ObjectType() != WzObjectType::Directory) break;
        auto* parentDir = static_cast<WzDirectory*>(parent_);
        auto* existing = parentDir->GetDirectoryByName(name);
        if (existing && existing != this) {
          return std::unexpected(
              Error::InvalidArgument("Duplicate WZ directory name: " + name));
        }
        break;
      }
      case WzObjectType::Image: {
        if (parent_->ObjectType() != WzObjectType::Directory) break;
        auto* parentDir = static_cast<WzDirectory*>(parent_);
        auto* existing = parentDir->GetImageByName(name);
        if (existing && existing != this) {
          return std::unexpected(
              Error::InvalidArgument("Duplicate WZ image name: " + name));
        }
        break;
      }
      case WzObjectType::Property: {
        WzImageProperty* existing = nullptr;
        if (parent_->ObjectType() == WzObjectType::Image) {
          auto result = static_cast<WzImage*>(parent_)->GetPropertyByName(name);
          if (!result.has_value()) return std::unexpected(result.error());
          existing = result.value();
        } else if (parent_->ObjectType() == WzObjectType::Property) {
          existing = static_cast<WzImageProperty*>(parent_)->operator[](name);
        }
        if (existing && existing != this) {
          return std::unexpected(Error::InvalidArgument(
              "Duplicate WZ image property name: " + name));
        }
        break;
      }
      default:
        break;
    }
  }
  name_ = name;
  if (ObjectType() == WzObjectType::Property) {
    static_cast<WzImageProperty*>(this)->MarkParentImageChanged();
  }
  return {};
}

std::string WzObject::FullPath() const {
  if (ObjectType() == WzObjectType::File) {
    auto* file = static_cast<const WzFile*>(this);
    if (file && file->GetWzDirectory()) return file->GetWzDirectory()->Name();
  }
  std::string result = Name();
  auto* currObj = parent_;
  while (currObj) {
    result = currObj->Name() + "\\" + result;
    currObj = currObj->Parent();
  }
  return result;
}

WzObject* WzObject::GetTopMostWzDirectory() {
  auto* p = Parent();
  if (!p) return this;
  while (p->Parent()) p = p->Parent();
  return p;
}

WzObject* WzObject::GetTopMostWzImage() {
  auto* p = Parent();
  if (!p) return this;
  while (p->Parent()) {
    p = p->Parent();
    if (p->ObjectType() == WzObjectType::Image) return p;
  }
  return p;
}

int32_t WzObject::GetInt() const {
  return 0;
}
int16_t WzObject::GetShort() const {
  return 0;
}
int64_t WzObject::GetLong() const {
  return 0;
}
float WzObject::GetFloat() const {
  return 0.0f;
}
double WzObject::GetDouble() const {
  return 0.0;
}
std::string WzObject::GetString() const {
  return {};
}
Result<std::vector<uint8_t>> WzObject::GetBytes() {
  return std::unexpected(Error::NotImplemented());
}

WzObject* WzObject::operator[](const std::string& name) {
  switch (ObjectType()) {
    case WzObjectType::File: {
      auto* f = static_cast<WzFile*>(this);
      return f->GetWzDirectory() ? (*f->GetWzDirectory())[name] : nullptr;
    }
    case WzObjectType::Directory:
      return static_cast<WzDirectory*>(this)->operator[](name);
    case WzObjectType::Image:
      return static_cast<WzImage*>(this)->operator[](name);
    case WzObjectType::Property:
      return static_cast<WzImageProperty*>(this)->operator[](name);
    default:
      return nullptr;
  }
}

}  // namespace wz

#include "wz/WzDirectory.h"
#include <algorithm>
#include "wz/Util/WzBinaryReader.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace wz {

WzDirectory::WzDirectory() = default;
WzDirectory::WzDirectory(const std::string& dirName) {
  SetName(dirName);
}
WzDirectory::WzDirectory(WzBinaryReader* reader,
                         const std::string& dirName,
                         uint32_t verHash,
                         const std::array<uint8_t, 4>& wzIv,
                         WzFile* wzFile)
    : reader_(reader), hash_(verHash), wz_iv_(wzIv), wzFile_(wzFile) {
  SetName(dirName);
}

WzDirectory::~WzDirectory() {
  name_.clear();
  reader_ = nullptr;
  images_.clear();
  subDirs_.clear();
}

std::vector<WzImage*> WzDirectory::WzImages() const {
  std::vector<WzImage*> result;
  result.reserve(images_.size());
  for (const auto& img : images_) result.push_back(img.get());
  return result;
}

std::vector<WzDirectory*> WzDirectory::WzDirectories() const {
  std::vector<WzDirectory*> result;
  result.reserve(subDirs_.size());
  for (const auto& dir : subDirs_) result.push_back(dir.get());
  return result;
}

void WzDirectory::Remove() {
  if (Parent()) {
    static_cast<WzDirectory*>(Parent())->RemoveDirectory(this);
  }
}

Result<void> WzDirectory::ParseDirectory() {
  if (!reader_) return {};
  if (reader_->Available() == 0) return {};

  int entryCount = reader_->ReadCompressedInt();
  if (entryCount < 0 || entryCount > 100000)
    return std::unexpected(
        Error::ParseError("Invalid wz version used for decryption, try "
                          "parsing other version numbers."));

  for (int i = 0; i < entryCount; i++) {
    uint8_t type = reader_->ReadByte();
    std::string fname;
    int fsize, checksum;
    int64_t woffset;
    int64_t rememberPos = 0;

    switch (static_cast<WzDirectoryType>(type)) {
      case WzDirectoryType::UnknownType_1: {
        reader_->ReadInt32();  // unknown
        reader_->ReadInt16();
        reader_->ReadOffset();
        continue;
      }
      case WzDirectoryType::RetrieveStringFromOffset_2: {
        int stringOffset = reader_->ReadInt32();
        rememberPos = reader_->Position();
        reader_->SetPosition(static_cast<int64_t>(
            reader_->GetHeader()->FStart() + stringOffset));
        type = reader_->ReadByte();
        fname = reader_->ReadString();
        break;
      }
      case WzDirectoryType::WzDirectory_3:
      case WzDirectoryType::WzImage_4: {
        fname = reader_->ReadString();
        rememberPos = reader_->Position();
        break;
      }
      default:
        return std::unexpected(Error::ParseError(
            "[WzDirectory] Unknown directory type = " + std::to_string(type)));
    }

    reader_->SetPosition(rememberPos);
    fsize = reader_->ReadCompressedInt();
    checksum = reader_->ReadCompressedInt();
    woffset = reader_->ReadOffset();

    if (type == static_cast<uint8_t>(WzDirectoryType::WzDirectory_3)) {
      auto subDir =
          std::make_unique<WzDirectory>(reader_, fname, hash_, wz_iv_, wzFile_);
      subDir->SetBlockSize(fsize);
      subDir->SetChecksum(checksum);
      subDir->SetOffset(woffset);
      subDir->SetParent(this);
      subDirs_.push_back(std::move(subDir));
    } else {
      auto img = std::make_unique<WzImage>(fname, *reader_, checksum);
      img->SetBlockSize(fsize);
      img->SetOffset(woffset);
      img->SetParent(this);
      images_.push_back(std::move(img));
    }
  }

  for (auto& subdir : subDirs_) {
    reader_->SetPosition(subdir->Offset());
    auto parseResult = subdir->ParseDirectory();
    if (!parseResult.has_value()) return std::unexpected(parseResult.error());
  }
  return {};
}

Result<void> WzDirectory::ParseImages() {
  for (auto& img : images_) {
    if (img->Reader() && img->Reader()->Position() != img->Offset()) {
      img->Reader()->SetPosition(img->Offset());
    }
    auto parseResult = img->ParseImage();
    if (!parseResult.has_value()) return std::unexpected(parseResult.error());
  }
  for (auto& subdir : subDirs_) {
    if (subdir->Reader() && subdir->Reader()->Position() != subdir->Offset()) {
      subdir->Reader()->SetPosition(subdir->Offset());
    }
    auto parseResult = subdir->ParseImages();
    if (!parseResult.has_value()) return std::unexpected(parseResult.error());
  }
  return {};
}

void WzDirectory::AddImage(WzImage* img) {
  img->SetParent(this);
  images_.push_back(std::unique_ptr<WzImage>(img));
}

void WzDirectory::AddDirectory(WzDirectory* dir) {
  dir->SetWzFile(wzFile_);
  dir->SetParent(this);
  subDirs_.push_back(std::unique_ptr<WzDirectory>(dir));
}

void WzDirectory::ClearImages() {
  images_.clear();
}

void WzDirectory::ClearDirectories() {
  subDirs_.clear();
}

static std::string ToLower(const std::string& s) {
  std::string r = s;
  std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return r;
}

WzImage* WzDirectory::GetImageByName(const std::string& name) {
  std::string lower = ToLower(name);
  for (auto& img : images_) {
    if (ToLower(img->Name()) == lower) return img.get();
  }
  return nullptr;
}

WzDirectory* WzDirectory::GetDirectoryByName(const std::string& name) {
  std::string lower = ToLower(name);
  for (auto& dir : subDirs_) {
    if (ToLower(dir->Name()) == lower) return dir.get();
  }
  return nullptr;
}

void WzDirectory::RemoveImage(WzImage* image) {
  auto it = std::find_if(images_.begin(), images_.end(), [image](auto& img) {
    return img.get() == image;
  });
  if (it != images_.end()) {
    images_.erase(it);
  }
}

void WzDirectory::RemoveDirectory(WzDirectory* dir) {
  auto it = std::find_if(subDirs_.begin(), subDirs_.end(), [dir](auto& subDir) {
    return subDir.get() == dir;
  });
  if (it != subDirs_.end()) {
    subDirs_.erase(it);
  }
}

int WzDirectory::CountImages() const {
  int count = static_cast<int>(images_.size());
  for (auto& dir : subDirs_) {
    count += dir->CountImages();
  }
  return count;
}

WzObject* WzDirectory::operator[](const std::string& name) const {
  std::string lower = ToLower(name);
  for (auto& img : images_) {
    if (ToLower(img->Name()) == lower) return img.get();
  }
  for (auto& dir : subDirs_) {
    if (ToLower(dir->Name()) == lower) return dir.get();
  }
  return nullptr;
}

}  // namespace wz

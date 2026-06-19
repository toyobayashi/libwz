#include "wz/WzDirectory.h"
#include <algorithm>
#include <mutex>
#include <sstream>
#include <vector>
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzTool.h"
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
  dir->SetHash(hash_);
  dir->SetWzIv(wz_iv_);
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

void WzDirectory::SetWzIv(const std::array<uint8_t, 4>& iv) {
  wz_iv_ = iv;
  for (auto& dir : subDirs_) {
    dir->SetWzIv(iv);
  }
}

void WzDirectory::SetHash(uint32_t h) {
  hash_ = h;
  for (auto& dir : subDirs_) {
    dir->SetHash(h);
  }
}

Result<int> WzDirectory::GenerateDataFile(const std::array<uint8_t, 4>* useIv,
                                          bool isDefaultUserKey,
                                          std::ostream* tempStream) {
  if (!tempStream) {
    return std::unexpected(
        Error::InvalidArgument("Cannot generate WZ data with null stream"));
  }

  const bool useCustomIv = useIv != nullptr;
  size_ = 0;
  const int entryCount = static_cast<int>(subDirs_.size() + images_.size());
  if (entryCount == 0) {
    offsetSize_ = 1;
    return size_;
  }

  size_ = WzTool::GetCompressedIntLength(entryCount);
  offsetSize_ = size_;

  for (auto& img : images_) {
    img->SetChanged(img->Changed() || useCustomIv || !isDefaultUserKey);
    if (img->Changed()) {
      std::ostringstream imageData(std::ios::out | std::ios::binary);
      WzBinaryWriter imageWriter(imageData, useCustomIv ? *useIv : wz_iv_);
      auto saveResult =
          img->SaveImage(&imageWriter, isDefaultUserKey, useCustomIv);
      if (!saveResult.has_value()) {
        return std::unexpected(saveResult.error());
      }

      const std::string bytes = imageData.str();
      img->CalculateAndSetImageChecksum(
          std::vector<uint8_t>(bytes.begin(), bytes.end()));
      img->SetTempFileStart(static_cast<int64_t>(tempStream->tellp()));
      tempStream->write(bytes.data(),
                        static_cast<std::streamsize>(bytes.size()));
      if (!*tempStream) {
        return std::unexpected(
            Error::IoError("Failed to write temporary WZ image data"));
      }
      img->SetTempFileEnd(static_cast<int64_t>(tempStream->tellp()));
    } else {
      img->SetTempFileStart(img->Offset());
      img->SetTempFileEnd(img->Offset() + img->BlockSize());
    }
    img->UnparseImage();

    const int nameLen = WzTool::GetWzObjectValueLength(
        img->Name(), static_cast<uint8_t>(WzDirectoryType::WzImage_4));
    const int imgLen = img->BlockSize();
    size_ += nameLen;
    size_ += WzTool::GetCompressedIntLength(imgLen);
    size_ += imgLen;
    size_ += WzTool::GetCompressedIntLength(img->Checksum());
    size_ += 4;
    offsetSize_ += nameLen;
    offsetSize_ += WzTool::GetCompressedIntLength(imgLen);
    offsetSize_ += WzTool::GetCompressedIntLength(img->Checksum());
    offsetSize_ += 4;
  }

  for (auto& dir : subDirs_) {
    const int nameLen = WzTool::GetWzObjectValueLength(
        dir->Name(), static_cast<uint8_t>(WzDirectoryType::WzDirectory_3));
    size_ += nameLen;
    auto childSize = dir->GenerateDataFile(useIv, isDefaultUserKey, tempStream);
    if (!childSize.has_value()) return std::unexpected(childSize.error());
    size_ += childSize.value();
    size_ += WzTool::GetCompressedIntLength(dir->BlockSize());
    size_ += WzTool::GetCompressedIntLength(dir->Checksum());
    size_ += 4;
    offsetSize_ += nameLen;
    offsetSize_ += WzTool::GetCompressedIntLength(dir->BlockSize());
    offsetSize_ += WzTool::GetCompressedIntLength(dir->Checksum());
    offsetSize_ += 4;
  }
  return size_;
}

Result<void> WzDirectory::SaveDirectory(WzBinaryWriter* writer) {
  if (!writer) {
    return std::unexpected(
        Error::InvalidArgument("Cannot save WZ directory with null writer"));
  }

  offset_ = writer->Position();
  const int entryCount = static_cast<int>(subDirs_.size() + images_.size());
  if (entryCount == 0) {
    size_ = 0;
    return {};
  }

  writer->WriteCompressedInt(entryCount);
  for (auto& img : images_) {
    writer->WriteWzObjectValue(img->Name(), WzDirectoryType::WzImage_4);
    writer->WriteCompressedInt(img->BlockSize());
    writer->WriteCompressedInt(img->Checksum());
    writer->WriteOffset(img->Offset());
  }
  for (auto& dir : subDirs_) {
    writer->WriteWzObjectValue(dir->Name(), WzDirectoryType::WzDirectory_3);
    writer->WriteCompressedInt(dir->BlockSize());
    writer->WriteCompressedInt(dir->Checksum());
    writer->WriteOffset(dir->Offset());
  }
  for (auto& dir : subDirs_) {
    if (dir->BlockSize() > 0) {
      auto saveResult = dir->SaveDirectory(writer);
      if (!saveResult.has_value()) {
        return std::unexpected(saveResult.error());
      }
    } else {
      writer->WriteByte(0);
    }
  }
  return {};
}

Result<void> WzDirectory::SaveImages(WzBinaryWriter* writer,
                                     std::istream* tempStream) {
  if (!writer || !tempStream) {
    return std::unexpected(
        Error::InvalidArgument("Cannot save WZ images with null stream"));
  }

  for (auto& img : images_) {
    const int64_t start = img->TempFileStart();
    const int64_t end = img->TempFileEnd();
    if (end < start) {
      return std::unexpected(
          Error::DataError("Invalid WZ image temporary byte range"));
    }
    const int64_t length = end - start;
    if (img->Changed()) {
      tempStream->seekg(start, std::ios::beg);
      std::vector<char> buffer(static_cast<size_t>(img->BlockSize()));
      tempStream->read(buffer.data(),
                       static_cast<std::streamsize>(buffer.size()));
      if (tempStream->gcount() != static_cast<std::streamsize>(buffer.size())) {
        return std::unexpected(
            Error::IoError("Failed to read changed WZ image data"));
      }
      writer->BaseStream().write(buffer.data(),
                                 static_cast<std::streamsize>(buffer.size()));
    } else {
      if (!img->Reader()) {
        return std::unexpected(Error::InvalidArgument(
            "Cannot copy unchanged WZ image without reader"));
      }
      std::lock_guard<std::recursive_mutex> lock(img->Reader()->Mutex());
      const int64_t originalPos = img->Reader()->Position();
      img->Reader()->SetPosition(start);
      auto bytes = img->Reader()->ReadBytes(static_cast<size_t>(length));
      writer->BaseStream().write(reinterpret_cast<const char*>(bytes.data()),
                                 static_cast<std::streamsize>(bytes.size()));
      img->Reader()->SetPosition(originalPos);
    }
    if (!writer->BaseStream()) {
      return std::unexpected(Error::IoError("Failed to write WZ image data"));
    }
  }

  for (auto& dir : subDirs_) {
    auto saveResult = dir->SaveImages(writer, tempStream);
    if (!saveResult.has_value()) return std::unexpected(saveResult.error());
  }
  return {};
}

uint32_t WzDirectory::GetOffsets(uint32_t currentOffset) {
  offset_ = currentOffset;
  currentOffset += static_cast<uint32_t>(offsetSize_);
  for (auto& dir : subDirs_) {
    currentOffset = dir->GetOffsets(currentOffset);
  }
  return currentOffset;
}

uint32_t WzDirectory::GetImgOffsets(uint32_t currentOffset) {
  for (auto& img : images_) {
    img->SetOffset(currentOffset);
    currentOffset += static_cast<uint32_t>(img->BlockSize());
  }
  for (auto& dir : subDirs_) {
    currentOffset = dir->GetImgOffsets(currentOffset);
  }
  return currentOffset;
}

}  // namespace wz

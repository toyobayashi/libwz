#include "wz/WzDirectory.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzTool.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace wz {

namespace {

class WzObjectValueSizer {
 public:
  int GetLength(const std::string& value, WzDirectoryType type) {
    std::string storeName =
        std::to_string(static_cast<uint8_t>(type)) + "_" + value;
    if (value.length() > 4 && cache_.contains(storeName)) {
      return 5;
    }
    cache_.insert(std::move(storeName));
    return 1 + WzTool::GetEncodedStringLength(value);
  }

 private:
  std::unordered_set<std::string> cache_;
};

Result<uint32_t> AddCheckedOffset(uint32_t current,
                                  uint64_t increment,
                                  const std::string& context) {
  if (increment > std::numeric_limits<uint32_t>::max() - current) {
    return std::unexpected(
        Error::DataError("WZ offset overflow while adding " + context));
  }
  return current + static_cast<uint32_t>(increment);
}

Result<void> AddCheckedInt(int* target, int increment) {
  if (increment < 0) {
    return std::unexpected(
        Error::DataError("Cannot add negative WZ directory size"));
  }
  if (*target > std::numeric_limits<int>::max() - increment) {
    return std::unexpected(Error::DataError("WZ directory size overflow"));
  }
  *target += increment;
  return {};
}

}  // namespace

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

Result<void> WzDirectory::TryRemove() {
  if (!Parent() || Parent()->ObjectType() != WzObjectType::Directory) {
    return std::unexpected(
        Error::NotFound("WZ directory has no parent directory"));
  }
  return static_cast<WzDirectory*>(Parent())->TryRemoveDirectory(this);
}

void WzDirectory::Remove() {
  (void)TryRemove();
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
  (void)TryAddImage(std::unique_ptr<WzImage>(img));
}

void WzDirectory::AddDirectory(WzDirectory* dir) {
  (void)TryAddDirectory(std::unique_ptr<WzDirectory>(dir));
}

Result<WzDirectory*> WzDirectory::CreateDirectory(const std::string& name) {
  auto dir = std::make_unique<WzDirectory>(name);
  auto* raw = dir.get();
  auto result = TryAddDirectory(std::move(dir));
  if (!result.has_value()) return std::unexpected(result.error());
  return raw;
}

Result<WzImage*> WzDirectory::CreateImage(const std::string& name) {
  auto img = std::make_unique<WzImage>(name);
  auto* raw = img.get();
  auto result = TryAddImage(std::move(img));
  if (!result.has_value()) return std::unexpected(result.error());
  return raw;
}

Result<void> WzDirectory::TryAddDirectory(std::unique_ptr<WzDirectory> dir) {
  if (!dir) {
    return std::unexpected(
        Error::InvalidArgument("Cannot add a null WZ directory"));
  }
  if (dir->Name().empty()) {
    return std::unexpected(
        Error::InvalidArgument("WZ directory name cannot be empty"));
  }
  if (dir->Parent()) {
    return std::unexpected(
        Error::InvalidArgument("WZ directory already has a parent"));
  }
  if (GetDirectoryByName(dir->Name())) {
    return std::unexpected(
        Error::InvalidArgument("Duplicate WZ directory name: " + dir->Name()));
  }
  dir->SetWzFile(wzFile_);
  dir->SetHash(hash_);
  dir->SetWzIv(wz_iv_);
  dir->SetParent(this);
  subDirs_.push_back(std::move(dir));
  return {};
}

Result<void> WzDirectory::TryAddImage(std::unique_ptr<WzImage> img) {
  if (!img) {
    return std::unexpected(
        Error::InvalidArgument("Cannot add a null WZ image"));
  }
  if (img->Name().empty()) {
    return std::unexpected(
        Error::InvalidArgument("WZ image name cannot be empty"));
  }
  if (img->Parent()) {
    return std::unexpected(
        Error::InvalidArgument("WZ image already has a parent"));
  }
  if (GetImageByName(img->Name())) {
    return std::unexpected(
        Error::InvalidArgument("Duplicate WZ image name: " + img->Name()));
  }
  img->SetParent(this);
  images_.push_back(std::move(img));
  return {};
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

Result<void> WzDirectory::TryRemoveImage(WzImage* image) {
  if (!image) {
    return std::unexpected(
        Error::InvalidArgument("Cannot remove a null WZ image"));
  }
  auto it = std::find_if(images_.begin(), images_.end(), [image](auto& img) {
    return img.get() == image;
  });
  if (it == images_.end()) {
    return std::unexpected(
        Error::NotFound("WZ image is not owned by this directory"));
  }
  images_.erase(it);
  return {};
}

void WzDirectory::RemoveImage(WzImage* image) {
  (void)TryRemoveImage(image);
}

Result<void> WzDirectory::TryRemoveDirectory(WzDirectory* dir) {
  if (!dir) {
    return std::unexpected(
        Error::InvalidArgument("Cannot remove a null WZ directory"));
  }
  auto it = std::find_if(subDirs_.begin(), subDirs_.end(), [dir](auto& subDir) {
    return subDir.get() == dir;
  });
  if (it == subDirs_.end()) {
    return std::unexpected(
        Error::NotFound("WZ directory is not owned by this directory"));
  }
  subDirs_.erase(it);
  return {};
}

void WzDirectory::RemoveDirectory(WzDirectory* dir) {
  (void)TryRemoveDirectory(dir);
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

  std::function<Result<void>(WzDirectory*)> prepareImageData =
      [&](WzDirectory* dir) -> Result<void> {
    for (auto& img : dir->images_) {
      img->SetChanged(img->Changed() || useCustomIv || !isDefaultUserKey);
      if (img->Changed()) {
        std::ostringstream imageData(std::ios::out | std::ios::binary);
        WzBinaryWriter imageWriter(imageData,
                                   useCustomIv ? *useIv : dir->wz_iv_);
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
      if (!img->Changed()) {
        img->UnparseImage();
      }
    }

    for (auto& childDir : dir->subDirs_) {
      auto childResult = prepareImageData(childDir.get());
      if (!childResult.has_value()) {
        return std::unexpected(childResult.error());
      }
    }
    return {};
  };

  auto prepareResult = prepareImageData(this);
  if (!prepareResult.has_value()) {
    return std::unexpected(prepareResult.error());
  }

  WzObjectValueSizer objectSizer;
  std::function<Result<int>(WzDirectory*)> calculateDirectorySize =
      [&](WzDirectory* dir) -> Result<int> {
    dir->size_ = 0;
    const int entryCount =
        static_cast<int>(dir->subDirs_.size() + dir->images_.size());
    if (entryCount == 0) {
      dir->offsetSize_ = 1;
      return dir->size_;
    }

    dir->size_ = WzTool::GetCompressedIntLength(entryCount);
    dir->offsetSize_ = dir->size_;

    for (auto& img : dir->images_) {
      const int nameLen =
          objectSizer.GetLength(img->Name(), WzDirectoryType::WzImage_4);
      const int imgLen = img->BlockSize();
      if (imgLen < 0) {
        return std::unexpected(
            Error::DataError("Cannot repack image with negative block size"));
      }
      auto addResult = AddCheckedInt(&dir->size_, nameLen);
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult =
          AddCheckedInt(&dir->size_, WzTool::GetCompressedIntLength(imgLen));
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(&dir->size_, imgLen);
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(
          &dir->size_, WzTool::GetCompressedIntLength(img->Checksum()));
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(&dir->size_, 4);
      if (!addResult.has_value()) return std::unexpected(addResult.error());

      addResult = AddCheckedInt(&dir->offsetSize_, nameLen);
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(&dir->offsetSize_,
                                WzTool::GetCompressedIntLength(imgLen));
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(
          &dir->offsetSize_, WzTool::GetCompressedIntLength(img->Checksum()));
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(&dir->offsetSize_, 4);
      if (!addResult.has_value()) return std::unexpected(addResult.error());
    }

    std::vector<std::pair<WzDirectory*, int>> childNameLengths;
    childNameLengths.reserve(dir->subDirs_.size());
    for (auto& childDir : dir->subDirs_) {
      childNameLengths.emplace_back(
          childDir.get(),
          objectSizer.GetLength(childDir->Name(),
                                WzDirectoryType::WzDirectory_3));
    }

    for (auto [childDir, nameLen] : childNameLengths) {
      auto childSize = calculateDirectorySize(childDir);
      if (!childSize.has_value()) return std::unexpected(childSize.error());
      auto addResult = AddCheckedInt(&dir->size_, nameLen);
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(&dir->size_, childSize.value());
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(
          &dir->size_, WzTool::GetCompressedIntLength(childDir->BlockSize()));
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(
          &dir->size_, WzTool::GetCompressedIntLength(childDir->Checksum()));
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(&dir->size_, 4);
      if (!addResult.has_value()) return std::unexpected(addResult.error());

      addResult = AddCheckedInt(&dir->offsetSize_, nameLen);
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult =
          AddCheckedInt(&dir->offsetSize_,
                        WzTool::GetCompressedIntLength(childDir->BlockSize()));
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult =
          AddCheckedInt(&dir->offsetSize_,
                        WzTool::GetCompressedIntLength(childDir->Checksum()));
      if (!addResult.has_value()) return std::unexpected(addResult.error());
      addResult = AddCheckedInt(&dir->offsetSize_, 4);
      if (!addResult.has_value()) return std::unexpected(addResult.error());
    }

    return dir->size_;
  };

  auto sizeResult = calculateDirectorySize(this);
  if (!sizeResult.has_value()) {
    return std::unexpected(sizeResult.error());
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
      if (img->BlockSize() < 0 ||
          length != static_cast<int64_t>(img->BlockSize())) {
        return std::unexpected(
            Error::DataError("Invalid changed WZ image byte range"));
      }
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
      const uint64_t sourceSize = img->Reader()->SourceSize();
      if (start < 0 || length < 0 ||
          static_cast<uint64_t>(start) > sourceSize ||
          static_cast<uint64_t>(length) >
              sourceSize - static_cast<uint64_t>(start)) {
        img->Reader()->SetPosition(originalPos);
        return std::unexpected(Error::IoError(
            "Unchanged WZ image source range is unavailable: start=" +
            std::to_string(start) + ", length=" + std::to_string(length) +
            ", sourceEnd=" + std::to_string(sourceSize)));
      }
      img->Reader()->SetPosition(start);
      auto bytes = img->Reader()->ReadBytes(static_cast<size_t>(length));
      if (bytes.size() != static_cast<size_t>(length)) {
        img->Reader()->SetPosition(originalPos);
        return std::unexpected(
            Error::IoError("Failed to read unchanged WZ image data"));
      }
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

Result<uint32_t> WzDirectory::GetOffsets(uint32_t currentOffset) {
  offset_ = currentOffset;
  if (offsetSize_ < 0) {
    return std::unexpected(
        Error::DataError("Cannot add negative WZ directory offset size"));
  }
  auto nextOffset = AddCheckedOffset(
      currentOffset, static_cast<uint64_t>(offsetSize_), "directory metadata");
  if (!nextOffset.has_value()) return std::unexpected(nextOffset.error());
  currentOffset = nextOffset.value();
  for (auto& dir : subDirs_) {
    auto childOffset = dir->GetOffsets(currentOffset);
    if (!childOffset.has_value()) return std::unexpected(childOffset.error());
    currentOffset = childOffset.value();
  }
  return currentOffset;
}

Result<uint32_t> WzDirectory::GetImgOffsets(uint32_t currentOffset) {
  for (auto& img : images_) {
    if (img->BlockSize() < 0) {
      return std::unexpected(
          Error::DataError("Cannot add negative WZ image block size"));
    }
    img->SetOffset(currentOffset);
    auto nextOffset = AddCheckedOffset(
        currentOffset, static_cast<uint64_t>(img->BlockSize()), "image block");
    if (!nextOffset.has_value()) return std::unexpected(nextOffset.error());
    currentOffset = nextOffset.value();
  }
  for (auto& dir : subDirs_) {
    auto childOffset = dir->GetImgOffsets(currentOffset);
    if (!childOffset.has_value()) return std::unexpected(childOffset.error());
    currentOffset = childOffset.value();
  }
  return currentOffset;
}

}  // namespace wz

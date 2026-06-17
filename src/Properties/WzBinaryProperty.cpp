#include "wz/Properties/WzBinaryProperty.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzPath.h"

namespace wz {

static constexpr size_t kSoundHeaderSize = 51;

WzBinaryProperty::WzBinaryProperty(const std::string& name,
                                   WzBinaryReader* reader,
                                   bool parseNow)
    : wzReader_(reader) {
  SetName(name);

  reader->ReadByte();  // skip 1

  soundDataLen_ = reader->ReadCompressedInt();
  lenMs_ = reader->ReadCompressedInt();

  // Peek wavFormatLen by jumping past the sound header GUIDs
  int64_t headerOff = reader->Position();
  reader->SetPosition(headerOff + kSoundHeaderSize);
  int wavFormatLen = reader->ReadByte();
  reader->SetPosition(headerOff);

  // Read header: soundHeader (51) + wavFormatLen (1) + waveFormat (N)
  auto soundHeaderBytes = reader->ReadBytes(kSoundHeaderSize);
  uint8_t unk1 = reader->ReadByte();  // wavFormatLen
  auto waveFormatBytes = reader->ReadBytes(wavFormatLen);

  header_.reserve(kSoundHeaderSize + 1 + wavFormatLen);
  header_.insert(
      header_.end(), soundHeaderBytes.begin(), soundHeaderBytes.end());
  header_.push_back(unk1);
  header_.insert(header_.end(), waveFormatBytes.begin(), waveFormatBytes.end());

  ParseWzSoundPropertyHeader();

  offs_ = reader->Position();
  if (parseNow)
    fileBytes_ = reader->ReadBytes(soundDataLen_);
  else
    reader->SetPosition(offs_ + soundDataLen_);
}

void WzBinaryProperty::ParseWzSoundPropertyHeader() {
  if (header_.size() < kSoundHeaderSize + 1 + sizeof(WzWaveFormat)) {
    type_ = WzBinaryPropertyType::Raw;
    return;
  }

  size_t wavHeaderLen = header_.size() - kSoundHeaderSize - 1;
  const uint8_t* wavBytes = header_.data() + kSoundHeaderSize + 1;

  auto* wfx = reinterpret_cast<const WzWaveFormat*>(wavBytes);
  size_t expectedSize = sizeof(WzWaveFormat) + wfx->cbSize;

  if (expectedSize != wavHeaderLen) {
    if (!wzReader_) {
      type_ = WzBinaryPropertyType::Raw;
      return;
    }

    // Try XOR decrypting the wave format header with WzKey (List.wz)
    std::vector<uint8_t> decrypted(wavHeaderLen);
    auto& wzKey = wzReader_->GetWzKey();
    for (size_t i = 0; i < wavHeaderLen; i++) {
      decrypted[i] = wavBytes[i] ^ wzKey[i];
    }

    wfx = reinterpret_cast<const WzWaveFormat*>(decrypted.data());
    expectedSize = sizeof(WzWaveFormat) + wfx->cbSize;
    if (expectedSize != wavHeaderLen) {
      type_ = WzBinaryPropertyType::Raw;
      return;
    }
    headerEncrypted_ = true;
  }

  wavFormat_ = *wfx;

  if (wfx->wFormatTag == 0x0055 && wavHeaderLen >= sizeof(WzMp3WaveFormat)) {
    type_ = WzBinaryPropertyType::MP3;
  } else if (wfx->wFormatTag == 1) {
    type_ = WzBinaryPropertyType::WAV;
  }
}

Result<std::vector<uint8_t>> WzBinaryProperty::GetBytes() {
  return GetBytes(false);
}

Result<std::vector<uint8_t>> WzBinaryProperty::GetBytes(bool saveInMemory) {
  if (!fileBytes_.empty()) return fileBytes_;
  if (!wzReader_)
    return std::unexpected(Error::DataError("No reader for binary property"));

  auto currentPos = wzReader_->Position();
  wzReader_->SetPosition(offs_);
  fileBytes_ = wzReader_->ReadBytes(soundDataLen_);
  wzReader_->SetPosition(currentPos);

  if (saveInMemory) return fileBytes_;
  auto result = fileBytes_;
  fileBytes_.clear();
  return result;
}

Result<std::vector<uint8_t>> WzBinaryProperty::GetBytesForWAVPlayback(
    bool saveInMemory) {
  static const uint8_t riffWaveHeader[] = {
      0x52, 0x49, 0x46, 0x46, 0x14, 0x60, 0x28, 0x00, 0x57, 0x41,
      0x56, 0x45, 0x66, 0x6D, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00,
      0x00, 0x01, 0x00, 0x01, 0x00, 0x22, 0x56, 0x00, 0x00,
  };

  auto bytesResult = GetBytes(saveInMemory);
  if (!bytesResult.has_value()) return std::unexpected(bytesResult.error());
  auto& soundBytes = bytesResult.value();
  if (soundBytes.empty())
    return std::unexpected(Error::DataError("Empty sound data"));

  size_t totalLen = sizeof(riffWaveHeader) + header_.size() + soundBytes.size();
  std::vector<uint8_t> combined(totalLen);

  size_t offset = 0;
  std::memcpy(combined.data() + offset, riffWaveHeader, sizeof(riffWaveHeader));
  offset += sizeof(riffWaveHeader);
  std::memcpy(combined.data() + offset, header_.data(), header_.size());
  offset += header_.size();
  std::memcpy(combined.data() + offset, soundBytes.data(), soundBytes.size());

  return combined;
}

Result<void> WzBinaryProperty::SaveToFile(const std::string& filePath) {
  auto result = GetBytes(false);
  if (!result.has_value()) return std::unexpected(result.error());
  auto& bytes = result.value();
  auto outPath = wz::to_path(filePath);
  auto parentPath = outPath.parent_path();
  std::error_code ec;
  if (!parentPath.empty()) {
    std::filesystem::create_directories(parentPath, ec);
    if (ec) return std::unexpected(Error::IoError(ec.message()));
  }
  std::ofstream out(outPath, std::ios::binary);
  if (!out)
    return std::unexpected(Error::IoError("Failed to open file for writing"));
  out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  return {};
}

}  // namespace wz

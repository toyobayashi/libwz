#include "wz/Properties/WzPngProperty.h"
#include <zlib.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include "wz/PngUtility.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzPath.h"
#include "wz/WzImage.h"

namespace wz {

namespace {

static bool png_crc32_table_initialized = false;
static uint32_t png_crc32_table[256];

static void PngCrc32Init() {
  if (png_crc32_table_initialized) return;
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t crc = i;
    for (int j = 0; j < 8; j++)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0);
    png_crc32_table[i] = crc;
  }
  png_crc32_table_initialized = true;
}

static uint32_t PngCrc32(const uint8_t* data, size_t len) {
  PngCrc32Init();
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; i++)
    crc = png_crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFu;
}

static uint32_t ToBigEndian32(uint32_t x) {
  return ((x & 0x000000FFu) << 24) | ((x & 0x0000FF00u) << 8) |
         ((x & 0x00FF0000u) >> 8) | ((x & 0xFF000000u) >> 24);
}

static void WritePngChunk(std::ofstream& out,
                          const char type[4],
                          const uint8_t* data,
                          uint32_t len) {
  uint32_t beLen = ToBigEndian32(len);
  out.write(reinterpret_cast<const char*>(&beLen), 4);
  out.write(type, 4);
  if (len > 0) out.write(reinterpret_cast<const char*>(data), len);

  std::vector<uint8_t> crcInput(4 + len);
  std::memcpy(crcInput.data(), type, 4);
  if (len > 0) std::memcpy(crcInput.data() + 4, data, len);
  uint32_t crc = PngCrc32(crcInput.data(), 4 + len);
  uint32_t beCrc = ToBigEndian32(crc);
  out.write(reinterpret_cast<const char*>(&beCrc), 4);
}

}  // namespace

WzPngProperty::WzPngProperty() = default;

WzPngProperty::WzPngProperty(WzBinaryReader* reader, bool parseNow)
    : wzReader_(reader) {
  width_ = reader->ReadCompressedInt();
  height_ = reader->ReadCompressedInt();

  int format1 = reader->ReadCompressedInt();
  int format2 = reader->ReadCompressedInt();
  format_ = static_cast<WzPngFormat>(format1 + (format2 << 8));

  reader->SetPosition(reader->Position() + 4);
  offs_ = reader->Position();
  int len = reader->ReadInt32() - 1;
  reader->ReadByte();  // skip 1

  if (len > 0) {
    if (parseNow) {
      compressedImageBytes_ = wzReader_->ReadBytes(len);
      ParsePng(true);
    } else {
      reader->SetPosition(reader->Position() + len);
    }
  }
}

int WzPngProperty::GetUncompressedSize() const {
  switch (format_) {
    case WzPngFormat::Format1:
      return width_ * height_ * 2;
    case WzPngFormat::Format2:
      return width_ * height_ * 4;
    case WzPngFormat::Format3:
      return width_ * height_ * 4;
    case WzPngFormat::Format257:
      return width_ * height_ * 2;
    case WzPngFormat::Format513:
      return width_ * height_ * 2;
    case WzPngFormat::Format517:
      return width_ * height_ / 128;
    case WzPngFormat::Format1026:
      return width_ * height_ * 4;
    case WzPngFormat::Format2050:
      return width_ * height_;
    default:
      return width_ * height_ * 4;
  }
}

Result<std::vector<uint8_t>> WzPngProperty::GetCompressedBytes(
    bool saveInMemory) {
  if (compressedImageBytes_.empty()) {
    if (!wzReader_) return Error::DataError("No reader for compressed bytes");

    auto pos = wzReader_->Position();
    wzReader_->SetPosition(offs_);
    int len = wzReader_->ReadInt32() - 1;
    wzReader_->ReadByte();  // skip 1

    if (len <= 0)
      return Error::DataError(
          "The length of the image is negative. Wrong WzIV?");

    compressedImageBytes_ = wzReader_->ReadBytes(len);
    wzReader_->SetPosition(pos);

    if (!saveInMemory) {
      auto result = compressedImageBytes_;
      compressedImageBytes_.clear();
      return result;
    }
  }
  return compressedImageBytes_;
}

Result<std::vector<uint8_t>> WzPngProperty::GetRawImage(bool saveInMemory) {
  auto rawImageBytes = GetCompressedBytes(saveInMemory);
  if (!rawImageBytes.is_ok()) return rawImageBytes.err();
  auto& rawBytes = rawImageBytes.ok();
  if (rawBytes.size() < 2) return Error::DataError("Raw image too small");

  uint16_t header = rawBytes[0] | (static_cast<uint16_t>(rawBytes[1]) << 8);
  bool isListWz = (header != 0x9C78 && header != 0xDA78 && header != 0x0178 &&
                   header != 0x5E78);
  listWzUsed_ = isListWz;

  if (!isListWz) {
    z_stream infstream;
    std::vector<uint8_t> result(GetUncompressedSize());
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = (uInt)rawBytes.size();   // size of input
    infstream.next_in = (Bytef*)rawBytes.data();  // input char array NOLINT
    infstream.avail_out = (uInt)result.size();    // size of output
    infstream.next_out = (Bytef*)result.data();   // output char array NOLINT

    inflateInit(&infstream);
    int r = inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
    return (r == Z_OK || r == Z_STREAM_END)
               ? result
               : Result<std::vector<uint8_t>>(
                     Error::DataError("inflate failed"));
  } else {
    // List.wz format - decrypt then decompress
    if (!wzReader_)
      return Error::DataError("No reader for list.wz decompression");

    std::vector<uint8_t> decryptedData;
    size_t pos = 0;
    auto& wzKey = wzReader_->GetWzKey();

    while (pos < rawBytes.size()) {
      int32_t blockSize =
          *reinterpret_cast<const int32_t*>(rawBytes.data() + pos);
      pos += 4;
      for (int i = 0; i < blockSize && pos < rawBytes.size(); i++, pos++) {
        decryptedData.push_back(rawBytes[pos] ^ wzKey[i]);
      }
    }

    if (decryptedData.size() < 3)
      return Error::DataError("Decrypted data too small");

    z_stream infstream;
    std::vector<uint8_t> result(GetUncompressedSize());
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = static_cast<uInt>(decryptedData.size());
    infstream.next_in = decryptedData.data();
    infstream.avail_out = (uInt)result.size();   // size of output
    infstream.next_out = (Bytef*)result.data();  // output char array NOLINT

    inflateInit(&infstream);
    int r = inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
    return (r == Z_OK || r == Z_STREAM_END)
               ? result
               : Result<std::vector<uint8_t>>(
                     Error::DataError("inflate failed"));
  }
}

Result<std::vector<uint8_t>> WzPngProperty::GetCompressedBytesForExtraction(
    bool saveInMemory) {
  auto rawBytes = GetCompressedBytes(saveInMemory);
  if (!rawBytes.is_ok()) return rawBytes.err();
  auto& rawData = rawBytes.ok();
  if (rawData.size() < 2) return std::move(rawData);

  uint16_t header = rawData[0] | (static_cast<uint16_t>(rawData[1]) << 8);
  bool isListWz = (header != 0x9C78 && header != 0xDA78 && header != 0x0178 &&
                   header != 0x5E78);
  if (!isListWz) return rawData;

  // Convert listWz to standard zlib
  auto raw = GetRawImage(true);
  if (!raw.is_ok()) return rawData;

  // Re-compress to standard zlib
  auto& rawImg = raw.ok();
  uLongf destLen = compressBound(static_cast<uLong>(rawImg.size()));
  std::vector<uint8_t> compressed(destLen + 2);
  compressed[0] = 0x78;
  compressed[1] = 0x9C;

  if (compress2(compressed.data() + 2,
                &destLen,
                rawImg.data(),
                static_cast<uLong>(rawImg.size()),
                Z_DEFAULT_COMPRESSION) != Z_OK)
    return rawData;

  compressed.resize(destLen + 2);
  return compressed;
}

void WzPngProperty::ParsePng(bool saveInMemory) {
  auto rawResult = GetRawImage(saveInMemory);
  if (!rawResult.is_ok()) return;
  auto rawBytes = std::move(rawResult.ok());

  switch (format_) {
    case WzPngFormat::Format1: {
      int pixelCount = width_ * height_;
      pngData_.resize(pixelCount * 4);
      PngUtility::DecompressImagePixelDataBgra4444(
          rawBytes, width_, height_, pngData_);
    } break;
    case WzPngFormat::Format2:
      pngData_ = std::move(rawBytes);
      break;
    case WzPngFormat::Format3: {
      pngData_.resize(width_ * height_ * 4);
      PngUtility::DecompressImageDXT3(rawBytes, width_, height_, pngData_);
    } break;
    case WzPngFormat::Format257: {
      // ARGB1555 (16bpp) -> BGRA32 upconversion
      int pixelCount = width_ * height_;
      pngData_.resize(pixelCount * 4);
      for (int i = 0; i < pixelCount; i++) {
        uint8_t low = rawBytes[i * 2];
        uint8_t high = rawBytes[i * 2 + 1];
        uint16_t val = static_cast<uint16_t>(low | (high << 8));
        // ARGB1555: bit 15 = alpha, 14-10 = R (5 bits), 9-5 = G (5 bits), 4-0 =
        // B (5 bits)
        uint8_t a = (val & 0x8000) ? 255 : 0;
        uint8_t r = static_cast<uint8_t>(((val >> 10) & 0x1F) << 3);
        uint8_t g = static_cast<uint8_t>(((val >> 5) & 0x1F) << 3);
        uint8_t b = static_cast<uint8_t>((val & 0x1F) << 3);
        pngData_[i * 4] = b;
        pngData_[i * 4 + 1] = g;
        pngData_[i * 4 + 2] = r;
        pngData_[i * 4 + 3] = a;
      }
    } break;
    case WzPngFormat::Format513: {
      // RGB565 (16bpp) -> BGRA32 upconversion
      int pixelCount = width_ * height_;
      pngData_.resize(pixelCount * 4);
      for (int i = 0; i < pixelCount; i++) {
        uint8_t low = rawBytes[i * 2];
        uint8_t high = rawBytes[i * 2 + 1];
        uint16_t val = static_cast<uint16_t>(low | (high << 8));
        auto c = PngUtility::RGB565ToColor(val);
        pngData_[i * 4] = c.B;
        pngData_[i * 4 + 1] = c.G;
        pngData_[i * 4 + 2] = c.R;
        pngData_[i * 4 + 3] = 255;
      }
    } break;
    case WzPngFormat::Format517: {
      int pixelCount = width_ * height_;
      std::vector<uint8_t> dec517(pixelCount * 2);
      PngUtility::DecompressImagePixelDataForm517(
          rawBytes, width_, height_, dec517);
      pngData_.resize(pixelCount * 4);
      for (int i = 0; i < pixelCount; i++) {
        uint8_t low = dec517[i * 2];
        uint8_t high = dec517[i * 2 + 1];
        uint16_t val = static_cast<uint16_t>(low | (high << 8));
        auto c = PngUtility::RGB565ToColor(val);
        pngData_[i * 4] = c.B;
        pngData_[i * 4 + 1] = c.G;
        pngData_[i * 4 + 2] = c.R;
        pngData_[i * 4 + 3] = 255;
      }
    } break;
    case WzPngFormat::Format1026: {
      pngData_.resize(width_ * height_ * 4);
      PngUtility::DecompressImageDXT3(rawBytes, width_, height_, pngData_);
    } break;
    case WzPngFormat::Format2050: {
      pngData_.resize(width_ * height_ * 4);
      PngUtility::DecompressImageDXT5(rawBytes, width_, height_, pngData_);
    } break;
    default:
      pngData_.resize(width_ * height_ * 4);
      break;
  }
}

Result<std::vector<uint8_t>> WzPngProperty::GetImage(bool saveInMemory) {
  if (pngData_.empty()) {
    ParsePng(saveInMemory);
  }
  if (pngData_.empty()) return Error::DataError("Failed to parse PNG");
  return pngData_;
}

Result<void> WzPngProperty::SaveToFile(const std::string& filePath) {
  auto result = GetImage(true);
  if (!result.is_ok()) return result.err();
  auto& bgra = result.ok();
  if (bgra.empty()) return Error::DataError("PNG image data is empty");

  std::vector<uint8_t> rgba(bgra.size());
  for (size_t i = 0; i < bgra.size(); i += 4) {
    rgba[i] = bgra[i + 2];
    rgba[i + 1] = bgra[i + 1];
    rgba[i + 2] = bgra[i];
    rgba[i + 3] = bgra[i + 3];
  }

  size_t rowSize = static_cast<size_t>(width_) * 4;
  std::vector<uint8_t> rawData(static_cast<size_t>(height_) * (1 + rowSize), 0);
  for (int y = 0; y < height_; y++) {
    rawData[y * (1 + rowSize)] = 0;
    std::memcpy(rawData.data() + y * (1 + rowSize) + 1,
                rgba.data() + static_cast<size_t>(y) * rowSize,
                rowSize);
  }

  z_stream zs = {};
  if (deflateInit2(
          &zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) !=
      Z_OK)
    return Error::DataError("deflateInit2 failed");

  zs.next_in = rawData.data();
  zs.avail_in = static_cast<uInt>(rawData.size());

  size_t compBound = deflateBound(&zs, static_cast<uLong>(rawData.size()));
  std::vector<uint8_t> compressed(compBound);
  zs.next_out = compressed.data();
  zs.avail_out = static_cast<uInt>(compressed.size());

  int ret = deflate(&zs, Z_FINISH);
  if (ret != Z_STREAM_END) {
    deflateEnd(&zs);
    return Error::DataError("deflate failed");
  }
  size_t compressedSize = zs.total_out;
  deflateEnd(&zs);

  auto outPath = wz::to_path(filePath);
  auto parentPath = outPath.parent_path();
  std::error_code ec;
  if (!parentPath.empty()) {
    std::filesystem::create_directories(parentPath, ec);
    if (ec) return Error::IoError(ec.message());
  }
  std::ofstream out(outPath, std::ios::binary);
  if (!out) return Error::IoError("Failed to open file for writing");

  const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  out.write(reinterpret_cast<const char*>(signature), 8);

  struct {
    uint32_t w, h;
    uint8_t bitDepth, colorType, compression, filter, interlace;
  } ihdr;
  ihdr.w = ToBigEndian32(static_cast<uint32_t>(width_));
  ihdr.h = ToBigEndian32(static_cast<uint32_t>(height_));
  ihdr.bitDepth = 8;
  ihdr.colorType = 6;
  ihdr.compression = 0;
  ihdr.filter = 0;
  ihdr.interlace = 0;
  WritePngChunk(out, "IHDR", reinterpret_cast<const uint8_t*>(&ihdr), 13);

  WritePngChunk(
      out, "IDAT", compressed.data(), static_cast<uint32_t>(compressedSize));

  WritePngChunk(out, "IEND", nullptr, 0);

  return {};
}

}  // namespace wz

#include "wz/Properties/WzPngProperty.h"
#include <zlib.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <mutex>
#include "wz/PngUtility.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzPath.h"
#include "wz/Util/WzStream.h"
#include "wz/WzImage.h"

namespace wz {

namespace {

static const std::array<uint32_t, 256>& PngCrc32Table() {
  static const std::array<uint32_t, 256> table = [] {
    std::array<uint32_t, 256> result{};
    for (uint32_t i = 0; i < result.size(); i++) {
      uint32_t crc = i;
      for (int j = 0; j < 8; j++)
        crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0);
      result[i] = crc;
    }
    return result;
  }();
  return table;
}

static uint32_t PngCrc32(const uint8_t* data, size_t len) {
  const auto& table = PngCrc32Table();
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; i++)
    crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFu;
}

static uint32_t ToBigEndian32(uint32_t x) {
  return ((x & 0x000000FFu) << 24) | ((x & 0x0000FF00u) << 8) |
         ((x & 0x00FF0000u) >> 8) | ((x & 0xFF000000u) >> 24);
}

static bool WritePngChunk(WzFileStream* out,
                          const char type[4],
                          const uint8_t* data,
                          uint32_t len) {
  uint32_t beLen = ToBigEndian32(len);
  if (!out->Write(&beLen, 4)) return false;
  if (!out->Write(type, 4)) return false;
  if (len > 0 && !out->Write(data, len)) return false;

  std::vector<uint8_t> crcInput(4 + len);
  std::memcpy(crcInput.data(), type, 4);
  if (len > 0) std::memcpy(crcInput.data() + 4, data, len);
  uint32_t crc = PngCrc32(crcInput.data(), 4 + len);
  uint32_t beCrc = ToBigEndian32(crc);
  return out->Write(&beCrc, 4);
}

static uint32_t ReadBigEndian32(const uint8_t* p) {
  return (static_cast<uint32_t>(p[0]) << 24) |
         (static_cast<uint32_t>(p[1]) << 16) |
         (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

static uint8_t PaethPredictor(uint8_t left, uint8_t up, uint8_t upLeft) {
  int p = static_cast<int>(left) + static_cast<int>(up) -
          static_cast<int>(upLeft);
  int pa = std::abs(p - static_cast<int>(left));
  int pb = std::abs(p - static_cast<int>(up));
  int pc = std::abs(p - static_cast<int>(upLeft));
  if (pa <= pb && pa <= pc) return left;
  if (pb <= pc) return up;
  return upLeft;
}

static Result<std::vector<uint8_t>> InflatePngData(
    const std::vector<uint8_t>& compressed,
    size_t expectedSize) {
  std::vector<uint8_t> result(expectedSize);
  z_stream zs = {};
  zs.next_in = const_cast<Bytef*>(compressed.data());  // NOLINT
  zs.avail_in = static_cast<uInt>(compressed.size());
  zs.next_out = result.data();
  zs.avail_out = static_cast<uInt>(result.size());

  if (inflateInit(&zs) != Z_OK)
    return std::unexpected(Error::DataError("inflateInit failed"));

  int ret = inflate(&zs, Z_FINISH);
  inflateEnd(&zs);
  if (ret != Z_STREAM_END || zs.total_out != expectedSize)
    return std::unexpected(Error::DataError("PNG inflate failed"));

  return result;
}

}  // namespace

WzPngProperty::WzPngProperty() = default;

Result<std::unique_ptr<WzPngProperty>> WzPngProperty::FromPngFile(
    const std::string& filePath) {
  auto inputPath = wz::to_path(filePath);
  WzFileStream in(inputPath, "rb");
  if (!in.IsOpen())
    return std::unexpected(Error::IoError("Failed to open PNG file"));

  const int64_t inputSize = in.Size();
  if (inputSize < 0)
    return std::unexpected(Error::IoError("Failed to get PNG file size"));
  std::vector<uint8_t> png(static_cast<size_t>(inputSize));
  if (!png.empty() && in.Read(png.data(), png.size()) != png.size())
    return std::unexpected(Error::IoError("Failed to read PNG file"));
  static const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  if (png.size() < 33 || std::memcmp(png.data(), signature, 8) != 0)
    return std::unexpected(Error::DataError("Invalid PNG signature"));

  int width = 0;
  int height = 0;
  uint8_t bitDepth = 0;
  uint8_t colorType = 0;
  std::vector<uint8_t> idat;

  size_t pos = 8;
  while (pos + 8 <= png.size()) {
    uint32_t len = ReadBigEndian32(png.data() + pos);
    pos += 4;
    if (pos + 4 + len + 4 > png.size())
      return std::unexpected(Error::DataError("Truncated PNG chunk"));

    const uint8_t* type = png.data() + pos;
    pos += 4;
    const uint8_t* data = png.data() + pos;

    if (std::memcmp(type, "IHDR", 4) == 0) {
      if (len != 13)
        return std::unexpected(Error::DataError("Invalid PNG IHDR"));
      width = static_cast<int>(ReadBigEndian32(data));
      height = static_cast<int>(ReadBigEndian32(data + 4));
      bitDepth = data[8];
      colorType = data[9];
      if (data[10] != 0 || data[11] != 0 || data[12] != 0)
        return std::unexpected(
            Error::DataError("Unsupported PNG compression/filter/interlace"));
    } else if (std::memcmp(type, "IDAT", 4) == 0) {
      idat.insert(idat.end(), data, data + len);
    } else if (std::memcmp(type, "IEND", 4) == 0) {
      break;
    }

    pos += len + 4;  // skip data and CRC
  }

  if (width <= 0 || height <= 0 || idat.empty())
    return std::unexpected(Error::DataError("PNG image is empty"));
  if (bitDepth != 8 || (colorType != 6 && colorType != 2))
    return std::unexpected(
        Error::DataError("Only 8-bit RGB/RGBA PNG files are supported"));

  const size_t channels = colorType == 6 ? 4 : 3;
  const size_t rowSize = static_cast<size_t>(width) * channels;
  const size_t inflatedSize = static_cast<size_t>(height) * (rowSize + 1);
  auto inflatedResult = InflatePngData(idat, inflatedSize);
  if (!inflatedResult.has_value())
    return std::unexpected(inflatedResult.error());
  auto& inflated = inflatedResult.value();

  std::vector<uint8_t> bgra(static_cast<size_t>(width) * height * 4);
  std::vector<uint8_t> previous(rowSize, 0);
  std::vector<uint8_t> current(rowSize, 0);
  size_t sourceOffset = 0;

  for (int y = 0; y < height; y++) {
    uint8_t filter = inflated[sourceOffset++];
    const uint8_t* scanline = inflated.data() + sourceOffset;
    sourceOffset += rowSize;

    for (size_t x = 0; x < rowSize; x++) {
      uint8_t left = x >= channels ? current[x - channels] : 0;
      uint8_t up = previous[x];
      uint8_t upLeft = x >= channels ? previous[x - channels] : 0;
      uint8_t value = scanline[x];
      switch (filter) {
        case 0:
          break;
        case 1:
          value = static_cast<uint8_t>(value + left);
          break;
        case 2:
          value = static_cast<uint8_t>(value + up);
          break;
        case 3:
          value = static_cast<uint8_t>(
              value + static_cast<uint8_t>((static_cast<int>(left) +
                                             static_cast<int>(up)) /
                                            2));
          break;
        case 4:
          value = static_cast<uint8_t>(
              value + PaethPredictor(left, up, upLeft));
          break;
        default:
          return std::unexpected(Error::DataError("Unsupported PNG filter"));
      }
      current[x] = value;
    }

    for (int x = 0; x < width; x++) {
      size_t src = static_cast<size_t>(x) * channels;
      size_t dst = (static_cast<size_t>(y) * width + x) * 4;
      bgra[dst] = current[src + 2];
      bgra[dst + 1] = current[src + 1];
      bgra[dst + 2] = current[src];
      bgra[dst + 3] = channels == 4 ? current[src + 3] : 255;
    }

    std::swap(previous, current);
    std::fill(current.begin(), current.end(), 0);
  }

  uLongf compressedSize = compressBound(static_cast<uLong>(bgra.size()));
  std::vector<uint8_t> compressed(compressedSize);
  if (compress2(compressed.data(),
                &compressedSize,
                bgra.data(),
                static_cast<uLong>(bgra.size()),
                Z_DEFAULT_COMPRESSION) != Z_OK) {
    return std::unexpected(Error::DataError("PNG recompress failed"));
  }
  compressed.resize(compressedSize);

  auto property = std::make_unique<WzPngProperty>();
  property->width_ = width;
  property->height_ = height;
  property->format_ = WzPngFormat::Format2;
  property->compressedImageBytes_ = std::move(compressed);
  property->pngData_ = std::move(bgra);
  property->listWzUsed_ = false;
  return property;
}

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
      auto parseResult = ParsePng(true);
      (void)parseResult;
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
  std::unique_lock<std::recursive_mutex> lock;
  if (wzReader_)
    lock = std::unique_lock<std::recursive_mutex>(wzReader_->Mutex());
  if (compressedImageBytes_.empty()) {
    if (!wzReader_) {
      return std::unexpected(
          Error::DataError("No reader for compressed bytes"));
    }

    auto pos = wzReader_->Position();
    wzReader_->SetPosition(offs_);
    int len = wzReader_->ReadInt32() - 1;
    wzReader_->ReadByte();  // skip 1

    if (len <= 0)
      return std::unexpected(
          Error::DataError("The length of the image is negative. Wrong WzIV?"));

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
  if (!rawImageBytes.has_value()) return std::unexpected(rawImageBytes.error());
  auto& rawBytes = rawImageBytes.value();
  if (rawBytes.size() < 2)
    return std::unexpected(Error::DataError("Raw image too small"));

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
    if (r == Z_OK || r == Z_STREAM_END) return result;
    return std::unexpected(Error::DataError("inflate failed"));
  } else {
    // List.wz format - decrypt then decompress
    if (!wzReader_)
      return std::unexpected(
          Error::DataError("No reader for list.wz decompression"));

    std::vector<uint8_t> decryptedData;
    size_t pos = 0;
    std::lock_guard<std::recursive_mutex> lock(wzReader_->Mutex());
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
      return std::unexpected(Error::DataError("Decrypted data too small"));

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
    if (r == Z_OK || r == Z_STREAM_END) return result;
    return std::unexpected(Error::DataError("inflate failed"));
  }
}

Result<std::vector<uint8_t>> WzPngProperty::GetCompressedBytesForExtraction(
    bool saveInMemory) {
  auto rawBytes = GetCompressedBytes(saveInMemory);
  if (!rawBytes.has_value()) return std::unexpected(rawBytes.error());
  auto& rawData = rawBytes.value();
  if (rawData.size() < 2) return std::move(rawData);

  uint16_t header = rawData[0] | (static_cast<uint16_t>(rawData[1]) << 8);
  bool isListWz = (header != 0x9C78 && header != 0xDA78 && header != 0x0178 &&
                   header != 0x5E78);
  if (!isListWz) return rawData;

  // Convert listWz to standard zlib
  auto raw = GetRawImage(true);
  if (!raw.has_value()) return rawData;

  // Re-compress to standard zlib
  auto& rawImg = raw.value();
  uLongf destLen = compressBound(static_cast<uLong>(rawImg.size()));
  std::vector<uint8_t> compressed(destLen);

  if (compress2(compressed.data(),
                &destLen,
                rawImg.data(),
                static_cast<uLong>(rawImg.size()),
                Z_DEFAULT_COMPRESSION) != Z_OK)
    return rawData;

  compressed.resize(destLen);
  return compressed;
}

Result<void> WzPngProperty::ParsePng(bool saveInMemory) {
  auto rawResult = GetRawImage(saveInMemory);
  if (!rawResult.has_value()) return std::unexpected(rawResult.error());
  auto rawBytes = std::move(rawResult.value());

  switch (format_) {
    case WzPngFormat::Format1: {
      int pixelCount = width_ * height_;
      pngData_.resize(pixelCount * 4);
      auto decodeResult = PngUtility::DecompressImagePixelDataBgra4444(
          rawBytes, width_, height_, pngData_);
      if (!decodeResult.has_value()) {
        pngData_.clear();
        return std::unexpected(decodeResult.error());
      }
    } break;
    case WzPngFormat::Format2:
      pngData_ = std::move(rawBytes);
      break;
    case WzPngFormat::Format3: {
      pngData_.resize(width_ * height_ * 4);
      auto decodeResult =
          PngUtility::DecompressImageDXT3(rawBytes, width_, height_, pngData_);
      if (!decodeResult.has_value()) {
        pngData_.clear();
        return std::unexpected(decodeResult.error());
      }
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
      auto decodeResult =
          PngUtility::DecompressImageDXT3(rawBytes, width_, height_, pngData_);
      if (!decodeResult.has_value()) {
        pngData_.clear();
        return std::unexpected(decodeResult.error());
      }
    } break;
    case WzPngFormat::Format2050: {
      pngData_.resize(width_ * height_ * 4);
      auto decodeResult =
          PngUtility::DecompressImageDXT5(rawBytes, width_, height_, pngData_);
      if (!decodeResult.has_value()) {
        pngData_.clear();
        return std::unexpected(decodeResult.error());
      }
    } break;
    default:
      pngData_.resize(width_ * height_ * 4);
      break;
  }
  return {};
}

Result<std::vector<uint8_t>> WzPngProperty::GetImage(bool saveInMemory) {
  std::unique_lock<std::recursive_mutex> lock;
  if (wzReader_)
    lock = std::unique_lock<std::recursive_mutex>(wzReader_->Mutex());
  if (pngData_.empty()) {
    auto parseResult = ParsePng(saveInMemory);
    if (!parseResult.has_value()) return std::unexpected(parseResult.error());
  }
  if (pngData_.empty())
    return std::unexpected(Error::DataError("Failed to parse PNG"));
  return pngData_;
}

Result<void> WzPngProperty::SaveToFile(const std::string& filePath) {
  auto result = GetImage(false);
  if (!result.has_value()) return std::unexpected(result.error());
  auto& bgra = result.value();
  if (bgra.empty())
    return std::unexpected(Error::DataError("PNG image data is empty"));

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
      Z_OK) {
    return std::unexpected(Error::DataError("deflateInit2 failed"));
  }

  zs.next_in = rawData.data();
  zs.avail_in = static_cast<uInt>(rawData.size());

  size_t compBound = deflateBound(&zs, static_cast<uLong>(rawData.size()));
  std::vector<uint8_t> compressed(compBound);
  zs.next_out = compressed.data();
  zs.avail_out = static_cast<uInt>(compressed.size());

  int ret = deflate(&zs, Z_FINISH);
  if (ret != Z_STREAM_END) {
    deflateEnd(&zs);
    return std::unexpected(Error::DataError("deflate failed"));
  }
  size_t compressedSize = zs.total_out;
  deflateEnd(&zs);

  auto outPath = wz::to_path(filePath);
  auto parentPath = outPath.parent_path();
  std::error_code ec;
  if (!parentPath.empty()) {
    std::filesystem::create_directories(parentPath, ec);
    if (ec) return std::unexpected(Error::IoError(ec.message()));
  }
  WzFileStream out;
  if (!out.Open(outPath, "wb"))
    return std::unexpected(Error::IoError("Failed to open file for writing"));

  const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  if (!out.Write(signature, sizeof(signature))) {
    return std::unexpected(Error::IoError("Failed to write PNG signature"));
  }

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
  if (!WritePngChunk(
          &out, "IHDR", reinterpret_cast<const uint8_t*>(&ihdr), 13)) {
    return std::unexpected(Error::IoError("Failed to write PNG IHDR"));
  }

  if (!WritePngChunk(&out,
                     "IDAT",
                     compressed.data(),
                     static_cast<uint32_t>(compressedSize))) {
    return std::unexpected(Error::IoError("Failed to write PNG IDAT"));
  }

  if (!WritePngChunk(&out, "IEND", nullptr, 0)) {
    return std::unexpected(Error::IoError("Failed to write PNG IEND"));
  }

  return {};
}

}  // namespace wz

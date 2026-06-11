#include "wz/PngUtility.h"
#include <cstring>

namespace wz {

RGBColor PngUtility::RGB565ToColor(uint16_t val) {
  constexpr int rgb565_mask_r = 0xf800;
  constexpr int rgb565_mask_g = 0x07e0;
  constexpr int rgb565_mask_b = 0x001f;
  int r = (val & rgb565_mask_r) >> 11;
  int g = (val & rgb565_mask_g) >> 5;
  int b = (val & rgb565_mask_b);
  return {static_cast<uint8_t>((r << 3) | (r >> 2)),
          static_cast<uint8_t>((g << 2) | (g >> 4)),
          static_cast<uint8_t>((b << 3) | (b >> 2))};
}

Result<void> PngUtility::DecompressImagePixelDataBgra4444(
    const std::vector<uint8_t>& rawData,
    int width,
    int height,
    std::vector<uint8_t>& output) {
  int uncompressedSize = width * height * 2;
  if (static_cast<int>(rawData.size()) < uncompressedSize)
    return Error::InvalidArgument(
        "Raw data length is insufficient for the specified dimensions.");

  output.resize(width * height * 4);

  // BGRA4444: each input byte has 2 nibbles (4-bit values)
  // byte 0: low nibble = B, high nibble = G (first pixel)
  // byte 1: low nibble = R, high nibble = A (first pixel)
  // Output format: B, G, R, A per pixel (BGRA32)
  for (int i = 0; i < uncompressedSize; i += 2) {
    int pixelIdx = i / 2;
    uint8_t b_lo = rawData[i] & 0x0F;
    uint8_t g_hi = (rawData[i] & 0xF0) >> 4;
    uint8_t r_lo = rawData[i + 1] & 0x0F;
    uint8_t a_hi = (rawData[i + 1] & 0xF0) >> 4;

    output[pixelIdx * 4 + 0] = static_cast<uint8_t>(b_lo | (b_lo << 4));
    output[pixelIdx * 4 + 1] = static_cast<uint8_t>(g_hi | (g_hi << 4));
    output[pixelIdx * 4 + 2] = static_cast<uint8_t>(r_lo | (r_lo << 4));
    output[pixelIdx * 4 + 3] = static_cast<uint8_t>(a_hi | (a_hi << 4));
  }
  return {};
}

void PngUtility::ExpandColorTable(RGBColor color[4], uint16_t c0, uint16_t c1) {
  color[0] = RGB565ToColor(c0);
  color[1] = RGB565ToColor(c1);
  if (c0 > c1) {
    color[2] = {static_cast<uint8_t>((static_cast<int>(color[0].R) * 2 +
                                      static_cast<int>(color[1].R) + 1) /
                                     3),
                static_cast<uint8_t>((static_cast<int>(color[0].G) * 2 +
                                      static_cast<int>(color[1].G) + 1) /
                                     3),
                static_cast<uint8_t>((static_cast<int>(color[0].B) * 2 +
                                      static_cast<int>(color[1].B) + 1) /
                                     3)};
    color[3] = {static_cast<uint8_t>((static_cast<int>(color[0].R) +
                                      static_cast<int>(color[1].R) * 2 + 1) /
                                     3),
                static_cast<uint8_t>((static_cast<int>(color[0].G) +
                                      static_cast<int>(color[1].G) * 2 + 1) /
                                     3),
                static_cast<uint8_t>((static_cast<int>(color[0].B) +
                                      static_cast<int>(color[1].B) * 2 + 1) /
                                     3)};
  } else {
    color[2] = {
        static_cast<uint8_t>(
            (static_cast<int>(color[0].R) + static_cast<int>(color[1].R)) / 2),
        static_cast<uint8_t>(
            (static_cast<int>(color[0].G) + static_cast<int>(color[1].G)) / 2),
        static_cast<uint8_t>(
            (static_cast<int>(color[0].B) + static_cast<int>(color[1].B)) / 2)};
    color[3] = {0, 0, 0};
  }
}

void PngUtility::ExpandColorIndexTable(int colorIndex[16],
                                       const uint8_t* rawData,
                                       int offset) {
  for (int i = 0; i < 16; i += 4, offset++) {
    colorIndex[i + 0] = (rawData[offset] & 0x03);
    colorIndex[i + 1] = (rawData[offset] & 0x0c) >> 2;
    colorIndex[i + 2] = (rawData[offset] & 0x30) >> 4;
    colorIndex[i + 3] = (rawData[offset] & 0xc0) >> 6;
  }
}

void PngUtility::ExpandAlphaTableDXT3(uint8_t alpha[16],
                                      const uint8_t* rawData,
                                      int offset) {
  for (int i = 0; i < 16; i += 2, offset++) {
    alpha[i + 0] = rawData[offset] & 0x0f;
    alpha[i + 1] = (rawData[offset] & 0xf0) >> 4;
  }
  for (int i = 0; i < 16; i++) {
    alpha[i] = static_cast<uint8_t>(alpha[i] | (alpha[i] << 4));
  }
}

void PngUtility::ExpandAlphaTableDXT5(uint8_t alpha[8],
                                      uint8_t a0,
                                      uint8_t a1) {
  alpha[0] = a0;
  alpha[1] = a1;
  if (a0 > a1) {
    for (int i = 2; i < 8; i++) {
      alpha[i] = static_cast<uint8_t>(((8 - i) * static_cast<int>(a0) +
                                       (i - 1) * static_cast<int>(a1) + 3) /
                                      7);
    }
  } else {
    for (int i = 2; i < 6; i++) {
      alpha[i] = static_cast<uint8_t>(((6 - i) * static_cast<int>(a0) +
                                       (i - 1) * static_cast<int>(a1) + 2) /
                                      5);
    }
    alpha[6] = 0;
    alpha[7] = 255;
  }
}

void PngUtility::ExpandAlphaIndexTableDXT5(int alphaIndex[16],
                                           const uint8_t* rawData,
                                           int offset) {
  for (int i = 0; i < 16; i += 8, offset += 3) {
    int flags = rawData[offset] | (rawData[offset + 1] << 8) |
                (rawData[offset + 2] << 16);
    for (int j = 0; j < 8; j++) {
      int mask = 0x07 << (3 * j);
      alphaIndex[i + j] = (flags & mask) >> (3 * j);
    }
  }
}

static void SetPixel(std::vector<uint8_t>& pixelData,
                     int x,
                     int y,
                     int width,
                     RGBColor color,
                     uint8_t alpha) {
  int offset = (y * width + x) * 4;
  pixelData[offset + 0] = color.B;
  pixelData[offset + 1] = color.G;
  pixelData[offset + 2] = color.R;
  pixelData[offset + 3] = alpha;
}

Result<void> PngUtility::DecompressImageDXT3(
    const std::vector<uint8_t>& rawData,
    int width,
    int height,
    std::vector<uint8_t>& output) {
  int blockCountX = (width + 3) / 4;
  int blockCountY = (height + 3) / 4;
  int expectedSize = blockCountX * blockCountY * 16;
  if (static_cast<int>(rawData.size()) < expectedSize)
    return Error::InvalidArgument(
        "Raw data length insufficient for DXT3 decompression.");

  output.resize(width * height * 4);

  RGBColor colorTable[4];
  int colorIdxTable[16];
  uint8_t alphaTable[16];

  for (int blockY = 0; blockY < blockCountY; blockY++) {
    for (int blockX = 0; blockX < blockCountX; blockX++) {
      int blockIndex = blockY * blockCountX + blockX;
      int off = blockIndex * 16;
      int x = blockX * 4;
      int y = blockY * 4;

      ExpandAlphaTableDXT3(alphaTable, rawData.data(), off);
      uint16_t u0 =
          static_cast<uint16_t>(rawData[off + 8] | (rawData[off + 9] << 8));
      uint16_t u1 =
          static_cast<uint16_t>(rawData[off + 10] | (rawData[off + 11] << 8));
      ExpandColorTable(colorTable, u0, u1);
      ExpandColorIndexTable(colorIdxTable, rawData.data(), off + 12);

      for (int j = 0; j < 4; j++) {
        int pixelY = y + j;
        if (pixelY >= height) continue;
        for (int i = 0; i < 4; i++) {
          int pixelX = x + i;
          if (pixelX >= width) continue;
          RGBColor c = colorTable[colorIdxTable[j * 4 + i]];
          uint8_t alpha = alphaTable[j * 4 + i];
          SetPixel(output, pixelX, pixelY, width, c, alpha);
        }
      }
    }
  }
  return {};
}

Result<void> PngUtility::DecompressImageDXT5(
    const std::vector<uint8_t>& rawData,
    int width,
    int height,
    std::vector<uint8_t>& output) {
  int blockCountX = (width + 3) / 4;
  int blockCountY = (height + 3) / 4;
  int expectedSize = blockCountX * blockCountY * 16;
  if (static_cast<int>(rawData.size()) < expectedSize)
    return Error::InvalidArgument(
        "Raw data length insufficient for DXT5 decompression.");

  if (width <= 0 || height <= 0)
    return Error::InvalidArgument("Width and height must be positive.");

  output.resize(width * height * 4);

  RGBColor colorTable[4];
  int colorIdxTable[16];
  uint8_t alphaTable[8];
  int alphaIdxTable[16];

  for (int blockY = 0; blockY < blockCountY; blockY++) {
    for (int blockX = 0; blockX < blockCountX; blockX++) {
      int blockIndex = blockY * blockCountX + blockX;
      int off = blockIndex * 16;
      int x = blockX * 4;
      int y = blockY * 4;

      ExpandAlphaTableDXT5(alphaTable, rawData[off], rawData[off + 1]);
      ExpandAlphaIndexTableDXT5(alphaIdxTable, rawData.data(), off + 2);
      uint16_t u0 =
          static_cast<uint16_t>(rawData[off + 8] | (rawData[off + 9] << 8));
      uint16_t u1 =
          static_cast<uint16_t>(rawData[off + 10] | (rawData[off + 11] << 8));
      ExpandColorTable(colorTable, u0, u1);
      ExpandColorIndexTable(colorIdxTable, rawData.data(), off + 12);

      for (int j = 0; j < 4; j++) {
        int pixelY = y + j;
        if (pixelY >= height) continue;
        for (int i = 0; i < 4; i++) {
          int pixelX = x + i;
          if (pixelX >= width) continue;
          RGBColor c = colorTable[colorIdxTable[j * 4 + i]];
          uint8_t alpha = alphaTable[alphaIdxTable[j * 4 + i]];
          SetPixel(output, pixelX, pixelY, width, c, alpha);
        }
      }
    }
  }
  return {};
}

void PngUtility::DecompressImagePixelDataForm517(
    const std::vector<uint8_t>& rawData,
    int width,
    int height,
    std::vector<uint8_t>& output) {
  int blockCountX = width / 16;
  int blockCountY = height / 16;
  output.resize(width * height * 2);

  int lineIndex = 0;
  for (int j0 = 0; j0 < blockCountY; j0++) {
    auto dstIndex = lineIndex;
    for (int i0 = 0; i0 < blockCountX; i0++) {
      int idx = (i0 + j0 * blockCountX) * 2;
      uint8_t b0 = rawData[idx];
      uint8_t b1 = rawData[idx + 1];
      for (int k = 0; k < 16; k++) {
        output[dstIndex++] = b0;
        output[dstIndex++] = b1;
      }
    }
    for (int k = 1; k < 16; k++) {
      std::memcpy(
          output.data() + dstIndex, output.data() + lineIndex, width * 2);
      dstIndex += width * 2;
    }
    lineIndex += width * 32;
  }
}

void PngUtility::CopyBmpDataWithStride(const std::vector<uint8_t>& source,
                                       int srcStride,
                                       int dstHeight,
                                       int dstStride,
                                       std::vector<uint8_t>& output) {
  output.resize(dstHeight * dstStride);
  if (dstStride == srcStride) {
    std::memcpy(output.data(), source.data(), source.size());
  } else {
    for (int y = 0; y < dstHeight; y++) {
      std::memcpy(output.data() + dstStride * y,
                  source.data() + srcStride * y,
                  std::min(srcStride, dstStride));
    }
  }
}

int PngUtility::GetUncompressedSizeByFormat(int width, int height, int format) {
  switch (format) {
    case 1:
      return width * height * 2;  // BGRA4444
    case 2:
      return width * height * 4;  // BGRA8888
    case 3:
      return width * height * 4;  // DXT3 grayscale
    case 257:
      return width * height * 2;  // ARGB1555
    case 513:
      return width * height * 2;  // RGB565
    case 517:
      return (width / 16) * (height / 16) * 2;  // Format517
    case 1026:
      return width * height * 4;  // DXT3 color
    case 2050:
      return width * height;  // DXT5 raw alpha only
    default:
      return width * height * 4;
  }
}

}  // namespace wz

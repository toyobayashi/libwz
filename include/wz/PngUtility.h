#ifndef WZ_PNGUTILITY_H_
#define WZ_PNGUTILITY_H_
#include <cmath>
#include <cstdint>
#include <vector>
#include "wz/Result.h"

namespace wz {

struct RGBColor {
  uint8_t R, G, B;
};

class PngUtility {
 public:
  static RGBColor RGB565ToColor(uint16_t val);

  // Decompress BGRA4444 raw data to BGRA32
  // rawData: input, 2 bytes per pixel
  // width, height: dimensions
  // output: pre-allocated, size = width * height * 4
  static Result<void> DecompressImagePixelDataBgra4444(
      const std::vector<uint8_t>& rawData,
      int width,
      int height,
      std::vector<uint8_t>& output);

  // Decompress DXT3 compressed data to BGRA32
  static Result<void> DecompressImageDXT3(const std::vector<uint8_t>& rawData,
                                          int width,
                                          int height,
                                          std::vector<uint8_t>& output);

  // Decompress DXT5 compressed data to BGRA32
  static Result<void> DecompressImageDXT5(const std::vector<uint8_t>& rawData,
                                          int width,
                                          int height,
                                          std::vector<uint8_t>& output);

  // Decompress Format517 (16x16 block RGB565) to RGB565 data
  static void DecompressImagePixelDataForm517(
      const std::vector<uint8_t>& rawData,
      int width,
      int height,
      std::vector<uint8_t>& output);

  // Copy bitmap data with stride handling (for Format257/Format2 round-trip)
  static void CopyBmpDataWithStride(const std::vector<uint8_t>& source,
                                    int srcStride,
                                    int dstHeight,
                                    int dstStride,
                                    std::vector<uint8_t>& output);

  // DXT color table expansion
  static void ExpandColorTable(RGBColor color[4], uint16_t c0, uint16_t c1);
  static void ExpandColorIndexTable(int colorIndex[16],
                                    const uint8_t* rawData,
                                    int offset);
  static void ExpandAlphaTableDXT3(uint8_t alpha[16],
                                   const uint8_t* rawData,
                                   int offset);
  static void ExpandAlphaTableDXT5(uint8_t alpha[8], uint8_t a0, uint8_t a1);
  static void ExpandAlphaIndexTableDXT5(int alphaIndex[16],
                                        const uint8_t* rawData,
                                        int offset);

  // Get uncompressed pixel data size by format
  static int GetUncompressedSizeByFormat(int width, int height, int format);
};

}  // namespace wz
#endif  // WZ_PNGUTILITY_H_

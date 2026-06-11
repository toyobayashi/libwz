#ifndef WZ_PROPERTIES_WZBINARYPROPERTY_H_
#define WZ_PROPERTIES_WZBINARYPROPERTY_H_
#include <cstdint>
#include <vector>
#include "wz/Result.h"
#include "wz/WzEnums.h"
#include "wz/WzImageProperty.h"

namespace wz {

class WzBinaryReader;

#pragma pack(push, 1)
struct WzWaveFormat {
  uint16_t wFormatTag = 0;
  uint16_t nChannels = 0;
  uint32_t nSamplesPerSec = 0;
  uint32_t nAvgBytesPerSec = 0;
  uint16_t nBlockAlign = 0;
  uint16_t wBitsPerSample = 0;
  uint16_t cbSize = 0;
};

struct WzMp3WaveFormat {
  WzWaveFormat wfx;
  uint16_t wID = 0;
  uint32_t fdwFlags = 0;
  uint16_t nBlockSize = 0;
  uint16_t nFramesPerBlock = 0;
  uint16_t nCodecDelay = 0;
};
#pragma pack(pop)

class WzBinaryProperty : public WzImageProperty {
 public:
  WzBinaryProperty(const std::string& name,
                   WzBinaryReader* reader,
                   bool parseNow);
  WZ_DISALLOW_COPY_AND_MOVE(WzBinaryProperty)
  WzPropertyType PropertyType() const override { return WzPropertyType::Sound; }

  const std::vector<uint8_t>& Header() const { return header_; }
  int Length() const { return lenMs_; }
  int Frequency() const { return wavFormat_.nSamplesPerSec; }
  WzBinaryPropertyType Type() const { return type_; }
  bool HeaderEncrypted() const { return headerEncrypted_; }
  const WzWaveFormat& WavFormat() const { return wavFormat_; }

  Result<std::vector<uint8_t>> GetBytes() override;
  Result<std::vector<uint8_t>> GetBytes(bool saveInMemory);
  Result<std::vector<uint8_t>> GetBytesForWAVPlayback(bool saveInMemory);
  bool SaveToFile(const std::string& filePath);

 private:
  void ParseWzSoundPropertyHeader();

  std::vector<uint8_t> fileBytes_;
  std::vector<uint8_t> header_;
  WzBinaryReader* wzReader_ = nullptr;
  WzWaveFormat wavFormat_;
  int lenMs_ = 0;
  int soundDataLen_ = 0;
  int64_t offs_ = 0;
  WzBinaryPropertyType type_ = WzBinaryPropertyType::Raw;
  bool headerEncrypted_ = false;
};

}  // namespace wz
#endif  // WZ_PROPERTIES_WZBINARYPROPERTY_H_

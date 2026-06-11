#ifndef WZ_WZHEADER_H_
#define WZ_WZHEADER_H_
#include <cstdint>
#include <string>

namespace wz {

class WzHeader {
 public:
  static constexpr const char* DEFAULT_WZ_HEADER_COPYRIGHT =
      "Package file v1.0 Copyright 2002 Wizet, ZMS";

  std::string Ident() const { return ident_; }
  void SetIdent(const std::string& v) { ident_ = v; }

  std::string Copyright() const { return copyright_; }
  void SetCopyright(const std::string& v) { copyright_ = v; }

  uint64_t FSize() const { return fsize_; }
  void SetFSize(uint64_t v) { fsize_ = v; }

  uint32_t FStart() const { return fstart_; }
  void SetFStart(uint32_t v) { fstart_ = v; }

  void RecalculateFileStart() {
    fstart_ = static_cast<uint32_t>(ident_.length() + sizeof(uint64_t) +
                                    sizeof(uint32_t) + copyright_.length() + 1);
  }

  static WzHeader GetDefault() {
    WzHeader h;
    h.ident_ = "PKG1";
    h.copyright_ = DEFAULT_WZ_HEADER_COPYRIGHT;
    h.fstart_ = 60;
    h.fsize_ = 0;
    return h;
  }

 private:
  std::string ident_ = "PKG1";
  std::string copyright_ = DEFAULT_WZ_HEADER_COPYRIGHT;
  uint64_t fsize_ = 0;
  uint32_t fstart_ = 60;
};

}  // namespace wz
#endif  // WZ_WZHEADER_H_

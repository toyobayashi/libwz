#ifndef WZ_RESULT_H_
#define WZ_RESULT_H_
#include <expected>
#include <string>
#include <utility>

namespace wz {

enum class ErrorCode {
  Ok = 0,
  NotImplemented,
  InvalidArgument,
  ParseError,
  IoError,
  DataError,
  NotFound,
};

class Error {
 public:
  // NOLINTNEXTLINE(runtime/explicit)
  Error(ErrorCode code, std::string message = {})
      : code_(code), message_(std::move(message)) {}

  ErrorCode code() const { return code_; }
  const std::string& message() const { return message_; }

  static Error NotImplemented(const std::string& msg = "Not implemented") {
    return {ErrorCode::NotImplemented, msg};
  }
  static Error InvalidArgument(const std::string& msg) {
    return {ErrorCode::InvalidArgument, msg};
  }
  static Error ParseError(const std::string& msg) {
    return {ErrorCode::ParseError, msg};
  }
  static Error IoError(const std::string& msg) {
    return {ErrorCode::IoError, msg};
  }
  static Error DataError(const std::string& msg) {
    return {ErrorCode::DataError, msg};
  }
  static Error NotFound(const std::string& msg) {
    return {ErrorCode::NotFound, msg};
  }

  explicit operator bool() const { return code_ == ErrorCode::Ok; }

 private:
  ErrorCode code_;
  std::string message_;
};

template <typename T, typename E = Error>
using Result = std::expected<T, E>;

}  // namespace wz
#endif  // WZ_RESULT_H_

#ifndef WZ_RESULT_H_
#define WZ_RESULT_H_
#include <cstdlib>
#include <string>
#include <utility>
#include <variant>

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

  static Error ok() { return {ErrorCode::Ok}; }
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
  bool is_ok() const { return code_ == ErrorCode::Ok; }
  bool is_err() const { return code_ != ErrorCode::Ok; }

 private:
  ErrorCode code_;
  std::string message_;
};

template <typename T, typename E = Error>
class Result {
 public:
  // NOLINTNEXTLINE(runtime/explicit)
  Result(T value) : data_(std::move(value)) {}
  // NOLINTNEXTLINE(runtime/explicit)
  Result(E error) : data_(std::move(error)) {}
  // NOLINTNEXTLINE(runtime/explicit)
  Result(ErrorCode code, std::string msg = {})
      : data_(E(code, std::move(msg))) {}

  bool is_ok() const { return std::holds_alternative<T>(data_); }
  bool is_err() const { return std::holds_alternative<E>(data_); }
  explicit operator bool() const { return is_ok(); }

  T& ok() & { return std::get<T>(data_); }
  const T& ok() const& { return std::get<T>(data_); }
  T&& ok() && { return std::get<T>(std::move(data_)); }

  E& err() & { return std::get<E>(data_); }
  const E& err() const& { return std::get<E>(data_); }
  E&& err() && { return std::get<E>(std::move(data_)); }

  T unwrap() && {
    if (is_err()) {
      // In no-exceptions mode we assert/abort
      std::abort();
    }
    return std::get<T>(std::move(data_));
  }

  T unwrap_or(T default_value) && {
    if (is_err()) return default_value;
    return std::get<T>(std::move(data_));
  }

  T unwrap_or(T default_value) const& {
    if (is_err()) return default_value;
    return std::get<T>(data_);
  }

  template <typename F>
  Result map(F&& f) && {
    if (is_err()) return std::get<E>(std::move(data_));
    return Result(f(std::get<T>(std::move(data_))));
  }

  template <typename F>
  Result map(F&& f) const& {
    if (is_err()) return std::get<E>(data_);
    return Result(f(std::get<T>(data_)));
  }

 private:
  std::variant<T, E> data_;
};

template <typename E>
class Result<void, E> {
 public:
  Result() : data_(E::ok()) {}
  Result(E error) : data_(std::move(error)) {}  // NOLINT(runtime/explicit)

  bool is_ok() const { return data_.is_ok(); }
  bool is_err() const { return data_.is_err(); }
  explicit operator bool() const { return is_ok(); }

  E& err() { return data_; }
  const E& err() const { return data_; }

 private:
  E data_;
};

}  // namespace wz
#endif  // WZ_RESULT_H_

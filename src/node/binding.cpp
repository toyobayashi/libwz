#include <node_api.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emnapi.h>
#endif

#include "wz/Properties/WzBinaryProperty.h"
#include "wz/Properties/WzCanvasProperty.h"
#include "wz/Properties/WzConvexProperty.h"
#include "wz/Properties/WzDoubleProperty.h"
#include "wz/Properties/WzFloatProperty.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzLongProperty.h"
#include "wz/Properties/WzLuaProperty.h"
#include "wz/Properties/WzNullProperty.h"
#include "wz/Properties/WzPngProperty.h"
#include "wz/Properties/WzRawDataProperty.h"
#include "wz/Properties/WzShortProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Properties/WzUOLProperty.h"
#include "wz/Properties/WzVectorProperty.h"
#include "wz/Properties/WzVideoProperty.h"
#include "wz/Result.h"
#include "wz/Util/WzBlobDataSource.h"
#include "wz/Util/WzDataSource.h"
#include "wz/Util/WzTool.h"
#include "wz/WzDirectory.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"
#include "wz/WzObject.h"
#include "wz/WzPropertyCollection.h"

void ThrowNodeApiError(napi_env env, const char* message) {
  const napi_extended_error_info* error = nullptr;
  (void)napi_get_last_error_info(env, &error);
  const char* detail =
      error && error->error_message ? error->error_message : message;
  (void)napi_throw_error(env, nullptr, detail);
}

#define NODE_API_CALL(env, expr, message)                                      \
  do {                                                                         \
    napi_status node_api_status = (expr);                                      \
    if (node_api_status != napi_ok) {                                          \
      ThrowNodeApiError((env), (message));                                     \
      return nullptr;                                                          \
    }                                                                          \
  } while (false)

#define NODE_API_THROW(env, message)                                           \
  do {                                                                         \
    (void)napi_throw_error((env), nullptr, (message));                         \
    return nullptr;                                                            \
  } while (false)

template <typename T>
inline bool ReadHandle(napi_env env, napi_value value, T* out) {
  bool lossless = false;
  uint64_t raw = 0;
  napi_status status =
      napi_get_value_bigint_uint64(env, value, &raw, &lossless);
  if (status != napi_ok) {
    (void)napi_throw_error(env, nullptr, "expected native handle bigint");
    return false;
  }
  if (!lossless) {
    (void)napi_throw_range_error(
        env, nullptr, "native handle bigint is outside uint64 range");
    return false;
  }
  *out = reinterpret_cast<T>(static_cast<uintptr_t>(raw));
  return true;
}

template <typename T>
inline napi_value ToHandle(napi_env env, T ptr) {
  napi_value value = nullptr;
  NODE_API_CALL(
      env,
      napi_create_bigint_uint64(
          env, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr)), &value),
      "failed to create native handle");
  return value;
}

template <typename T>
inline bool CheckResult(napi_env env, const wz::Result<T>& result) {
  if (result.has_value()) return true;
  (void)napi_throw_error(env, nullptr, result.error().message().c_str());
  return false;
}

inline bool CheckResult(napi_env env, const wz::Result<void>& result) {
  if (result.has_value()) return true;
  (void)napi_throw_error(env, nullptr, result.error().message().c_str());
  return false;
}

inline bool CheckNotNull(napi_env env, const void* ptr, const char* name) {
  if (ptr) return true;
  const std::string message = std::string(name) + ": null handle";
  (void)napi_throw_error(env, nullptr, message.c_str());
  return false;
}

inline bool CheckPropertyType(napi_env env,
                              wz::WzImageProperty* prop,
                              wz::WzPropertyType type,
                              const char* name) {
  if (!CheckNotNull(env, prop, name)) return false;
  if (prop->PropertyType() == type) return true;
  const std::string message = std::string(name) + ": wrong property type";
  (void)napi_throw_error(env, nullptr, message.c_str());
  return false;
}

inline bool ReadString(napi_env env, napi_value value, std::string* out) {
  size_t length = 0;
  napi_status status =
      napi_get_value_string_utf8(env, value, nullptr, 0, &length);
  if (status != napi_ok) {
    (void)napi_throw_error(env, nullptr, "expected string");
    return false;
  }
  std::string result(length + 1, '\0');
  size_t copied = 0;
  status = napi_get_value_string_utf8(
      env, value, result.data(), length + 1, &copied);
  if (status != napi_ok) {
    (void)napi_throw_error(env, nullptr, "failed to read string");
    return false;
  }
  result.resize(copied);
  *out = std::move(result);
  return true;
}

inline bool ReadInt(napi_env env, napi_value value, int32_t* out) {
  napi_status status = napi_get_value_int32(env, value, out);
  if (status == napi_ok) return true;
  (void)napi_throw_error(env, nullptr, "expected int32");
  return false;
}

inline bool ReadDouble(napi_env env, napi_value value, double* out) {
  napi_status status = napi_get_value_double(env, value, out);
  if (status == napi_ok) return true;
  (void)napi_throw_error(env, nullptr, "expected number");
  return false;
}

inline bool ReadBool(napi_env env, napi_value value, bool* out) {
  napi_status status = napi_get_value_bool(env, value, out);
  if (status == napi_ok) return true;
  (void)napi_throw_error(env, nullptr, "expected boolean");
  return false;
}

inline bool ReadInt64(napi_env env, napi_value value, int64_t* out) {
  bool lossless = false;
  napi_status status = napi_get_value_bigint_int64(env, value, out, &lossless);
  if (status != napi_ok) {
    (void)napi_throw_error(env, nullptr, "expected int64 bigint");
    return false;
  }
  if (!lossless) {
    (void)napi_throw_range_error(
        env, nullptr, "bigint value is outside int64 range");
    return false;
  }
  return true;
}

inline bool ReadBytesView(napi_env env,
                          napi_value value,
                          uint8_t** data,
                          size_t* length) {
  napi_typedarray_type type;
  size_t byte_offset = 0;
  napi_value array_buffer = nullptr;
  napi_status status = napi_get_typedarray_info(env,
                                                value,
                                                &type,
                                                length,
                                                reinterpret_cast<void**>(data),
                                                &array_buffer,
                                                &byte_offset);
  if (status != napi_ok) {
    (void)napi_throw_error(env, nullptr, "expected Uint8Array");
    return false;
  }
  if (type == napi_uint8_array) return true;
  (void)napi_throw_error(env, nullptr, "expected Uint8Array");
  return false;
}

inline bool ReadIv(napi_env env,
                   napi_value value,
                   std::array<uint8_t, 4>* out) {
  uint8_t* data = nullptr;
  size_t length = 0;
  *out = {0, 0, 0, 0};
  if (!ReadBytesView(env, value, &data, &length)) return false;
  if (data) {
    std::memcpy(out->data(), data, length < out->size() ? length : out->size());
  }
  return true;
}

#ifdef __EMSCRIPTEN__
inline bool SyncMemory(napi_env env,
                       bool js_to_wasm,
                       napi_value value,
                       size_t byte_offset,
                       size_t length) {
  napi_value raw = value;
  napi_status status =
      emnapi_sync_memory(env, js_to_wasm, &raw, byte_offset, length);
  if (status == napi_ok) return true;
  (void)napi_throw_error(env, nullptr, "emnapi_sync_memory failed");
  return false;
}
#else
inline bool SyncMemory(napi_env, bool, napi_value, size_t, size_t) {
  return true;
}
#endif

inline napi_value Bytes(napi_env env, const std::vector<uint8_t>& data) {
  napi_value array_buffer = nullptr;
  void* raw = nullptr;
  NODE_API_CALL(env,
                napi_create_arraybuffer(env, data.size(), &raw, &array_buffer),
                "failed to create ArrayBuffer");
  if (!data.empty()) {
    std::memcpy(raw, data.data(), data.size());
  }
  if (!SyncMemory(env, false, array_buffer, 0, data.size())) return nullptr;
  napi_value result = nullptr;
  NODE_API_CALL(
      env,
      napi_create_typedarray(
          env, napi_uint8_array, data.size(), array_buffer, 0, &result),
      "failed to create Uint8Array");
  return result;
}

inline napi_value NullableHandle(napi_env env, void* ptr) {
  if (ptr) return ToHandle(env, ptr);
  napi_value result = nullptr;
  NODE_API_CALL(env, napi_get_null(env, &result), "failed to get null");
  return result;
}

inline napi_value NullableObject(napi_env env, wz::WzObject* obj) {
  if (!obj) {
    napi_value null_value = nullptr;
    NODE_API_CALL(env, napi_get_null(env, &null_value), "failed to get null");
    return null_value;
  }
  napi_value result = nullptr;
  NODE_API_CALL(
      env, napi_create_object(env, &result), "failed to create object");
  napi_value type = nullptr;
  NODE_API_CALL(
      env,
      napi_create_int32(env, static_cast<int>(obj->ObjectType()), &type),
      "failed to create object type value");
  NODE_API_CALL(env,
                napi_set_named_property(env, result, "type", type),
                "failed to set object type");
  napi_value ptr = ToHandle(env, obj);
  if (ptr == nullptr) return nullptr;
  NODE_API_CALL(env,
                napi_set_named_property(env, result, "ptr", ptr),
                "failed to set object pointer");
  return result;
}

bool IsMutablePropertyContainer(wz::WzImageProperty* prop) {
  if (!prop) return false;
  switch (prop->PropertyType()) {
    case wz::WzPropertyType::SubProperty:
    case wz::WzPropertyType::Canvas:
    case wz::WzPropertyType::Convex:
    case wz::WzPropertyType::Raw:
      return prop->WzProperties() != nullptr;
    default:
      return false;
  }
}

struct BlobReadCallbackState {
  napi_env env;
  napi_ref callback;
};

struct BlobReadCallbackStateDeleter {
  void operator()(BlobReadCallbackState* ptr) const {
    if (!ptr) return;
    napi_status status = napi_delete_reference(ptr->env, ptr->callback);
    if (status != napi_ok) {
      (void)napi_throw_error(
          ptr->env, nullptr, "failed to release Blob read callback");
    }
    delete ptr;
  }
};

wz::Result<size_t> BlobReadRange(BlobReadCallbackState* state,
                                 uint64_t offset,
                                 std::span<uint8_t> destination) {
  napi_value callback = nullptr;
  napi_value global = nullptr;
  napi_status status =
      napi_get_reference_value(state->env, state->callback, &callback);
  if (status != napi_ok) {
    (void)napi_throw_error(state->env, nullptr, "Blob read callback failed");
    return std::unexpected(wz::Error::IoError("Blob read callback failed"));
  }
  status = napi_get_global(state->env, &global);
  if (status != napi_ok) {
    (void)napi_throw_error(state->env, nullptr, "Blob read callback failed");
    return std::unexpected(wz::Error::IoError("Blob read callback failed"));
  }

  napi_value argv[2] = {};
  status =
      napi_create_double(state->env, static_cast<double>(offset), &argv[0]);
  if (status != napi_ok) {
    (void)napi_throw_error(state->env, nullptr, "Blob read callback failed");
    return std::unexpected(wz::Error::IoError("Blob read callback failed"));
  }
  status = napi_create_double(
      state->env, static_cast<double>(destination.size()), &argv[1]);
  if (status != napi_ok) {
    (void)napi_throw_error(state->env, nullptr, "Blob read callback failed");
    return std::unexpected(wz::Error::IoError("Blob read callback failed"));
  }

  napi_value result = nullptr;
  status = napi_call_function(state->env, global, callback, 2, argv, &result);
  if (status != napi_ok) {
    (void)napi_throw_error(state->env, nullptr, "Blob read callback failed");
    return std::unexpected(wz::Error::IoError("Blob read callback failed"));
  }

  uint8_t* data = nullptr;
  size_t length = 0;
  if (!ReadBytesView(state->env, result, &data, &length) ||
      length != destination.size()) {
    return std::unexpected(
        wz::Error::IoError("Blob read callback returned invalid data"));
  }
  SyncMemory(state->env, true, result, 0, length);
  if (length > 0) std::memcpy(destination.data(), data, length);
  return length;
}

class BlobReadCallback {
 public:
  explicit BlobReadCallback(std::shared_ptr<BlobReadCallbackState> state)
      : state_(std::move(state)) {}

  wz::Result<size_t> operator()(uint64_t offset,
                                std::span<uint8_t> destination) const {
    return BlobReadRange(state_.get(), offset, destination);
  }

 private:
  std::shared_ptr<BlobReadCallbackState> state_;
};

std::shared_ptr<wz::WzBlobDataSource> MakeBlobSource(napi_env env,
                                                     uint64_t size,
                                                     napi_value callback) {
  auto* state = new BlobReadCallbackState{env, nullptr};
  napi_status status =
      napi_create_reference(env, callback, 1, &state->callback);
  if (status != napi_ok) {
    (void)napi_throw_error(env, nullptr, "failed to retain Blob read callback");
    delete state;
    return nullptr;
  }
  auto owner = std::shared_ptr<BlobReadCallbackState>(
      state, BlobReadCallbackStateDeleter());
  return std::make_shared<wz::WzBlobDataSource>(size, BlobReadCallback(owner));
}

#define FN(name) napi_value name(napi_env env, napi_callback_info info)
#define GET_ARGS(n)                                                            \
  napi_value args[n] = {};                                                     \
  size_t argc = n;                                                             \
  NODE_API_CALL(env,                                                           \
                napi_get_cb_info(env, info, &argc, args, nullptr, nullptr),    \
                "failed to read callback arguments")
#define READ_HANDLE(type, name, value)                                         \
  type name = nullptr;                                                         \
  if (!ReadHandle<type>(env, value, &name)) return nullptr
#define READ_STRING(name, value)                                               \
  std::string name;                                                            \
  if (!ReadString(env, value, &name)) return nullptr
#define READ_INT(name, value)                                                  \
  int32_t name = 0;                                                            \
  if (!ReadInt(env, value, &name)) return nullptr
#define READ_DOUBLE(name, value)                                               \
  double name = 0;                                                             \
  if (!ReadDouble(env, value, &name)) return nullptr
#define READ_BOOL(name, value)                                                 \
  bool name = false;                                                           \
  if (!ReadBool(env, value, &name)) return nullptr
#define READ_IV(name, value)                                                   \
  std::array<uint8_t, 4> name = {0, 0, 0, 0};                                  \
  if (!ReadIv(env, value, &name)) return nullptr
#define RETURN_NULL()                                                          \
  do {                                                                         \
    napi_value node_api_result = nullptr;                                      \
    NODE_API_CALL(                                                             \
        env, napi_get_null(env, &node_api_result), "failed to get null");      \
    return node_api_result;                                                    \
  } while (false)
#define RETURN_UNDEFINED()                                                     \
  do {                                                                         \
    napi_value node_api_result = nullptr;                                      \
    NODE_API_CALL(env,                                                         \
                  napi_get_undefined(env, &node_api_result),                   \
                  "failed to get undefined");                                  \
    return node_api_result;                                                    \
  } while (false)
#define RETURN_BOOL(value)                                                     \
  do {                                                                         \
    napi_value node_api_result = nullptr;                                      \
    NODE_API_CALL(env,                                                         \
                  napi_get_boolean(env, (value), &node_api_result),            \
                  "failed to create boolean");                                 \
    return node_api_result;                                                    \
  } while (false)
#define RETURN_INT(value)                                                      \
  do {                                                                         \
    napi_value node_api_result = nullptr;                                      \
    NODE_API_CALL(env,                                                         \
                  napi_create_int32(env, (value), &node_api_result),           \
                  "failed to create int32");                                   \
    return node_api_result;                                                    \
  } while (false)
#define RETURN_UINT32(value)                                                   \
  do {                                                                         \
    napi_value node_api_result = nullptr;                                      \
    NODE_API_CALL(env,                                                         \
                  napi_create_uint32(env, (value), &node_api_result),          \
                  "failed to create uint32");                                  \
    return node_api_result;                                                    \
  } while (false)
#define RETURN_DOUBLE(value)                                                   \
  do {                                                                         \
    napi_value node_api_result = nullptr;                                      \
    NODE_API_CALL(env,                                                         \
                  napi_create_double(env, (value), &node_api_result),          \
                  "failed to create number");                                  \
    return node_api_result;                                                    \
  } while (false)
#define RETURN_BIGINT(value)                                                   \
  do {                                                                         \
    napi_value node_api_result = nullptr;                                      \
    NODE_API_CALL(env,                                                         \
                  napi_create_bigint_int64(env, (value), &node_api_result),    \
                  "failed to create bigint");                                  \
    return node_api_result;                                                    \
  } while (false)
#define RETURN_STRING(value)                                                   \
  do {                                                                         \
    const std::string& node_api_string = (value);                              \
    napi_value node_api_result = nullptr;                                      \
    NODE_API_CALL(env,                                                         \
                  napi_create_string_utf8(env,                                 \
                                          node_api_string.c_str(),             \
                                          node_api_string.size(),              \
                                          &node_api_result),                   \
                  "failed to create string");                                  \
    return node_api_result;                                                    \
  } while (false)
#define EXPORT(name, fn)                                                       \
  do {                                                                         \
    napi_value node_api_export = nullptr;                                      \
    NODE_API_CALL(                                                             \
        env,                                                                   \
        napi_create_function(                                                  \
            env, name, NAPI_AUTO_LENGTH, fn, nullptr, &node_api_export),       \
        "failed to create export function");                                   \
    NODE_API_CALL(                                                             \
        env,                                                                   \
        napi_set_named_property(env, exports, name, node_api_export),          \
        "failed to set export function");                                      \
  } while (false)

FN(OpenFile) {
  GET_ARGS(3);
  READ_STRING(path, args[0]);
  READ_INT(game_version, args[1]);
  READ_INT(version, args[2]);
  return ToHandle(env,
                  new wz::WzFile(path,
                                 static_cast<short>(game_version),
                                 static_cast<wz::WzMapleVersion>(version)));
}

FN(OpenFileWithIv) {
  GET_ARGS(2);
  READ_STRING(path, args[0]);
  READ_IV(iv, args[1]);
  return ToHandle(env, new wz::WzFile(path, iv));
}

FN(OpenMemory) {
  GET_ARGS(4);
  READ_STRING(name, args[0]);
  uint8_t* data = nullptr;
  size_t size = 0;
  if (!ReadBytesView(env, args[1], &data, &size)) return nullptr;
  std::vector<uint8_t> bytes(data, data + size);
  auto source = std::make_shared<wz::WzMemoryDataSource>(std::move(bytes));
  READ_INT(game_version, args[2]);
  READ_INT(version, args[3]);
  return ToHandle(env,
                  new wz::WzFile(name,
                                 source,
                                 static_cast<short>(game_version),
                                 static_cast<wz::WzMapleVersion>(version)));
}

FN(OpenMemoryWithIv) {
  GET_ARGS(3);
  READ_STRING(name, args[0]);
  uint8_t* data = nullptr;
  size_t size = 0;
  if (!ReadBytesView(env, args[1], &data, &size)) return nullptr;
  std::vector<uint8_t> bytes(data, data + size);
  auto source = std::make_shared<wz::WzMemoryDataSource>(std::move(bytes));
  READ_IV(iv, args[2]);
  return ToHandle(env, new wz::WzFile(name, source, iv));
}

FN(OpenBlobSource) {
  GET_ARGS(5);
  READ_DOUBLE(size, args[0]);
  READ_STRING(name, args[1]);
  READ_INT(game_version, args[2]);
  READ_INT(version, args[3]);
  auto source = MakeBlobSource(env, size, args[4]);
  if (!source) return nullptr;
  return ToHandle(env,
                  new wz::WzFile(name,
                                 source,
                                 static_cast<short>(game_version),
                                 static_cast<wz::WzMapleVersion>(version)));
}

FN(OpenBlobSourceWithIv) {
  GET_ARGS(4);
  READ_DOUBLE(size, args[0]);
  READ_STRING(name, args[1]);
  READ_IV(iv, args[2]);
  auto source = MakeBlobSource(env, size, args[3]);
  if (!source) return nullptr;
  return ToHandle(env, new wz::WzFile(name, source, iv));
}

FN(CreateFile) {
  GET_ARGS(2);
  READ_INT(game_version, args[0]);
  READ_INT(version, args[1]);
  return ToHandle(env,
                  new wz::WzFile(static_cast<short>(game_version),
                                 static_cast<wz::WzMapleVersion>(version)));
}

FN(CloseFile) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  delete file;
  RETURN_UNDEFINED();
}

FN(ParseFile) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  if (!CheckNotNull(env, file, "parseFile")) return nullptr;
  auto status = file->ParseWzFile();
  if (status != wz::WzFileParseStatus::Success) {
    const auto message = wz::GetErrorDescription(status);
    NODE_API_THROW(env, message.c_str());
  }
  RETURN_INT(static_cast<int>(status));
}

FN(FileSaveToDisk) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  READ_STRING(path, args[1]);
  auto result = file->SaveToDisk(path);
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(FileName) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  RETURN_STRING(file->Name());
}

FN(FilePath) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  RETURN_STRING(file->FilePath());
}

FN(FileVersion) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  RETURN_INT(file->Version());
}

FN(FileMapleVersion) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  RETURN_INT(static_cast<int>(file->MapleVersion()));
}

FN(FileWzDirectory) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  return NullableHandle(env, file->GetWzDirectory());
}

FN(FileIs64Bit) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  RETURN_BOOL(file->Is64BitWzFile());
}

FN(FileIsUnloaded) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  RETURN_BOOL(file->IsUnloaded());
}

FN(FileVersionHash) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  RETURN_UINT32(file->VersionHash());
}

FN(FileObjectFromPath) {
  GET_ARGS(3);
  READ_HANDLE(wz::WzFile*, file, args[0]);
  READ_STRING(path, args[1]);
  READ_BOOL(check_first_directory_name, args[2]);
  auto* obj = file->GetObjectFromPath(path, check_first_directory_name);
  return NullableObject(env, obj);
}

FN(DirName) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  RETURN_STRING(dir->Name());
}

FN(DirCountImages) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  RETURN_INT(static_cast<int>(dir->WzImages().size()));
}

FN(DirCountDirectories) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  RETURN_INT(static_cast<int>(dir->WzDirectories().size()));
}

FN(DirCountImagesTotal) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  RETURN_INT(dir->CountImages());
}

FN(DirGetImage) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  READ_INT(index, args[1]);
  auto images = dir->WzImages();
  if (index < 0 || index >= static_cast<int>(images.size())) RETURN_NULL();
  return NullableHandle(env, images[static_cast<size_t>(index)]);
}

FN(DirGetImageByName) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  READ_STRING(name, args[1]);
  return NullableHandle(env, dir->GetImageByName(name));
}

FN(DirGetDirectory) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  READ_INT(index, args[1]);
  auto dirs = dir->WzDirectories();
  if (index < 0 || index >= static_cast<int>(dirs.size())) RETURN_NULL();
  return NullableHandle(env, dirs[static_cast<size_t>(index)]);
}

FN(DirGetDirectoryByName) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  READ_STRING(name, args[1]);
  return NullableHandle(env, dir->GetDirectoryByName(name));
}

FN(DirCreateDirectory) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  READ_STRING(name, args[1]);
  auto result = dir->CreateDirectory(name);
  if (!CheckResult(env, result)) return nullptr;
  return ToHandle(env, result.value());
}

FN(DirCreateImage) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  READ_STRING(name, args[1]);
  auto result = dir->CreateImage(name);
  if (!CheckResult(env, result)) return nullptr;
  return ToHandle(env, result.value());
}

FN(DirRemoveDirectory) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  READ_HANDLE(wz::WzDirectory*, child, args[1]);
  auto result = dir->TryRemoveDirectory(child);
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(DirRemoveImage) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  READ_HANDLE(wz::WzImage*, image, args[1]);
  auto result = dir->TryRemoveImage(image);
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(DirBlockSize) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  RETURN_INT(dir->BlockSize());
}

FN(DirChecksum) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  RETURN_INT(dir->Checksum());
}

FN(DirOffset) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzDirectory*, dir, args[0]);
  RETURN_BIGINT(dir->Offset());
}

FN(ImageName) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  RETURN_STRING(image->Name());
}

FN(ImageParsed) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  RETURN_BOOL(image->Parsed());
}

FN(ImageChanged) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  RETURN_BOOL(image->Changed());
}

FN(ImageBlockSize) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  RETURN_INT(image->BlockSize());
}

FN(ImageChecksum) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  RETURN_INT(image->Checksum());
}

FN(ImageOffset) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  RETURN_BIGINT(image->Offset());
}

FN(ImageIsLua) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  RETURN_BOOL(image->IsLuaWzImage());
}

FN(ImageParse) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  auto result = image->ParseImage();
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(ImageCountProperties) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  RETURN_INT(static_cast<int>(image->WzProperties()->size()));
}

FN(ImageGetProperty) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  READ_INT(index, args[1]);
  auto* props = image->WzProperties();
  if (index < 0 || index >= static_cast<int>(props->size())) RETURN_NULL();
  return ToHandle(env, (*props)[static_cast<size_t>(index)]);
}

FN(ImageGetFromPath) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  READ_STRING(path, args[1]);
  return NullableHandle(env, image->GetFromPath(path));
}

FN(ImageAddProperty) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  READ_HANDLE(wz::WzImageProperty*, prop, args[1]);
  auto result = image->TryAddProperty(prop);
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(ImageRemoveProperty) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  READ_HANDLE(wz::WzImageProperty*, prop, args[1]);
  auto result = image->TryRemoveProperty(prop);
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(ImageClearProperties) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImage*, image, args[0]);
  image->ClearProperties();
  RETURN_UNDEFINED();
}

FN(ObjectType) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  RETURN_INT(static_cast<int>(obj->ObjectType()));
}

FN(ObjectName) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  RETURN_STRING(obj->Name());
}

FN(ObjectParent) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  return NullableObject(env, obj->Parent());
}

FN(ObjectFullPath) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  RETURN_STRING(obj->FullPath());
}

FN(ObjectWzFileParent) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  return NullableHandle(env, obj->WzFileParent());
}

FN(ObjectTopMostDirectory) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  return NullableObject(env, obj->GetTopMostWzDirectory());
}

FN(ObjectTopMostImage) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  return NullableObject(env, obj->GetTopMostWzImage());
}

FN(ObjectAt) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  READ_STRING(name, args[1]);
  if (obj->ObjectType() == wz::WzObjectType::Image) {
    auto result = static_cast<wz::WzImage*>(obj)->GetPropertyByName(name);
    if (!CheckResult(env, result)) return nullptr;
    return NullableObject(env, result.value());
  }
  return NullableObject(env, (*obj)[name]);
}

FN(ObjectSetName) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  READ_STRING(name, args[1]);
  auto result = obj->Rename(name);
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(ObjectRemove) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzObject*, obj, args[0]);
  auto result = obj->TryRemove();
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(PropType) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_INT(static_cast<int>(prop->PropertyType()));
}

FN(PropIsRaw) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_BOOL(prop->IsRawDataProperty());
}

FN(PropIsVideo) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_BOOL(prop->IsVideoProperty());
}

FN(PropName) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_STRING(prop->Name());
}

FN(PropCountChildren) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  auto* props = prop->WzProperties();
  RETURN_INT(props ? static_cast<int>(props->size()) : 0);
}

FN(PropGetChild) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  READ_INT(index, args[1]);
  auto* props = prop->WzProperties();
  if (!props) RETURN_NULL();
  if (index < 0 || index >= static_cast<int>(props->size())) RETURN_NULL();
  return ToHandle(env, (*props)[static_cast<size_t>(index)]);
}

FN(PropGetChildByName) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  READ_STRING(name, args[1]);
  auto* props = prop->WzProperties();
  if (!props) RETURN_NULL();
  for (auto* child : *props) {
    if (child && child->Name() == name) return ToHandle(env, child);
  }
  RETURN_NULL();
}

FN(PropGetFromPath) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  READ_STRING(path, args[1]);
  return NullableHandle(env, prop->GetFromPath(path));
}

FN(PropLinked) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  return NullableHandle(env, prop->GetLinkedWzImageProperty());
}

FN(PropGetInt) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_INT(prop->GetInt());
}

FN(PropGetShort) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_INT(prop->GetShort());
}

FN(PropGetLong) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_BIGINT(prop->GetLong());
}

FN(PropGetFloat) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_DOUBLE(prop->GetFloat());
}

FN(PropGetDouble) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_DOUBLE(prop->GetDouble());
}

FN(PropGetString) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  RETURN_STRING(prop->GetString());
}

FN(PropGetBytes) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  auto result = prop->GetBytes();
  if (!CheckResult(env, result)) return nullptr;
  return Bytes(env, result.value());
}

FN(PropertyFree) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  if (!CheckNotNull(env, prop, "propertyFree")) return nullptr;
  if (prop->Parent()) {
    NODE_API_THROW(env, "propertyFree: property is owned by a WZ tree");
  }
  delete prop;
  RETURN_UNDEFINED();
}

FN(PropertyAddChild) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImageProperty*, parent, args[0]);
  READ_HANDLE(wz::WzImageProperty*, child, args[1]);
  if (!IsMutablePropertyContainer(parent)) {
    NODE_API_THROW(env, "propertyAddChild: wrong property type");
  }
  auto result = parent->TryAddChildProperty(child);
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(PropertyRemoveChild) {
  GET_ARGS(2);
  READ_HANDLE(wz::WzImageProperty*, parent, args[0]);
  READ_HANDLE(wz::WzImageProperty*, child, args[1]);
  if (!IsMutablePropertyContainer(parent)) {
    NODE_API_THROW(env, "propertyRemoveChild: wrong property type");
  }
  auto result = parent->TryRemoveChildProperty(child);
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(PropertyClearChildren) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  if (!IsMutablePropertyContainer(prop)) {
    NODE_API_THROW(env, "propertyClearChildren: wrong property type");
  }
  auto result = prop->TryClearChildProperties();
  if (!CheckResult(env, result)) return nullptr;
  RETURN_UNDEFINED();
}

FN(CreatePropertyNull) {
  GET_ARGS(1);
  READ_STRING(name, args[0]);
  return ToHandle(env, new wz::WzNullProperty(name));
}

FN(CreatePropertyShort) {
  GET_ARGS(2);
  READ_STRING(name, args[0]);
  READ_INT(value, args[1]);
  return ToHandle(env,
                  new wz::WzShortProperty(name, static_cast<int16_t>(value)));
}

FN(CreatePropertyInt) {
  GET_ARGS(2);
  READ_STRING(name, args[0]);
  READ_INT(value, args[1]);
  return ToHandle(env, new wz::WzIntProperty(name, value));
}

FN(CreatePropertyLong) {
  GET_ARGS(2);
  READ_STRING(name, args[0]);
  int64_t value = 0;
  if (!ReadInt64(env, args[1], &value)) return nullptr;
  return ToHandle(env, new wz::WzLongProperty(name, value));
}

FN(CreatePropertyFloat) {
  GET_ARGS(2);
  READ_STRING(name, args[0]);
  READ_DOUBLE(value, args[1]);
  return ToHandle(env,
                  new wz::WzFloatProperty(name, static_cast<float>(value)));
}

FN(CreatePropertyDouble) {
  GET_ARGS(2);
  READ_STRING(name, args[0]);
  READ_DOUBLE(value, args[1]);
  return ToHandle(env, new wz::WzDoubleProperty(name, value));
}

FN(CreatePropertyString) {
  GET_ARGS(2);
  READ_STRING(name, args[0]);
  READ_STRING(value, args[1]);
  return ToHandle(env, new wz::WzStringProperty(name, value));
}

FN(CreatePropertySub) {
  GET_ARGS(1);
  READ_STRING(name, args[0]);
  return ToHandle(env, new wz::WzSubProperty(name));
}

FN(CreatePropertyVector) {
  GET_ARGS(3);
  READ_STRING(name, args[0]);
  READ_INT(x, args[1]);
  READ_INT(y, args[2]);
  return ToHandle(env, new wz::WzVectorProperty(name, x, y));
}

FN(CreatePropertyUol) {
  GET_ARGS(2);
  READ_STRING(name, args[0]);
  READ_STRING(value, args[1]);
  return ToHandle(env, new wz::WzUOLProperty(name, value));
}

template <typename T>
T* TypedProp(napi_env env,
             napi_value value,
             wz::WzPropertyType type,
             const char* name) {
  wz::WzImageProperty* prop = nullptr;
  if (!ReadHandle<wz::WzImageProperty*>(env, value, &prop)) return nullptr;
  return CheckPropertyType(env, prop, type, name) ? static_cast<T*>(prop)
                                                  : nullptr;
}

FN(ShortValue) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzShortProperty>(
      env, args[0], wz::WzPropertyType::Short, "shortValue");
  if (!prop) return nullptr;
  RETURN_INT(prop->Value());
}

FN(ShortSetValue) {
  GET_ARGS(2);
  auto* prop = TypedProp<wz::WzShortProperty>(
      env, args[0], wz::WzPropertyType::Short, "shortSetValue");
  if (!prop) return nullptr;
  READ_INT(value, args[1]);
  prop->SetValue(static_cast<int16_t>(value));
  RETURN_UNDEFINED();
}

FN(IntValue) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzIntProperty>(
      env, args[0], wz::WzPropertyType::Int, "intValue");
  if (!prop) return nullptr;
  RETURN_INT(prop->Value());
}

FN(IntSetValue) {
  GET_ARGS(2);
  auto* prop = TypedProp<wz::WzIntProperty>(
      env, args[0], wz::WzPropertyType::Int, "intSetValue");
  if (!prop) return nullptr;
  READ_INT(value, args[1]);
  prop->SetValue(value);
  RETURN_UNDEFINED();
}

FN(LongValue) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzLongProperty>(
      env, args[0], wz::WzPropertyType::Long, "longValue");
  if (!prop) return nullptr;
  RETURN_BIGINT(prop->Value());
}

FN(LongSetValue) {
  GET_ARGS(2);
  auto* prop = TypedProp<wz::WzLongProperty>(
      env, args[0], wz::WzPropertyType::Long, "longSetValue");
  int64_t value = 0;
  if (!prop || !ReadInt64(env, args[1], &value)) return nullptr;
  prop->SetValue(value);
  RETURN_UNDEFINED();
}

FN(FloatValue) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzFloatProperty>(
      env, args[0], wz::WzPropertyType::Float, "floatValue");
  if (!prop) return nullptr;
  RETURN_DOUBLE(prop->Value());
}

FN(FloatSetValue) {
  GET_ARGS(2);
  auto* prop = TypedProp<wz::WzFloatProperty>(
      env, args[0], wz::WzPropertyType::Float, "floatSetValue");
  if (!prop) return nullptr;
  READ_DOUBLE(value, args[1]);
  prop->SetValue(static_cast<float>(value));
  RETURN_UNDEFINED();
}

FN(DoubleValue) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzDoubleProperty>(
      env, args[0], wz::WzPropertyType::Double, "doubleValue");
  if (!prop) return nullptr;
  RETURN_DOUBLE(prop->Value());
}

FN(DoubleSetValue) {
  GET_ARGS(2);
  auto* prop = TypedProp<wz::WzDoubleProperty>(
      env, args[0], wz::WzPropertyType::Double, "doubleSetValue");
  if (!prop) return nullptr;
  READ_DOUBLE(value, args[1]);
  prop->SetValue(value);
  RETURN_UNDEFINED();
}

FN(StringValue) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzStringProperty>(
      env, args[0], wz::WzPropertyType::String, "stringValue");
  if (!prop) return nullptr;
  RETURN_STRING(prop->Value());
}

FN(StringSetValue) {
  GET_ARGS(2);
  auto* prop = TypedProp<wz::WzStringProperty>(
      env, args[0], wz::WzPropertyType::String, "stringSetValue");
  if (!prop) return nullptr;
  READ_STRING(value, args[1]);
  prop->SetValue(value);
  RETURN_UNDEFINED();
}

FN(CanvasPng) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzCanvasProperty>(
      env, args[0], wz::WzPropertyType::Canvas, "canvasPng");
  if (!prop) return nullptr;
  return NullableHandle(env, prop->PngProperty());
}

FN(CanvasContainsInlink) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzCanvasProperty>(
      env, args[0], wz::WzPropertyType::Canvas, "canvasContainsInlink");
  if (!prop) return nullptr;
  RETURN_BOOL(prop->ContainsInlinkProperty());
}

FN(CanvasContainsOutlink) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzCanvasProperty>(
      env, args[0], wz::WzPropertyType::Canvas, "canvasContainsOutlink");
  if (!prop) return nullptr;
  RETURN_BOOL(prop->ContainsOutlinkProperty());
}

FN(CanvasLinked) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzCanvasProperty>(
      env, args[0], wz::WzPropertyType::Canvas, "canvasLinked");
  if (!prop) return nullptr;
  auto result = prop->GetLinkedWzImageProperty();
  if (!CheckResult(env, result)) return nullptr;
  return NullableHandle(env, result.value());
}

FN(PngWidth) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzPngProperty*, png, args[0]);
  RETURN_INT(png->Width());
}

FN(PngHeight) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzPngProperty*, png, args[0]);
  RETURN_INT(png->Height());
}

FN(PngFormat) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzPngProperty*, png, args[0]);
  RETURN_INT(static_cast<int>(png->Format()));
}

FN(PngListWzUsed) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzPngProperty*, png, args[0]);
  RETURN_BOOL(png->ListWzUsed());
}

FN(PngImage) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzPngProperty*, png, args[0]);
  auto result = png->GetImage(false);
  if (!CheckResult(env, result)) return nullptr;
  return Bytes(env, result.value());
}

FN(PngCompressedBytes) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzPngProperty*, png, args[0]);
  auto result = png->GetCompressedBytes(false);
  if (!CheckResult(env, result)) return nullptr;
  return Bytes(env, result.value());
}

FN(VectorX) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzVectorProperty>(
      env, args[0], wz::WzPropertyType::Vector, "vectorX");
  if (!prop) return nullptr;
  RETURN_INT(prop->X ? prop->X->Value() : 0);
}

FN(VectorY) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzVectorProperty>(
      env, args[0], wz::WzPropertyType::Vector, "vectorY");
  if (!prop) return nullptr;
  RETURN_INT(prop->Y ? prop->Y->Value() : 0);
}

FN(BinaryData) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzBinaryProperty>(
      env, args[0], wz::WzPropertyType::Sound, "binaryData");
  if (!prop) return nullptr;
  auto result = prop->GetBytes(false);
  if (!CheckResult(env, result)) return nullptr;
  return Bytes(env, result.value());
}

FN(BinaryWav) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzBinaryProperty>(
      env, args[0], wz::WzPropertyType::Sound, "binaryWav");
  if (!prop) return nullptr;
  auto result = prop->GetBytesForWAVPlayback(false);
  if (!CheckResult(env, result)) return nullptr;
  return Bytes(env, result.value());
}

FN(BinaryLength) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzBinaryProperty>(
      env, args[0], wz::WzPropertyType::Sound, "binaryLength");
  if (!prop) return nullptr;
  RETURN_INT(prop->Length());
}

FN(BinaryFrequency) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzBinaryProperty>(
      env, args[0], wz::WzPropertyType::Sound, "binaryFrequency");
  if (!prop) return nullptr;
  RETURN_INT(prop->Frequency());
}

FN(BinaryType) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzBinaryProperty>(
      env, args[0], wz::WzPropertyType::Sound, "binaryType");
  if (!prop) return nullptr;
  RETURN_INT(static_cast<int>(prop->Type()));
}

FN(BinaryHeaderEncrypted) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzBinaryProperty>(
      env, args[0], wz::WzPropertyType::Sound, "binaryHeaderEncrypted");
  if (!prop) return nullptr;
  RETURN_BOOL(prop->HeaderEncrypted());
}

FN(RawData) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  if (!prop || !prop->IsRawDataProperty()) {
    NODE_API_THROW(env, "rawData: wrong property type");
  }
  auto result = static_cast<wz::WzRawDataProperty*>(prop)->GetBytes(false);
  if (!CheckResult(env, result)) return nullptr;
  return Bytes(env, result.value());
}

FN(RawType) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  if (!prop || !prop->IsRawDataProperty()) {
    NODE_API_THROW(env, "rawType: wrong property type");
  }
  RETURN_INT(static_cast<wz::WzRawDataProperty*>(prop)->Type());
}

FN(VideoData) {
  GET_ARGS(1);
  READ_HANDLE(wz::WzImageProperty*, prop, args[0]);
  if (!prop || !prop->IsVideoProperty()) {
    NODE_API_THROW(env, "videoData: wrong property type");
  }
  auto result = static_cast<wz::WzVideoProperty*>(prop)->GetBytes(false);
  if (!CheckResult(env, result)) return nullptr;
  return Bytes(env, result.value());
}

FN(UolValue) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzUOLProperty>(
      env, args[0], wz::WzPropertyType::UOL, "uolValue");
  if (!prop) return nullptr;
  RETURN_STRING(prop->Value());
}

FN(UolSetValue) {
  GET_ARGS(2);
  auto* prop = TypedProp<wz::WzUOLProperty>(
      env, args[0], wz::WzPropertyType::UOL, "uolSetValue");
  if (!prop) return nullptr;
  READ_STRING(value, args[1]);
  prop->SetValue(value);
  RETURN_UNDEFINED();
}

FN(UolLinkValue) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzUOLProperty>(
      env, args[0], wz::WzPropertyType::UOL, "uolLinkValue");
  if (!prop) return nullptr;
  return NullableObject(env, prop->LinkValue());
}

FN(LuaData) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzLuaProperty>(
      env, args[0], wz::WzPropertyType::Lua, "luaData");
  if (!prop) return nullptr;
  return Bytes(env, prop->Value());
}

FN(LuaString) {
  GET_ARGS(1);
  auto* prop = TypedProp<wz::WzLuaProperty>(
      env, args[0], wz::WzPropertyType::Lua, "luaString");
  if (!prop) return nullptr;
  RETURN_STRING(prop->GetString());
}

FN(DetectMapleVersion) {
  GET_ARGS(1);
  READ_STRING(path, args[0]);
  int16_t version = 0;
  auto maple_version = wz::WzTool::DetectMapleVersion(path, &version);
  napi_value result = nullptr;
  NODE_API_CALL(
      env, napi_create_object(env, &result), "failed to create version result");
  napi_value maple_version_value = nullptr;
  NODE_API_CALL(env,
                napi_create_int32(
                    env, static_cast<int>(maple_version), &maple_version_value),
                "failed to create maple version value");
  NODE_API_CALL(
      env,
      napi_set_named_property(env, result, "mapleVersion", maple_version_value),
      "failed to set mapleVersion");
  napi_value version_value = nullptr;
  NODE_API_CALL(env,
                napi_create_int32(env, version, &version_value),
                "failed to create file version value");
  NODE_API_CALL(env,
                napi_set_named_property(env, result, "version", version_value),
                "failed to set version");
  return result;
}

FN(IvForVersion) {
  GET_ARGS(1);
  READ_INT(version, args[0]);
  auto iv =
      wz::WzTool::GetIvByMapleVersion(static_cast<wz::WzMapleVersion>(version));
  return Bytes(env, std::vector<uint8_t>(iv.begin(), iv.end()));
}

napi_value Init(napi_env env, napi_value exports) {
  EXPORT("openFile", OpenFile);
  EXPORT("openFileWithIv", OpenFileWithIv);
  EXPORT("openMemory", OpenMemory);
  EXPORT("openMemoryWithIv", OpenMemoryWithIv);
  EXPORT("openBlobSource", OpenBlobSource);
  EXPORT("openBlobSourceWithIv", OpenBlobSourceWithIv);
  EXPORT("createFile", CreateFile);
  EXPORT("parseFile", ParseFile);
  EXPORT("closeFile", CloseFile);
  EXPORT("fileSaveToDisk", FileSaveToDisk);
  EXPORT("fileName", FileName);
  EXPORT("filePath", FilePath);
  EXPORT("fileVersion", FileVersion);
  EXPORT("fileMapleVersion", FileMapleVersion);
  EXPORT("fileWzDirectory", FileWzDirectory);
  EXPORT("fileIs64Bit", FileIs64Bit);
  EXPORT("fileIsUnloaded", FileIsUnloaded);
  EXPORT("fileVersionHash", FileVersionHash);
  EXPORT("fileObjectFromPath", FileObjectFromPath);
  EXPORT("dirName", DirName);
  EXPORT("dirCountImages", DirCountImages);
  EXPORT("dirCountDirectories", DirCountDirectories);
  EXPORT("dirCountImagesTotal", DirCountImagesTotal);
  EXPORT("dirGetImage", DirGetImage);
  EXPORT("dirGetImageByName", DirGetImageByName);
  EXPORT("dirGetDirectory", DirGetDirectory);
  EXPORT("dirGetDirectoryByName", DirGetDirectoryByName);
  EXPORT("dirCreateDirectory", DirCreateDirectory);
  EXPORT("dirCreateImage", DirCreateImage);
  EXPORT("dirRemoveDirectory", DirRemoveDirectory);
  EXPORT("dirRemoveImage", DirRemoveImage);
  EXPORT("dirBlockSize", DirBlockSize);
  EXPORT("dirChecksum", DirChecksum);
  EXPORT("dirOffset", DirOffset);
  EXPORT("imageName", ImageName);
  EXPORT("imageParsed", ImageParsed);
  EXPORT("imageChanged", ImageChanged);
  EXPORT("imageBlockSize", ImageBlockSize);
  EXPORT("imageChecksum", ImageChecksum);
  EXPORT("imageOffset", ImageOffset);
  EXPORT("imageIsLua", ImageIsLua);
  EXPORT("imageParse", ImageParse);
  EXPORT("imageCountProperties", ImageCountProperties);
  EXPORT("imageGetProperty", ImageGetProperty);
  EXPORT("imageGetFromPath", ImageGetFromPath);
  EXPORT("imageAddProperty", ImageAddProperty);
  EXPORT("imageRemoveProperty", ImageRemoveProperty);
  EXPORT("imageClearProperties", ImageClearProperties);
  EXPORT("objectType", ObjectType);
  EXPORT("objectName", ObjectName);
  EXPORT("objectParent", ObjectParent);
  EXPORT("objectFullPath", ObjectFullPath);
  EXPORT("objectWzFileParent", ObjectWzFileParent);
  EXPORT("objectTopMostDirectory", ObjectTopMostDirectory);
  EXPORT("objectTopMostImage", ObjectTopMostImage);
  EXPORT("objectAt", ObjectAt);
  EXPORT("objectSetName", ObjectSetName);
  EXPORT("objectRemove", ObjectRemove);
  EXPORT("propType", PropType);
  EXPORT("propIsRaw", PropIsRaw);
  EXPORT("propIsVideo", PropIsVideo);
  EXPORT("propName", PropName);
  EXPORT("propCountChildren", PropCountChildren);
  EXPORT("propGetChild", PropGetChild);
  EXPORT("propGetChildByName", PropGetChildByName);
  EXPORT("propGetFromPath", PropGetFromPath);
  EXPORT("propGetInt", PropGetInt);
  EXPORT("propGetShort", PropGetShort);
  EXPORT("propGetLong", PropGetLong);
  EXPORT("propGetFloat", PropGetFloat);
  EXPORT("propGetDouble", PropGetDouble);
  EXPORT("propGetString", PropGetString);
  EXPORT("propGetBytes", PropGetBytes);
  EXPORT("propLinked", PropLinked);
  EXPORT("propertyCreateNull", CreatePropertyNull);
  EXPORT("propertyCreateShort", CreatePropertyShort);
  EXPORT("propertyCreateInt", CreatePropertyInt);
  EXPORT("propertyCreateLong", CreatePropertyLong);
  EXPORT("propertyCreateFloat", CreatePropertyFloat);
  EXPORT("propertyCreateDouble", CreatePropertyDouble);
  EXPORT("propertyCreateString", CreatePropertyString);
  EXPORT("propertyCreateSub", CreatePropertySub);
  EXPORT("propertyCreateVector", CreatePropertyVector);
  EXPORT("propertyCreateUol", CreatePropertyUol);
  EXPORT("propertyFree", PropertyFree);
  EXPORT("propertyAddChild", PropertyAddChild);
  EXPORT("propertyRemoveChild", PropertyRemoveChild);
  EXPORT("propertyClearChildren", PropertyClearChildren);
  EXPORT("shortValue", ShortValue);
  EXPORT("shortSetValue", ShortSetValue);
  EXPORT("intValue", IntValue);
  EXPORT("intSetValue", IntSetValue);
  EXPORT("longValue", LongValue);
  EXPORT("longSetValue", LongSetValue);
  EXPORT("floatValue", FloatValue);
  EXPORT("floatSetValue", FloatSetValue);
  EXPORT("doubleValue", DoubleValue);
  EXPORT("doubleSetValue", DoubleSetValue);
  EXPORT("stringValue", StringValue);
  EXPORT("stringSetValue", StringSetValue);
  EXPORT("pngWidth", PngWidth);
  EXPORT("pngHeight", PngHeight);
  EXPORT("pngFormat", PngFormat);
  EXPORT("pngListWzUsed", PngListWzUsed);
  EXPORT("pngImage", PngImage);
  EXPORT("pngCompressedBytes", PngCompressedBytes);
  EXPORT("canvasPng", CanvasPng);
  EXPORT("canvasContainsInlink", CanvasContainsInlink);
  EXPORT("canvasContainsOutlink", CanvasContainsOutlink);
  EXPORT("canvasLinked", CanvasLinked);
  EXPORT("uolValue", UolValue);
  EXPORT("uolSetValue", UolSetValue);
  EXPORT("uolLinkValue", UolLinkValue);
  EXPORT("vectorX", VectorX);
  EXPORT("vectorY", VectorY);
  EXPORT("luaData", LuaData);
  EXPORT("luaString", LuaString);
  EXPORT("binaryData", BinaryData);
  EXPORT("binaryWav", BinaryWav);
  EXPORT("binaryLength", BinaryLength);
  EXPORT("binaryFrequency", BinaryFrequency);
  EXPORT("binaryType", BinaryType);
  EXPORT("binaryHeaderEncrypted", BinaryHeaderEncrypted);
  EXPORT("rawData", RawData);
  EXPORT("rawType", RawType);
  EXPORT("videoData", VideoData);
  EXPORT("detectMapleVersion", DetectMapleVersion);
  EXPORT("ivForVersion", IvForVersion);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

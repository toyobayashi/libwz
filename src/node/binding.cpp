#include <napi.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "wz/wz_api.h"

namespace {

template <typename T>
inline T FromHandle(const Napi::Value& value) {
  bool lossless = false;
  uint64_t raw = value.As<Napi::BigInt>().Uint64Value(&lossless);
  return reinterpret_cast<T>(static_cast<uintptr_t>(raw));
}

template <typename T>
inline Napi::BigInt ToHandle(Napi::Env env, T ptr) {
  return Napi::BigInt::New(
      env, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr)));
}

inline void ThrowIfError(Napi::Env env) {
  const wz_last_error_info* info = nullptr;
  wz_get_last_error_info(&info);
  const char* msg = info ? info->message : nullptr;
  Napi::Error::New(env, msg ? msg : "libwz error").ThrowAsJavaScriptException();
}

#define WZ_NODE_API_CALL(env, expr)                                            \
  do {                                                                         \
    wz_error_code wz_api_err = (expr);                                         \
    if (wz_api_err != WZ_ERROR_NONE) {                                         \
      ThrowIfError((env));                                                     \
      return Napi::Value();                                                    \
    }                                                                          \
  } while (false)

bool ReadInt64BigInt(Napi::Env env, const Napi::Value& value, int64_t* out) {
  bool lossless = false;
  *out = value.As<Napi::BigInt>().Int64Value(&lossless);
  if (!lossless) {
    Napi::RangeError::New(env, "bigint value is outside int64 range")
        .ThrowAsJavaScriptException();
    return false;
  }
  return true;
}

Napi::Value NullableString(Napi::Env env, const char* s) {
  if (!s) return env.Null();
  return Napi::String::New(env, s);
}

Napi::Value NullableObject(Napi::Env env, wz_object obj) {
  if (!obj) return env.Null();
  Napi::Object result = Napi::Object::New(env);
  wz_object_type type = WZ_OBJECT_FILE;
  WZ_NODE_API_CALL(env, wz_get_object_type(obj, &type));
  result.Set("type", static_cast<int>(type));
  result.Set("ptr", ToHandle(env, obj));
  return result;
}

template <typename SizeFn>
Napi::Value ReadBytes(const Napi::CallbackInfo& info, SizeFn fn) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  size_t size = 0;
  WZ_NODE_API_CALL(env, fn(prop, nullptr, 0, &size));
  Napi::ArrayBuffer storage = Napi::ArrayBuffer::New(env, size);
  auto bytes = static_cast<uint8_t*>(storage.Data());
  if (size > 0) {
    WZ_NODE_API_CALL(env, fn(prop, bytes, size, &size));
  }
  return Napi::Uint8Array::New(env, size, storage, 0);
}

template <typename SizeFn>
Napi::Value ReadPngBytes(const Napi::CallbackInfo& info, SizeFn fn) {
  Napi::Env env = info.Env();
  wz_png_property png = FromHandle<wz_png_property>(info[0]);
  size_t size = 0;
  WZ_NODE_API_CALL(env, fn(png, nullptr, 0, &size));
  Napi::ArrayBuffer storage = Napi::ArrayBuffer::New(env, size);
  auto bytes = static_cast<uint8_t*>(storage.Data());
  if (size > 0) {
    WZ_NODE_API_CALL(env, fn(png, bytes, size, &size));
  }
  return Napi::Uint8Array::New(env, size, storage, 0);
}

Napi::Value OpenFile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  int gameVersion = info[1].As<Napi::Number>().Int32Value();
  int version = info[2].As<Napi::Number>().Int32Value();
  wz_file file = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_open_file(path.c_str(),
                                static_cast<short>(gameVersion),
                                static_cast<wz_maple_version>(version),
                                &file));
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value OpenFileWithIv(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  Napi::Uint8Array iv = info[1].As<Napi::Uint8Array>();
  uint8_t bytes[4] = {0, 0, 0, 0};
  std::memcpy(bytes, iv.Data(), iv.ByteLength() < 4 ? iv.ByteLength() : 4);
  wz_file file = nullptr;
  WZ_NODE_API_CALL(env, wz_open_file_with_iv(path.c_str(), bytes, &file));
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value OpenMemory(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  Napi::Uint8Array bytes = info[1].As<Napi::Uint8Array>();
  int gameVersion = info[2].As<Napi::Number>().Int32Value();
  int version = info[3].As<Napi::Number>().Int32Value();
  wz_file file = nullptr;
  WZ_NODE_API_CALL(
      env,
      wz_open_memory(name.c_str(),
                     bytes.Data(),
                     bytes.ByteLength(),
                     static_cast<short>(gameVersion),
                     static_cast<wz_maple_version>(version),
                     &file));
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value OpenMemoryWithIv(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  Napi::Uint8Array data = info[1].As<Napi::Uint8Array>();
  Napi::Uint8Array iv = info[2].As<Napi::Uint8Array>();
  uint8_t iv_bytes[4] = {0, 0, 0, 0};
  std::memcpy(iv_bytes, iv.Data(), iv.ByteLength() < 4 ? iv.ByteLength() : 4);
  wz_file file = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_open_memory_with_iv(name.c_str(),
                                          data.Data(),
                                          data.ByteLength(),
                                          iv_bytes,
                                          &file));
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value CreateFile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int gameVersion = info[0].As<Napi::Number>().Int32Value();
  int version = info[1].As<Napi::Number>().Int32Value();
  wz_file file = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_create_file(static_cast<short>(gameVersion),
                                  static_cast<wz_maple_version>(version),
                                  &file));
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value ParseFile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_parse_status status = WZ_PARSE_FAILED_UNKNOWN;
  WZ_NODE_API_CALL(env, wz_parse(FromHandle<wz_file>(info[0]), &status));
  return Napi::Number::New(env, static_cast<int>(status));
}

Napi::Value CloseFile(const Napi::CallbackInfo& info) {
  wz_close_file(FromHandle<wz_file>(info[0]));
  return info.Env().Undefined();
}

Napi::Value FileSaveToDisk(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[1].As<Napi::String>().Utf8Value();
  WZ_NODE_API_CALL(
      env, wz_file_save_to_disk(FromHandle<wz_file>(info[0]), path.c_str()));
  return env.Undefined();
}

Napi::Value FileName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  const char* name = nullptr;
  WZ_NODE_API_CALL(env, wz_file_name(FromHandle<wz_file>(info[0]), &name));
  return NullableString(env, name);
}

Napi::Value FilePath(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  const char* path = nullptr;
  WZ_NODE_API_CALL(env, wz_file_path(FromHandle<wz_file>(info[0]), &path));
  return NullableString(env, path);
}

Napi::Value FileVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  short v = 0;
  WZ_NODE_API_CALL(env, wz_file_version(FromHandle<wz_file>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value FileMapleVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_file file = FromHandle<wz_file>(info[0]);
  wz_maple_version v = WZ_UNKNOWN;
  WZ_NODE_API_CALL(env, wz_file_maple_version(file, &v));
  return Napi::Number::New(env, static_cast<int>(v));
}

Napi::Value FileWzDirectory(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_file file = FromHandle<wz_file>(info[0]);
  wz_dir dir = nullptr;
  WZ_NODE_API_CALL(env, wz_file_get_wz_directory(file, &dir));
  return dir ? ToHandle(env, dir) : env.Null();
}

Napi::Value FileBool(const Napi::CallbackInfo& info,
                     wz_error_code (*fn)(wz_file, int*)) {
  Napi::Env env = info.Env();
  int result = 0;
  WZ_NODE_API_CALL(env, fn(FromHandle<wz_file>(info[0]), &result));
  return Napi::Boolean::New(env, result != 0);
}

Napi::Value FileVersionHash(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  uint32_t v = 0;
  WZ_NODE_API_CALL(env, wz_file_version_hash(FromHandle<wz_file>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value FileObjectFromPath(const Napi::CallbackInfo& info) {
  std::string path = info[1].As<Napi::String>().Utf8Value();
  bool check = info[2].As<Napi::Boolean>().Value();
  Napi::Env env = info.Env();
  wz_file file = FromHandle<wz_file>(info[0]);
  wz_object obj = nullptr;
  WZ_NODE_API_CALL(
      env,
      wz_file_get_object_from_path(file, path.c_str(), check ? 1 : 0, &obj));
  return NullableObject(env, obj);
}

Napi::Value DirName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  const char* name = nullptr;
  WZ_NODE_API_CALL(env, wz_dir_name(FromHandle<wz_dir>(info[0]), &name));
  return NullableString(env, name);
}

Napi::Value DirCountImages(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = 0;
  WZ_NODE_API_CALL(env, wz_dir_count_images(FromHandle<wz_dir>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value DirCountDirectories(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_dir dir = FromHandle<wz_dir>(info[0]);
  int v = 0;
  WZ_NODE_API_CALL(env, wz_dir_count_directories(dir, &v));
  return Napi::Number::New(env, v);
}

Napi::Value DirCountImagesTotal(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_dir dir = FromHandle<wz_dir>(info[0]);
  int v = 0;
  WZ_NODE_API_CALL(env, wz_dir_count_images_total(dir, &v));
  return Napi::Number::New(env, v);
}

Napi::Value DirGetImage(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_dir dir = FromHandle<wz_dir>(info[0]);
  int index = info[1].As<Napi::Number>().Int32Value();
  wz_image image = nullptr;
  WZ_NODE_API_CALL(env, wz_dir_get_image(dir, index, &image));
  return image ? ToHandle(env, image) : env.Null();
}

Napi::Value DirGetImageByName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  wz_image image = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_dir_get_image_by_name(
                       FromHandle<wz_dir>(info[0]), name.c_str(), &image));
  return image ? ToHandle(env, image) : env.Null();
}

Napi::Value DirGetDirectory(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_dir parent = FromHandle<wz_dir>(info[0]);
  int index = info[1].As<Napi::Number>().Int32Value();
  wz_dir dir = nullptr;
  WZ_NODE_API_CALL(env, wz_dir_get_directory(parent, index, &dir));
  return dir ? ToHandle(env, dir) : env.Null();
}

Napi::Value DirGetDirectoryByName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  wz_dir dir = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_dir_get_directory_by_name(
                       FromHandle<wz_dir>(info[0]), name.c_str(), &dir));
  return dir ? ToHandle(env, dir) : env.Null();
}

Napi::Value DirCreateDirectory(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  wz_dir dir = nullptr;
  WZ_NODE_API_CALL(
      env,
      wz_dir_create_directory(FromHandle<wz_dir>(info[0]), name.c_str(), &dir));
  return dir ? ToHandle(env, dir) : env.Null();
}

Napi::Value DirCreateImage(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  wz_image image = nullptr;
  WZ_NODE_API_CALL(
      env,
      wz_dir_create_image(FromHandle<wz_dir>(info[0]), name.c_str(), &image));
  return image ? ToHandle(env, image) : env.Null();
}

Napi::Value DirRemoveDirectory(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env,
                   wz_dir_remove_directory(FromHandle<wz_dir>(info[0]),
                                           FromHandle<wz_dir>(info[1])));
  return env.Undefined();
}

Napi::Value DirRemoveImage(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env,
                   wz_dir_remove_image(FromHandle<wz_dir>(info[0]),
                                       FromHandle<wz_image>(info[1])));
  return env.Undefined();
}

Napi::Value DirInt64(const Napi::CallbackInfo& info,
                     wz_error_code (*fn)(wz_dir, int64_t*)) {
  Napi::Env env = info.Env();
  int64_t v = 0;
  WZ_NODE_API_CALL(env, fn(FromHandle<wz_dir>(info[0]), &v));
  return Napi::BigInt::New(env, v);
}

Napi::Value DirInt(const Napi::CallbackInfo& info,
                   wz_error_code (*fn)(wz_dir, int*)) {
  Napi::Env env = info.Env();
  int v = 0;
  WZ_NODE_API_CALL(env, fn(FromHandle<wz_dir>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value ImageName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  const char* name = nullptr;
  WZ_NODE_API_CALL(env, wz_image_name(FromHandle<wz_image>(info[0]), &name));
  return NullableString(env, name);
}

Napi::Value ImageBool(const Napi::CallbackInfo& info,
                      wz_error_code (*fn)(wz_image, int*)) {
  Napi::Env env = info.Env();
  int result = 0;
  WZ_NODE_API_CALL(env, fn(FromHandle<wz_image>(info[0]), &result));
  return Napi::Boolean::New(env, result != 0);
}

Napi::Value ImageInt(const Napi::CallbackInfo& info,
                     wz_error_code (*fn)(wz_image, int*)) {
  Napi::Env env = info.Env();
  int v = 0;
  WZ_NODE_API_CALL(env, fn(FromHandle<wz_image>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value ImageOffset(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int64_t v = 0;
  WZ_NODE_API_CALL(env, wz_image_offset(FromHandle<wz_image>(info[0]), &v));
  return Napi::BigInt::New(env, v);
}

Napi::Value ImageParse(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int parsed = 0;
  WZ_NODE_API_CALL(env, wz_image_parse(FromHandle<wz_image>(info[0]), &parsed));
  return env.Undefined();
}

Napi::Value ImageCountProperties(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_image image = FromHandle<wz_image>(info[0]);
  int v = 0;
  WZ_NODE_API_CALL(env, wz_image_count_properties(image, &v));
  return Napi::Number::New(env, v);
}

Napi::Value ImageGetProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_image image = FromHandle<wz_image>(info[0]);
  int index = info[1].As<Napi::Number>().Int32Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_image_get_property(image, index, &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value ImageGetFromPath(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[1].As<Napi::String>().Utf8Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_image_get_from_path(
                       FromHandle<wz_image>(info[0]), path.c_str(), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value ObjectType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_object obj = FromHandle<wz_object>(info[0]);
  wz_object_type type = WZ_OBJECT_FILE;
  WZ_NODE_API_CALL(env, wz_get_object_type(obj, &type));
  return Napi::Number::New(env, static_cast<int>(type));
}

Napi::Value ObjectName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  const char* name = nullptr;
  WZ_NODE_API_CALL(env, wz_object_name(FromHandle<wz_object>(info[0]), &name));
  return NullableString(env, name);
}

Napi::Value ObjectParent(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_object obj = FromHandle<wz_object>(info[0]);
  wz_object parent = nullptr;
  WZ_NODE_API_CALL(env, wz_object_get_parent(obj, &parent));
  return NullableObject(env, parent);
}

Napi::Value ObjectFullPath(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_object obj = FromHandle<wz_object>(info[0]);
  const char* path = nullptr;
  WZ_NODE_API_CALL(env, wz_object_full_path(obj, &path));
  return NullableString(env, path);
}

Napi::Value ObjectWzFileParent(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_object obj = FromHandle<wz_object>(info[0]);
  wz_file file = nullptr;
  WZ_NODE_API_CALL(env, wz_object_get_wz_file_parent(obj, &file));
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value ObjectTopMostDirectory(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_object obj = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_object_get_top_most_wz_directory(
                       FromHandle<wz_object>(info[0]), &obj));
  return NullableObject(env, obj);
}

Napi::Value ObjectTopMostImage(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_object obj = nullptr;
  WZ_NODE_API_CALL(
      env,
      wz_object_get_top_most_wz_image(FromHandle<wz_object>(info[0]), &obj));
  return NullableObject(env, obj);
}

Napi::Value ObjectAt(const Napi::CallbackInfo& info) {
  std::string name = info[1].As<Napi::String>().Utf8Value();
  Napi::Env env = info.Env();
  wz_object parent = FromHandle<wz_object>(info[0]);
  wz_object obj = nullptr;
  WZ_NODE_API_CALL(env, wz_object_at(parent, name.c_str(), &obj));
  return NullableObject(env, obj);
}

Napi::Value ObjectSetName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  WZ_NODE_API_CALL(
      env, wz_object_set_name(FromHandle<wz_object>(info[0]), name.c_str()));
  return env.Undefined();
}

Napi::Value ObjectRemove(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env, wz_object_remove(FromHandle<wz_object>(info[0])));
  return env.Undefined();
}

Napi::Value PropType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  wz_property_type type = WZ_PROP_NULL;
  WZ_NODE_API_CALL(env, wz_property_get_type(prop, &type));
  return Napi::Number::New(env, static_cast<int>(type));
}

Napi::Value PropIsRaw(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int is_raw = 0;
  WZ_NODE_API_CALL(env, wz_property_is_raw_data(prop, &is_raw));
  bool result = is_raw != 0;
  return Napi::Boolean::New(env, result);
}

Napi::Value PropIsVideo(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int is_video = 0;
  WZ_NODE_API_CALL(env, wz_property_is_video(prop, &is_video));
  bool result = is_video != 0;
  return Napi::Boolean::New(env, result);
}

Napi::Value PropName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  const char* name = nullptr;
  WZ_NODE_API_CALL(env, wz_property_name(prop, &name));
  return NullableString(env, name);
}

Napi::Value PropCountChildren(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int v = 0;
  WZ_NODE_API_CALL(env, wz_property_count_children(prop, &v));
  return Napi::Number::New(env, v);
}

Napi::Value PropGetChild(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property parent = FromHandle<wz_property>(info[0]);
  int index = info[1].As<Napi::Number>().Int32Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_property_get_child(parent, index, &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value PropGetChildByName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_property_get_child_by_name(
                       FromHandle<wz_property>(info[0]), name.c_str(), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value PropGetFromPath(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[1].As<Napi::String>().Utf8Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_property_get_from_path(
                       FromHandle<wz_property>(info[0]), path.c_str(), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value PropertyFree(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env, wz_property_free(FromHandle<wz_property>(info[0])));
  return env.Undefined();
}

Napi::Value ImageAddProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env,
                   wz_image_add_property(FromHandle<wz_image>(info[0]),
                                         FromHandle<wz_property>(info[1])));
  return env.Undefined();
}

Napi::Value PropertyAddChild(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env,
                   wz_property_add_child(FromHandle<wz_property>(info[0]),
                                         FromHandle<wz_property>(info[1])));
  return env.Undefined();
}

Napi::Value ImageRemoveProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env,
                   wz_image_remove_property(FromHandle<wz_image>(info[0]),
                                            FromHandle<wz_property>(info[1])));
  return env.Undefined();
}

Napi::Value PropertyRemoveChild(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env,
                   wz_property_remove_child(FromHandle<wz_property>(info[0]),
                                            FromHandle<wz_property>(info[1])));
  return env.Undefined();
}

Napi::Value ImageClearProperties(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(env,
                   wz_image_clear_properties(FromHandle<wz_image>(info[0])));
  return env.Undefined();
}

Napi::Value PropertyClearChildren(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  WZ_NODE_API_CALL(
      env, wz_property_clear_children(FromHandle<wz_property>(info[0])));
  return env.Undefined();
}

Napi::Value PropGetInt(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int32_t v = 0;
  WZ_NODE_API_CALL(env, wz_property_get_int(prop, &v));
  return Napi::Number::New(env, v);
}

Napi::Value PropGetShort(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int16_t v = 0;
  WZ_NODE_API_CALL(env, wz_property_get_short(prop, &v));
  return Napi::Number::New(env, v);
}

Napi::Value PropGetLong(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int64_t v = 0;
  WZ_NODE_API_CALL(env, wz_property_get_long(prop, &v));
  return Napi::BigInt::New(env, v);
}

Napi::Value PropGetFloat(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  float v = 0.0f;
  WZ_NODE_API_CALL(env, wz_property_get_float(prop, &v));
  return Napi::Number::New(env, v);
}

Napi::Value PropGetDouble(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  double v = 0.0;
  WZ_NODE_API_CALL(env, wz_property_get_double(prop, &v));
  return Napi::Number::New(env, v);
}

Napi::Value PropGetString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  const char* value = nullptr;
  WZ_NODE_API_CALL(env, wz_property_get_string(prop, &value));
  return NullableString(env, value);
}

Napi::Value CreatePropertyNull(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_property_create_null(name.c_str(), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertyShort(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  int value = info[1].As<Napi::Number>().Int32Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_property_create_short(
                       name.c_str(), static_cast<int16_t>(value), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertyInt(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  int value = info[1].As<Napi::Number>().Int32Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_property_create_int(name.c_str(), value, &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertyLong(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  int64_t value = 0;
  if (!ReadInt64BigInt(env, info[1], &value)) return Napi::Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_property_create_long(name.c_str(), value, &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertyFloat(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  double value = info[1].As<Napi::Number>().DoubleValue();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(
      env,
      wz_property_create_float(name.c_str(), static_cast<float>(value), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertyDouble(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  double value = info[1].As<Napi::Number>().DoubleValue();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_property_create_double(name.c_str(), value, &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertyString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  std::string value = info[1].As<Napi::String>().Utf8Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(
      env, wz_property_create_string(name.c_str(), value.c_str(), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertySub(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_property_create_sub(name.c_str(), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertyVector(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  int x = info[1].As<Napi::Number>().Int32Value();
  int y = info[2].As<Napi::Number>().Int32Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env, wz_property_create_vector(name.c_str(), x, y, &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value CreatePropertyUol(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[0].As<Napi::String>().Utf8Value();
  std::string value = info[1].As<Napi::String>().Utf8Value();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_property_create_uol(name.c_str(), value.c_str(), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value PropGetBytes(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_property_get_bytes);
}

Napi::Value PropLinked(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_property_get_linked_wz_image_property(
                       FromHandle<wz_property>(info[0]), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value ScalarValue(const Napi::CallbackInfo& info,
                        wz_error_code (*fn)(wz_property, int32_t*)) {
  Napi::Env env = info.Env();
  int32_t v = 0;
  WZ_NODE_API_CALL(env, fn(FromHandle<wz_property>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value LongValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int64_t v = 0;
  WZ_NODE_API_CALL(env, wz_long_get_value(prop, &v));
  return Napi::BigInt::New(env, v);
}

Napi::Value FloatValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  float v = 0.0f;
  WZ_NODE_API_CALL(env, wz_float_get_value(prop, &v));
  return Napi::Number::New(env, v);
}

Napi::Value DoubleValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  double v = 0.0;
  WZ_NODE_API_CALL(env, wz_double_get_value(prop, &v));
  return Napi::Number::New(env, v);
}

Napi::Value StringValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  const char* value = nullptr;
  WZ_NODE_API_CALL(env, wz_string_get_value(prop, &value));
  return NullableString(env, value);
}

Napi::Value SetIntValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int value = info[1].As<Napi::Number>().Int32Value();
  WZ_NODE_API_CALL(env, wz_int_set_value(prop, value));
  return env.Undefined();
}

Napi::Value SetShortValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int value = info[1].As<Napi::Number>().Int32Value();
  WZ_NODE_API_CALL(env, wz_short_set_value(prop, static_cast<int16_t>(value)));
  return env.Undefined();
}

Napi::Value SetLongValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int64_t value = 0;
  if (!ReadInt64BigInt(env, info[1], &value)) return Napi::Value();
  WZ_NODE_API_CALL(env, wz_long_set_value(prop, value));
  return env.Undefined();
}

Napi::Value SetFloatValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  double value = info[1].As<Napi::Number>().DoubleValue();
  WZ_NODE_API_CALL(env, wz_float_set_value(prop, static_cast<float>(value)));
  return env.Undefined();
}

Napi::Value SetDoubleValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  double value = info[1].As<Napi::Number>().DoubleValue();
  WZ_NODE_API_CALL(env, wz_double_set_value(prop, value));
  return env.Undefined();
}

Napi::Value SetStringValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  std::string value = info[1].As<Napi::String>().Utf8Value();
  WZ_NODE_API_CALL(env, wz_string_set_value(prop, value.c_str()));
  return env.Undefined();
}

Napi::Value PngWidth(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_png_property png = FromHandle<wz_png_property>(info[0]);
  int v = 0;
  WZ_NODE_API_CALL(env, wz_png_width(png, &v));
  return Napi::Number::New(env, v);
}

Napi::Value PngHeight(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_png_property png = FromHandle<wz_png_property>(info[0]);
  int v = 0;
  WZ_NODE_API_CALL(env, wz_png_height(png, &v));
  return Napi::Number::New(env, v);
}

Napi::Value PngFormat(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_png_property png = FromHandle<wz_png_property>(info[0]);
  int v = 0;
  WZ_NODE_API_CALL(env, wz_png_format(png, &v));
  return Napi::Number::New(env, v);
}

Napi::Value PngListWzUsed(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_png_property png = FromHandle<wz_png_property>(info[0]);
  int is_used = 0;
  WZ_NODE_API_CALL(env, wz_png_is_list_wz_used(png, &is_used));
  bool v = is_used != 0;
  return Napi::Boolean::New(env, v);
}

Napi::Value CanvasPng(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  wz_png_property png = nullptr;
  WZ_NODE_API_CALL(env, wz_canvas_get_png(prop, &png));
  return png ? ToHandle(env, png) : env.Null();
}

Napi::Value CanvasBool(const Napi::CallbackInfo& info,
                       wz_error_code (*fn)(wz_property, int*)) {
  Napi::Env env = info.Env();
  int v = 0;
  WZ_NODE_API_CALL(env, fn(FromHandle<wz_property>(info[0]), &v));
  return Napi::Boolean::New(env, v != 0);
}

Napi::Value CanvasLinked(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = nullptr;
  WZ_NODE_API_CALL(env,
                   wz_canvas_get_linked_wz_image_property(
                       FromHandle<wz_property>(info[0]), &prop));
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value UolValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  const char* value = nullptr;
  WZ_NODE_API_CALL(env, wz_uol_get_value(prop, &value));
  return NullableString(env, value);
}

Napi::Value SetUolValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  std::string value = info[1].As<Napi::String>().Utf8Value();
  WZ_NODE_API_CALL(env, wz_uol_set_value(prop, value.c_str()));
  return env.Undefined();
}

Napi::Value UolLinkValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  wz_object obj = nullptr;
  WZ_NODE_API_CALL(env, wz_uol_get_link_value(prop, &obj));
  return NullableObject(env, obj);
}

Napi::Value VectorX(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int32_t v = 0;
  WZ_NODE_API_CALL(env, wz_vector_get_x(FromHandle<wz_property>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value VectorY(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int32_t v = 0;
  WZ_NODE_API_CALL(env, wz_vector_get_y(FromHandle<wz_property>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value LuaData(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_lua_get_data);
}

Napi::Value LuaString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  const char* value = nullptr;
  WZ_NODE_API_CALL(env, wz_lua_get_string(prop, &value));
  return NullableString(env, value);
}

Napi::Value BinaryData(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_binary_get_data);
}

Napi::Value BinaryWav(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_binary_get_wav_playback);
}

Napi::Value BinaryInt(const Napi::CallbackInfo& info,
                      wz_error_code (*fn)(wz_property, int*)) {
  Napi::Env env = info.Env();
  int v = 0;
  WZ_NODE_API_CALL(env, fn(FromHandle<wz_property>(info[0]), &v));
  return Napi::Number::New(env, v);
}

Napi::Value BinaryBool(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int is_encrypted = 0;
  WZ_NODE_API_CALL(env,
                   wz_binary_is_header_encrypted(
                       FromHandle<wz_property>(info[0]), &is_encrypted));
  bool v = is_encrypted != 0;
  return Napi::Boolean::New(env, v);
}

Napi::Value BinaryType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  wz_binary_type v = WZ_BINARY_RAW;
  WZ_NODE_API_CALL(env, wz_binary_get_type(prop, &v));
  return Napi::Number::New(env, static_cast<int>(v));
}

Napi::Value RawData(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_rawdata_get_data);
}

Napi::Value RawType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  int v = 0;
  WZ_NODE_API_CALL(env, wz_rawdata_get_type(prop, &v));
  return Napi::Number::New(env, v);
}

Napi::Value VideoData(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_video_get_data);
}

Napi::Value ToolDetectMapleVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  short outVersion = 0;
  wz_maple_version version = WZ_UNKNOWN;
  WZ_NODE_API_CALL(
      env, wz_detect_maple_version(path.c_str(), &version, &outVersion));
  Napi::Object result = Napi::Object::New(env);
  result.Set("mapleVersion", static_cast<int>(version));
  result.Set("version", outVersion);
  return result;
}

Napi::Value ToolIvForVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int version = info[0].As<Napi::Number>().Int32Value();
  const uint8_t* iv = nullptr;
  WZ_NODE_API_CALL(
      env, wz_get_iv_for_version(static_cast<wz_maple_version>(version), &iv));
  Napi::ArrayBuffer storage = Napi::ArrayBuffer::New(env, 4);
  auto bytes = static_cast<uint8_t*>(storage.Data());
  if (iv) std::memcpy(bytes, iv, 4);
  return Napi::Uint8Array::New(env, 4, storage, 0);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("openFile", Napi::Function::New(env, OpenFile));
  exports.Set("openFileWithIv", Napi::Function::New(env, OpenFileWithIv));
  exports.Set("openMemory", Napi::Function::New(env, OpenMemory));
  exports.Set("openMemoryWithIv", Napi::Function::New(env, OpenMemoryWithIv));
  exports.Set("createFile", Napi::Function::New(env, CreateFile));
  exports.Set("parseFile", Napi::Function::New(env, ParseFile));
  exports.Set("closeFile", Napi::Function::New(env, CloseFile));
  exports.Set("fileSaveToDisk", Napi::Function::New(env, FileSaveToDisk));
  exports.Set("fileName", Napi::Function::New(env, FileName));
  exports.Set("filePath", Napi::Function::New(env, FilePath));
  exports.Set("fileVersion", Napi::Function::New(env, FileVersion));
  exports.Set("fileMapleVersion", Napi::Function::New(env, FileMapleVersion));
  exports.Set("fileWzDirectory", Napi::Function::New(env, FileWzDirectory));
  exports.Set("fileIs64Bit",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return FileBool(i, wz_file_is_64bit);
              }));
  exports.Set("fileIsUnloaded",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return FileBool(i, wz_file_is_unloaded);
              }));
  exports.Set("fileVersionHash", Napi::Function::New(env, FileVersionHash));
  exports.Set("fileObjectFromPath",
              Napi::Function::New(env, FileObjectFromPath));

  exports.Set("dirName", Napi::Function::New(env, DirName));
  exports.Set("dirCountImages", Napi::Function::New(env, DirCountImages));
  exports.Set("dirCountDirectories",
              Napi::Function::New(env, DirCountDirectories));
  exports.Set("dirCountImagesTotal",
              Napi::Function::New(env, DirCountImagesTotal));
  exports.Set("dirGetImage", Napi::Function::New(env, DirGetImage));
  exports.Set("dirGetImageByName", Napi::Function::New(env, DirGetImageByName));
  exports.Set("dirGetDirectory", Napi::Function::New(env, DirGetDirectory));
  exports.Set("dirGetDirectoryByName",
              Napi::Function::New(env, DirGetDirectoryByName));
  exports.Set("dirCreateDirectory",
              Napi::Function::New(env, DirCreateDirectory));
  exports.Set("dirCreateImage", Napi::Function::New(env, DirCreateImage));
  exports.Set("dirRemoveDirectory",
              Napi::Function::New(env, DirRemoveDirectory));
  exports.Set("dirRemoveImage", Napi::Function::New(env, DirRemoveImage));
  exports.Set("dirBlockSize",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return DirInt(i, wz_dir_block_size);
              }));
  exports.Set("dirChecksum",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return DirInt(i, wz_dir_checksum);
              }));
  exports.Set("dirOffset",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return DirInt64(i, wz_dir_offset);
              }));

  exports.Set("imageName", Napi::Function::New(env, ImageName));
  exports.Set("imageParsed",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ImageBool(i, wz_image_is_parsed);
              }));
  exports.Set("imageChanged",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ImageBool(i, wz_image_is_changed);
              }));
  exports.Set("imageBlockSize",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ImageInt(i, wz_image_block_size);
              }));
  exports.Set("imageChecksum",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ImageInt(i, wz_image_checksum);
              }));
  exports.Set("imageOffset", Napi::Function::New(env, ImageOffset));
  exports.Set("imageIsLua",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ImageBool(i, wz_image_is_lua_wz_image);
              }));
  exports.Set("imageParse", Napi::Function::New(env, ImageParse));
  exports.Set("imageCountProperties",
              Napi::Function::New(env, ImageCountProperties));
  exports.Set("imageGetProperty", Napi::Function::New(env, ImageGetProperty));
  exports.Set("imageGetFromPath", Napi::Function::New(env, ImageGetFromPath));
  exports.Set("imageAddProperty", Napi::Function::New(env, ImageAddProperty));
  exports.Set("imageRemoveProperty",
              Napi::Function::New(env, ImageRemoveProperty));
  exports.Set("imageClearProperties",
              Napi::Function::New(env, ImageClearProperties));

  exports.Set("objectType", Napi::Function::New(env, ObjectType));
  exports.Set("objectName", Napi::Function::New(env, ObjectName));
  exports.Set("objectParent", Napi::Function::New(env, ObjectParent));
  exports.Set("objectFullPath", Napi::Function::New(env, ObjectFullPath));
  exports.Set("objectWzFileParent",
              Napi::Function::New(env, ObjectWzFileParent));
  exports.Set("objectTopMostDirectory",
              Napi::Function::New(env, ObjectTopMostDirectory));
  exports.Set("objectTopMostImage",
              Napi::Function::New(env, ObjectTopMostImage));
  exports.Set("objectAt", Napi::Function::New(env, ObjectAt));
  exports.Set("objectSetName", Napi::Function::New(env, ObjectSetName));
  exports.Set("objectRemove", Napi::Function::New(env, ObjectRemove));

  exports.Set("propType", Napi::Function::New(env, PropType));
  exports.Set("propIsRaw", Napi::Function::New(env, PropIsRaw));
  exports.Set("propIsVideo", Napi::Function::New(env, PropIsVideo));
  exports.Set("propName", Napi::Function::New(env, PropName));
  exports.Set("propCountChildren", Napi::Function::New(env, PropCountChildren));
  exports.Set("propGetChild", Napi::Function::New(env, PropGetChild));
  exports.Set("propGetChildByName",
              Napi::Function::New(env, PropGetChildByName));
  exports.Set("propGetFromPath", Napi::Function::New(env, PropGetFromPath));
  exports.Set("propGetInt", Napi::Function::New(env, PropGetInt));
  exports.Set("propGetShort", Napi::Function::New(env, PropGetShort));
  exports.Set("propGetLong", Napi::Function::New(env, PropGetLong));
  exports.Set("propGetFloat", Napi::Function::New(env, PropGetFloat));
  exports.Set("propGetDouble", Napi::Function::New(env, PropGetDouble));
  exports.Set("propGetString", Napi::Function::New(env, PropGetString));
  exports.Set("propGetBytes", Napi::Function::New(env, PropGetBytes));
  exports.Set("propLinked", Napi::Function::New(env, PropLinked));
  exports.Set("propertyCreateNull",
              Napi::Function::New(env, CreatePropertyNull));
  exports.Set("propertyCreateShort",
              Napi::Function::New(env, CreatePropertyShort));
  exports.Set("propertyCreateInt", Napi::Function::New(env, CreatePropertyInt));
  exports.Set("propertyCreateLong",
              Napi::Function::New(env, CreatePropertyLong));
  exports.Set("propertyCreateFloat",
              Napi::Function::New(env, CreatePropertyFloat));
  exports.Set("propertyCreateDouble",
              Napi::Function::New(env, CreatePropertyDouble));
  exports.Set("propertyCreateString",
              Napi::Function::New(env, CreatePropertyString));
  exports.Set("propertyCreateSub", Napi::Function::New(env, CreatePropertySub));
  exports.Set("propertyCreateVector",
              Napi::Function::New(env, CreatePropertyVector));
  exports.Set("propertyCreateUol", Napi::Function::New(env, CreatePropertyUol));
  exports.Set("propertyFree", Napi::Function::New(env, PropertyFree));
  exports.Set("propertyAddChild", Napi::Function::New(env, PropertyAddChild));
  exports.Set("propertyRemoveChild",
              Napi::Function::New(env, PropertyRemoveChild));
  exports.Set("propertyClearChildren",
              Napi::Function::New(env, PropertyClearChildren));

  exports.Set("intValue",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ScalarValue(i, wz_int_get_value);
              }));
  exports.Set("intSetValue", Napi::Function::New(env, SetIntValue));
  exports.Set(
      "shortValue",
      Napi::Function::New(env, [](const Napi::CallbackInfo& i) -> Napi::Value {
        Napi::Env env = i.Env();
        int16_t raw = 0;
        wz_property prop = FromHandle<wz_property>(i[0]);
        WZ_NODE_API_CALL(env, wz_short_get_value(prop, &raw));
        int32_t v = static_cast<int32_t>(raw);
        return Napi::Number::New(env, v);
      }));
  exports.Set("shortSetValue", Napi::Function::New(env, SetShortValue));
  exports.Set("longValue", Napi::Function::New(env, LongValue));
  exports.Set("longSetValue", Napi::Function::New(env, SetLongValue));
  exports.Set("floatValue", Napi::Function::New(env, FloatValue));
  exports.Set("floatSetValue", Napi::Function::New(env, SetFloatValue));
  exports.Set("doubleValue", Napi::Function::New(env, DoubleValue));
  exports.Set("doubleSetValue", Napi::Function::New(env, SetDoubleValue));
  exports.Set("stringValue", Napi::Function::New(env, StringValue));
  exports.Set("stringSetValue", Napi::Function::New(env, SetStringValue));

  exports.Set("pngWidth", Napi::Function::New(env, PngWidth));
  exports.Set("pngHeight", Napi::Function::New(env, PngHeight));
  exports.Set("pngFormat", Napi::Function::New(env, PngFormat));
  exports.Set("pngListWzUsed", Napi::Function::New(env, PngListWzUsed));
  exports.Set("pngImage",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ReadPngBytes(i, wz_png_get_image);
              }));
  exports.Set("pngCompressedBytes",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ReadPngBytes(i, wz_png_get_compressed_bytes);
              }));

  exports.Set("canvasPng", Napi::Function::New(env, CanvasPng));
  exports.Set("canvasContainsInlink",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return CanvasBool(i, wz_canvas_contains_inlink);
              }));
  exports.Set("canvasContainsOutlink",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return CanvasBool(i, wz_canvas_contains_outlink);
              }));
  exports.Set("canvasLinked", Napi::Function::New(env, CanvasLinked));

  exports.Set("uolValue", Napi::Function::New(env, UolValue));
  exports.Set("uolSetValue", Napi::Function::New(env, SetUolValue));
  exports.Set("uolLinkValue", Napi::Function::New(env, UolLinkValue));
  exports.Set("vectorX", Napi::Function::New(env, VectorX));
  exports.Set("vectorY", Napi::Function::New(env, VectorY));
  exports.Set("luaData", Napi::Function::New(env, LuaData));
  exports.Set("luaString", Napi::Function::New(env, LuaString));
  exports.Set("binaryData", Napi::Function::New(env, BinaryData));
  exports.Set("binaryWav", Napi::Function::New(env, BinaryWav));
  exports.Set("binaryLength",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return BinaryInt(i, wz_binary_get_length);
              }));
  exports.Set("binaryFrequency",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return BinaryInt(i, wz_binary_get_frequency);
              }));
  exports.Set("binaryType", Napi::Function::New(env, BinaryType));
  exports.Set("binaryHeaderEncrypted", Napi::Function::New(env, BinaryBool));
  exports.Set("rawData", Napi::Function::New(env, RawData));
  exports.Set("rawType", Napi::Function::New(env, RawType));
  exports.Set("videoData", Napi::Function::New(env, VideoData));
  exports.Set("detectMapleVersion",
              Napi::Function::New(env, ToolDetectMapleVersion));
  exports.Set("ivForVersion", Napi::Function::New(env, ToolIvForVersion));

  return exports;
}

}  // namespace

NODE_API_MODULE(libwz, Init)

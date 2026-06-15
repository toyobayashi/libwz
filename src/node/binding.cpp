#include <napi.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "wz/wz_api.h"

namespace {

template <typename T>
T FromHandle(const Napi::Value& value) {
  bool lossless = false;
  uint64_t raw = value.As<Napi::BigInt>().Uint64Value(&lossless);
  return reinterpret_cast<T>(static_cast<uintptr_t>(raw));
}

template <typename T>
Napi::BigInt ToHandle(Napi::Env env, T ptr) {
  return Napi::BigInt::New(
      env, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr)));
}

void ThrowIfError(Napi::Env env) {
  if (wz_get_last_error() == WZ_ERROR_NONE) return;
  const char* msg = wz_get_last_error_message();
  Napi::Error::New(env, msg ? msg : "libwz error").ThrowAsJavaScriptException();
}

Napi::Value NullableString(Napi::Env env, const char* s) {
  ThrowIfError(env);
  if (!s) return env.Null();
  return Napi::String::New(env, s);
}

Napi::Value NullableObject(Napi::Env env, wz_object obj) {
  ThrowIfError(env);
  if (!obj) return env.Null();
  Napi::Object result = Napi::Object::New(env);
  result.Set("type", static_cast<int>(wz_get_object_type(obj)));
  ThrowIfError(env);
  result.Set("ptr", ToHandle(env, obj));
  return result;
}

template <typename SizeFn>
Napi::Value ReadBytes(const Napi::CallbackInfo& info, SizeFn fn) {
  Napi::Env env = info.Env();
  wz_property prop = FromHandle<wz_property>(info[0]);
  size_t size = fn(prop, nullptr, 0);
  if (wz_get_last_error() != WZ_ERROR_NONE) {
    ThrowIfError(env);
    return env.Null();
  }
  Napi::ArrayBuffer storage = Napi::ArrayBuffer::New(env, size);
  auto bytes = static_cast<uint8_t*>(storage.Data());
  if (size > 0) {
    fn(prop, bytes, size);
    ThrowIfError(env);
  }
  return Napi::Uint8Array::New(env, size, storage, 0);
}

template <typename SizeFn>
Napi::Value ReadPngBytes(const Napi::CallbackInfo& info, SizeFn fn) {
  Napi::Env env = info.Env();
  wz_png_property png = FromHandle<wz_png_property>(info[0]);
  size_t size = fn(png, nullptr, 0);
  if (wz_get_last_error() != WZ_ERROR_NONE) {
    ThrowIfError(env);
    return env.Null();
  }
  Napi::ArrayBuffer storage = Napi::ArrayBuffer::New(env, size);
  auto bytes = static_cast<uint8_t*>(storage.Data());
  if (size > 0) {
    fn(png, bytes, size);
    ThrowIfError(env);
  }
  return Napi::Uint8Array::New(env, size, storage, 0);
}

Napi::Value OpenFile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  int gameVersion = info[1].As<Napi::Number>().Int32Value();
  int version = info[2].As<Napi::Number>().Int32Value();
  wz_file file = wz_open_file(path.c_str(),
                              static_cast<short>(gameVersion),
                              static_cast<wz_maple_version>(version));
  ThrowIfError(env);
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value OpenFileWithIv(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  Napi::Uint8Array iv = info[1].As<Napi::Uint8Array>();
  uint8_t bytes[4] = {0, 0, 0, 0};
  std::memcpy(bytes, iv.Data(), iv.ByteLength() < 4 ? iv.ByteLength() : 4);
  wz_file file = wz_open_file_with_iv(path.c_str(), bytes);
  ThrowIfError(env);
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value ParseFile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto status = wz_parse(FromHandle<wz_file>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, static_cast<int>(status));
}

Napi::Value CloseFile(const Napi::CallbackInfo& info) {
  wz_close_file(FromHandle<wz_file>(info[0]));
  return info.Env().Undefined();
}

Napi::Value FileName(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(), wz_file_name(FromHandle<wz_file>(info[0])));
}

Napi::Value FilePath(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(), wz_file_path(FromHandle<wz_file>(info[0])));
}

Napi::Value FileVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_file_version(FromHandle<wz_file>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value FileMapleVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_file_maple_version(FromHandle<wz_file>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, static_cast<int>(v));
}

Napi::Value FileWzDirectory(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto dir = wz_file_get_wz_directory(FromHandle<wz_file>(info[0]));
  ThrowIfError(env);
  return dir ? ToHandle(env, dir) : env.Null();
}

Napi::Value FileBool(const Napi::CallbackInfo& info, int (*fn)(wz_file)) {
  Napi::Env env = info.Env();
  bool result = fn(FromHandle<wz_file>(info[0])) != 0;
  ThrowIfError(env);
  return Napi::Boolean::New(env, result);
}

Napi::Value FileVersionHash(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_file_version_hash(FromHandle<wz_file>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value FileObjectFromPath(const Napi::CallbackInfo& info) {
  std::string path = info[1].As<Napi::String>().Utf8Value();
  bool check = info[2].As<Napi::Boolean>().Value();
  auto obj = wz_file_get_object_from_path(
      FromHandle<wz_file>(info[0]), path.c_str(), check ? 1 : 0);
  return NullableObject(info.Env(), obj);
}

Napi::Value DirName(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(), wz_dir_name(FromHandle<wz_dir>(info[0])));
}

Napi::Value DirCountImages(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_dir_count_images(FromHandle<wz_dir>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value DirCountDirectories(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_dir_count_directories(FromHandle<wz_dir>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value DirCountImagesTotal(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_dir_count_images_total(FromHandle<wz_dir>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value DirGetImage(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int index = info[1].As<Napi::Number>().Int32Value();
  auto image = wz_dir_get_image(FromHandle<wz_dir>(info[0]), index);
  ThrowIfError(env);
  return image ? ToHandle(env, image) : env.Null();
}

Napi::Value DirGetImageByName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  auto image =
      wz_dir_get_image_by_name(FromHandle<wz_dir>(info[0]), name.c_str());
  ThrowIfError(env);
  return image ? ToHandle(env, image) : env.Null();
}

Napi::Value DirGetDirectory(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int index = info[1].As<Napi::Number>().Int32Value();
  auto dir = wz_dir_get_directory(FromHandle<wz_dir>(info[0]), index);
  ThrowIfError(env);
  return dir ? ToHandle(env, dir) : env.Null();
}

Napi::Value DirGetDirectoryByName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  auto dir =
      wz_dir_get_directory_by_name(FromHandle<wz_dir>(info[0]), name.c_str());
  ThrowIfError(env);
  return dir ? ToHandle(env, dir) : env.Null();
}

Napi::Value DirInt64(const Napi::CallbackInfo& info, int64_t (*fn)(wz_dir)) {
  Napi::Env env = info.Env();
  auto v = fn(FromHandle<wz_dir>(info[0]));
  ThrowIfError(env);
  return Napi::BigInt::New(env, v);
}

Napi::Value DirInt(const Napi::CallbackInfo& info, int (*fn)(wz_dir)) {
  Napi::Env env = info.Env();
  int v = fn(FromHandle<wz_dir>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value ImageName(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(),
                        wz_image_name(FromHandle<wz_image>(info[0])));
}

Napi::Value ImageBool(const Napi::CallbackInfo& info, int (*fn)(wz_image)) {
  Napi::Env env = info.Env();
  bool result = fn(FromHandle<wz_image>(info[0])) != 0;
  ThrowIfError(env);
  return Napi::Boolean::New(env, result);
}

Napi::Value ImageInt(const Napi::CallbackInfo& info, int (*fn)(wz_image)) {
  Napi::Env env = info.Env();
  int v = fn(FromHandle<wz_image>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value ImageOffset(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_image_offset(FromHandle<wz_image>(info[0]));
  ThrowIfError(env);
  return Napi::BigInt::New(env, v);
}

Napi::Value ImageParse(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  wz_image_parse(FromHandle<wz_image>(info[0]));
  ThrowIfError(env);
  return env.Undefined();
}

Napi::Value ImageCountProperties(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_image_count_properties(FromHandle<wz_image>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value ImageGetProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int index = info[1].As<Napi::Number>().Int32Value();
  auto prop = wz_image_get_property(FromHandle<wz_image>(info[0]), index);
  ThrowIfError(env);
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value ImageGetFromPath(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[1].As<Napi::String>().Utf8Value();
  auto prop =
      wz_image_get_from_path(FromHandle<wz_image>(info[0]), path.c_str());
  ThrowIfError(env);
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value ObjectType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto type = wz_get_object_type(FromHandle<wz_object>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, static_cast<int>(type));
}

Napi::Value ObjectName(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(),
                        wz_object_name(FromHandle<wz_object>(info[0])));
}

Napi::Value ObjectParent(const Napi::CallbackInfo& info) {
  return NullableObject(info.Env(),
                        wz_object_get_parent(FromHandle<wz_object>(info[0])));
}

Napi::Value ObjectFullPath(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(),
                        wz_object_full_path(FromHandle<wz_object>(info[0])));
}

Napi::Value ObjectWzFileParent(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto file = wz_object_get_wz_file_parent(FromHandle<wz_object>(info[0]));
  ThrowIfError(env);
  return file ? ToHandle(env, file) : env.Null();
}

Napi::Value ObjectTopMostDirectory(const Napi::CallbackInfo& info) {
  return NullableObject(
      info.Env(),
      wz_object_get_top_most_wz_directory(FromHandle<wz_object>(info[0])));
}

Napi::Value ObjectTopMostImage(const Napi::CallbackInfo& info) {
  return NullableObject(
      info.Env(),
      wz_object_get_top_most_wz_image(FromHandle<wz_object>(info[0])));
}

Napi::Value ObjectAt(const Napi::CallbackInfo& info) {
  std::string name = info[1].As<Napi::String>().Utf8Value();
  return NullableObject(
      info.Env(), wz_object_at(FromHandle<wz_object>(info[0]), name.c_str()));
}

Napi::Value PropType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto type = wz_property_get_type(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, static_cast<int>(type));
}

Napi::Value PropIsRaw(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  bool result = wz_property_is_raw_data(FromHandle<wz_property>(info[0])) != 0;
  ThrowIfError(env);
  return Napi::Boolean::New(env, result);
}

Napi::Value PropIsVideo(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  bool result = wz_property_is_video(FromHandle<wz_property>(info[0])) != 0;
  ThrowIfError(env);
  return Napi::Boolean::New(env, result);
}

Napi::Value PropName(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(),
                        wz_property_name(FromHandle<wz_property>(info[0])));
}

Napi::Value PropCountChildren(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_property_count_children(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value PropGetChild(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int index = info[1].As<Napi::Number>().Int32Value();
  auto prop = wz_property_get_child(FromHandle<wz_property>(info[0]), index);
  ThrowIfError(env);
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value PropGetChildByName(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string name = info[1].As<Napi::String>().Utf8Value();
  auto prop = wz_property_get_child_by_name(FromHandle<wz_property>(info[0]),
                                            name.c_str());
  ThrowIfError(env);
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value PropGetFromPath(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[1].As<Napi::String>().Utf8Value();
  auto prop =
      wz_property_get_from_path(FromHandle<wz_property>(info[0]), path.c_str());
  ThrowIfError(env);
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value PropGetInt(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_property_get_int(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value PropGetShort(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_property_get_short(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value PropGetLong(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_property_get_long(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::BigInt::New(env, v);
}

Napi::Value PropGetFloat(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_property_get_float(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value PropGetDouble(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_property_get_double(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value PropGetString(const Napi::CallbackInfo& info) {
  return NullableString(
      info.Env(), wz_property_get_string(FromHandle<wz_property>(info[0])));
}

Napi::Value PropGetBytes(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_property_get_bytes);
}

Napi::Value PropLinked(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto prop = wz_property_get_linked_wz_image_property(
      FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value ScalarValue(const Napi::CallbackInfo& info,
                        int32_t (*fn)(wz_property)) {
  Napi::Env env = info.Env();
  auto v = fn(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value LongValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_long_get_value(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::BigInt::New(env, v);
}

Napi::Value FloatValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_float_get_value(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value DoubleValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_double_get_value(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value StringValue(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(),
                        wz_string_get_value(FromHandle<wz_property>(info[0])));
}

Napi::Value SetIntValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int value = info[1].As<Napi::Number>().Int32Value();
  wz_int_set_value(FromHandle<wz_property>(info[0]), value);
  ThrowIfError(env);
  return env.Undefined();
}

Napi::Value PngWidth(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_png_width(FromHandle<wz_png_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value PngHeight(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_png_height(FromHandle<wz_png_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value PngFormat(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_png_format(FromHandle<wz_png_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value PngListWzUsed(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  bool v = wz_png_is_list_wz_used(FromHandle<wz_png_property>(info[0])) != 0;
  ThrowIfError(env);
  return Napi::Boolean::New(env, v);
}

Napi::Value CanvasPng(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto png = wz_canvas_get_png(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return png ? ToHandle(env, png) : env.Null();
}

Napi::Value CanvasBool(const Napi::CallbackInfo& info, int (*fn)(wz_property)) {
  Napi::Env env = info.Env();
  bool v = fn(FromHandle<wz_property>(info[0])) != 0;
  ThrowIfError(env);
  return Napi::Boolean::New(env, v);
}

Napi::Value CanvasLinked(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto prop =
      wz_canvas_get_linked_wz_image_property(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return prop ? ToHandle(env, prop) : env.Null();
}

Napi::Value UolValue(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(),
                        wz_uol_get_value(FromHandle<wz_property>(info[0])));
}

Napi::Value UolLinkValue(const Napi::CallbackInfo& info) {
  return NullableObject(
      info.Env(), wz_uol_get_link_value(FromHandle<wz_property>(info[0])));
}

Napi::Value VectorX(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_vector_get_x(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value VectorY(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_vector_get_y(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value LuaData(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_lua_get_data);
}

Napi::Value LuaString(const Napi::CallbackInfo& info) {
  return NullableString(info.Env(),
                        wz_lua_get_string(FromHandle<wz_property>(info[0])));
}

Napi::Value BinaryData(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_binary_get_data);
}

Napi::Value BinaryWav(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_binary_get_wav_playback);
}

Napi::Value BinaryInt(const Napi::CallbackInfo& info, int (*fn)(wz_property)) {
  Napi::Env env = info.Env();
  int v = fn(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value BinaryBool(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  bool v = wz_binary_is_header_encrypted(FromHandle<wz_property>(info[0])) != 0;
  ThrowIfError(env);
  return Napi::Boolean::New(env, v);
}

Napi::Value BinaryType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  auto v = wz_binary_get_type(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, static_cast<int>(v));
}

Napi::Value RawData(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_rawdata_get_data);
}

Napi::Value RawType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int v = wz_rawdata_get_type(FromHandle<wz_property>(info[0]));
  ThrowIfError(env);
  return Napi::Number::New(env, v);
}

Napi::Value VideoData(const Napi::CallbackInfo& info) {
  return ReadBytes(info, wz_video_get_data);
}

Napi::Value ToolDetectMapleVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>().Utf8Value();
  short outVersion = 0;
  auto version = wz_detect_maple_version(path.c_str(), &outVersion);
  ThrowIfError(env);
  Napi::Object result = Napi::Object::New(env);
  result.Set("mapleVersion", static_cast<int>(version));
  result.Set("version", outVersion);
  return result;
}

Napi::Value ToolIvForVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int version = info[0].As<Napi::Number>().Int32Value();
  const uint8_t* iv =
      wz_get_iv_for_version(static_cast<wz_maple_version>(version));
  ThrowIfError(env);
  Napi::ArrayBuffer storage = Napi::ArrayBuffer::New(env, 4);
  auto bytes = static_cast<uint8_t*>(storage.Data());
  if (iv) std::memcpy(bytes, iv, 4);
  return Napi::Uint8Array::New(env, 4, storage, 0);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("openFile", Napi::Function::New(env, OpenFile));
  exports.Set("openFileWithIv", Napi::Function::New(env, OpenFileWithIv));
  exports.Set("parseFile", Napi::Function::New(env, ParseFile));
  exports.Set("closeFile", Napi::Function::New(env, CloseFile));
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

  exports.Set("intValue",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ScalarValue(i, wz_int_get_value);
              }));
  exports.Set("intSetValue", Napi::Function::New(env, SetIntValue));
  exports.Set("shortValue",
              Napi::Function::New(env, [](const Napi::CallbackInfo& i) {
                return ScalarValue(i, [](wz_property p) {
                  return static_cast<int32_t>(wz_short_get_value(p));
                });
              }));
  exports.Set("longValue", Napi::Function::New(env, LongValue));
  exports.Set("floatValue", Napi::Function::New(env, FloatValue));
  exports.Set("doubleValue", Napi::Function::New(env, DoubleValue));
  exports.Set("stringValue", Napi::Function::New(env, StringValue));

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

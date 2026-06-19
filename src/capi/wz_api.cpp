#include "wz/wz_api.h"

#include <array>
#include <cstring>
#include <memory>
#include <optional>
#include <string>

#include "wz/wz.h"

static thread_local std::string g_last_error_msg;
static thread_local wz_last_error_info g_last_error_info = {WZ_ERROR_NONE, ""};

static inline wz_error_code wz_clear_last_error() {
  g_last_error_msg.clear();
  g_last_error_info.code = WZ_ERROR_NONE;
  g_last_error_info.message = g_last_error_msg.c_str();
  return WZ_ERROR_NONE;
}

static_assert(static_cast<int>(wz::ErrorCode::Ok) == WZ_ERROR_NONE);
static_assert(static_cast<int>(wz::ErrorCode::NotImplemented) ==
              WZ_ERROR_NOT_IMPLEMENTED);
static_assert(static_cast<int>(wz::ErrorCode::InvalidArgument) ==
              WZ_ERROR_INVALID_ARGUMENT);
static_assert(static_cast<int>(wz::ErrorCode::ParseError) ==
              WZ_ERROR_PARSE_ERROR);
static_assert(static_cast<int>(wz::ErrorCode::IoError) == WZ_ERROR_IO_ERROR);
static_assert(static_cast<int>(wz::ErrorCode::DataError) ==
              WZ_ERROR_DATA_ERROR);
static_assert(static_cast<int>(wz::ErrorCode::NotFound) == WZ_ERROR_NOT_FOUND);

static inline wz_error_code wz_set_last_error(wz_error_code code,
                                              const std::string& msg) {
  g_last_error_msg = msg;
  g_last_error_info.code = code;
  g_last_error_info.message = g_last_error_msg.c_str();
  return code;
}

static wz_error_code set_error_null(const char* fn) {
  return wz_set_last_error(WZ_ERROR_NULL_HANDLE,
                           std::string(fn) + ": null handle");
}

static wz_error_code set_error_invalid_arg(const char* fn,
                                           const char* arg_name) {
  return wz_set_last_error(WZ_ERROR_INVALID_ARGUMENT,
                           std::string(fn) + ": " + arg_name + " is null");
}

static wz_error_code set_error_wrong_type(const char* fn) {
  return wz_set_last_error(WZ_ERROR_WRONG_TYPE,
                           std::string(fn) + ": wrong property type");
}

static wz_error_code set_error_idx(const char* fn, int idx, size_t size) {
  return wz_set_last_error(WZ_ERROR_INDEX_OUT_OF_RANGE,
                           std::string(fn) + ": index " + std::to_string(idx) +
                               " out of range [0, " + std::to_string(size) +
                               ")");
}

template <typename T>
static wz_error_code init_out(const char* fn, T* out) {
  if (!out) return set_error_invalid_arg(fn, "out");
  *out = T{};
  return WZ_ERROR_NONE;
}

static wz_error_code result_void(const wz::Result<void>& result) {
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  return wz_clear_last_error();
}

static wz_error_code result_bool(const wz::Result<bool>& result,
                                 int* out_value) {
  if (auto ec = init_out("result_bool", out_value); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  *out_value = result.value() ? 1 : 0;
  return wz_clear_last_error();
}

template <typename T>
static wz_error_code result_error(const wz::Result<T>& result) {
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  return WZ_ERROR_NONE;
}

static wz::WzImageProperty* unwrap_prop(wz_property p) {
  return reinterpret_cast<wz::WzImageProperty*>(p);
}

static wz_property wrap_prop(wz::WzImageProperty* p) {
  return reinterpret_cast<wz_property>(p);
}

static wz::WzFile* unwrap_file(wz_file f) {
  return reinterpret_cast<wz::WzFile*>(f);
}

static wz::WzDirectory* unwrap_dir(wz_dir d) {
  return reinterpret_cast<wz::WzDirectory*>(d);
}

static wz::WzImage* unwrap_image(wz_image i) {
  return reinterpret_cast<wz::WzImage*>(i);
}

static wz::WzObject* unwrap_object(wz_object o) {
  return reinterpret_cast<wz::WzObject*>(o);
}

extern "C" {

wz_error_code wz_get_last_error_info(const wz_last_error_info** info) {
  if (!info) {
    return set_error_invalid_arg("wz_get_last_error_info", "info");
  }
  *info = &g_last_error_info;
  return WZ_ERROR_NONE;
}

wz_error_code wz_open_file(const char* file_path,
                           int16_t game_version,
                           wz_maple_version version,
                           wz_file* out_file) {
  if (auto ec = init_out("wz_open_file", out_file); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file_path) return set_error_invalid_arg("wz_open_file", "file_path");
  auto* f = new wz::WzFile(
      file_path, game_version, static_cast<wz::WzMapleVersion>(version));
  *out_file = reinterpret_cast<wz_file>(f);
  return wz_clear_last_error();
}

wz_error_code wz_create_file(int16_t game_version,
                             wz_maple_version version,
                             wz_file* out_file) {
  if (auto ec = init_out("wz_create_file", out_file); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* f =
      new wz::WzFile(game_version, static_cast<wz::WzMapleVersion>(version));
  *out_file = reinterpret_cast<wz_file>(f);
  return wz_clear_last_error();
}

wz_error_code wz_open_file_with_iv(const char* file_path,
                                   const uint8_t iv[4],
                                   wz_file* out_file) {
  if (auto ec = init_out("wz_open_file_with_iv", out_file);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file_path) {
    return set_error_invalid_arg("wz_open_file_with_iv", "file_path");
  }
  if (!iv) return set_error_invalid_arg("wz_open_file_with_iv", "iv");
  std::array<uint8_t, 4> iv_arr = {iv[0], iv[1], iv[2], iv[3]};
  auto* f = new wz::WzFile(file_path, iv_arr);
  *out_file = reinterpret_cast<wz_file>(f);
  return wz_clear_last_error();
}

wz_error_code wz_parse(wz_file file, wz_parse_status* out_status) {
  if (auto ec = init_out("wz_parse", out_status); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_parse");
  auto status = reinterpret_cast<wz::WzFile*>(file)->ParseWzFile();
  *out_status = static_cast<wz_parse_status>(status);
  if (status != wz::WzFileParseStatus::Success) {
    return wz_set_last_error(WZ_ERROR_PARSE_ERROR,
                             wz::GetErrorDescription(status));
  }
  return wz_clear_last_error();
}

wz_error_code wz_close_file(wz_file file) {
  if (!file) return wz_clear_last_error();
  delete unwrap_file(file);
  return wz_clear_last_error();
}

wz_error_code wz_file_save_to_disk(wz_file file, const char* file_path) {
  if (!file) return set_error_null("wz_file_save_to_disk");
  if (!file_path) {
    return set_error_invalid_arg("wz_file_save_to_disk", "file_path");
  }
  return result_void(unwrap_file(file)->SaveToDisk(file_path));
}

wz_error_code wz_file_save_to_disk_ex(wz_file file,
                                      const char* file_path,
                                      int has_save_as_64bit,
                                      int save_as_64bit,
                                      wz_maple_version version) {
  if (!file) return set_error_null("wz_file_save_to_disk_ex");
  if (!file_path) {
    return set_error_invalid_arg("wz_file_save_to_disk_ex", "file_path");
  }
  std::optional<bool> save_as_64;
  if (has_save_as_64bit) save_as_64 = save_as_64bit != 0;
  return result_void(unwrap_file(file)->SaveToDisk(
      file_path, save_as_64, static_cast<wz::WzMapleVersion>(version)));
}

wz_error_code wz_file_name(wz_file file, const char** out_name) {
  if (auto ec = init_out("wz_file_name", out_name); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_name");
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzFile*>(file)->Name();
  *out_name = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_file_path(wz_file file, const char** out_path) {
  if (auto ec = init_out("wz_file_path", out_path); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_path");
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzFile*>(file)->FilePath();
  *out_path = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_file_version(wz_file file, int16_t* out_version) {
  if (auto ec = init_out("wz_file_version", out_version); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_version");
  *out_version = reinterpret_cast<wz::WzFile*>(file)->Version();
  return wz_clear_last_error();
}

wz_error_code wz_file_maple_version(wz_file file,
                                    wz_maple_version* out_version) {
  if (auto ec = init_out("wz_file_maple_version", out_version);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_maple_version");
  *out_version = static_cast<wz_maple_version>(
      reinterpret_cast<wz::WzFile*>(file)->MapleVersion());
  return wz_clear_last_error();
}

wz_error_code wz_file_get_wz_directory(wz_file file, wz_dir* out_dir) {
  if (auto ec = init_out("wz_file_get_wz_directory", out_dir);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_get_wz_directory");
  *out_dir = reinterpret_cast<wz_dir>(
      reinterpret_cast<wz::WzFile*>(file)->GetWzDirectory());
  return wz_clear_last_error();
}

wz_error_code wz_file_is_64bit(wz_file file, int* out_is_64bit) {
  if (auto ec = init_out("wz_file_is_64bit", out_is_64bit);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_is_64bit");
  *out_is_64bit = reinterpret_cast<wz::WzFile*>(file)->Is64BitWzFile() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_file_is_unloaded(wz_file file, int* out_is_unloaded) {
  if (auto ec = init_out("wz_file_is_unloaded", out_is_unloaded);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_is_unloaded");
  *out_is_unloaded = reinterpret_cast<wz::WzFile*>(file)->IsUnloaded() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_file_version_hash(wz_file file, uint32_t* out_hash) {
  if (auto ec = init_out("wz_file_version_hash", out_hash);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_version_hash");
  *out_hash = reinterpret_cast<wz::WzFile*>(file)->VersionHash();
  return wz_clear_last_error();
}

wz_error_code wz_file_get_object_from_path(wz_file file,
                                           const char* path,
                                           int check_first_directory_name,
                                           wz_object* out_object) {
  if (auto ec = init_out("wz_file_get_object_from_path", out_object);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file) return set_error_null("wz_file_get_object_from_path");
  if (!path) {
    return set_error_invalid_arg("wz_file_get_object_from_path", "path");
  }
  *out_object = reinterpret_cast<wz_object>(
      reinterpret_cast<wz::WzFile*>(file)->GetObjectFromPath(
          path, check_first_directory_name != 0));
  return wz_clear_last_error();
}

wz_error_code wz_dir_name(wz_dir dir, const char** out_name) {
  if (auto ec = init_out("wz_dir_name", out_name); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_name");
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzDirectory*>(dir)->Name();
  *out_name = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_dir_count_images(wz_dir dir, int* out_count) {
  if (auto ec = init_out("wz_dir_count_images", out_count);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_count_images");
  *out_count = static_cast<int>(
      reinterpret_cast<wz::WzDirectory*>(dir)->WzImages().size());
  return wz_clear_last_error();
}

wz_error_code wz_dir_count_directories(wz_dir dir, int* out_count) {
  if (auto ec = init_out("wz_dir_count_directories", out_count);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_count_directories");
  *out_count = static_cast<int>(
      reinterpret_cast<wz::WzDirectory*>(dir)->WzDirectories().size());
  return wz_clear_last_error();
}

wz_error_code wz_dir_count_images_total(wz_dir dir, int* out_count) {
  if (auto ec = init_out("wz_dir_count_images_total", out_count);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_count_images_total");
  *out_count = reinterpret_cast<wz::WzDirectory*>(dir)->CountImages();
  return wz_clear_last_error();
}

wz_error_code wz_dir_get_image(wz_dir dir, int index, wz_image* out_image) {
  if (auto ec = init_out("wz_dir_get_image", out_image); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_get_image");
  auto* d = reinterpret_cast<wz::WzDirectory*>(dir);
  size_t sz = d->WzImages().size();
  if (index < 0 || index >= static_cast<int>(sz)) {
    return set_error_idx("wz_dir_get_image", index, sz);
  }
  *out_image =
      reinterpret_cast<wz_image>(d->WzImages()[static_cast<size_t>(index)]);
  return wz_clear_last_error();
}

wz_error_code wz_dir_get_image_by_name(wz_dir dir,
                                       const char* name,
                                       wz_image* out_image) {
  if (auto ec = init_out("wz_dir_get_image_by_name", out_image);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_get_image_by_name");
  if (!name) return set_error_invalid_arg("wz_dir_get_image_by_name", "name");
  *out_image = reinterpret_cast<wz_image>(
      reinterpret_cast<wz::WzDirectory*>(dir)->GetImageByName(name));
  return wz_clear_last_error();
}

wz_error_code wz_dir_get_directory(wz_dir dir, int index, wz_dir* out_dir) {
  if (auto ec = init_out("wz_dir_get_directory", out_dir);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_get_directory");
  auto* d = reinterpret_cast<wz::WzDirectory*>(dir);
  size_t sz = d->WzDirectories().size();
  if (index < 0 || index >= static_cast<int>(sz)) {
    return set_error_idx("wz_dir_get_directory", index, sz);
  }
  *out_dir =
      reinterpret_cast<wz_dir>(d->WzDirectories()[static_cast<size_t>(index)]);
  return wz_clear_last_error();
}

wz_error_code wz_dir_get_directory_by_name(wz_dir dir,
                                           const char* name,
                                           wz_dir* out_dir) {
  if (auto ec = init_out("wz_dir_get_directory_by_name", out_dir);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_get_directory_by_name");
  if (!name) {
    return set_error_invalid_arg("wz_dir_get_directory_by_name", "name");
  }
  *out_dir = reinterpret_cast<wz_dir>(
      reinterpret_cast<wz::WzDirectory*>(dir)->GetDirectoryByName(name));
  return wz_clear_last_error();
}

wz_error_code wz_dir_create_directory(wz_dir dir,
                                      const char* name,
                                      wz_dir* out_dir) {
  if (auto ec = init_out("wz_dir_create_directory", out_dir);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_create_directory");
  if (!name) return set_error_invalid_arg("wz_dir_create_directory", "name");
  auto result = unwrap_dir(dir)->CreateDirectory(name);
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  *out_dir = reinterpret_cast<wz_dir>(result.value());
  return wz_clear_last_error();
}

wz_error_code wz_dir_create_image(wz_dir dir,
                                  const char* name,
                                  wz_image* out_image) {
  if (auto ec = init_out("wz_dir_create_image", out_image);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_create_image");
  if (!name) return set_error_invalid_arg("wz_dir_create_image", "name");
  auto result = unwrap_dir(dir)->CreateImage(name);
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  *out_image = reinterpret_cast<wz_image>(result.value());
  return wz_clear_last_error();
}

wz_error_code wz_dir_remove_directory(wz_dir dir, wz_dir child) {
  if (!dir) return set_error_null("wz_dir_remove_directory");
  if (!child) return set_error_null("wz_dir_remove_directory");
  return result_void(unwrap_dir(dir)->TryRemoveDirectory(unwrap_dir(child)));
}

wz_error_code wz_dir_remove_image(wz_dir dir, wz_image child) {
  if (!dir) return set_error_null("wz_dir_remove_image");
  if (!child) return set_error_null("wz_dir_remove_image");
  return result_void(unwrap_dir(dir)->TryRemoveImage(unwrap_image(child)));
}

wz_error_code wz_dir_block_size(wz_dir dir, int* out_block_size) {
  if (auto ec = init_out("wz_dir_block_size", out_block_size);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_block_size");
  *out_block_size = reinterpret_cast<wz::WzDirectory*>(dir)->BlockSize();
  return wz_clear_last_error();
}

wz_error_code wz_dir_checksum(wz_dir dir, int* out_checksum) {
  if (auto ec = init_out("wz_dir_checksum", out_checksum);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_checksum");
  *out_checksum = reinterpret_cast<wz::WzDirectory*>(dir)->Checksum();
  return wz_clear_last_error();
}

wz_error_code wz_dir_offset(wz_dir dir, int64_t* out_offset) {
  if (auto ec = init_out("wz_dir_offset", out_offset); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!dir) return set_error_null("wz_dir_offset");
  *out_offset = reinterpret_cast<wz::WzDirectory*>(dir)->Offset();
  return wz_clear_last_error();
}

wz_error_code wz_image_name(wz_image img, const char** out_name) {
  if (auto ec = init_out("wz_image_name", out_name); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_name");
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzImage*>(img)->Name();
  *out_name = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_image_is_parsed(wz_image img, int* out_is_parsed) {
  if (auto ec = init_out("wz_image_is_parsed", out_is_parsed);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_is_parsed");
  *out_is_parsed = reinterpret_cast<wz::WzImage*>(img)->Parsed() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_image_is_changed(wz_image img, int* out_is_changed) {
  if (auto ec = init_out("wz_image_is_changed", out_is_changed);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_is_changed");
  *out_is_changed = reinterpret_cast<wz::WzImage*>(img)->Changed() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_image_block_size(wz_image img, int* out_block_size) {
  if (auto ec = init_out("wz_image_block_size", out_block_size);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_block_size");
  *out_block_size = reinterpret_cast<wz::WzImage*>(img)->BlockSize();
  return wz_clear_last_error();
}

wz_error_code wz_image_checksum(wz_image img, int* out_checksum) {
  if (auto ec = init_out("wz_image_checksum", out_checksum);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_checksum");
  *out_checksum = reinterpret_cast<wz::WzImage*>(img)->Checksum();
  return wz_clear_last_error();
}

wz_error_code wz_image_offset(wz_image img, int64_t* out_offset) {
  if (auto ec = init_out("wz_image_offset", out_offset); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_offset");
  *out_offset = reinterpret_cast<wz::WzImage*>(img)->Offset();
  return wz_clear_last_error();
}

wz_error_code wz_image_is_lua_wz_image(wz_image img, int* out_is_lua) {
  if (auto ec = init_out("wz_image_is_lua_wz_image", out_is_lua);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_is_lua_wz_image");
  *out_is_lua = reinterpret_cast<wz::WzImage*>(img)->IsLuaWzImage() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_image_parse(wz_image img, int* out_parsed) {
  if (auto ec = init_out("wz_image_parse", out_parsed); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_parse");
  return result_bool(reinterpret_cast<wz::WzImage*>(img)->ParseImage(),
                     out_parsed);
}

wz_error_code wz_image_count_properties(wz_image img, int* out_count) {
  if (auto ec = init_out("wz_image_count_properties", out_count);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_count_properties");
  auto propsResult = reinterpret_cast<wz::WzImage*>(img)->WzPropertiesResult();
  if (auto ec = result_error(propsResult); ec != WZ_ERROR_NONE) return ec;
  auto* props = propsResult.value();
  *out_count = props ? static_cast<int>(props->size()) : 0;
  return wz_clear_last_error();
}

wz_error_code wz_image_get_property(wz_image img,
                                    int index,
                                    wz_property* out_property) {
  if (auto ec = init_out("wz_image_get_property", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_get_property");
  auto propsResult = reinterpret_cast<wz::WzImage*>(img)->WzPropertiesResult();
  if (auto ec = result_error(propsResult); ec != WZ_ERROR_NONE) return ec;
  auto* props = propsResult.value();
  if (!props) return wz_clear_last_error();
  size_t sz = props->size();
  if (index < 0 || index >= static_cast<int>(sz)) {
    return set_error_idx("wz_image_get_property", index, sz);
  }
  *out_property = wrap_prop((*props)[static_cast<size_t>(index)]);
  return wz_clear_last_error();
}

wz_error_code wz_image_get_from_path(wz_image img,
                                     const char* path,
                                     wz_property* out_property) {
  if (auto ec = init_out("wz_image_get_from_path", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!img) return set_error_null("wz_image_get_from_path");
  if (!path) return set_error_invalid_arg("wz_image_get_from_path", "path");
  auto propResult =
      reinterpret_cast<wz::WzImage*>(img)->GetFromPathResult(path);
  if (auto ec = result_error(propResult); ec != WZ_ERROR_NONE) return ec;
  *out_property = wrap_prop(propResult.value());
  return wz_clear_last_error();
}

wz_error_code wz_get_object_type(wz_object obj, wz_object_type* out_type) {
  if (auto ec = init_out("wz_get_object_type", out_type); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!obj) return set_error_null("wz_get_object_type");
  *out_type = static_cast<wz_object_type>(
      reinterpret_cast<wz::WzObject*>(obj)->ObjectType());
  return wz_clear_last_error();
}

wz_error_code wz_object_name(wz_object obj, const char** out_name) {
  if (auto ec = init_out("wz_object_name", out_name); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!obj) return set_error_null("wz_object_name");
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzObject*>(obj)->Name();
  *out_name = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_object_get_parent(wz_object obj, wz_object* out_parent) {
  if (auto ec = init_out("wz_object_get_parent", out_parent);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!obj) return set_error_null("wz_object_get_parent");
  *out_parent = reinterpret_cast<wz_object>(
      reinterpret_cast<wz::WzObject*>(obj)->Parent());
  return wz_clear_last_error();
}

wz_error_code wz_object_full_path(wz_object obj, const char** out_path) {
  if (auto ec = init_out("wz_object_full_path", out_path);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!obj) return set_error_null("wz_object_full_path");
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzObject*>(obj)->FullPath();
  *out_path = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_object_get_wz_file_parent(wz_object obj, wz_file* out_file) {
  if (auto ec = init_out("wz_object_get_wz_file_parent", out_file);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!obj) return set_error_null("wz_object_get_wz_file_parent");
  *out_file = reinterpret_cast<wz_file>(
      reinterpret_cast<wz::WzObject*>(obj)->WzFileParent());
  return wz_clear_last_error();
}

wz_error_code wz_object_get_top_most_wz_directory(wz_object obj,
                                                  wz_object* out_object) {
  if (auto ec = init_out("wz_object_get_top_most_wz_directory", out_object);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!obj) return set_error_null("wz_object_get_top_most_wz_directory");
  *out_object = reinterpret_cast<wz_object>(
      reinterpret_cast<wz::WzObject*>(obj)->GetTopMostWzDirectory());
  return wz_clear_last_error();
}

wz_error_code wz_object_get_top_most_wz_image(wz_object obj,
                                              wz_object* out_object) {
  if (auto ec = init_out("wz_object_get_top_most_wz_image", out_object);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!obj) return set_error_null("wz_object_get_top_most_wz_image");
  *out_object = reinterpret_cast<wz_object>(
      reinterpret_cast<wz::WzObject*>(obj)->GetTopMostWzImage());
  return wz_clear_last_error();
}

wz_error_code wz_object_at(wz_object obj,
                           const char* name,
                           wz_object* out_object) {
  if (auto ec = init_out("wz_object_at", out_object); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!obj) return set_error_null("wz_object_at");
  if (!name) return set_error_invalid_arg("wz_object_at", "name");
  auto* wzObj = reinterpret_cast<wz::WzObject*>(obj);
  if (wzObj->ObjectType() == wz::WzObjectType::Image) {
    auto propResult = static_cast<wz::WzImage*>(wzObj)->GetPropertyByName(name);
    if (auto ec = result_error(propResult); ec != WZ_ERROR_NONE) return ec;
    *out_object = reinterpret_cast<wz_object>(propResult.value());
  } else {
    *out_object = reinterpret_cast<wz_object>((*wzObj)[name]);
  }
  return wz_clear_last_error();
}

wz_error_code wz_object_set_name(wz_object obj, const char* name) {
  if (!obj) return set_error_null("wz_object_set_name");
  if (!name) return set_error_invalid_arg("wz_object_set_name", "name");
  return result_void(unwrap_object(obj)->Rename(name));
}

wz_error_code wz_object_remove(wz_object obj) {
  if (!obj) return set_error_null("wz_object_remove");
  return result_void(unwrap_object(obj)->TryRemove());
}

wz_error_code wz_property_get_type(wz_property prop,
                                   wz_property_type* out_type) {
  if (auto ec = init_out("wz_property_get_type", out_type);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_type");
  *out_type = static_cast<wz_property_type>(unwrap_prop(prop)->PropertyType());
  return wz_clear_last_error();
}

wz_error_code wz_property_is_raw_data(wz_property prop, int* out_is_raw) {
  if (auto ec = init_out("wz_property_is_raw_data", out_is_raw);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_is_raw_data");
  *out_is_raw = unwrap_prop(prop)->IsRawDataProperty() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_property_is_video(wz_property prop, int* out_is_video) {
  if (auto ec = init_out("wz_property_is_video", out_is_video);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_is_video");
  *out_is_video = unwrap_prop(prop)->IsVideoProperty() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_property_name(wz_property prop, const char** out_name) {
  if (auto ec = init_out("wz_property_name", out_name); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_name");
  static thread_local std::string s;
  s = unwrap_prop(prop)->Name();
  *out_name = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_property_count_children(wz_property prop, int* out_count) {
  if (auto ec = init_out("wz_property_count_children", out_count);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_count_children");
  auto* wps = unwrap_prop(prop)->WzProperties();
  *out_count = wps ? static_cast<int>(wps->size()) : 0;
  return wz_clear_last_error();
}

wz_error_code wz_property_get_child(wz_property prop,
                                    int index,
                                    wz_property* out_property) {
  if (auto ec = init_out("wz_property_get_child", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_child");
  auto* wps = unwrap_prop(prop)->WzProperties();
  if (!wps) return wz_clear_last_error();
  size_t sz = wps->size();
  if (index < 0 || index >= static_cast<int>(sz)) {
    return set_error_idx("wz_property_get_child", index, sz);
  }
  *out_property = wrap_prop((*wps)[static_cast<size_t>(index)]);
  return wz_clear_last_error();
}

wz_error_code wz_property_get_child_by_name(wz_property prop,
                                            const char* name,
                                            wz_property* out_property) {
  if (auto ec = init_out("wz_property_get_child_by_name", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_child_by_name");
  if (!name) {
    return set_error_invalid_arg("wz_property_get_child_by_name", "name");
  }
  auto* wps = unwrap_prop(prop)->WzProperties();
  if (!wps) return wz_clear_last_error();
  for (auto* child : *wps) {
    if (child && child->Name() == name) {
      *out_property = wrap_prop(child);
      break;
    }
  }
  return wz_clear_last_error();
}

wz_error_code wz_property_get_from_path(wz_property prop,
                                        const char* path,
                                        wz_property* out_property) {
  if (auto ec = init_out("wz_property_get_from_path", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_from_path");
  if (!path) return set_error_invalid_arg("wz_property_get_from_path", "path");
  *out_property = wrap_prop(unwrap_prop(prop)->GetFromPath(path));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_null(const char* name,
                                      wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_null", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_null", "name");
  *out_property = wrap_prop(new wz::WzNullProperty(name));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_short(const char* name,
                                       int16_t value,
                                       wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_short", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_short", "name");
  *out_property = wrap_prop(new wz::WzShortProperty(name, value));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_int(const char* name,
                                     int32_t value,
                                     wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_int", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_int", "name");
  *out_property = wrap_prop(new wz::WzIntProperty(name, value));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_long(const char* name,
                                      int64_t value,
                                      wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_long", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_long", "name");
  *out_property = wrap_prop(new wz::WzLongProperty(name, value));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_float(const char* name,
                                       float value,
                                       wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_float", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_float", "name");
  *out_property = wrap_prop(new wz::WzFloatProperty(name, value));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_double(const char* name,
                                        double value,
                                        wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_double", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_double", "name");
  *out_property = wrap_prop(new wz::WzDoubleProperty(name, value));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_string(const char* name,
                                        const char* value,
                                        wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_string", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_string", "name");
  if (!value) {
    return set_error_invalid_arg("wz_property_create_string", "value");
  }
  *out_property = wrap_prop(new wz::WzStringProperty(name, value));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_sub(const char* name,
                                     wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_sub", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_sub", "name");
  *out_property = wrap_prop(new wz::WzSubProperty(name));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_vector(const char* name,
                                        int32_t x,
                                        int32_t y,
                                        wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_vector", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_vector", "name");
  *out_property = wrap_prop(new wz::WzVectorProperty(name, x, y));
  return wz_clear_last_error();
}

wz_error_code wz_property_create_uol(const char* name,
                                     const char* value,
                                     wz_property* out_property) {
  if (auto ec = init_out("wz_property_create_uol", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!name) return set_error_invalid_arg("wz_property_create_uol", "name");
  if (!value) return set_error_invalid_arg("wz_property_create_uol", "value");
  *out_property = wrap_prop(new wz::WzUOLProperty(name, value));
  return wz_clear_last_error();
}

wz_error_code wz_image_add_property(wz_image img, wz_property prop) {
  if (!img) return set_error_null("wz_image_add_property");
  auto* p = unwrap_prop(prop);
  if (!p) return set_error_null("wz_image_add_property");
  return result_void(unwrap_image(img)->TryAddProperty(p));
}

wz_error_code wz_property_add_child(wz_property parent, wz_property child) {
  auto* parentProp = unwrap_prop(parent);
  if (!parentProp) return set_error_null("wz_property_add_child");
  auto* childProp = unwrap_prop(child);
  if (!childProp) return set_error_null("wz_property_add_child");
  if (!parentProp->WzProperties())
    return set_error_wrong_type("wz_property_add_child");
  return result_void(parentProp->TryAddChildProperty(childProp));
}

wz_error_code wz_image_remove_property(wz_image img, wz_property prop) {
  if (!img) return set_error_null("wz_image_remove_property");
  auto* p = unwrap_prop(prop);
  if (!p) return set_error_null("wz_image_remove_property");
  return result_void(unwrap_image(img)->TryRemoveProperty(p));
}

wz_error_code wz_property_remove_child(wz_property parent, wz_property child) {
  auto* parentProp = unwrap_prop(parent);
  if (!parentProp) return set_error_null("wz_property_remove_child");
  auto* childProp = unwrap_prop(child);
  if (!childProp) return set_error_null("wz_property_remove_child");
  if (!parentProp->WzProperties()) {
    return set_error_wrong_type("wz_property_remove_child");
  }
  return result_void(parentProp->TryRemoveChildProperty(childProp));
}

wz_error_code wz_image_clear_properties(wz_image img) {
  if (!img) return set_error_null("wz_image_clear_properties");
  unwrap_image(img)->ClearProperties();
  return wz_clear_last_error();
}

wz_error_code wz_property_clear_children(wz_property prop) {
  auto* p = unwrap_prop(prop);
  if (!p) return set_error_null("wz_property_clear_children");
  if (!p->WzProperties())
    return set_error_wrong_type("wz_property_clear_children");
  return result_void(p->TryClearChildProperties());
}

wz_error_code wz_property_get_int(wz_property prop, int32_t* out_value) {
  if (auto ec = init_out("wz_property_get_int", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_int");
  *out_value = unwrap_prop(prop)->GetInt();
  return wz_clear_last_error();
}

wz_error_code wz_property_get_short(wz_property prop, int16_t* out_value) {
  if (auto ec = init_out("wz_property_get_short", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_short");
  *out_value = unwrap_prop(prop)->GetShort();
  return wz_clear_last_error();
}

wz_error_code wz_property_get_long(wz_property prop, int64_t* out_value) {
  if (auto ec = init_out("wz_property_get_long", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_long");
  *out_value = unwrap_prop(prop)->GetLong();
  return wz_clear_last_error();
}

wz_error_code wz_property_get_float(wz_property prop, float* out_value) {
  if (auto ec = init_out("wz_property_get_float", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_float");
  *out_value = unwrap_prop(prop)->GetFloat();
  return wz_clear_last_error();
}

wz_error_code wz_property_get_double(wz_property prop, double* out_value) {
  if (auto ec = init_out("wz_property_get_double", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_double");
  *out_value = unwrap_prop(prop)->GetDouble();
  return wz_clear_last_error();
}

wz_error_code wz_property_get_string(wz_property prop, const char** out_value) {
  if (auto ec = init_out("wz_property_get_string", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_string");
  static thread_local std::string s;
  s = unwrap_prop(prop)->GetString();
  *out_value = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_property_get_bytes(wz_property prop,
                                    uint8_t* buffer,
                                    size_t buffer_size,
                                    size_t* out_size) {
  if (auto ec = init_out("wz_property_get_bytes", out_size);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_bytes");
  auto result = unwrap_prop(prop)->GetBytes();
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  auto data = result.value();
  *out_size = data.size();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return wz_clear_last_error();
}

wz_error_code wz_int_get_value(wz_property prop, int32_t* out_value) {
  if (auto ec = init_out("wz_int_get_value", out_value); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_int_get_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Int) {
    return set_error_wrong_type("wz_int_get_value");
  }
  *out_value = static_cast<wz::WzIntProperty*>(unwrap_prop(prop))->Value();
  return wz_clear_last_error();
}

wz_error_code wz_int_set_value(wz_property prop, int32_t value) {
  if (!prop) return set_error_null("wz_int_set_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Int) {
    return set_error_wrong_type("wz_int_set_value");
  }
  static_cast<wz::WzIntProperty*>(unwrap_prop(prop))->SetValue(value);
  return wz_clear_last_error();
}

wz_error_code wz_short_get_value(wz_property prop, int16_t* out_value) {
  if (auto ec = init_out("wz_short_get_value", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_short_get_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Short) {
    return set_error_wrong_type("wz_short_get_value");
  }
  *out_value = static_cast<wz::WzShortProperty*>(unwrap_prop(prop))->Value();
  return wz_clear_last_error();
}

wz_error_code wz_short_set_value(wz_property prop, int16_t value) {
  if (!prop) return set_error_null("wz_short_set_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Short) {
    return set_error_wrong_type("wz_short_set_value");
  }
  static_cast<wz::WzShortProperty*>(unwrap_prop(prop))->SetValue(value);
  return wz_clear_last_error();
}

wz_error_code wz_long_get_value(wz_property prop, int64_t* out_value) {
  if (auto ec = init_out("wz_long_get_value", out_value); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_long_get_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Long) {
    return set_error_wrong_type("wz_long_get_value");
  }
  *out_value = static_cast<wz::WzLongProperty*>(unwrap_prop(prop))->Value();
  return wz_clear_last_error();
}

wz_error_code wz_long_set_value(wz_property prop, int64_t value) {
  if (!prop) return set_error_null("wz_long_set_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Long) {
    return set_error_wrong_type("wz_long_set_value");
  }
  static_cast<wz::WzLongProperty*>(unwrap_prop(prop))->SetValue(value);
  return wz_clear_last_error();
}

wz_error_code wz_float_get_value(wz_property prop, float* out_value) {
  if (auto ec = init_out("wz_float_get_value", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_float_get_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Float) {
    return set_error_wrong_type("wz_float_get_value");
  }
  *out_value = static_cast<wz::WzFloatProperty*>(unwrap_prop(prop))->Value();
  return wz_clear_last_error();
}

wz_error_code wz_float_set_value(wz_property prop, float value) {
  if (!prop) return set_error_null("wz_float_set_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Float) {
    return set_error_wrong_type("wz_float_set_value");
  }
  static_cast<wz::WzFloatProperty*>(unwrap_prop(prop))->SetValue(value);
  return wz_clear_last_error();
}

wz_error_code wz_double_get_value(wz_property prop, double* out_value) {
  if (auto ec = init_out("wz_double_get_value", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_double_get_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Double) {
    return set_error_wrong_type("wz_double_get_value");
  }
  *out_value = static_cast<wz::WzDoubleProperty*>(unwrap_prop(prop))->Value();
  return wz_clear_last_error();
}

wz_error_code wz_double_set_value(wz_property prop, double value) {
  if (!prop) return set_error_null("wz_double_set_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Double) {
    return set_error_wrong_type("wz_double_set_value");
  }
  static_cast<wz::WzDoubleProperty*>(unwrap_prop(prop))->SetValue(value);
  return wz_clear_last_error();
}

wz_error_code wz_string_get_value(wz_property prop, const char** out_value) {
  if (auto ec = init_out("wz_string_get_value", out_value);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_string_get_value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::String) {
    return set_error_wrong_type("wz_string_get_value");
  }
  static thread_local std::string s;
  s = static_cast<wz::WzStringProperty*>(unwrap_prop(prop))->Value();
  *out_value = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_string_set_value(wz_property prop, const char* value) {
  if (!prop) return set_error_null("wz_string_set_value");
  if (!value) return set_error_invalid_arg("wz_string_set_value", "value");
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::String) {
    return set_error_wrong_type("wz_string_set_value");
  }
  static_cast<wz::WzStringProperty*>(unwrap_prop(prop))->SetValue(value);
  return wz_clear_last_error();
}

wz_error_code wz_string_save_to_file(wz_property prop, const char* file_path) {
  if (!prop) return set_error_null("wz_string_save_to_file");
  if (!file_path) {
    return set_error_invalid_arg("wz_string_save_to_file", "file_path");
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::String) {
    return set_error_wrong_type("wz_string_save_to_file");
  }
  return result_void(static_cast<wz::WzStringProperty*>(unwrap_prop(prop))
                         ->SaveToFile(file_path));
}

wz_error_code wz_png_width(wz_png_property png, int* out_width) {
  if (auto ec = init_out("wz_png_width", out_width); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!png) return set_error_null("wz_png_width");
  *out_width = reinterpret_cast<wz::WzPngProperty*>(png)->Width();
  return wz_clear_last_error();
}

wz_error_code wz_png_height(wz_png_property png, int* out_height) {
  if (auto ec = init_out("wz_png_height", out_height); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!png) return set_error_null("wz_png_height");
  *out_height = reinterpret_cast<wz::WzPngProperty*>(png)->Height();
  return wz_clear_last_error();
}

wz_error_code wz_png_format(wz_png_property png, int* out_format) {
  if (auto ec = init_out("wz_png_format", out_format); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!png) return set_error_null("wz_png_format");
  *out_format =
      static_cast<int>(reinterpret_cast<wz::WzPngProperty*>(png)->Format());
  return wz_clear_last_error();
}

wz_error_code wz_png_is_list_wz_used(wz_png_property png, int* out_is_used) {
  if (auto ec = init_out("wz_png_is_list_wz_used", out_is_used);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!png) return set_error_null("wz_png_is_list_wz_used");
  *out_is_used =
      reinterpret_cast<wz::WzPngProperty*>(png)->ListWzUsed() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_png_get_image(wz_png_property png,
                               uint8_t* buffer,
                               size_t buffer_size,
                               size_t* out_size) {
  if (auto ec = init_out("wz_png_get_image", out_size); ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!png) return set_error_null("wz_png_get_image");
  auto result = reinterpret_cast<wz::WzPngProperty*>(png)->GetImage(false);
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  auto& data = result.value();
  *out_size = data.size();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return wz_clear_last_error();
}

wz_error_code wz_png_get_compressed_bytes(wz_png_property png,
                                          uint8_t* buffer,
                                          size_t buffer_size,
                                          size_t* out_size) {
  if (auto ec = init_out("wz_png_get_compressed_bytes", out_size);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!png) return set_error_null("wz_png_get_compressed_bytes");
  auto result =
      reinterpret_cast<wz::WzPngProperty*>(png)->GetCompressedBytes(false);
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  auto& data = result.value();
  *out_size = data.size();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return wz_clear_last_error();
}

wz_error_code wz_png_save_to_file(wz_png_property png, const char* file_path) {
  if (!png) return set_error_null("wz_png_save_to_file");
  if (!file_path) {
    return set_error_invalid_arg("wz_png_save_to_file", "file_path");
  }
  return result_void(
      reinterpret_cast<wz::WzPngProperty*>(png)->SaveToFile(file_path));
}

wz_error_code wz_canvas_get_png(wz_property canvas_prop,
                                wz_png_property* out_png) {
  if (auto ec = init_out("wz_canvas_get_png", out_png); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(canvas_prop);
  if (!p) return set_error_null("wz_canvas_get_png");
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    return set_error_wrong_type("wz_canvas_get_png");
  }
  *out_png = reinterpret_cast<wz_png_property>(
      static_cast<wz::WzCanvasProperty*>(p)->PngProperty());
  return wz_clear_last_error();
}

wz_error_code wz_canvas_contains_inlink(wz_property canvas_prop,
                                        int* out_contains) {
  if (auto ec = init_out("wz_canvas_contains_inlink", out_contains);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(canvas_prop);
  if (!p) return set_error_null("wz_canvas_contains_inlink");
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    return set_error_wrong_type("wz_canvas_contains_inlink");
  }
  *out_contains =
      static_cast<wz::WzCanvasProperty*>(p)->ContainsInlinkProperty() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_canvas_contains_outlink(wz_property canvas_prop,
                                         int* out_contains) {
  if (auto ec = init_out("wz_canvas_contains_outlink", out_contains);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(canvas_prop);
  if (!p) return set_error_null("wz_canvas_contains_outlink");
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    return set_error_wrong_type("wz_canvas_contains_outlink");
  }
  *out_contains =
      static_cast<wz::WzCanvasProperty*>(p)->ContainsOutlinkProperty() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_canvas_save_to_file(wz_property canvas_prop,
                                     const char* file_path) {
  auto* p = unwrap_prop(canvas_prop);
  if (!p) return set_error_null("wz_canvas_save_to_file");
  if (!file_path) {
    return set_error_invalid_arg("wz_canvas_save_to_file", "file_path");
  }
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    return set_error_wrong_type("wz_canvas_save_to_file");
  }
  return result_void(
      static_cast<wz::WzCanvasProperty*>(p)->SaveToFile(file_path));
}

wz_error_code wz_canvas_get_linked_wz_image_property(
    wz_property canvas_prop, wz_property* out_property) {
  if (auto ec =
          init_out("wz_canvas_get_linked_wz_image_property", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(canvas_prop);
  if (!p) return set_error_null("wz_canvas_get_linked_wz_image_property");
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    return set_error_wrong_type("wz_canvas_get_linked_wz_image_property");
  }
  auto linked_result =
      static_cast<wz::WzCanvasProperty*>(p)->GetLinkedWzImageProperty();
  if (!linked_result.has_value()) {
    return wz_set_last_error(
        static_cast<wz_error_code>(linked_result.error().code()),
        linked_result.error().message());
  }
  auto* linked = linked_result.value();
  *out_property = linked ? wrap_prop(linked) : nullptr;
  return wz_clear_last_error();
}

wz_error_code wz_uol_get_value(wz_property uol_prop, const char** out_value) {
  if (auto ec = init_out("wz_uol_get_value", out_value); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(uol_prop);
  if (!p) return set_error_null("wz_uol_get_value");
  if (p->PropertyType() != wz::WzPropertyType::UOL) {
    return set_error_wrong_type("wz_uol_get_value");
  }
  static thread_local std::string s;
  s = static_cast<wz::WzUOLProperty*>(p)->Value();
  *out_value = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_uol_set_value(wz_property uol_prop, const char* value) {
  auto* p = unwrap_prop(uol_prop);
  if (!p) return set_error_null("wz_uol_set_value");
  if (!value) return set_error_invalid_arg("wz_uol_set_value", "value");
  if (p->PropertyType() != wz::WzPropertyType::UOL) {
    return set_error_wrong_type("wz_uol_set_value");
  }
  static_cast<wz::WzUOLProperty*>(p)->SetValue(value);
  return wz_clear_last_error();
}

wz_error_code wz_uol_get_link_value(wz_property uol_prop,
                                    wz_object* out_object) {
  if (auto ec = init_out("wz_uol_get_link_value", out_object);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(uol_prop);
  if (!p) return set_error_null("wz_uol_get_link_value");
  if (p->PropertyType() != wz::WzPropertyType::UOL) {
    return set_error_wrong_type("wz_uol_get_link_value");
  }
  *out_object = reinterpret_cast<wz_object>(
      static_cast<wz::WzUOLProperty*>(p)->LinkValue());
  return wz_clear_last_error();
}

wz_error_code wz_property_get_linked_wz_image_property(
    wz_property prop, wz_property* out_property) {
  if (auto ec =
          init_out("wz_property_get_linked_wz_image_property", out_property);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!prop) return set_error_null("wz_property_get_linked_wz_image_property");
  auto* linked = unwrap_prop(prop)->GetLinkedWzImageProperty();
  *out_property = linked ? wrap_prop(linked) : nullptr;
  return wz_clear_last_error();
}

wz_error_code wz_vector_get_x(wz_property vec_prop, int32_t* out_value) {
  if (auto ec = init_out("wz_vector_get_x", out_value); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(vec_prop);
  if (!p) return set_error_null("wz_vector_get_x");
  if (p->PropertyType() != wz::WzPropertyType::Vector) {
    return set_error_wrong_type("wz_vector_get_x");
  }
  auto* vec = static_cast<wz::WzVectorProperty*>(p);
  *out_value = vec->X ? vec->X->Value() : 0;
  return wz_clear_last_error();
}

wz_error_code wz_vector_get_y(wz_property vec_prop, int32_t* out_value) {
  if (auto ec = init_out("wz_vector_get_y", out_value); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(vec_prop);
  if (!p) return set_error_null("wz_vector_get_y");
  if (p->PropertyType() != wz::WzPropertyType::Vector) {
    return set_error_wrong_type("wz_vector_get_y");
  }
  auto* vec = static_cast<wz::WzVectorProperty*>(p);
  *out_value = vec->Y ? vec->Y->Value() : 0;
  return wz_clear_last_error();
}

wz_error_code wz_lua_get_data(wz_property lua_prop,
                              uint8_t* buffer,
                              size_t buffer_size,
                              size_t* out_size) {
  if (auto ec = init_out("wz_lua_get_data", out_size); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(lua_prop);
  if (!p) return set_error_null("wz_lua_get_data");
  if (p->PropertyType() != wz::WzPropertyType::Lua) {
    return set_error_wrong_type("wz_lua_get_data");
  }
  auto& data = static_cast<wz::WzLuaProperty*>(p)->Value();
  *out_size = data.size();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return wz_clear_last_error();
}

wz_error_code wz_lua_get_string(wz_property lua_prop, const char** out_value) {
  if (auto ec = init_out("wz_lua_get_string", out_value); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(lua_prop);
  if (!p) return set_error_null("wz_lua_get_string");
  if (p->PropertyType() != wz::WzPropertyType::Lua) {
    return set_error_wrong_type("wz_lua_get_string");
  }
  static thread_local std::string s;
  s = static_cast<wz::WzLuaProperty*>(p)->GetString();
  *out_value = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_lua_save_to_file(wz_property lua_prop, const char* file_path) {
  auto* p = unwrap_prop(lua_prop);
  if (!p) return set_error_null("wz_lua_save_to_file");
  if (!file_path) {
    return set_error_invalid_arg("wz_lua_save_to_file", "file_path");
  }
  if (p->PropertyType() != wz::WzPropertyType::Lua) {
    return set_error_wrong_type("wz_lua_save_to_file");
  }
  return result_void(static_cast<wz::WzLuaProperty*>(p)->SaveToFile(file_path));
}

wz_error_code wz_binary_get_data(wz_property binary_prop,
                                 uint8_t* buffer,
                                 size_t buffer_size,
                                 size_t* out_size) {
  if (auto ec = init_out("wz_binary_get_data", out_size); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(binary_prop);
  if (!p) return set_error_null("wz_binary_get_data");
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    return set_error_wrong_type("wz_binary_get_data");
  }
  auto result = static_cast<wz::WzBinaryProperty*>(p)->GetBytes(false);
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  auto& data = result.value();
  *out_size = data.size();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return wz_clear_last_error();
}

wz_error_code wz_binary_get_wav_playback(wz_property binary_prop,
                                         uint8_t* buffer,
                                         size_t buffer_size,
                                         size_t* out_size) {
  if (auto ec = init_out("wz_binary_get_wav_playback", out_size);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(binary_prop);
  if (!p) return set_error_null("wz_binary_get_wav_playback");
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    return set_error_wrong_type("wz_binary_get_wav_playback");
  }
  auto result =
      static_cast<wz::WzBinaryProperty*>(p)->GetBytesForWAVPlayback(false);
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  auto& data = result.value();
  *out_size = data.size();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return wz_clear_last_error();
}

wz_error_code wz_binary_get_length(wz_property binary_prop, int* out_length) {
  if (auto ec = init_out("wz_binary_get_length", out_length);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(binary_prop);
  if (!p) return set_error_null("wz_binary_get_length");
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    return set_error_wrong_type("wz_binary_get_length");
  }
  *out_length = static_cast<wz::WzBinaryProperty*>(p)->Length();
  return wz_clear_last_error();
}

wz_error_code wz_binary_get_frequency(wz_property binary_prop,
                                      int* out_frequency) {
  if (auto ec = init_out("wz_binary_get_frequency", out_frequency);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(binary_prop);
  if (!p) return set_error_null("wz_binary_get_frequency");
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    return set_error_wrong_type("wz_binary_get_frequency");
  }
  *out_frequency = static_cast<wz::WzBinaryProperty*>(p)->Frequency();
  return wz_clear_last_error();
}

wz_error_code wz_binary_get_type(wz_property binary_prop,
                                 wz_binary_type* out_type) {
  if (auto ec = init_out("wz_binary_get_type", out_type); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(binary_prop);
  if (!p) return set_error_null("wz_binary_get_type");
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    return set_error_wrong_type("wz_binary_get_type");
  }
  *out_type = static_cast<wz_binary_type>(
      static_cast<wz::WzBinaryProperty*>(p)->Type());
  return wz_clear_last_error();
}

wz_error_code wz_binary_is_header_encrypted(wz_property binary_prop,
                                            int* out_is_encrypted) {
  if (auto ec = init_out("wz_binary_is_header_encrypted", out_is_encrypted);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(binary_prop);
  if (!p) return set_error_null("wz_binary_is_header_encrypted");
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    return set_error_wrong_type("wz_binary_is_header_encrypted");
  }
  *out_is_encrypted =
      static_cast<wz::WzBinaryProperty*>(p)->HeaderEncrypted() ? 1 : 0;
  return wz_clear_last_error();
}

wz_error_code wz_binary_save_to_file(wz_property binary_prop,
                                     const char* file_path) {
  auto* p = unwrap_prop(binary_prop);
  if (!p) return set_error_null("wz_binary_save_to_file");
  if (!file_path) {
    return set_error_invalid_arg("wz_binary_save_to_file", "file_path");
  }
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    return set_error_wrong_type("wz_binary_save_to_file");
  }
  return result_void(
      static_cast<wz::WzBinaryProperty*>(p)->SaveToFile(file_path));
}

wz_error_code wz_rawdata_get_data(wz_property raw_prop,
                                  uint8_t* buffer,
                                  size_t buffer_size,
                                  size_t* out_size) {
  if (auto ec = init_out("wz_rawdata_get_data", out_size);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(raw_prop);
  if (!p) return set_error_null("wz_rawdata_get_data");
  if (!p->IsRawDataProperty()) {
    return set_error_wrong_type("wz_rawdata_get_data");
  }
  auto result = static_cast<wz::WzRawDataProperty*>(p)->GetBytes(false);
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  auto& data = result.value();
  *out_size = data.size();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return wz_clear_last_error();
}

wz_error_code wz_rawdata_get_type(wz_property raw_prop, int* out_type) {
  if (auto ec = init_out("wz_rawdata_get_type", out_type);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(raw_prop);
  if (!p) return set_error_null("wz_rawdata_get_type");
  if (!p->IsRawDataProperty()) {
    return set_error_wrong_type("wz_rawdata_get_type");
  }
  *out_type = static_cast<int>(static_cast<wz::WzRawDataProperty*>(p)->Type());
  return wz_clear_last_error();
}

wz_error_code wz_rawdata_save_to_file(wz_property raw_prop,
                                      const char* file_path) {
  auto* p = unwrap_prop(raw_prop);
  if (!p) return set_error_null("wz_rawdata_save_to_file");
  if (!file_path) {
    return set_error_invalid_arg("wz_rawdata_save_to_file", "file_path");
  }
  if (!p->IsRawDataProperty()) {
    return set_error_wrong_type("wz_rawdata_save_to_file");
  }
  return result_void(
      static_cast<wz::WzRawDataProperty*>(p)->SaveToFile(file_path));
}

wz_error_code wz_video_get_data(wz_property video_prop,
                                uint8_t* buffer,
                                size_t buffer_size,
                                size_t* out_size) {
  if (auto ec = init_out("wz_video_get_data", out_size); ec != WZ_ERROR_NONE) {
    return ec;
  }
  auto* p = unwrap_prop(video_prop);
  if (!p) return set_error_null("wz_video_get_data");
  if (!p->IsVideoProperty()) {
    return set_error_wrong_type("wz_video_get_data");
  }
  auto result = static_cast<wz::WzVideoProperty*>(p)->GetBytes(false);
  if (!result.has_value()) {
    return wz_set_last_error(static_cast<wz_error_code>(result.error().code()),
                             result.error().message());
  }
  auto& data = result.value();
  *out_size = data.size();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return wz_clear_last_error();
}

wz_error_code wz_video_save_to_file(wz_property video_prop,
                                    const char* file_path) {
  auto* p = unwrap_prop(video_prop);
  if (!p) return set_error_null("wz_video_save_to_file");
  if (!file_path) {
    return set_error_invalid_arg("wz_video_save_to_file", "file_path");
  }
  if (!p->IsVideoProperty()) {
    return set_error_wrong_type("wz_video_save_to_file");
  }
  return result_void(
      static_cast<wz::WzVideoProperty*>(p)->SaveToFile(file_path));
}

wz_error_code wz_get_error_description(wz_parse_status status,
                                       const char** out_description) {
  if (auto ec = init_out("wz_get_error_description", out_description);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  static thread_local std::string s;
  s = wz::GetErrorDescription(static_cast<wz::WzFileParseStatus>(status));
  *out_description = s.c_str();
  return wz_clear_last_error();
}

wz_error_code wz_detect_maple_version(const char* file_path,
                                      wz_maple_version* out_maple_version,
                                      int16_t* out_version) {
  if (auto ec = init_out("wz_detect_maple_version", out_maple_version);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (auto ec = init_out("wz_detect_maple_version", out_version);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  if (!file_path) {
    return set_error_invalid_arg("wz_detect_maple_version", "file_path");
  }
  int16_t ver = 0;
  auto mv = wz::WzTool::DetectMapleVersion(file_path, &ver);
  *out_maple_version = static_cast<wz_maple_version>(mv);
  *out_version = ver;
  return wz_clear_last_error();
}

wz_error_code wz_get_iv_for_version(wz_maple_version ver,
                                    const uint8_t** out_iv) {
  if (auto ec = init_out("wz_get_iv_for_version", out_iv);
      ec != WZ_ERROR_NONE) {
    return ec;
  }
  static thread_local std::array<uint8_t, 4> iv;
  iv = wz::WzTool::GetIvByMapleVersion(static_cast<wz::WzMapleVersion>(ver));
  *out_iv = iv.data();
  return wz_clear_last_error();
}

}  // extern "C"

#include "wz/wz_api.h"
#include <cstring>
#include "wz/wz.h"

static thread_local wz_error_code g_last_error = WZ_ERROR_NONE;
static thread_local std::string g_last_error_msg;

static void set_error(wz::ErrorCode code, const std::string& msg) {
  switch (code) {
    case wz::ErrorCode::Ok:
      g_last_error = WZ_ERROR_NONE;
      break;
    case wz::ErrorCode::NotImplemented:
      g_last_error = WZ_ERROR_NOT_IMPLEMENTED;
      break;
    case wz::ErrorCode::InvalidArgument:
      g_last_error = WZ_ERROR_INVALID_ARGUMENT;
      break;
    case wz::ErrorCode::ParseError:
      g_last_error = WZ_ERROR_PARSE_ERROR;
      break;
    case wz::ErrorCode::IoError:
      g_last_error = WZ_ERROR_IO_ERROR;
      break;
    case wz::ErrorCode::DataError:
      g_last_error = WZ_ERROR_DATA_ERROR;
      break;
    case wz::ErrorCode::NotFound:
      g_last_error = WZ_ERROR_NOT_FOUND;
      break;
    default:
      g_last_error = WZ_ERROR_NOT_IMPLEMENTED;
      break;
  }
  g_last_error_msg = msg;
}

static void set_error_null(const char* fn) {
  g_last_error = WZ_ERROR_NULL_HANDLE;
  g_last_error_msg = std::string(fn) + ": null handle";
}

static void set_error_wrong_type(const char* fn) {
  g_last_error = WZ_ERROR_WRONG_TYPE;
  g_last_error_msg = std::string(fn) + ": wrong property type";
}

static void set_error_idx(const char* fn, int idx, size_t size) {
  g_last_error = WZ_ERROR_INDEX_OUT_OF_RANGE;
  g_last_error_msg = std::string(fn) + ": index " + std::to_string(idx) +
                     " out of range [0, " + std::to_string(size) + ")";
}

static wz::WzImageProperty* unwrap_prop(wz_property p) {
  return reinterpret_cast<wz::WzImageProperty*>(p);
}

static wz_property wrap_prop(wz::WzImageProperty* p) {
  return reinterpret_cast<wz_property>(p);
}

extern "C" {

void wz_clear_error(void) {
  g_last_error = WZ_ERROR_NONE;
  g_last_error_msg.clear();
}

wz_error_code wz_get_last_error(void) {
  return g_last_error;
}

const char* wz_get_last_error_message(void) {
  return g_last_error_msg.c_str();
}

// ==================== WzFile ====================

wz_file wz_open_file(const char* file_path,
                     int16_t game_version,
                     wz_maple_version version) {
  auto* f = new wz::WzFile(
      file_path, game_version, static_cast<wz::WzMapleVersion>(version));
  return reinterpret_cast<wz_file>(f);
}

wz_file wz_open_file_with_iv(const char* file_path, const uint8_t iv[4]) {
  std::array<uint8_t, 4> ivArr = {iv[0], iv[1], iv[2], iv[3]};
  auto* f = new wz::WzFile(file_path, ivArr);
  return reinterpret_cast<wz_file>(f);
}

wz_parse_status wz_parse(wz_file file) {
  wz_clear_error();
  if (!file) {
    set_error_null("wz_parse");
    return WZ_PARSE_PATH_IS_NULL;
  }
  auto* f = reinterpret_cast<wz::WzFile*>(file);
  auto status = f->ParseWzFile();
  if (status != wz::WzFileParseStatus::Success) {
    set_error(wz::ErrorCode::ParseError, wz::GetErrorDescription(status));
  }
  return static_cast<wz_parse_status>(status);
}

void wz_close_file(wz_file file) {
  if (!file) return;
  auto* f = reinterpret_cast<wz::WzFile*>(file);
  delete f;
}

const char* wz_file_name(wz_file file) {
  if (!file) {
    set_error_null("wz_file_name");
    return nullptr;
  }
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzFile*>(file)->Name();
  return s.c_str();
}

const char* wz_file_path(wz_file file) {
  if (!file) {
    set_error_null("wz_file_path");
    return nullptr;
  }
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzFile*>(file)->FilePath();
  return s.c_str();
}

int16_t wz_file_version(wz_file file) {
  if (!file) {
    set_error_null("wz_file_version");
    return 0;
  }
  return reinterpret_cast<wz::WzFile*>(file)->Version();
}

wz_maple_version wz_file_maple_version(wz_file file) {
  if (!file) {
    set_error_null("wz_file_maple_version");
    return WZ_UNKNOWN;
  }
  return static_cast<wz_maple_version>(
      reinterpret_cast<wz::WzFile*>(file)->MapleVersion());
}

wz_dir wz_file_get_wz_directory(wz_file file) {
  if (!file) {
    set_error_null("wz_file_get_wz_directory");
    return nullptr;
  }
  return reinterpret_cast<wz_dir>(
      reinterpret_cast<wz::WzFile*>(file)->GetWzDirectory());
}

int wz_file_is_64bit(wz_file file) {
  if (!file) {
    set_error_null("wz_file_is_64bit");
    return 0;
  }
  return reinterpret_cast<wz::WzFile*>(file)->Is64BitWzFile() ? 1 : 0;
}

int wz_file_is_unloaded(wz_file file) {
  if (!file) {
    set_error_null("wz_file_is_unloaded");
    return 1;
  }
  return reinterpret_cast<wz::WzFile*>(file)->IsUnloaded() ? 1 : 0;
}

uint32_t wz_file_version_hash(wz_file file) {
  if (!file) {
    set_error_null("wz_file_version_hash");
    return 0;
  }
  return reinterpret_cast<wz::WzFile*>(file)->VersionHash();
}

wz_object wz_file_get_object_from_path(wz_file file,
                                       const char* path,
                                       int checkFirstDirectoryName) {
  wz_clear_error();
  if (!file) {
    set_error_null("wz_file_get_object_from_path");
    return nullptr;
  }
  if (!path) {
    set_error_null("wz_file_get_object_from_path");
    return nullptr;
  }
  return reinterpret_cast<wz_object>(
      reinterpret_cast<wz::WzFile*>(file)->GetObjectFromPath(
          path, checkFirstDirectoryName != 0));
}

// ==================== WzDirectory ====================

const char* wz_dir_name(wz_dir dir) {
  if (!dir) {
    set_error_null("wz_dir_name");
    return nullptr;
  }
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzDirectory*>(dir)->Name();
  return s.c_str();
}

int wz_dir_count_images(wz_dir dir) {
  if (!dir) {
    set_error_null("wz_dir_count_images");
    return 0;
  }
  return static_cast<int>(
      reinterpret_cast<wz::WzDirectory*>(dir)->WzImages().size());
}

int wz_dir_count_directories(wz_dir dir) {
  if (!dir) {
    set_error_null("wz_dir_count_directories");
    return 0;
  }
  return static_cast<int>(
      reinterpret_cast<wz::WzDirectory*>(dir)->WzDirectories().size());
}

int wz_dir_count_images_total(wz_dir dir) {
  if (!dir) {
    set_error_null("wz_dir_count_images_total");
    return 0;
  }
  return reinterpret_cast<wz::WzDirectory*>(dir)->CountImages();
}

wz_image wz_dir_get_image(wz_dir dir, int index) {
  wz_clear_error();
  if (!dir) {
    set_error_null("wz_dir_get_image");
    return nullptr;
  }
  auto* d = reinterpret_cast<wz::WzDirectory*>(dir);
  size_t sz = d->WzImages().size();
  if (index < 0 || index >= static_cast<int>(sz)) {
    set_error_idx("wz_dir_get_image", index, sz);
    return nullptr;
  }
  return reinterpret_cast<wz_image>(d->WzImages()[static_cast<size_t>(index)]);
}

wz_image wz_dir_get_image_by_name(wz_dir dir, const char* name) {
  wz_clear_error();
  if (!dir) {
    set_error_null("wz_dir_get_image_by_name");
    return nullptr;
  }
  if (!name) {
    set_error_null("wz_dir_get_image_by_name");
    return nullptr;
  }
  return reinterpret_cast<wz_image>(
      reinterpret_cast<wz::WzDirectory*>(dir)->GetImageByName(name));
}

wz_dir wz_dir_get_directory(wz_dir dir, int index) {
  wz_clear_error();
  if (!dir) {
    set_error_null("wz_dir_get_directory");
    return nullptr;
  }
  auto* d = reinterpret_cast<wz::WzDirectory*>(dir);
  size_t sz = d->WzDirectories().size();
  if (index < 0 || index >= static_cast<int>(sz)) {
    set_error_idx("wz_dir_get_directory", index, sz);
    return nullptr;
  }
  return reinterpret_cast<wz_dir>(
      d->WzDirectories()[static_cast<size_t>(index)]);
}

wz_dir wz_dir_get_directory_by_name(wz_dir dir, const char* name) {
  wz_clear_error();
  if (!dir) {
    set_error_null("wz_dir_get_directory_by_name");
    return nullptr;
  }
  if (!name) {
    set_error_null("wz_dir_get_directory_by_name");
    return nullptr;
  }
  return reinterpret_cast<wz_dir>(
      reinterpret_cast<wz::WzDirectory*>(dir)->GetDirectoryByName(name));
}

int wz_dir_block_size(wz_dir dir) {
  if (!dir) {
    set_error_null("wz_dir_block_size");
    return 0;
  }
  return reinterpret_cast<wz::WzDirectory*>(dir)->BlockSize();
}

int wz_dir_checksum(wz_dir dir) {
  if (!dir) {
    set_error_null("wz_dir_checksum");
    return 0;
  }
  return reinterpret_cast<wz::WzDirectory*>(dir)->Checksum();
}

int64_t wz_dir_offset(wz_dir dir) {
  if (!dir) {
    set_error_null("wz_dir_offset");
    return 0;
  }
  return reinterpret_cast<wz::WzDirectory*>(dir)->Offset();
}

// ==================== WzImage ====================

const char* wz_image_name(wz_image img) {
  if (!img) {
    set_error_null("wz_image_name");
    return nullptr;
  }
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzImage*>(img)->Name();
  return s.c_str();
}

int wz_image_is_parsed(wz_image img) {
  if (!img) {
    set_error_null("wz_image_is_parsed");
    return 0;
  }
  return reinterpret_cast<wz::WzImage*>(img)->Parsed() ? 1 : 0;
}

int wz_image_is_changed(wz_image img) {
  if (!img) {
    set_error_null("wz_image_is_changed");
    return 0;
  }
  return reinterpret_cast<wz::WzImage*>(img)->Changed() ? 1 : 0;
}

int wz_image_block_size(wz_image img) {
  if (!img) {
    set_error_null("wz_image_block_size");
    return 0;
  }
  return reinterpret_cast<wz::WzImage*>(img)->BlockSize();
}

int wz_image_checksum(wz_image img) {
  if (!img) {
    set_error_null("wz_image_checksum");
    return 0;
  }
  return reinterpret_cast<wz::WzImage*>(img)->Checksum();
}

int64_t wz_image_offset(wz_image img) {
  if (!img) {
    set_error_null("wz_image_offset");
    return 0;
  }
  return reinterpret_cast<wz::WzImage*>(img)->Offset();
}

int wz_image_is_lua_wz_image(wz_image img) {
  if (!img) {
    set_error_null("wz_image_is_lua_wz_image");
    return 0;
  }
  return reinterpret_cast<wz::WzImage*>(img)->IsLuaWzImage() ? 1 : 0;
}

void wz_image_parse(wz_image img) {
  if (!img) {
    set_error_null("wz_image_parse");
    return;
  }
  reinterpret_cast<wz::WzImage*>(img)->ParseImage();
}

int wz_image_count_properties(wz_image img) {
  if (!img) {
    set_error_null("wz_image_count_properties");
    return 0;
  }
  auto* p = reinterpret_cast<wz::WzImage*>(img)->WzProperties();
  return p ? static_cast<int>(p->size()) : 0;
}

wz_property wz_image_get_property(wz_image img, int index) {
  wz_clear_error();
  if (!img) {
    set_error_null("wz_image_get_property");
    return nullptr;
  }
  auto* props = reinterpret_cast<wz::WzImage*>(img)->WzProperties();
  if (!props) {
    set_error_null("wz_image_get_property");
    return nullptr;
  }
  size_t sz = props->size();
  if (index < 0 || index >= static_cast<int>(sz)) {
    set_error_idx("wz_image_get_property", index, sz);
    return nullptr;
  }
  return wrap_prop((*props)[static_cast<size_t>(index)]);
}

wz_property wz_image_get_from_path(wz_image img, const char* path) {
  wz_clear_error();
  if (!img) {
    set_error_null("wz_image_get_from_path");
    return nullptr;
  }
  if (!path) {
    set_error_null("wz_image_get_from_path");
    return nullptr;
  }
  return wrap_prop(reinterpret_cast<wz::WzImage*>(img)->GetFromPath(path));
}

// ==================== WzObject ====================

wz_object_type wz_get_object_type(wz_object obj) {
  if (!obj) {
    set_error_null("wz_get_object_type");
    return WZ_OBJECT_FILE;
  }
  return static_cast<wz_object_type>(
      reinterpret_cast<wz::WzObject*>(obj)->ObjectType());
}

const char* wz_object_name(wz_object obj) {
  if (!obj) {
    set_error_null("wz_object_name");
    return nullptr;
  }
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzObject*>(obj)->Name();
  return s.c_str();
}

wz_object wz_object_get_parent(wz_object obj) {
  if (!obj) {
    set_error_null("wz_object_get_parent");
    return nullptr;
  }
  return reinterpret_cast<wz_object>(
      reinterpret_cast<wz::WzObject*>(obj)->Parent());
}

const char* wz_object_full_path(wz_object obj) {
  if (!obj) {
    set_error_null("wz_object_full_path");
    return nullptr;
  }
  static thread_local std::string s;
  s = reinterpret_cast<wz::WzObject*>(obj)->FullPath();
  return s.c_str();
}

wz_file wz_object_get_wz_file_parent(wz_object obj) {
  if (!obj) {
    set_error_null("wz_object_get_wz_file_parent");
    return nullptr;
  }
  return reinterpret_cast<wz_file>(
      reinterpret_cast<wz::WzObject*>(obj)->WzFileParent());
}

wz_object wz_object_get_top_most_wz_directory(wz_object obj) {
  if (!obj) {
    set_error_null("wz_object_get_top_most_wz_directory");
    return nullptr;
  }
  return reinterpret_cast<wz_object>(
      reinterpret_cast<wz::WzObject*>(obj)->GetTopMostWzDirectory());
}

wz_object wz_object_get_top_most_wz_image(wz_object obj) {
  if (!obj) {
    set_error_null("wz_object_get_top_most_wz_image");
    return nullptr;
  }
  return reinterpret_cast<wz_object>(
      reinterpret_cast<wz::WzObject*>(obj)->GetTopMostWzImage());
}

wz_object wz_object_at(wz_object obj, const char* name) {
  wz_clear_error();
  if (!obj) {
    set_error_null("wz_object_at");
    return nullptr;
  }
  if (!name) {
    set_error_null("wz_object_at");
    return nullptr;
  }
  return reinterpret_cast<wz_object>(
      (*reinterpret_cast<wz::WzObject*>(obj))[name]);
}

// ==================== WzProperty ====================

wz_property_type wz_property_get_type(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_get_type");
    return WZ_PROP_NULL;
  }
  return static_cast<wz_property_type>(unwrap_prop(prop)->PropertyType());
}

int wz_property_is_raw_data(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_is_raw_data");
    return 0;
  }
  return unwrap_prop(prop)->IsRawDataProperty() ? 1 : 0;
}

int wz_property_is_video(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_is_video");
    return 0;
  }
  return unwrap_prop(prop)->IsVideoProperty() ? 1 : 0;
}

const char* wz_property_name(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_name");
    return nullptr;
  }
  static thread_local std::string s;
  s = unwrap_prop(prop)->Name();
  return s.c_str();
}

int wz_property_count_children(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_count_children");
    return 0;
  }
  auto* wps = unwrap_prop(prop)->WzProperties();
  return wps ? static_cast<int>(wps->size()) : 0;
}

wz_property wz_property_get_child(wz_property prop, int index) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_property_get_child");
    return nullptr;
  }
  auto* wps = unwrap_prop(prop)->WzProperties();
  if (!wps) {
    set_error_null("wz_property_get_child");
    return nullptr;
  }
  size_t sz = wps->size();
  if (index < 0 || index >= static_cast<int>(sz)) {
    set_error_idx("wz_property_get_child", index, sz);
    return nullptr;
  }
  return wrap_prop((*wps)[static_cast<size_t>(index)]);
}

wz_property wz_property_get_child_by_name(wz_property prop, const char* name) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_property_get_child_by_name");
    return nullptr;
  }
  if (!name) {
    set_error_null("wz_property_get_child_by_name");
    return nullptr;
  }
  auto* wps = unwrap_prop(prop)->WzProperties();
  if (!wps) {
    set_error_null("wz_property_get_child_by_name");
    return nullptr;
  }
  for (auto* c : *wps) {
    if (c && c->Name() == name) return wrap_prop(c);
  }
  return nullptr;
}

wz_property wz_property_get_from_path(wz_property prop, const char* path) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_property_get_from_path");
    return nullptr;
  }
  if (!path) {
    set_error_null("wz_property_get_from_path");
    return nullptr;
  }
  return wrap_prop(unwrap_prop(prop)->GetFromPath(path));
}

int32_t wz_property_get_int(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_get_int");
    return 0;
  }
  return unwrap_prop(prop)->GetInt();
}

int16_t wz_property_get_short(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_get_short");
    return 0;
  }
  return unwrap_prop(prop)->GetShort();
}

int64_t wz_property_get_long(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_get_long");
    return 0;
  }
  return unwrap_prop(prop)->GetLong();
}

float wz_property_get_float(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_get_float");
    return 0.0f;
  }
  return unwrap_prop(prop)->GetFloat();
}

double wz_property_get_double(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_get_double");
    return 0.0;
  }
  return unwrap_prop(prop)->GetDouble();
}

const char* wz_property_get_string(wz_property prop) {
  if (!prop) {
    set_error_null("wz_property_get_string");
    return nullptr;
  }
  static thread_local std::string s;
  s = unwrap_prop(prop)->GetString();
  return s.c_str();
}

size_t wz_property_get_bytes(wz_property prop,
                             uint8_t* buffer,
                             size_t buffer_size) {
  if (!prop) {
    set_error_null("wz_property_get_bytes");
    return 0;
  }
  auto result = unwrap_prop(prop)->GetBytes();
  if (result.is_err()) {
    set_error(result.err().code(), result.err().message());
    return 0;
  }
  auto data = result.ok();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return data.size();
}

// ==================== Scalar Properties ====================

int32_t wz_int_get_value(wz_property prop) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_int_get_value");
    return 0;
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Int) {
    set_error_wrong_type("wz_int_get_value");
    return 0;
  }
  return static_cast<wz::WzIntProperty*>(unwrap_prop(prop))->Value();
}

void wz_int_set_value(wz_property prop, int32_t value) {
  if (!prop) {
    set_error_null("wz_int_set_value");
    return;
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Int) {
    set_error_wrong_type("wz_int_set_value");
    return;
  }
  static_cast<wz::WzIntProperty*>(unwrap_prop(prop))->SetValue(value);
}

int16_t wz_short_get_value(wz_property prop) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_short_get_value");
    return 0;
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Short) {
    set_error_wrong_type("wz_short_get_value");
    return 0;
  }
  return static_cast<wz::WzShortProperty*>(unwrap_prop(prop))->Value();
}

int64_t wz_long_get_value(wz_property prop) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_long_get_value");
    return 0;
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Long) {
    set_error_wrong_type("wz_long_get_value");
    return 0;
  }
  return static_cast<wz::WzLongProperty*>(unwrap_prop(prop))->Value();
}

float wz_float_get_value(wz_property prop) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_float_get_value");
    return 0.0f;
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Float) {
    set_error_wrong_type("wz_float_get_value");
    return 0.0f;
  }
  return static_cast<wz::WzFloatProperty*>(unwrap_prop(prop))->Value();
}

double wz_double_get_value(wz_property prop) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_double_get_value");
    return 0.0;
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::Double) {
    set_error_wrong_type("wz_double_get_value");
    return 0.0;
  }
  return static_cast<wz::WzDoubleProperty*>(unwrap_prop(prop))->Value();
}

const char* wz_string_get_value(wz_property prop) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_string_get_value");
    return nullptr;
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::String) {
    set_error_wrong_type("wz_string_get_value");
    return nullptr;
  }
  static thread_local std::string s;
  s = static_cast<wz::WzStringProperty*>(unwrap_prop(prop))->Value();
  return s.c_str();
}

int wz_string_save_to_file(wz_property prop, const char* file_path) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_string_save_to_file");
    return 0;
  }
  if (!file_path) {
    set_error_null("wz_string_save_to_file");
    return 0;
  }
  if (unwrap_prop(prop)->PropertyType() != wz::WzPropertyType::String) {
    set_error_wrong_type("wz_string_save_to_file");
    return 0;
  }
  return static_cast<wz::WzStringProperty*>(unwrap_prop(prop))
                 ->SaveToFile(file_path)
             ? 1
             : 0;
}

// ==================== WzPngProperty ====================

int wz_png_width(wz_png_property png) {
  if (!png) {
    set_error_null("wz_png_width");
    return 0;
  }
  return reinterpret_cast<wz::WzPngProperty*>(png)->Width();
}

int wz_png_height(wz_png_property png) {
  if (!png) {
    set_error_null("wz_png_height");
    return 0;
  }
  return reinterpret_cast<wz::WzPngProperty*>(png)->Height();
}

int wz_png_format(wz_png_property png) {
  if (!png) {
    set_error_null("wz_png_format");
    return 0;
  }
  return static_cast<int>(reinterpret_cast<wz::WzPngProperty*>(png)->Format());
}

int wz_png_is_list_wz_used(wz_png_property png) {
  if (!png) {
    set_error_null("wz_png_is_list_wz_used");
    return 0;
  }
  return reinterpret_cast<wz::WzPngProperty*>(png)->ListWzUsed() ? 1 : 0;
}

size_t wz_png_get_image(wz_png_property png,
                        uint8_t* buffer,
                        size_t buffer_size) {
  wz_clear_error();
  if (!png) {
    set_error_null("wz_png_get_image");
    return 0;
  }
  auto result = reinterpret_cast<wz::WzPngProperty*>(png)->GetImage(false);
  if (!result.is_ok()) {
    set_error(result.err().code(), result.err().message());
    return 0;
  }
  auto& data = result.ok();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return data.size();
}

size_t wz_png_get_compressed_bytes(wz_png_property png,
                                   uint8_t* buffer,
                                   size_t buffer_size) {
  wz_clear_error();
  if (!png) {
    set_error_null("wz_png_get_compressed_bytes");
    return 0;
  }
  auto result =
      reinterpret_cast<wz::WzPngProperty*>(png)->GetCompressedBytes(false);
  if (!result.is_ok()) {
    set_error(result.err().code(), result.err().message());
    return 0;
  }
  auto& data = result.ok();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return data.size();
}

int wz_png_save_to_file(wz_png_property png, const char* file_path) {
  wz_clear_error();
  if (!png) {
    set_error_null("wz_png_save_to_file");
    return 0;
  }
  if (!file_path) {
    set_error_null("wz_png_save_to_file");
    return 0;
  }
  return reinterpret_cast<wz::WzPngProperty*>(png)->SaveToFile(file_path) ? 1
                                                                          : 0;
}

// ==================== WzCanvasProperty ====================

wz_png_property wz_canvas_get_png(wz_property canvas_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(canvas_prop);
  if (!p) {
    set_error_null("wz_canvas_get_png");
    return nullptr;
  }
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    set_error_wrong_type("wz_canvas_get_png");
    return nullptr;
  }
  return reinterpret_cast<wz_png_property>(
      static_cast<wz::WzCanvasProperty*>(p)->PngProperty());
}

int wz_canvas_contains_inlink(wz_property canvas_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(canvas_prop);
  if (!p) {
    set_error_null("wz_canvas_contains_inlink");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    set_error_wrong_type("wz_canvas_contains_inlink");
    return 0;
  }
  return static_cast<wz::WzCanvasProperty*>(p)->ContainsInlinkProperty() ? 1
                                                                         : 0;
}

int wz_canvas_contains_outlink(wz_property canvas_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(canvas_prop);
  if (!p) {
    set_error_null("wz_canvas_contains_outlink");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    set_error_wrong_type("wz_canvas_contains_outlink");
    return 0;
  }
  return static_cast<wz::WzCanvasProperty*>(p)->ContainsOutlinkProperty() ? 1
                                                                          : 0;
}

int wz_canvas_save_to_file(wz_property canvas_prop, const char* file_path) {
  wz_clear_error();
  auto* p = unwrap_prop(canvas_prop);
  if (!p) {
    set_error_null("wz_canvas_save_to_file");
    return 0;
  }
  if (!file_path) {
    set_error_null("wz_canvas_save_to_file");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    set_error_wrong_type("wz_canvas_save_to_file");
    return 0;
  }
  return static_cast<wz::WzCanvasProperty*>(p)->SaveToFile(file_path) ? 1 : 0;
}

wz_property wz_canvas_get_linked_wz_image_property(wz_property canvas_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(canvas_prop);
  if (!p) {
    set_error_null("wz_canvas_get_linked_wz_image_property");
    return nullptr;
  }
  if (p->PropertyType() != wz::WzPropertyType::Canvas) {
    set_error_wrong_type("wz_canvas_get_linked_wz_image_property");
    return nullptr;
  }
  auto* linked =
      static_cast<wz::WzCanvasProperty*>(p)->GetLinkedWzImageProperty();
  return linked ? wrap_prop(linked) : nullptr;
}

// ==================== WzUOLProperty ====================

const char* wz_uol_get_value(wz_property uol_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(uol_prop);
  if (!p) {
    set_error_null("wz_uol_get_value");
    return nullptr;
  }
  if (p->PropertyType() != wz::WzPropertyType::UOL) {
    set_error_wrong_type("wz_uol_get_value");
    return nullptr;
  }
  static thread_local std::string s;
  s = static_cast<wz::WzUOLProperty*>(p)->Value();
  return s.c_str();
}

wz_object wz_uol_get_link_value(wz_property uol_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(uol_prop);
  if (!p) {
    set_error_null("wz_uol_get_link_value");
    return nullptr;
  }
  if (p->PropertyType() != wz::WzPropertyType::UOL) {
    set_error_wrong_type("wz_uol_get_link_value");
    return nullptr;
  }
  return reinterpret_cast<wz_object>(
      static_cast<wz::WzUOLProperty*>(p)->LinkValue());
}

// ==================== Property Link Resolution ====================

wz_property wz_property_get_linked_wz_image_property(wz_property prop) {
  wz_clear_error();
  if (!prop) {
    set_error_null("wz_property_get_linked_wz_image_property");
    return nullptr;
  }
  auto* linked = unwrap_prop(prop)->GetLinkedWzImageProperty();
  return linked ? wrap_prop(linked) : nullptr;
}

// ==================== WzVectorProperty ====================

int32_t wz_vector_get_x(wz_property vec_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(vec_prop);
  if (!p) {
    set_error_null("wz_vector_get_x");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Vector) {
    set_error_wrong_type("wz_vector_get_x");
    return 0;
  }
  auto* vec = static_cast<wz::WzVectorProperty*>(p);
  return vec->X ? vec->X->Value() : 0;
}

int32_t wz_vector_get_y(wz_property vec_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(vec_prop);
  if (!p) {
    set_error_null("wz_vector_get_y");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Vector) {
    set_error_wrong_type("wz_vector_get_y");
    return 0;
  }
  auto* vec = static_cast<wz::WzVectorProperty*>(p);
  return vec->Y ? vec->Y->Value() : 0;
}

// ==================== WzLuaProperty ====================

size_t wz_lua_get_data(wz_property lua_prop,
                       uint8_t* buffer,
                       size_t buffer_size) {
  wz_clear_error();
  auto* p = unwrap_prop(lua_prop);
  if (!p) {
    set_error_null("wz_lua_get_data");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Lua) {
    set_error_wrong_type("wz_lua_get_data");
    return 0;
  }
  auto& data = static_cast<wz::WzLuaProperty*>(p)->Value();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return data.size();
}

const char* wz_lua_get_string(wz_property lua_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(lua_prop);
  if (!p) {
    set_error_null("wz_lua_get_string");
    return nullptr;
  }
  if (p->PropertyType() != wz::WzPropertyType::Lua) {
    set_error_wrong_type("wz_lua_get_string");
    return nullptr;
  }
  static thread_local std::string s;
  s = static_cast<wz::WzLuaProperty*>(p)->GetString();
  return s.c_str();
}

int wz_lua_save_to_file(wz_property lua_prop, const char* file_path) {
  wz_clear_error();
  auto* p = unwrap_prop(lua_prop);
  if (!p) {
    set_error_null("wz_lua_save_to_file");
    return 0;
  }
  if (!file_path) {
    set_error_null("wz_lua_save_to_file");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Lua) {
    set_error_wrong_type("wz_lua_save_to_file");
    return 0;
  }
  return static_cast<wz::WzLuaProperty*>(p)->SaveToFile(file_path) ? 1 : 0;
}

// ==================== WzBinaryProperty ====================

size_t wz_binary_get_data(wz_property binary_prop,
                          uint8_t* buffer,
                          size_t buffer_size) {
  wz_clear_error();
  auto* p = unwrap_prop(binary_prop);
  if (!p) {
    set_error_null("wz_binary_get_data");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    set_error_wrong_type("wz_binary_get_data");
    return 0;
  }
  auto result = static_cast<wz::WzBinaryProperty*>(p)->GetBytes(false);
  if (!result.is_ok()) {
    set_error(result.err().code(), result.err().message());
    return 0;
  }
  auto& data = result.ok();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return data.size();
}

size_t wz_binary_get_wav_playback(wz_property binary_prop,
                                  uint8_t* buffer,
                                  size_t buffer_size) {
  wz_clear_error();
  auto* p = unwrap_prop(binary_prop);
  if (!p) {
    set_error_null("wz_binary_get_wav_playback");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    set_error_wrong_type("wz_binary_get_wav_playback");
    return 0;
  }
  auto result =
      static_cast<wz::WzBinaryProperty*>(p)->GetBytesForWAVPlayback(false);
  if (!result.is_ok()) {
    set_error(result.err().code(), result.err().message());
    return 0;
  }
  auto& data = result.ok();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return data.size();
}

int wz_binary_get_length(wz_property binary_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(binary_prop);
  if (!p) {
    set_error_null("wz_binary_get_length");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    set_error_wrong_type("wz_binary_get_length");
    return 0;
  }
  return static_cast<wz::WzBinaryProperty*>(p)->Length();
}

int wz_binary_get_frequency(wz_property binary_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(binary_prop);
  if (!p) {
    set_error_null("wz_binary_get_frequency");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    set_error_wrong_type("wz_binary_get_frequency");
    return 0;
  }
  return static_cast<wz::WzBinaryProperty*>(p)->Frequency();
}

wz_binary_type wz_binary_get_type(wz_property binary_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(binary_prop);
  if (!p) {
    set_error_null("wz_binary_get_type");
    return WZ_BINARY_RAW;
  }
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    set_error_wrong_type("wz_binary_get_type");
    return WZ_BINARY_RAW;
  }
  return static_cast<wz_binary_type>(
      static_cast<wz::WzBinaryProperty*>(p)->Type());
}

int wz_binary_is_header_encrypted(wz_property binary_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(binary_prop);
  if (!p) {
    set_error_null("wz_binary_is_header_encrypted");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    set_error_wrong_type("wz_binary_is_header_encrypted");
    return 0;
  }
  return static_cast<wz::WzBinaryProperty*>(p)->HeaderEncrypted() ? 1 : 0;
}

int wz_binary_save_to_file(wz_property binary_prop, const char* file_path) {
  wz_clear_error();
  auto* p = unwrap_prop(binary_prop);
  if (!p) {
    set_error_null("wz_binary_save_to_file");
    return 0;
  }
  if (!file_path) {
    set_error_null("wz_binary_save_to_file");
    return 0;
  }
  if (p->PropertyType() != wz::WzPropertyType::Sound) {
    set_error_wrong_type("wz_binary_save_to_file");
    return 0;
  }
  return static_cast<wz::WzBinaryProperty*>(p)->SaveToFile(file_path) ? 1 : 0;
}

// ==================== WzRawDataProperty ====================

size_t wz_rawdata_get_data(wz_property raw_prop,
                           uint8_t* buffer,
                           size_t buffer_size) {
  wz_clear_error();
  auto* p = unwrap_prop(raw_prop);
  if (!p) {
    set_error_null("wz_rawdata_get_data");
    return 0;
  }
  if (!p->IsRawDataProperty()) {
    set_error_wrong_type("wz_rawdata_get_data");
    return 0;
  }
  auto result = static_cast<wz::WzRawDataProperty*>(p)->GetBytes(false);
  if (!result.is_ok()) {
    set_error(result.err().code(), result.err().message());
    return 0;
  }
  auto& data = result.ok();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return data.size();
}

int wz_rawdata_get_type(wz_property raw_prop) {
  wz_clear_error();
  auto* p = unwrap_prop(raw_prop);
  if (!p) {
    set_error_null("wz_rawdata_get_type");
    return 0;
  }
  if (!p->IsRawDataProperty()) {
    set_error_wrong_type("wz_rawdata_get_type");
    return 0;
  }
  return static_cast<int>(static_cast<wz::WzRawDataProperty*>(p)->Type());
}

int wz_rawdata_save_to_file(wz_property raw_prop, const char* file_path) {
  wz_clear_error();
  auto* p = unwrap_prop(raw_prop);
  if (!p) {
    set_error_null("wz_rawdata_save_to_file");
    return 0;
  }
  if (!file_path) {
    set_error_null("wz_rawdata_save_to_file");
    return 0;
  }
  if (!p->IsRawDataProperty()) {
    set_error_wrong_type("wz_rawdata_save_to_file");
    return 0;
  }
  return static_cast<wz::WzRawDataProperty*>(p)->SaveToFile(file_path) ? 1 : 0;
}

// ==================== WzVideoProperty ====================

size_t wz_video_get_data(wz_property video_prop,
                         uint8_t* buffer,
                         size_t buffer_size) {
  wz_clear_error();
  auto* p = unwrap_prop(video_prop);
  if (!p) {
    set_error_null("wz_video_get_data");
    return 0;
  }
  if (!p->IsVideoProperty()) {
    set_error_wrong_type("wz_video_get_data");
    return 0;
  }
  auto* vp = static_cast<wz::WzVideoProperty*>(p);
  auto result = vp->GetBytes(false);
  if (!result.is_ok()) {
    set_error(result.err().code(), result.err().message());
    return 0;
  }
  auto& data = result.ok();
  if (buffer && buffer_size >= data.size()) {
    std::memcpy(buffer, data.data(), data.size());
  }
  return data.size();
}

int wz_video_save_to_file(wz_property video_prop, const char* file_path) {
  wz_clear_error();
  auto* p = unwrap_prop(video_prop);
  if (!p) {
    set_error_null("wz_video_save_to_file");
    return 0;
  }
  if (!file_path) {
    set_error_null("wz_video_save_to_file");
    return 0;
  }
  if (!p->IsVideoProperty()) {
    set_error_wrong_type("wz_video_save_to_file");
    return 0;
  }
  auto* vp = static_cast<wz::WzVideoProperty*>(p);
  return vp->SaveToFile(file_path) ? 1 : 0;
}

// ==================== Utility ====================

const char* wz_get_error_description(wz_parse_status status) {
  static thread_local std::string s;
  s = wz::GetErrorDescription(static_cast<wz::WzFileParseStatus>(status));
  return s.c_str();
}

wz_maple_version wz_detect_maple_version(const char* file_path,
                                         int16_t* out_version) {
  int16_t ver = 0;
  auto mv = wz::WzTool::DetectMapleVersion(file_path, &ver);
  if (out_version) *out_version = ver;
  return static_cast<wz_maple_version>(mv);
}

const uint8_t* wz_get_iv_for_version(wz_maple_version ver) {
  static thread_local std::array<uint8_t, 4> iv;
  iv = wz::WzTool::GetIvByMapleVersion(static_cast<wz::WzMapleVersion>(ver));
  return iv.data();
}

}  // extern "C"

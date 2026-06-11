#ifndef WZ_WZ_API_H_
#define WZ_WZ_API_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle types
typedef struct wz_file_handle* wz_file;
typedef struct wz_dir_handle* wz_dir;
typedef struct wz_image_handle* wz_image;
typedef struct wz_property_handle* wz_property;
typedef struct wz_png_property_handle* wz_png_property;
typedef struct wz_object_handle* wz_object;

typedef enum {
  WZ_GMS = 0,
  WZ_EMS = 1,
  WZ_BMS = 2,
  WZ_CLASSIC = 3,
  WZ_GENERATE = 4,
  WZ_GETFROMZLZ = 5,
  WZ_CUSTOM = 6,
  WZ_UNKNOWN = 99
} wz_maple_version;

typedef enum {
  WZ_PARSE_PATH_IS_NULL = -1,
  WZ_PARSE_ERROR_GAME_VER_HASH = -2,
  WZ_PARSE_FAILED_UNKNOWN = 0,
  WZ_PARSE_SUCCESS = 1
} wz_parse_status;

typedef enum {
  WZ_PROP_NULL = 0,
  WZ_PROP_SHORT = 1,
  WZ_PROP_INT = 2,
  WZ_PROP_LONG = 3,
  WZ_PROP_FLOAT = 4,
  WZ_PROP_DOUBLE = 5,
  WZ_PROP_STRING = 6,
  WZ_PROP_SUB = 7,
  WZ_PROP_CANVAS = 8,
  WZ_PROP_VECTOR = 9,
  WZ_PROP_CONVEX = 10,
  WZ_PROP_SOUND = 11,
  WZ_PROP_RAW = 12,
  WZ_PROP_UOL = 13,
  WZ_PROP_LUA = 14,
  WZ_PROP_PNG = 15
} wz_property_type;

typedef enum {
  WZ_OBJECT_FILE = 0,
  WZ_OBJECT_IMAGE = 1,
  WZ_OBJECT_DIRECTORY = 2,
  WZ_OBJECT_PROPERTY = 3,
  WZ_OBJECT_LIST = 4
} wz_object_type;

typedef enum {
  WZ_BINARY_RAW = 0,
  WZ_BINARY_MP3 = 1,
  WZ_BINARY_WAV = 2
} wz_binary_type;

typedef enum {
  WZ_ERROR_NONE = 0,
  WZ_ERROR_NOT_IMPLEMENTED,
  WZ_ERROR_INVALID_ARGUMENT,
  WZ_ERROR_PARSE_ERROR,
  WZ_ERROR_IO_ERROR,
  WZ_ERROR_DATA_ERROR,
  WZ_ERROR_NOT_FOUND,
  WZ_ERROR_NULL_HANDLE,
  WZ_ERROR_WRONG_TYPE,
  WZ_ERROR_INDEX_OUT_OF_RANGE,
} wz_error_code;

void wz_clear_error(void);
wz_error_code wz_get_last_error(void);
const char* wz_get_last_error_message(void);

// ==================== WzFile ====================

wz_file wz_open_file(const char* file_path,
                     short game_version,
                     wz_maple_version version);
wz_file wz_open_file_with_iv(const char* file_path, const uint8_t iv[4]);
wz_parse_status wz_parse(wz_file file);
void wz_close_file(wz_file file);

const char* wz_file_name(wz_file file);
const char* wz_file_path(wz_file file);
short wz_file_version(wz_file file);
wz_maple_version wz_file_maple_version(wz_file file);
wz_dir wz_file_get_wz_directory(wz_file file);
int wz_file_is_64bit(wz_file file);
int wz_file_is_unloaded(wz_file file);
uint32_t wz_file_version_hash(wz_file file);
wz_object wz_file_get_object_from_path(wz_file file,
                                       const char* path,
                                       int checkFirstDirectoryName);

// ==================== WzDirectory ====================

const char* wz_dir_name(wz_dir dir);
int wz_dir_count_images(wz_dir dir);
int wz_dir_count_directories(wz_dir dir);
int wz_dir_count_images_total(wz_dir dir);
wz_image wz_dir_get_image(wz_dir dir, int index);
wz_image wz_dir_get_image_by_name(wz_dir dir, const char* name);
wz_dir wz_dir_get_directory(wz_dir dir, int index);
wz_dir wz_dir_get_directory_by_name(wz_dir dir, const char* name);
int wz_dir_block_size(wz_dir dir);
int wz_dir_checksum(wz_dir dir);
int64_t wz_dir_offset(wz_dir dir);

// ==================== WzImage ====================

const char* wz_image_name(wz_image img);
int wz_image_is_parsed(wz_image img);
int wz_image_is_changed(wz_image img);
int wz_image_block_size(wz_image img);
int wz_image_checksum(wz_image img);
int64_t wz_image_offset(wz_image img);
int wz_image_is_lua_wz_image(wz_image img);
void wz_image_parse(wz_image img);
int wz_image_count_properties(wz_image img);
wz_property wz_image_get_property(wz_image img, int index);
wz_property wz_image_get_from_path(wz_image img, const char* path);

// ==================== WzObject / WzProperty ====================

wz_object_type wz_get_object_type(wz_object obj);
const char* wz_object_name(wz_object obj);
wz_object wz_object_get_parent(wz_object obj);
const char* wz_object_full_path(wz_object obj);
wz_file wz_object_get_wz_file_parent(wz_object obj);
wz_object wz_object_get_top_most_wz_directory(wz_object obj);
wz_object wz_object_get_top_most_wz_image(wz_object obj);
wz_object wz_object_at(wz_object obj, const char* name);

wz_property_type wz_property_get_type(wz_property prop);
const char* wz_property_name(wz_property prop);
int wz_property_count_children(wz_property prop);
wz_property wz_property_get_child(wz_property prop, int index);
wz_property wz_property_get_child_by_name(wz_property prop, const char* name);
wz_property wz_property_get_from_path(wz_property prop, const char* path);

// Cast methods
int32_t wz_property_get_int(wz_property prop);
int16_t wz_property_get_short(wz_property prop);
int64_t wz_property_get_long(wz_property prop);
float wz_property_get_float(wz_property prop);
double wz_property_get_double(wz_property prop);
const char* wz_property_get_string(wz_property prop);
size_t wz_property_get_bytes(wz_property prop,
                             uint8_t* buffer,
                             size_t buffer_size);

// ==================== Scalar Properties ====================

// WzIntProperty
int32_t wz_int_get_value(wz_property prop);
void wz_int_set_value(wz_property prop, int32_t value);

// WzShortProperty
int16_t wz_short_get_value(wz_property prop);

// WzLongProperty
int64_t wz_long_get_value(wz_property prop);

// WzFloatProperty
float wz_float_get_value(wz_property prop);

// WzDoubleProperty
double wz_double_get_value(wz_property prop);

// WzStringProperty
const char* wz_string_get_value(wz_property prop);
int wz_string_save_to_file(wz_property prop, const char* file_path);

// ==================== WzPngProperty ====================

int wz_png_width(wz_png_property png);
int wz_png_height(wz_png_property png);
int wz_png_format(wz_png_property png);
int wz_png_is_list_wz_used(wz_png_property png);
size_t wz_png_get_image(wz_png_property png,
                        uint8_t* buffer,
                        size_t buffer_size);
size_t wz_png_get_compressed_bytes(wz_png_property png,
                                   uint8_t* buffer,
                                   size_t buffer_size);
int wz_png_save_to_file(wz_png_property png, const char* file_path);

// ==================== WzCanvasProperty ====================

wz_png_property wz_canvas_get_png(wz_property canvas_prop);
int wz_canvas_contains_inlink(wz_property canvas_prop);
int wz_canvas_contains_outlink(wz_property canvas_prop);
int wz_canvas_save_to_file(wz_property canvas_prop, const char* file_path);

// ==================== WzUOLProperty ====================

const char* wz_uol_get_value(wz_property uol_prop);

// ==================== Property Link Resolution ====================

wz_property wz_property_get_linked_wz_image_property(wz_property prop);

// ==================== WzVectorProperty ====================

int32_t wz_vector_get_x(wz_property vec_prop);
int32_t wz_vector_get_y(wz_property vec_prop);

// ==================== WzConvexProperty ====================

// Uses generic property children methods (wz_property_count_children etc.)

// ==================== WzLuaProperty ====================

size_t wz_lua_get_data(wz_property lua_prop,
                       uint8_t* buffer,
                       size_t buffer_size);
const char* wz_lua_get_string(wz_property lua_prop);
int wz_lua_save_to_file(wz_property lua_prop, const char* file_path);

// ==================== WzBinaryProperty (Sound) ====================

size_t wz_binary_get_data(wz_property binary_prop,
                          uint8_t* buffer,
                          size_t buffer_size);
size_t wz_binary_get_wav_playback(wz_property binary_prop,
                                  uint8_t* buffer,
                                  size_t buffer_size);
int wz_binary_get_length(wz_property binary_prop);
int wz_binary_get_frequency(wz_property binary_prop);
wz_binary_type wz_binary_get_type(wz_property binary_prop);
int wz_binary_is_header_encrypted(wz_property binary_prop);
int wz_binary_save_to_file(wz_property binary_prop, const char* file_path);

// ==================== WzRawDataProperty ====================

size_t wz_rawdata_get_data(wz_property raw_prop,
                           uint8_t* buffer,
                           size_t buffer_size);
int wz_rawdata_get_type(wz_property raw_prop);
int wz_rawdata_save_to_file(wz_property raw_prop, const char* file_path);

// ==================== WzVideoProperty ====================

size_t wz_video_get_data(wz_property video_prop,
                         uint8_t* buffer,
                         size_t buffer_size);
int wz_video_save_to_file(wz_property video_prop, const char* file_path);

// ==================== Utility ====================

const char* wz_get_error_description(wz_parse_status status);
wz_maple_version wz_detect_maple_version(const char* file_path,
                                         short* out_version);
const uint8_t* wz_get_iv_for_version(wz_maple_version ver);

#ifdef __cplusplus
}
#endif

#endif  // WZ_WZ_API_H_

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

typedef struct wz_last_error_info {
  wz_error_code code;
  const char* message;
} wz_last_error_info;

typedef int (*wz_blob_read_callback)(void* userdata,
                                     uint64_t offset,
                                     uint8_t* destination,
                                     size_t length,
                                     size_t* out_bytes_read);
typedef void (*wz_blob_release_callback)(void* userdata);

wz_error_code wz_get_last_error_info(const wz_last_error_info** info);

// ==================== WzFile ====================

wz_error_code wz_open_file(const char* file_path,
                           short game_version,
                           wz_maple_version version,
                           wz_file* out_file);
wz_error_code wz_open_memory(const char* file_name,
                             const uint8_t* data,
                             size_t data_size,
                             short game_version,
                             wz_maple_version version,
                             wz_file* out_file);
wz_error_code wz_open_blob_source_with_callback(
    void* userdata,
    wz_blob_read_callback read_callback,
    wz_blob_release_callback release_callback,
    uint64_t size,
    const char* file_name,
    short game_version,
    wz_maple_version version,
    wz_file* out_file);
wz_error_code wz_create_file(short game_version,
                             wz_maple_version version,
                             wz_file* out_file);
wz_error_code wz_open_file_with_iv(const char* file_path,
                                   const uint8_t iv[4],
                                   wz_file* out_file);
wz_error_code wz_open_memory_with_iv(const char* file_name,
                                     const uint8_t* data,
                                     size_t data_size,
                                     const uint8_t iv[4],
                                     wz_file* out_file);
wz_error_code wz_open_blob_source_with_callback_and_iv(
    void* userdata,
    wz_blob_read_callback read_callback,
    wz_blob_release_callback release_callback,
    uint64_t size,
    const char* file_name,
    const uint8_t iv[4],
    wz_file* out_file);
wz_error_code wz_parse(wz_file file, wz_parse_status* out_status);
wz_error_code wz_close_file(wz_file file);
wz_error_code wz_file_save_to_disk(wz_file file, const char* file_path);

wz_error_code wz_file_name(wz_file file, const char** out_name);
wz_error_code wz_file_path(wz_file file, const char** out_path);
wz_error_code wz_file_version(wz_file file, short* out_version);
wz_error_code wz_file_maple_version(wz_file file,
                                    wz_maple_version* out_version);
wz_error_code wz_file_get_wz_directory(wz_file file, wz_dir* out_dir);
wz_error_code wz_file_is_64bit(wz_file file, int* out_is_64bit);
wz_error_code wz_file_is_unloaded(wz_file file, int* out_is_unloaded);
wz_error_code wz_file_version_hash(wz_file file, uint32_t* out_hash);
wz_error_code wz_file_get_object_from_path(wz_file file,
                                           const char* path,
                                           int checkFirstDirectoryName,
                                           wz_object* out_object);

// ==================== WzDirectory ====================

wz_error_code wz_dir_name(wz_dir dir, const char** out_name);
wz_error_code wz_dir_count_images(wz_dir dir, int* out_count);
wz_error_code wz_dir_count_directories(wz_dir dir, int* out_count);
wz_error_code wz_dir_count_images_total(wz_dir dir, int* out_count);
wz_error_code wz_dir_get_image(wz_dir dir, int index, wz_image* out_image);
wz_error_code wz_dir_get_image_by_name(wz_dir dir,
                                       const char* name,
                                       wz_image* out_image);
wz_error_code wz_dir_get_directory(wz_dir dir, int index, wz_dir* out_dir);
wz_error_code wz_dir_get_directory_by_name(wz_dir dir,
                                           const char* name,
                                           wz_dir* out_dir);
wz_error_code wz_dir_create_directory(wz_dir dir,
                                      const char* name,
                                      wz_dir* out_dir);
wz_error_code wz_dir_create_image(wz_dir dir,
                                  const char* name,
                                  wz_image* out_image);
wz_error_code wz_dir_remove_directory(wz_dir dir, wz_dir child);
wz_error_code wz_dir_remove_image(wz_dir dir, wz_image child);
wz_error_code wz_dir_block_size(wz_dir dir, int* out_block_size);
wz_error_code wz_dir_checksum(wz_dir dir, int* out_checksum);
wz_error_code wz_dir_offset(wz_dir dir, int64_t* out_offset);

// ==================== WzImage ====================

wz_error_code wz_image_name(wz_image img, const char** out_name);
wz_error_code wz_image_is_parsed(wz_image img, int* out_is_parsed);
wz_error_code wz_image_is_changed(wz_image img, int* out_is_changed);
wz_error_code wz_image_block_size(wz_image img, int* out_block_size);
wz_error_code wz_image_checksum(wz_image img, int* out_checksum);
wz_error_code wz_image_offset(wz_image img, int64_t* out_offset);
wz_error_code wz_image_is_lua_wz_image(wz_image img, int* out_is_lua);
wz_error_code wz_image_parse(wz_image img, int* out_parsed);
wz_error_code wz_image_count_properties(wz_image img, int* out_count);
wz_error_code wz_image_get_property(wz_image img,
                                    int index,
                                    wz_property* out_property);
wz_error_code wz_image_get_from_path(wz_image img,
                                     const char* path,
                                     wz_property* out_property);

// ==================== WzObject / WzProperty ====================

wz_error_code wz_get_object_type(wz_object obj, wz_object_type* out_type);
wz_error_code wz_object_name(wz_object obj, const char** out_name);
wz_error_code wz_object_get_parent(wz_object obj, wz_object* out_parent);
wz_error_code wz_object_full_path(wz_object obj, const char** out_path);
wz_error_code wz_object_get_wz_file_parent(wz_object obj, wz_file* out_file);
wz_error_code wz_object_get_top_most_wz_directory(wz_object obj,
                                                  wz_object* out_object);
wz_error_code wz_object_get_top_most_wz_image(wz_object obj,
                                              wz_object* out_object);
wz_error_code wz_object_at(wz_object obj,
                           const char* name,
                           wz_object* out_object);
wz_error_code wz_object_set_name(wz_object obj, const char* name);
// Removes obj from its parent and transfers ownership back to the caller.
// Call wz_object_free(obj) unless obj is re-added to another container.
wz_error_code wz_object_remove(wz_object obj);
// Frees a detached caller-owned directory, image, or property object. File
// objects must be closed with wz_close_file.
wz_error_code wz_object_free(wz_object obj);

wz_error_code wz_property_get_type(wz_property prop,
                                   wz_property_type* out_type);
wz_error_code wz_property_is_raw_data(wz_property prop, int* out_is_raw);
wz_error_code wz_property_is_video(wz_property prop, int* out_is_video);
wz_error_code wz_property_name(wz_property prop, const char** out_name);
wz_error_code wz_property_count_children(wz_property prop, int* out_count);
wz_error_code wz_property_get_child(wz_property prop,
                                    int index,
                                    wz_property* out_property);
wz_error_code wz_property_get_child_by_name(wz_property prop,
                                            const char* name,
                                            wz_property* out_property);
wz_error_code wz_property_get_from_path(wz_property prop,
                                        const char* path,
                                        wz_property* out_property);

// Property handles created by wz_property_create_* are owned by the caller
// until passed to wz_image_add_property or wz_property_add_child. After a
// successful add call, ownership transfers to the WZ tree and the caller must
// not separately dispose or free the property handle. If a created property is
// not added to a WZ tree, or an add call fails, release it with
// wz_property_free or wz_object_free. Do not call free APIs for properties
// owned by a WZ tree; use the remove APIs instead.
wz_error_code wz_property_create_null(const char* name,
                                      wz_property* out_property);
wz_error_code wz_property_create_short(const char* name,
                                       int16_t value,
                                       wz_property* out_property);
wz_error_code wz_property_create_int(const char* name,
                                     int32_t value,
                                     wz_property* out_property);
wz_error_code wz_property_create_long(const char* name,
                                      int64_t value,
                                      wz_property* out_property);
wz_error_code wz_property_create_float(const char* name,
                                       float value,
                                       wz_property* out_property);
wz_error_code wz_property_create_double(const char* name,
                                        double value,
                                        wz_property* out_property);
wz_error_code wz_property_create_string(const char* name,
                                        const char* value,
                                        wz_property* out_property);
wz_error_code wz_property_create_sub(const char* name,
                                     wz_property* out_property);
wz_error_code wz_property_create_vector(const char* name,
                                        int32_t x,
                                        int32_t y,
                                        wz_property* out_property);
wz_error_code wz_property_create_uol(const char* name,
                                     const char* value,
                                     wz_property* out_property);
wz_error_code wz_property_free(wz_property prop);
wz_error_code wz_image_add_property(wz_image img, wz_property prop);
wz_error_code wz_property_add_child(wz_property parent, wz_property child);
// Removes prop from img and transfers ownership back to the caller.
// Call wz_property_free(prop) unless prop is re-added to another container.
wz_error_code wz_image_remove_property(wz_image img, wz_property prop);
// Removes child from parent and transfers ownership back to the caller.
// Call wz_property_free(child) unless child is re-added to another container.
wz_error_code wz_property_remove_child(wz_property parent, wz_property child);
// Clears and destroys all child properties owned by the container.
wz_error_code wz_image_clear_properties(wz_image img);
// Clears and destroys all child properties owned by the container.
wz_error_code wz_property_clear_children(wz_property prop);

// Cast methods
wz_error_code wz_property_get_int(wz_property prop, int32_t* out_value);
wz_error_code wz_property_get_short(wz_property prop, int16_t* out_value);
wz_error_code wz_property_get_long(wz_property prop, int64_t* out_value);
wz_error_code wz_property_get_float(wz_property prop, float* out_value);
wz_error_code wz_property_get_double(wz_property prop, double* out_value);
wz_error_code wz_property_get_string(wz_property prop, const char** out_value);
wz_error_code wz_property_get_bytes(wz_property prop,
                                    uint8_t* buffer,
                                    size_t buffer_size,
                                    size_t* out_size);

// ==================== Scalar Properties ====================

// WzIntProperty
wz_error_code wz_int_get_value(wz_property prop, int32_t* out_value);
wz_error_code wz_int_set_value(wz_property prop, int32_t value);

// WzShortProperty
wz_error_code wz_short_get_value(wz_property prop, int16_t* out_value);
wz_error_code wz_short_set_value(wz_property prop, int16_t value);

// WzLongProperty
wz_error_code wz_long_get_value(wz_property prop, int64_t* out_value);
wz_error_code wz_long_set_value(wz_property prop, int64_t value);

// WzFloatProperty
wz_error_code wz_float_get_value(wz_property prop, float* out_value);
wz_error_code wz_float_set_value(wz_property prop, float value);

// WzDoubleProperty
wz_error_code wz_double_get_value(wz_property prop, double* out_value);
wz_error_code wz_double_set_value(wz_property prop, double value);

// WzStringProperty
wz_error_code wz_string_get_value(wz_property prop, const char** out_value);
wz_error_code wz_string_set_value(wz_property prop, const char* value);

// ==================== WzPngProperty ====================

wz_error_code wz_png_width(wz_png_property png, int* out_width);
wz_error_code wz_png_height(wz_png_property png, int* out_height);
wz_error_code wz_png_format(wz_png_property png, int* out_format);
wz_error_code wz_png_is_list_wz_used(wz_png_property png, int* out_is_used);
wz_error_code wz_png_get_image(wz_png_property png,
                               uint8_t* buffer,
                               size_t buffer_size,
                               size_t* out_size);
wz_error_code wz_png_get_compressed_bytes(wz_png_property png,
                                          uint8_t* buffer,
                                          size_t buffer_size,
                                          size_t* out_size);
wz_error_code wz_png_save_to_file(wz_png_property png, const char* file_path);

// ==================== WzCanvasProperty ====================

wz_error_code wz_canvas_get_png(wz_property canvas_prop,
                                wz_png_property* out_png);
wz_error_code wz_canvas_contains_inlink(wz_property canvas_prop,
                                        int* out_contains);
wz_error_code wz_canvas_contains_outlink(wz_property canvas_prop,
                                         int* out_contains);
wz_error_code wz_canvas_save_to_file(wz_property canvas_prop,
                                     const char* file_path);
wz_error_code wz_canvas_get_linked_wz_image_property(wz_property canvas_prop,
                                                     wz_property* out_property);

// ==================== WzUOLProperty ====================

wz_error_code wz_uol_get_value(wz_property uol_prop, const char** out_value);
wz_error_code wz_uol_set_value(wz_property uol_prop, const char* value);
wz_error_code wz_uol_get_link_value(wz_property uol_prop,
                                    wz_object* out_object);

// ==================== Property Link Resolution ====================

wz_error_code wz_property_get_linked_wz_image_property(
    wz_property prop, wz_property* out_property);

// ==================== WzVectorProperty ====================

wz_error_code wz_vector_get_x(wz_property vec_prop, int32_t* out_value);
wz_error_code wz_vector_get_y(wz_property vec_prop, int32_t* out_value);

// ==================== WzConvexProperty ====================

// Uses generic property children methods (wz_property_count_children etc.)

// ==================== WzLuaProperty ====================

wz_error_code wz_lua_get_data(wz_property lua_prop,
                              uint8_t* buffer,
                              size_t buffer_size,
                              size_t* out_size);
wz_error_code wz_lua_get_string(wz_property lua_prop, const char** out_value);

// ==================== WzBinaryProperty (Sound) ====================

wz_error_code wz_binary_get_data(wz_property binary_prop,
                                 uint8_t* buffer,
                                 size_t buffer_size,
                                 size_t* out_size);
wz_error_code wz_binary_get_wav_playback(wz_property binary_prop,
                                         uint8_t* buffer,
                                         size_t buffer_size,
                                         size_t* out_size);
wz_error_code wz_binary_get_length(wz_property binary_prop, int* out_length);
wz_error_code wz_binary_get_frequency(wz_property binary_prop,
                                      int* out_frequency);
wz_error_code wz_binary_get_type(wz_property binary_prop,
                                 wz_binary_type* out_type);
wz_error_code wz_binary_is_header_encrypted(wz_property binary_prop,
                                            int* out_is_encrypted);
wz_error_code wz_binary_save_to_file(wz_property binary_prop,
                                     const char* file_path);

// ==================== WzRawDataProperty ====================

wz_error_code wz_rawdata_get_data(wz_property raw_prop,
                                  uint8_t* buffer,
                                  size_t buffer_size,
                                  size_t* out_size);
wz_error_code wz_rawdata_get_type(wz_property raw_prop, int* out_type);

// ==================== WzVideoProperty ====================

wz_error_code wz_video_get_data(wz_property video_prop,
                                uint8_t* buffer,
                                size_t buffer_size,
                                size_t* out_size);

// ==================== Utility ====================

wz_error_code wz_get_error_description(wz_parse_status status,
                                       const char** out_description);
wz_error_code wz_detect_maple_version(const char* file_path,
                                      wz_maple_version* out_maple_version,
                                      short* out_version);
wz_error_code wz_get_iv_for_version(wz_maple_version ver,
                                    const uint8_t** out_iv);

#ifdef __cplusplus
}
#endif

#endif  // WZ_WZ_API_H_

#ifndef SRC_CAPI_WZ_API_COMPAT_HPP_
#define SRC_CAPI_WZ_API_COMPAT_HPP_

#include "wz/wz_api.h"

template <typename T, typename Fn, typename... Args>
static inline T wz_capi_value_compat(Fn fn, T fallback, Args... args) {
  T out = fallback;
  if (fn(args..., &out) != WZ_ERROR_NONE) {
    return fallback;
  }
  return out;
}

static inline size_t wz_property_get_bytes_compat(wz_property prop,
                                                  uint8_t* buffer,
                                                  size_t buffer_size) {
  size_t out_size = 0;
  if (::wz_property_get_bytes(prop, buffer, buffer_size, &out_size) !=
      WZ_ERROR_NONE) {
    return 0;
  }
  return out_size;
}

static inline size_t wz_png_get_image_compat(wz_png_property png,
                                             uint8_t* buffer,
                                             size_t buffer_size) {
  size_t out_size = 0;
  if (::wz_png_get_image(png, buffer, buffer_size, &out_size) !=
      WZ_ERROR_NONE) {
    return 0;
  }
  return out_size;
}

static inline size_t wz_png_get_compressed_bytes_compat(wz_png_property png,
                                                        uint8_t* buffer,
                                                        size_t buffer_size) {
  size_t out_size = 0;
  if (::wz_png_get_compressed_bytes(png, buffer, buffer_size, &out_size) !=
      WZ_ERROR_NONE) {
    return 0;
  }
  return out_size;
}

static inline size_t wz_lua_get_data_compat(wz_property prop,
                                            uint8_t* buffer,
                                            size_t buffer_size) {
  size_t out_size = 0;
  if (::wz_lua_get_data(prop, buffer, buffer_size, &out_size) !=
      WZ_ERROR_NONE) {
    return 0;
  }
  return out_size;
}

static inline size_t wz_binary_get_data_compat(wz_property prop,
                                               uint8_t* buffer,
                                               size_t buffer_size) {
  size_t out_size = 0;
  if (::wz_binary_get_data(prop, buffer, buffer_size, &out_size) !=
      WZ_ERROR_NONE) {
    return 0;
  }
  return out_size;
}

static inline size_t wz_binary_get_wav_playback_compat(wz_property prop,
                                                       uint8_t* buffer,
                                                       size_t buffer_size) {
  size_t out_size = 0;
  if (::wz_binary_get_wav_playback(prop, buffer, buffer_size, &out_size) !=
      WZ_ERROR_NONE) {
    return 0;
  }
  return out_size;
}

static inline size_t wz_rawdata_get_data_compat(wz_property prop,
                                                uint8_t* buffer,
                                                size_t buffer_size) {
  size_t out_size = 0;
  if (::wz_rawdata_get_data(prop, buffer, buffer_size, &out_size) !=
      WZ_ERROR_NONE) {
    return 0;
  }
  return out_size;
}

static inline size_t wz_video_get_data_compat(wz_property prop,
                                              uint8_t* buffer,
                                              size_t buffer_size) {
  size_t out_size = 0;
  if (::wz_video_get_data(prop, buffer, buffer_size, &out_size) !=
      WZ_ERROR_NONE) {
    return 0;
  }
  return out_size;
}

static inline wz_file wz_open_file_compat(const char* file_path,
                                          short game_version,
                                          wz_maple_version version) {
  return wz_capi_value_compat<wz_file>(
      ::wz_open_file, nullptr, file_path, game_version, version);
}

static inline wz_file wz_open_file_with_iv_compat(const char* file_path,
                                                  const uint8_t iv[4]) {
  return wz_capi_value_compat<wz_file>(
      ::wz_open_file_with_iv, nullptr, file_path, iv);
}

static inline wz_parse_status wz_parse_compat(wz_file file) {
  return wz_capi_value_compat<wz_parse_status>(
      ::wz_parse, WZ_PARSE_FAILED_UNKNOWN, file);
}

static inline const char* wz_get_error_description_compat(wz_parse_status s) {
  return wz_capi_value_compat<const char*>(
      ::wz_get_error_description, nullptr, s);
}

static inline void wz_image_parse_compat(wz_image img) {
  int out_parsed = 0;
  (void)::wz_image_parse(img, &out_parsed);
}

static inline wz_maple_version wz_detect_maple_version_compat(
    const char* file_path, short* out_version) {
  wz_maple_version out_maple_version = WZ_UNKNOWN;
  if (::wz_detect_maple_version(file_path, &out_maple_version, out_version) !=
      WZ_ERROR_NONE) {
    return WZ_UNKNOWN;
  }
  return out_maple_version;
}

static inline const uint8_t* wz_get_iv_for_version_compat(
    wz_maple_version version) {
  return wz_capi_value_compat<const uint8_t*>(
      ::wz_get_iv_for_version, nullptr, version);
}

static inline int wz_result_to_success_compat(wz_error_code code) {
  return code == WZ_ERROR_NONE ? 1 : 0;
}

#define wz_open_file wz_open_file_compat
#define wz_open_file_with_iv wz_open_file_with_iv_compat
#define wz_parse wz_parse_compat
#define wz_file_name(...)                                                      \
  wz_capi_value_compat<const char*>(::wz_file_name, nullptr, __VA_ARGS__)
#define wz_file_path(...)                                                      \
  wz_capi_value_compat<const char*>(::wz_file_path, nullptr, __VA_ARGS__)
#define wz_file_version(...)                                                   \
  wz_capi_value_compat<short>(::wz_file_version, 0, __VA_ARGS__)
#define wz_file_maple_version(...)                                             \
  wz_capi_value_compat<wz_maple_version>(                                      \
      ::wz_file_maple_version, WZ_UNKNOWN, __VA_ARGS__)
#define wz_file_get_wz_directory(...)                                          \
  wz_capi_value_compat<wz_dir>(::wz_file_get_wz_directory, nullptr, __VA_ARGS__)
#define wz_file_is_64bit(...)                                                  \
  wz_capi_value_compat<int>(::wz_file_is_64bit, 0, __VA_ARGS__)
#define wz_file_is_unloaded(...)                                               \
  wz_capi_value_compat<int>(::wz_file_is_unloaded, 0, __VA_ARGS__)
#define wz_file_version_hash(...)                                              \
  wz_capi_value_compat<uint32_t>(::wz_file_version_hash, 0, __VA_ARGS__)
#define wz_file_get_object_from_path(...)                                      \
  wz_capi_value_compat<wz_object>(                                             \
      ::wz_file_get_object_from_path, nullptr, __VA_ARGS__)
#define wz_dir_name(...)                                                       \
  wz_capi_value_compat<const char*>(::wz_dir_name, nullptr, __VA_ARGS__)
#define wz_dir_count_images(...)                                               \
  wz_capi_value_compat<int>(::wz_dir_count_images, 0, __VA_ARGS__)
#define wz_dir_count_directories(...)                                          \
  wz_capi_value_compat<int>(::wz_dir_count_directories, 0, __VA_ARGS__)
#define wz_dir_count_images_total(...)                                         \
  wz_capi_value_compat<int>(::wz_dir_count_images_total, 0, __VA_ARGS__)
#define wz_dir_get_image(...)                                                  \
  wz_capi_value_compat<wz_image>(::wz_dir_get_image, nullptr, __VA_ARGS__)
#define wz_dir_get_image_by_name(...)                                          \
  wz_capi_value_compat<wz_image>(                                              \
      ::wz_dir_get_image_by_name, nullptr, __VA_ARGS__)
#define wz_dir_get_directory(...)                                              \
  wz_capi_value_compat<wz_dir>(::wz_dir_get_directory, nullptr, __VA_ARGS__)
#define wz_dir_get_directory_by_name(...)                                      \
  wz_capi_value_compat<wz_dir>(                                                \
      ::wz_dir_get_directory_by_name, nullptr, __VA_ARGS__)
#define wz_dir_block_size(...)                                                 \
  wz_capi_value_compat<int>(::wz_dir_block_size, 0, __VA_ARGS__)
#define wz_dir_checksum(...)                                                   \
  wz_capi_value_compat<int>(::wz_dir_checksum, 0, __VA_ARGS__)
#define wz_dir_offset(...)                                                     \
  wz_capi_value_compat<int64_t>(::wz_dir_offset, 0, __VA_ARGS__)
#define wz_image_name(...)                                                     \
  wz_capi_value_compat<const char*>(::wz_image_name, nullptr, __VA_ARGS__)
#define wz_image_is_parsed(...)                                                \
  wz_capi_value_compat<int>(::wz_image_is_parsed, 0, __VA_ARGS__)
#define wz_image_is_changed(...)                                               \
  wz_capi_value_compat<int>(::wz_image_is_changed, 0, __VA_ARGS__)
#define wz_image_block_size(...)                                               \
  wz_capi_value_compat<int>(::wz_image_block_size, 0, __VA_ARGS__)
#define wz_image_checksum(...)                                                 \
  wz_capi_value_compat<int>(::wz_image_checksum, 0, __VA_ARGS__)
#define wz_image_offset(...)                                                   \
  wz_capi_value_compat<int64_t>(::wz_image_offset, 0, __VA_ARGS__)
#define wz_image_is_lua_wz_image(...)                                          \
  wz_capi_value_compat<int>(::wz_image_is_lua_wz_image, 0, __VA_ARGS__)
#define wz_image_parse wz_image_parse_compat
#define wz_image_count_properties(...)                                         \
  wz_capi_value_compat<int>(::wz_image_count_properties, 0, __VA_ARGS__)
#define wz_image_get_property(...)                                             \
  wz_capi_value_compat<wz_property>(                                           \
      ::wz_image_get_property, nullptr, __VA_ARGS__)
#define wz_image_get_from_path(...)                                            \
  wz_capi_value_compat<wz_property>(                                           \
      ::wz_image_get_from_path, nullptr, __VA_ARGS__)
#define wz_get_object_type(...)                                                \
  wz_capi_value_compat<wz_object_type>(                                        \
      ::wz_get_object_type, WZ_OBJECT_FILE, __VA_ARGS__)
#define wz_object_name(...)                                                    \
  wz_capi_value_compat<const char*>(::wz_object_name, nullptr, __VA_ARGS__)
#define wz_object_get_parent(...)                                              \
  wz_capi_value_compat<wz_object>(::wz_object_get_parent, nullptr, __VA_ARGS__)
#define wz_object_full_path(...)                                               \
  wz_capi_value_compat<const char*>(::wz_object_full_path, nullptr, __VA_ARGS__)
#define wz_object_get_wz_file_parent(...)                                      \
  wz_capi_value_compat<wz_file>(                                               \
      ::wz_object_get_wz_file_parent, nullptr, __VA_ARGS__)
#define wz_object_get_top_most_wz_directory(...)                               \
  wz_capi_value_compat<wz_object>(                                             \
      ::wz_object_get_top_most_wz_directory, nullptr, __VA_ARGS__)
#define wz_object_get_top_most_wz_image(...)                                   \
  wz_capi_value_compat<wz_object>(                                             \
      ::wz_object_get_top_most_wz_image, nullptr, __VA_ARGS__)
#define wz_object_at(...)                                                      \
  wz_capi_value_compat<wz_object>(::wz_object_at, nullptr, __VA_ARGS__)
#define wz_property_get_type(...)                                              \
  wz_capi_value_compat<wz_property_type>(                                      \
      ::wz_property_get_type, WZ_PROP_NULL, __VA_ARGS__)
#define wz_property_is_raw_data(...)                                           \
  wz_capi_value_compat<int>(::wz_property_is_raw_data, 0, __VA_ARGS__)
#define wz_property_is_video(...)                                              \
  wz_capi_value_compat<int>(::wz_property_is_video, 0, __VA_ARGS__)
#define wz_property_name(...)                                                  \
  wz_capi_value_compat<const char*>(::wz_property_name, nullptr, __VA_ARGS__)
#define wz_property_count_children(...)                                        \
  wz_capi_value_compat<int>(::wz_property_count_children, 0, __VA_ARGS__)
#define wz_property_get_child(...)                                             \
  wz_capi_value_compat<wz_property>(                                           \
      ::wz_property_get_child, nullptr, __VA_ARGS__)
#define wz_property_get_child_by_name(...)                                     \
  wz_capi_value_compat<wz_property>(                                           \
      ::wz_property_get_child_by_name, nullptr, __VA_ARGS__)
#define wz_property_get_from_path(...)                                         \
  wz_capi_value_compat<wz_property>(                                           \
      ::wz_property_get_from_path, nullptr, __VA_ARGS__)
#define wz_property_get_int(...)                                               \
  wz_capi_value_compat<int32_t>(::wz_property_get_int, 0, __VA_ARGS__)
#define wz_property_get_short(...)                                             \
  wz_capi_value_compat<int16_t>(::wz_property_get_short, 0, __VA_ARGS__)
#define wz_property_get_long(...)                                              \
  wz_capi_value_compat<int64_t>(::wz_property_get_long, 0, __VA_ARGS__)
#define wz_property_get_float(...)                                             \
  wz_capi_value_compat<float>(::wz_property_get_float, 0.0f, __VA_ARGS__)
#define wz_property_get_double(...)                                            \
  wz_capi_value_compat<double>(::wz_property_get_double, 0.0, __VA_ARGS__)
#define wz_property_get_string(...)                                            \
  wz_capi_value_compat<const char*>(                                           \
      ::wz_property_get_string, nullptr, __VA_ARGS__)
#define wz_property_get_bytes wz_property_get_bytes_compat
#define wz_int_get_value(...)                                                  \
  wz_capi_value_compat<int32_t>(::wz_int_get_value, 0, __VA_ARGS__)
#define wz_short_get_value(...)                                                \
  wz_capi_value_compat<int16_t>(::wz_short_get_value, 0, __VA_ARGS__)
#define wz_long_get_value(...)                                                 \
  wz_capi_value_compat<int64_t>(::wz_long_get_value, 0, __VA_ARGS__)
#define wz_float_get_value(...)                                                \
  wz_capi_value_compat<float>(::wz_float_get_value, 0.0f, __VA_ARGS__)
#define wz_double_get_value(...)                                               \
  wz_capi_value_compat<double>(::wz_double_get_value, 0.0, __VA_ARGS__)
#define wz_string_get_value(...)                                               \
  wz_capi_value_compat<const char*>(::wz_string_get_value, nullptr, __VA_ARGS__)
#define wz_string_save_to_file(...)                                            \
  wz_result_to_success_compat(::wz_string_save_to_file(__VA_ARGS__))
#define wz_png_width(...)                                                      \
  wz_capi_value_compat<int>(::wz_png_width, 0, __VA_ARGS__)
#define wz_png_height(...)                                                     \
  wz_capi_value_compat<int>(::wz_png_height, 0, __VA_ARGS__)
#define wz_png_format(...)                                                     \
  wz_capi_value_compat<int>(::wz_png_format, 0, __VA_ARGS__)
#define wz_png_is_list_wz_used(...)                                            \
  wz_capi_value_compat<int>(::wz_png_is_list_wz_used, 0, __VA_ARGS__)
#define wz_png_get_image wz_png_get_image_compat
#define wz_png_get_compressed_bytes wz_png_get_compressed_bytes_compat
#define wz_png_save_to_file(...)                                               \
  wz_result_to_success_compat(::wz_png_save_to_file(__VA_ARGS__))
#define wz_canvas_get_png(...)                                                 \
  wz_capi_value_compat<wz_png_property>(                                       \
      ::wz_canvas_get_png, nullptr, __VA_ARGS__)
#define wz_canvas_contains_inlink(...)                                         \
  wz_capi_value_compat<int>(::wz_canvas_contains_inlink, 0, __VA_ARGS__)
#define wz_canvas_contains_outlink(...)                                        \
  wz_capi_value_compat<int>(::wz_canvas_contains_outlink, 0, __VA_ARGS__)
#define wz_canvas_save_to_file(...)                                            \
  wz_result_to_success_compat(::wz_canvas_save_to_file(__VA_ARGS__))
#define wz_canvas_get_linked_wz_image_property(...)                            \
  wz_capi_value_compat<wz_property>(                                           \
      ::wz_canvas_get_linked_wz_image_property, nullptr, __VA_ARGS__)
#define wz_uol_get_value(...)                                                  \
  wz_capi_value_compat<const char*>(::wz_uol_get_value, nullptr, __VA_ARGS__)
#define wz_uol_get_link_value(...)                                             \
  wz_capi_value_compat<wz_object>(::wz_uol_get_link_value, nullptr, __VA_ARGS__)
#define wz_property_get_linked_wz_image_property(...)                          \
  wz_capi_value_compat<wz_property>(                                           \
      ::wz_property_get_linked_wz_image_property, nullptr, __VA_ARGS__)
#define wz_vector_get_x(...)                                                   \
  wz_capi_value_compat<int32_t>(::wz_vector_get_x, 0, __VA_ARGS__)
#define wz_vector_get_y(...)                                                   \
  wz_capi_value_compat<int32_t>(::wz_vector_get_y, 0, __VA_ARGS__)
#define wz_lua_get_data wz_lua_get_data_compat
#define wz_lua_get_string(...)                                                 \
  wz_capi_value_compat<const char*>(::wz_lua_get_string, nullptr, __VA_ARGS__)
#define wz_lua_save_to_file(...)                                               \
  wz_result_to_success_compat(::wz_lua_save_to_file(__VA_ARGS__))
#define wz_binary_get_data wz_binary_get_data_compat
#define wz_binary_get_wav_playback wz_binary_get_wav_playback_compat
#define wz_binary_get_length(...)                                              \
  wz_capi_value_compat<int>(::wz_binary_get_length, 0, __VA_ARGS__)
#define wz_binary_get_frequency(...)                                           \
  wz_capi_value_compat<int>(::wz_binary_get_frequency, 0, __VA_ARGS__)
#define wz_binary_get_type(...)                                                \
  wz_capi_value_compat<wz_binary_type>(                                        \
      ::wz_binary_get_type, WZ_BINARY_RAW, __VA_ARGS__)
#define wz_binary_is_header_encrypted(...)                                     \
  wz_capi_value_compat<int>(::wz_binary_is_header_encrypted, 0, __VA_ARGS__)
#define wz_binary_save_to_file(...)                                            \
  wz_result_to_success_compat(::wz_binary_save_to_file(__VA_ARGS__))
#define wz_rawdata_get_data wz_rawdata_get_data_compat
#define wz_rawdata_get_type(...)                                               \
  wz_capi_value_compat<int>(::wz_rawdata_get_type, 0, __VA_ARGS__)
#define wz_rawdata_save_to_file(...)                                           \
  wz_result_to_success_compat(::wz_rawdata_save_to_file(__VA_ARGS__))
#define wz_video_get_data wz_video_get_data_compat
#define wz_video_save_to_file(...)                                             \
  wz_result_to_success_compat(::wz_video_save_to_file(__VA_ARGS__))
#define wz_get_error_description wz_get_error_description_compat
#define wz_detect_maple_version wz_detect_maple_version_compat
#define wz_get_iv_for_version wz_get_iv_for_version_compat

#endif  // SRC_CAPI_WZ_API_COMPAT_HPP_

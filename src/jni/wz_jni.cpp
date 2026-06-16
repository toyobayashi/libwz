#include <jni.h>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include "wz/WzDirectory.h"
#include "wz/WzFile.h"
#include "wz/WzFileManager.h"
#include "wz/WzImage.h"
#include "wz/wz_api.h"
#define JNI_FUNC(cls, method) Java_io_github_toyobayashi_libwz_##cls##_##method

class JniUtfString {
 public:
  JniUtfString(JNIEnv* env, jstring str)
      : env_(env),
        str_(str),
        chars_(str ? env->GetStringUTFChars(str, nullptr) : nullptr) {}

  ~JniUtfString() {
    if (str_ && chars_) env_->ReleaseStringUTFChars(str_, chars_);
  }

  JniUtfString(const JniUtfString&) = delete;
  JniUtfString& operator=(const JniUtfString&) = delete;

  const char* c_str() const { return chars_; }
  operator const char*() const { return chars_; }

 private:
  JNIEnv* env_;
  jstring str_;
  const char* chars_;
};

static inline void CheckErrorAndThrow(JNIEnv* env) {
  const wz_last_error_info* info = nullptr;
  wz_get_last_error_info(&info);
  const char* msg = info ? info->message : nullptr;
  jclass exClass = env->FindClass("java/lang/RuntimeException");
  if (exClass) {
    env->ThrowNew(exClass, msg ? msg : "libwz error");
  }
}

#define WZ_API_CALL_RETURN(env, expr, err_ret)                                 \
  do {                                                                         \
    wz_error_code wz_api_err = (expr);                                         \
    if (wz_api_err != WZ_ERROR_NONE) {                                         \
      CheckErrorAndThrow((env));                                               \
      return (err_ret);                                                        \
    }                                                                          \
  } while (false);

#define WZ_API_CALL(env, expr)                                                 \
  do {                                                                         \
    wz_error_code wz_api_err = (expr);                                         \
    if (wz_api_err != WZ_ERROR_NONE) {                                         \
      CheckErrorAndThrow((env));                                               \
      return;                                                                  \
    }                                                                          \
  } while (false);

extern "C" {

// ==================== WzFile ====================

JNIEXPORT jlong JNICALL JNI_FUNC(WzFile, nativeOpen)(
    JNIEnv* env, jclass, jstring filePath, jshort gameVersion, jint version) {
  JniUtfString path(env, filePath);
  wz_file f = nullptr;
  wz_error_code err = wz_open_file(
      path, gameVersion, static_cast<wz_maple_version>(version), &f);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(f);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzFile, nativeOpenWithIv)(JNIEnv* env,
                                                           jclass,
                                                           jstring filePath,
                                                           jbyteArray iv) {
  JniUtfString path(env, filePath);
  jbyte* ivBytes = env->GetByteArrayElements(iv, nullptr);
  wz_file f = nullptr;
  wz_error_code err =
      wz_open_file_with_iv(path, reinterpret_cast<uint8_t*>(ivBytes), &f);
  env->ReleaseByteArrayElements(iv, ivBytes, JNI_ABORT);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(f);
}

JNIEXPORT jint JNICALL JNI_FUNC(WzFile, nativeParseWzFile)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  wz_parse_status result = WZ_PARSE_FAILED_UNKNOWN;
  WZ_API_CALL_RETURN(
      env, wz_parse(reinterpret_cast<wz_file>(ptr), &result), result)

  return result;
}

JNIEXPORT void JNICALL JNI_FUNC(WzFile,
                                nativeDispose)(JNIEnv*, jclass, jlong ptr) {
  wz_close_file(reinterpret_cast<wz_file>(ptr));
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzFile,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  const char* name = nullptr;
  WZ_API_CALL_RETURN(
      env, wz_file_name(reinterpret_cast<wz_file>(ptr), &name), nullptr)
  jstring result = env->NewStringUTF(name);
  return result;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzFile, nativeFilePath)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  const char* path = nullptr;
  WZ_API_CALL_RETURN(
      env, wz_file_path(reinterpret_cast<wz_file>(ptr), &path), nullptr)
  jstring result = env->NewStringUTF(path);
  return result;
}

JNIEXPORT jshort JNICALL JNI_FUNC(WzFile, nativeVersion)(JNIEnv* env,
                                                         jclass,
                                                         jlong ptr) {
  short result = 0;
  WZ_API_CALL_RETURN(
      env, wz_file_version(reinterpret_cast<wz_file>(ptr), &result), result)

  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzFile, nativeMapleVersion)(JNIEnv* env,
                                                            jclass,
                                                            jlong ptr) {
  wz_maple_version result = WZ_UNKNOWN;
  WZ_API_CALL_RETURN(
      env,
      wz_file_maple_version(reinterpret_cast<wz_file>(ptr), &result),
      static_cast<jint>(result))

  return static_cast<jint>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzFile, nativeGetWzDirectory)(JNIEnv* env,
                                                               jclass,
                                                               jlong ptr) {
  wz_dir result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_file_get_wz_directory(reinterpret_cast<wz_file>(ptr), &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzFile, nativeIs64BitWzFile)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(env,
                     wz_file_is_64bit(reinterpret_cast<wz_file>(ptr), &result),
                     result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzFile, nativeIsUnloaded)(JNIEnv* env,
                                                              jclass,
                                                              jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_file_is_unloaded(reinterpret_cast<wz_file>(ptr), &result),
      result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzFile, nativeVersionHash)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  uint32_t result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_file_version_hash(reinterpret_cast<wz_file>(ptr), &result),
      static_cast<jint>(result))

  return static_cast<jint>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzFile, nativeGetObjectFromPath)(
    JNIEnv* env, jclass, jlong ptr, jstring path, jboolean checkFirstDir) {
  JniUtfString p(env, path);
  wz_object obj = nullptr;
  wz_error_code err = wz_file_get_object_from_path(
      reinterpret_cast<wz_file>(ptr), p, checkFirstDir, &obj);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(obj);
}

// ==================== WzDirectory ====================

JNIEXPORT void JNICALL JNI_FUNC(WzDirectory,
                                nativeDispose)(JNIEnv*, jclass, jlong ptr) {
  auto* dir = reinterpret_cast<wz::WzDirectory*>(ptr);
  delete dir;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzDirectory,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  const char* name = nullptr;
  WZ_API_CALL_RETURN(
      env, wz_dir_name(reinterpret_cast<wz_dir>(ptr), &name), nullptr)
  jstring result = env->NewStringUTF(name);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzDirectory, nativeCountImages)(JNIEnv* env,
                                                                jclass,
                                                                jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env, wz_dir_count_images(reinterpret_cast<wz_dir>(ptr), &result), result)

  return result;
}

JNIEXPORT jint JNICALL
JNI_FUNC(WzDirectory, nativeCountDirectories)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_dir_count_directories(reinterpret_cast<wz_dir>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jint JNICALL
JNI_FUNC(WzDirectory, nativeCountImagesTotal)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_dir_count_images_total(reinterpret_cast<wz_dir>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory, nativeGetImage)(JNIEnv* env,
                                                              jclass,
                                                              jlong ptr,
                                                              jint index) {
  wz_image result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_dir_get_image(reinterpret_cast<wz_dir>(ptr), index, &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory, nativeGetImageByName)(
    JNIEnv* env, jclass, jlong ptr, jstring name) {
  JniUtfString n(env, name);
  wz_image img = nullptr;
  wz_error_code err =
      wz_dir_get_image_by_name(reinterpret_cast<wz_dir>(ptr), n, &img);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(img);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory, nativeGetDirectory)(JNIEnv* env,
                                                                  jclass,
                                                                  jlong ptr,
                                                                  jint index) {
  wz_dir result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_dir_get_directory(reinterpret_cast<wz_dir>(ptr), index, &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory, nativeGetDirectoryByName)(
    JNIEnv* env, jclass, jlong ptr, jstring name) {
  JniUtfString n(env, name);
  wz_dir dir = nullptr;
  wz_error_code err =
      wz_dir_get_directory_by_name(reinterpret_cast<wz_dir>(ptr), n, &dir);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(dir);
}

JNIEXPORT jint JNICALL JNI_FUNC(WzDirectory, nativeBlockSize)(JNIEnv* env,
                                                              jclass,
                                                              jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env, wz_dir_block_size(reinterpret_cast<wz_dir>(ptr), &result), result)

  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzDirectory, nativeChecksum)(JNIEnv* env,
                                                             jclass,
                                                             jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env, wz_dir_checksum(reinterpret_cast<wz_dir>(ptr), &result), result)

  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory,
                                 nativeOffset)(JNIEnv* env, jclass, jlong ptr) {
  int64_t result = 0;
  WZ_API_CALL_RETURN(env,
                     wz_dir_offset(reinterpret_cast<wz_dir>(ptr), &result),
                     static_cast<jlong>(result))

  return static_cast<jlong>(result);
}

// ==================== WzImage ====================

JNIEXPORT void JNICALL JNI_FUNC(WzImage,
                                nativeDispose)(JNIEnv*, jclass, jlong ptr) {
  auto* img = reinterpret_cast<wz::WzImage*>(ptr);
  delete img;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzImage,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  const char* name = nullptr;
  WZ_API_CALL_RETURN(
      env, wz_image_name(reinterpret_cast<wz_image>(ptr), &name), nullptr)
  jstring result = env->NewStringUTF(name);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzImage, nativeParsed)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_image_is_parsed(reinterpret_cast<wz_image>(ptr), &result),
      result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzImage, nativeChanged)(JNIEnv* env,
                                                            jclass,
                                                            jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_image_is_changed(reinterpret_cast<wz_image>(ptr), &result),
      result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzImage, nativeBlockSize)(JNIEnv* env,
                                                          jclass,
                                                          jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_image_block_size(reinterpret_cast<wz_image>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzImage, nativeChecksum)(JNIEnv* env,
                                                         jclass,
                                                         jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env, wz_image_checksum(reinterpret_cast<wz_image>(ptr), &result), result)

  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImage,
                                 nativeOffset)(JNIEnv* env, jclass, jlong ptr) {
  int64_t result = 0;
  WZ_API_CALL_RETURN(env,
                     wz_image_offset(reinterpret_cast<wz_image>(ptr), &result),
                     static_cast<jlong>(result))

  return static_cast<jlong>(result);
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzImage, nativeIsLuaWzImage)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_image_is_lua_wz_image(reinterpret_cast<wz_image>(ptr), &result),
      result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzImage, nativeCountProperties)(JNIEnv* env,
                                                                jclass,
                                                                jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_image_count_properties(reinterpret_cast<wz_image>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImage, nativeGetProperty)(JNIEnv* env,
                                                             jclass,
                                                             jlong ptr,
                                                             jint index) {
  wz_property result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_image_get_property(reinterpret_cast<wz_image>(ptr), index, &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImage, nativeGetFromPath)(JNIEnv* env,
                                                             jclass,
                                                             jlong ptr,
                                                             jstring path) {
  JniUtfString p(env, path);
  wz_property prop = nullptr;
  wz_error_code err =
      wz_image_get_from_path(reinterpret_cast<wz_image>(ptr), p, &prop);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(prop);
}

JNIEXPORT void JNICALL JNI_FUNC(WzImage, nativeParseImage)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  int parsed = 0;
  WZ_API_CALL(env, wz_image_parse(reinterpret_cast<wz_image>(ptr), &parsed))
}

// ==================== WzObject ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzObject, nativeObjectType)(JNIEnv* env,
                                                            jclass,
                                                            jlong ptr) {
  wz_object_type result = WZ_OBJECT_FILE;
  WZ_API_CALL_RETURN(
      env,
      wz_get_object_type(reinterpret_cast<wz_object>(ptr), &result),
      static_cast<jint>(result))

  return static_cast<jint>(result);
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzObject,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  const char* name = nullptr;
  WZ_API_CALL_RETURN(
      env, wz_object_name(reinterpret_cast<wz_object>(ptr), &name), nullptr)
  jstring result = env->NewStringUTF(name);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject,
                                 nativeParent)(JNIEnv* env, jclass, jlong ptr) {
  wz_object result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_object_get_parent(reinterpret_cast<wz_object>(ptr), &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzObject, nativeFullPath)(JNIEnv* env,
                                                             jclass,
                                                             jlong ptr) {
  const char* path = nullptr;
  wz_error_code err =
      wz_object_full_path(reinterpret_cast<wz_object>(ptr), &path);
  WZ_API_CALL_RETURN(env, err, nullptr)
  jstring result = env->NewStringUTF(path);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject, nativeWzFileParent)(JNIEnv* env,
                                                               jclass,
                                                               jlong ptr) {
  wz_file result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_object_get_wz_file_parent(reinterpret_cast<wz_object>(ptr), &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject,
                                 nativeGetTopMostWzDirectory)(JNIEnv* env,
                                                              jclass,
                                                              jlong ptr) {
  wz_object result = nullptr;
  WZ_API_CALL_RETURN(env,
                     wz_object_get_top_most_wz_directory(
                         reinterpret_cast<wz_object>(ptr), &result),
                     reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject, nativeGetTopMostWzImage)(JNIEnv* env,
                                                                    jclass,
                                                                    jlong ptr) {
  wz_object result = nullptr;
  WZ_API_CALL_RETURN(env,
                     wz_object_get_top_most_wz_image(
                         reinterpret_cast<wz_object>(ptr), &result),
                     reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject, nativeAt)(JNIEnv* env,
                                                     jclass,
                                                     jlong ptr,
                                                     jstring name) {
  JniUtfString n(env, name);
  wz_object obj = nullptr;
  wz_error_code err = wz_object_at(reinterpret_cast<wz_object>(ptr), n, &obj);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(obj);
}

// ==================== WzImageProperty (was WzProperty) ====================

JNIEXPORT jstring JNICALL JNI_FUNC(WzImageProperty,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  const char* name = nullptr;
  wz_error_code err =
      wz_property_name(reinterpret_cast<wz_property>(ptr), &name);
  WZ_API_CALL_RETURN(env, err, nullptr)
  jstring result = env->NewStringUTF(name);
  return result;
}

JNIEXPORT jint JNICALL
JNI_FUNC(WzImageProperty, nativePropertyType)(JNIEnv* env, jclass, jlong ptr) {
  wz_property_type result = WZ_PROP_NULL;
  WZ_API_CALL_RETURN(
      env,
      wz_property_get_type(reinterpret_cast<wz_property>(ptr), &result),
      static_cast<jint>(result))

  return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL JNI_FUNC(WzImageProperty,
                                nativeGetInt)(JNIEnv* env, jclass, jlong ptr) {
  int32_t result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_property_get_int(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jshort JNICALL JNI_FUNC(WzImageProperty, nativeGetShort)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  int16_t result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_property_get_short(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty, nativeGetLong)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  int64_t result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_property_get_long(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jfloat JNICALL JNI_FUNC(WzImageProperty, nativeGetFloat)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  float result = 0.0f;
  WZ_API_CALL_RETURN(
      env,
      wz_property_get_float(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jdouble JNICALL
JNI_FUNC(WzImageProperty, nativeGetDouble)(JNIEnv* env, jclass, jlong ptr) {
  double result = 0.0;
  WZ_API_CALL_RETURN(
      env,
      wz_property_get_double(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jstring JNICALL
JNI_FUNC(WzImageProperty, nativeGetString)(JNIEnv* env, jclass, jlong ptr) {
  const char* value = nullptr;
  wz_error_code err =
      wz_property_get_string(reinterpret_cast<wz_property>(ptr), &value);
  WZ_API_CALL_RETURN(env, err, nullptr)
  jstring result = env->NewStringUTF(value);
  return result;
}

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzImageProperty, nativeGetBytes)(JNIEnv* env, jclass, jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = 0;
  WZ_API_CALL_RETURN(
      env, wz_property_get_bytes(prop, nullptr, 0, &size), nullptr)
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    WZ_API_CALL_RETURN(
        env, wz_property_get_bytes(prop, buf.data(), size, &size), nullptr)
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jint JNICALL
JNI_FUNC(WzImageProperty, nativeCountChildren)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_property_count_children(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty, nativeGetChild)(JNIEnv* env,
                                                                  jclass,
                                                                  jlong ptr,
                                                                  jint index) {
  wz_property result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_property_get_child(reinterpret_cast<wz_property>(ptr), index, &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty, nativeGetChildByName)(
    JNIEnv* env, jclass, jlong ptr, jstring name) {
  JniUtfString n(env, name);
  wz_property child = nullptr;
  wz_error_code err = wz_property_get_child_by_name(
      reinterpret_cast<wz_property>(ptr), n, &child);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(child);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty, nativeGetFromPath)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  JniUtfString p(env, path);
  wz_property prop = nullptr;
  wz_error_code err =
      wz_property_get_from_path(reinterpret_cast<wz_property>(ptr), p, &prop);
  WZ_API_CALL_RETURN(env, err, 0)
  return reinterpret_cast<jlong>(prop);
}

// ==================== WzPropertyFactory ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzPropertyFactory,
                                nativePropertyType)(JNIEnv* env,
                                                    jclass,
                                                    jlong ptr) {
  wz_property_type result = WZ_PROP_NULL;
  WZ_API_CALL_RETURN(
      env,
      wz_property_get_type(reinterpret_cast<wz_property>(ptr), &result),
      static_cast<jint>(result))

  return static_cast<jint>(result);
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzPropertyFactory,
                                    nativeIsVideoProperty)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_property_is_video(reinterpret_cast<wz_property>(ptr), &result),
      result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

// ==================== Scalar Properties ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzIntProperty, nativeGetValue)(JNIEnv* env,
                                                               jclass,
                                                               jlong ptr) {
  int32_t result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_int_get_value(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT void JNICALL JNI_FUNC(WzIntProperty, nativeSetValue)(JNIEnv* env,
                                                               jclass,
                                                               jlong ptr,
                                                               jint value) {
  WZ_API_CALL(env, wz_int_set_value(reinterpret_cast<wz_property>(ptr), value));
}

JNIEXPORT jshort JNICALL JNI_FUNC(WzShortProperty, nativeGetValue)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  int16_t result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_short_get_value(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzLongProperty, nativeGetValue)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  int64_t result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_long_get_value(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jfloat JNICALL JNI_FUNC(WzFloatProperty, nativeGetValue)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  float result = 0.0f;
  WZ_API_CALL_RETURN(
      env,
      wz_float_get_value(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jdouble JNICALL
JNI_FUNC(WzDoubleProperty, nativeGetValue)(JNIEnv* env, jclass, jlong ptr) {
  double result = 0.0;
  WZ_API_CALL_RETURN(
      env,
      wz_double_get_value(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jstring JNICALL
JNI_FUNC(WzStringProperty, nativeGetValue)(JNIEnv* env, jclass, jlong ptr) {
  const char* value = nullptr;
  wz_error_code err =
      wz_string_get_value(reinterpret_cast<wz_property>(ptr), &value);
  WZ_API_CALL_RETURN(env, err, nullptr)
  jstring result = env->NewStringUTF(value);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzStringProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  JniUtfString p(env, path);
  wz_error_code err =
      wz_string_save_to_file(reinterpret_cast<wz_property>(ptr), p);
  WZ_API_CALL_RETURN(env, err, JNI_FALSE)
  return JNI_TRUE;
}

// ==================== WzPngProperty ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzPngProperty,
                                nativeWidth)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_png_width(reinterpret_cast<wz_png_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzPngProperty,
                                nativeHeight)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_png_height(reinterpret_cast<wz_png_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzPngProperty,
                                nativeFormat)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_png_format(reinterpret_cast<wz_png_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jboolean JNICALL
JNI_FUNC(WzPngProperty, nativeListWzUsed)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_png_is_list_wz_used(reinterpret_cast<wz_png_property>(ptr), &result),
      result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzPngProperty, nativeGetImage)(JNIEnv* env, jclass, jlong ptr) {
  auto* png = reinterpret_cast<wz_png_property>(ptr);
  size_t size = 0;
  WZ_API_CALL_RETURN(env, wz_png_get_image(png, nullptr, 0, &size), nullptr)
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    WZ_API_CALL_RETURN(
        env, wz_png_get_image(png, buf.data(), size, &size), nullptr)
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jbyteArray JNICALL JNI_FUNC(WzPngProperty,
                                      nativeGetCompressedBytes)(JNIEnv* env,
                                                                jclass,
                                                                jlong ptr) {
  auto* png = reinterpret_cast<wz_png_property>(ptr);
  size_t size = 0;
  WZ_API_CALL_RETURN(
      env, wz_png_get_compressed_bytes(png, nullptr, 0, &size), nullptr)
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    WZ_API_CALL_RETURN(
        env, wz_png_get_compressed_bytes(png, buf.data(), size, &size), nullptr)
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzPngProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring filePath) {
  JniUtfString path(env, filePath);
  wz_error_code err =
      wz_png_save_to_file(reinterpret_cast<wz_png_property>(ptr), path);
  WZ_API_CALL_RETURN(env, err, JNI_FALSE)
  return JNI_TRUE;
}

// ==================== WzCanvasProperty ====================

JNIEXPORT jlong JNICALL
JNI_FUNC(WzCanvasProperty, nativePngProperty)(JNIEnv* env, jclass, jlong ptr) {
  wz_png_property result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_canvas_get_png(reinterpret_cast<wz_property>(ptr), &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzCanvasProperty,
                                    nativeContainsInlink)(JNIEnv* env,
                                                          jclass,
                                                          jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_canvas_contains_inlink(reinterpret_cast<wz_property>(ptr), &result),
      result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzCanvasProperty,
                                    nativeContainsOutlink)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_canvas_contains_outlink(reinterpret_cast<wz_property>(ptr), &result),
      result ? JNI_TRUE : JNI_FALSE)

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzCanvasProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  JniUtfString p(env, path);
  wz_error_code err =
      wz_canvas_save_to_file(reinterpret_cast<wz_property>(ptr), p);
  WZ_API_CALL_RETURN(env, err, JNI_FALSE)
  return JNI_TRUE;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzCanvasProperty,
                                 nativeGetLinkedWzImageProperty)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  wz_property result = nullptr;
  WZ_API_CALL_RETURN(env,
                     wz_canvas_get_linked_wz_image_property(
                         reinterpret_cast<wz_property>(ptr), &result),
                     0)
  return reinterpret_cast<jlong>(result);
}

// ==================== WzUOLProperty ====================

JNIEXPORT jstring JNICALL JNI_FUNC(WzUOLProperty, nativeGetValue)(JNIEnv* env,
                                                                  jclass,
                                                                  jlong ptr) {
  const char* value = nullptr;
  wz_error_code err =
      wz_uol_get_value(reinterpret_cast<wz_property>(ptr), &value);
  WZ_API_CALL_RETURN(env, err, nullptr)
  jstring result = env->NewStringUTF(value);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzUOLProperty, nativeGetLinkValue)(JNIEnv* env,
                                                                    jclass,
                                                                    jlong ptr) {
  wz_object result = nullptr;
  WZ_API_CALL_RETURN(
      env,
      wz_uol_get_link_value(reinterpret_cast<wz_property>(ptr), &result),
      reinterpret_cast<jlong>(result))

  return reinterpret_cast<jlong>(result);
}

// ==================== Property Link Resolution ====================

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty,
                                 nativeGetLinkedWzImageProperty)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  wz_property result = nullptr;
  WZ_API_CALL_RETURN(env,
                     wz_property_get_linked_wz_image_property(
                         reinterpret_cast<wz_property>(ptr), &result),
                     0)
  return reinterpret_cast<jlong>(result);
}

// ==================== WzVectorProperty ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzVectorProperty,
                                nativeGetX)(JNIEnv* env, jclass, jlong ptr) {
  int32_t result = 0;
  WZ_API_CALL_RETURN(
      env, wz_vector_get_x(reinterpret_cast<wz_property>(ptr), &result), result)

  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzVectorProperty,
                                nativeGetY)(JNIEnv* env, jclass, jlong ptr) {
  int32_t result = 0;
  WZ_API_CALL_RETURN(
      env, wz_vector_get_y(reinterpret_cast<wz_property>(ptr), &result), result)

  return result;
}

// ==================== WzLuaProperty ====================

JNIEXPORT jbyteArray JNICALL JNI_FUNC(WzLuaProperty, nativeGetData)(JNIEnv* env,
                                                                    jclass,
                                                                    jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = 0;
  WZ_API_CALL_RETURN(env, wz_lua_get_data(prop, nullptr, 0, &size), nullptr)
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    WZ_API_CALL_RETURN(
        env, wz_lua_get_data(prop, buf.data(), size, &size), nullptr)
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzLuaProperty, nativeGetString)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  const char* value = nullptr;
  wz_error_code err =
      wz_lua_get_string(reinterpret_cast<wz_property>(ptr), &value);
  WZ_API_CALL_RETURN(env, err, nullptr)
  jstring result = env->NewStringUTF(value);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzLuaProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring filePath) {
  JniUtfString path(env, filePath);
  wz_error_code err =
      wz_lua_save_to_file(reinterpret_cast<wz_property>(ptr), path);
  WZ_API_CALL_RETURN(env, err, JNI_FALSE)
  return JNI_TRUE;
}

// ==================== WzBinaryProperty ====================

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzBinaryProperty, nativeGetBytes)(JNIEnv* env, jclass, jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = 0;
  WZ_API_CALL_RETURN(env, wz_binary_get_data(prop, nullptr, 0, &size), nullptr)
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    WZ_API_CALL_RETURN(
        env, wz_binary_get_data(prop, buf.data(), size, &size), nullptr)
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jbyteArray JNICALL JNI_FUNC(WzBinaryProperty,
                                      nativeGetWAVPlayback)(JNIEnv* env,
                                                            jclass,
                                                            jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = 0;
  WZ_API_CALL_RETURN(
      env, wz_binary_get_wav_playback(prop, nullptr, 0, &size), nullptr)
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    WZ_API_CALL_RETURN(
        env, wz_binary_get_wav_playback(prop, buf.data(), size, &size), nullptr)
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzBinaryProperty,
                                nativeLength)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_binary_get_length(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzBinaryProperty, nativeFrequency)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_binary_get_frequency(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzBinaryProperty,
                                nativeType)(JNIEnv* env, jclass, jlong ptr) {
  wz_binary_type result = WZ_BINARY_RAW;
  WZ_API_CALL_RETURN(
      env,
      wz_binary_get_type(reinterpret_cast<wz_property>(ptr), &result),
      static_cast<jint>(result))

  return static_cast<jint>(result);
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzBinaryProperty,
                                    nativeHeaderEncrypted)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(env,
                     wz_binary_is_header_encrypted(
                         reinterpret_cast<wz_property>(ptr), &result),
                     0)
  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzBinaryProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  JniUtfString p(env, path);
  wz_error_code err =
      wz_binary_save_to_file(reinterpret_cast<wz_property>(ptr), p);
  WZ_API_CALL_RETURN(env, err, JNI_FALSE)
  return JNI_TRUE;
}

// ==================== WzRawDataProperty ====================

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzRawDataProperty, nativeGetBytes)(JNIEnv* env, jclass, jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = 0;
  WZ_API_CALL_RETURN(env, wz_rawdata_get_data(prop, nullptr, 0, &size), nullptr)
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    WZ_API_CALL_RETURN(
        env, wz_rawdata_get_data(prop, buf.data(), size, &size), nullptr)
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzRawDataProperty,
                                nativeType)(JNIEnv* env, jclass, jlong ptr) {
  int result = 0;
  WZ_API_CALL_RETURN(
      env,
      wz_rawdata_get_type(reinterpret_cast<wz_property>(ptr), &result),
      result)

  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzRawDataProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring filePath) {
  JniUtfString path(env, filePath);
  wz_error_code err =
      wz_rawdata_save_to_file(reinterpret_cast<wz_property>(ptr), path);
  WZ_API_CALL_RETURN(env, err, JNI_FALSE)
  return JNI_TRUE;
}

// ==================== WzVideoProperty ====================

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzVideoProperty, nativeGetBytes)(JNIEnv* env, jclass, jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = 0;
  WZ_API_CALL_RETURN(env, wz_video_get_data(prop, nullptr, 0, &size), nullptr)
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    WZ_API_CALL_RETURN(
        env, wz_video_get_data(prop, buf.data(), size, &size), nullptr)
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzVideoProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring filePath) {
  JniUtfString path(env, filePath);
  wz_error_code err =
      wz_video_save_to_file(reinterpret_cast<wz_property>(ptr), path);
  WZ_API_CALL_RETURN(env, err, JNI_FALSE)
  return JNI_TRUE;
}

// ==================== Utility ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzTool, nativeDetectMapleVersion)(
    JNIEnv* env, jclass, jstring filePath, jshortArray outVersion) {
  JniUtfString path(env, filePath);
  int16_t ver = 0;
  wz_maple_version result = WZ_UNKNOWN;
  wz_error_code err = wz_detect_maple_version(path, &result, &ver);
  WZ_API_CALL_RETURN(env, err, static_cast<jint>(result))
  if (outVersion) {
    env->SetShortArrayRegion(outVersion, 0, 1, &ver);
  }
  return static_cast<jint>(result);
}

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzTool, nativeGetIvForVersion)(JNIEnv* env, jclass, jint version) {
  const uint8_t* iv = nullptr;
  wz_error_code err =
      wz_get_iv_for_version(static_cast<wz_maple_version>(version), &iv);
  WZ_API_CALL_RETURN(env, err, nullptr)
  jbyteArray arr = env->NewByteArray(4);
  env->SetByteArrayRegion(arr, 0, 4, reinterpret_cast<const jbyte*>(iv));
  return arr;
}

// ==================== WzFileManager ====================

JNIEXPORT jlong JNICALL JNI_FUNC(WzFileManager, nativeCreate)(
    JNIEnv* env, jclass, jstring directory, jboolean isStandalone) {
  JniUtfString dir(env, directory);
  auto* manager = new wz::WzFileManager(dir.c_str(), isStandalone);
  return reinterpret_cast<jlong>(manager);
}

JNIEXPORT void JNICALL
JNI_FUNC(WzFileManager, nativeBuildWzFileList)(JNIEnv*, jclass, jlong ptr) {
  reinterpret_cast<wz::WzFileManager*>(ptr)->BuildWzFileList();
}

JNIEXPORT jobjectArray JNICALL JNI_FUNC(WzFileManager,
                                        nativeGetWzFileListKeys)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  auto* manager = reinterpret_cast<wz::WzFileManager*>(ptr);
  const auto& map = manager->GetWzFilesList();
  jclass stringClass = env->FindClass("java/lang/String");
  jobjectArray arr =
      env->NewObjectArray(static_cast<jsize>(map.size()), stringClass, nullptr);
  jsize i = 0;
  for (const auto& pair : map) {
    env->SetObjectArrayElement(arr, i++, env->NewStringUTF(pair.first.c_str()));
  }
  return arr;
}

JNIEXPORT jobjectArray JNICALL JNI_FUNC(WzFileManager,
                                        nativeGetWzFileListValues)(
    JNIEnv* env, jclass, jlong ptr, jstring baseName) {
  auto* manager = reinterpret_cast<wz::WzFileManager*>(ptr);
  JniUtfString key(env, baseName);
  const auto& map = manager->GetWzFilesList();
  auto it = map.find(key.c_str());
  jclass stringClass = env->FindClass("java/lang/String");
  if (it == map.end()) {
    return env->NewObjectArray(0, stringClass, nullptr);
  }
  const auto& list = it->second;
  jobjectArray arr = env->NewObjectArray(
      static_cast<jsize>(list.size()), stringClass, nullptr);
  for (jsize i = 0; i < static_cast<jsize>(list.size()); i++) {
    env->SetObjectArrayElement(arr, i, env->NewStringUTF(list[i].c_str()));
  }
  return arr;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzFileManager, nativeLoadWzFile)(
    JNIEnv* env, jclass, jlong ptr, jstring baseName, jint mapVersion) {
  auto* manager = reinterpret_cast<wz::WzFileManager*>(ptr);
  JniUtfString name(env, baseName);
  wz::WzFile* file = manager->LoadWzFile(
      name.c_str(), static_cast<wz::WzMapleVersion>(mapVersion));
  if (!file) return 0;
  return reinterpret_cast<jlong>(file);
}

JNIEXPORT void JNICALL JNI_FUNC(WzFileManager, nativeUnloadWzFile)(
    JNIEnv*, jclass, jlong ptr, jlong wzFilePtr) {
  auto* manager = reinterpret_cast<wz::WzFileManager*>(ptr);
  auto* file = reinterpret_cast<wz::WzFile*>(wzFilePtr);
  if (file) {
    manager->UnloadWzFile(file);
  }
}

JNIEXPORT void JNICALL JNI_FUNC(WzFileManager,
                                nativeDispose)(JNIEnv*, jclass, jlong ptr) {
  auto* manager = reinterpret_cast<wz::WzFileManager*>(ptr);
  delete manager;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzFileManager, nativeLoadDataWzHotfixFile)(
    JNIEnv* env, jclass, jlong ptr, jstring basePath, jint mapVersion) {
  auto* manager = reinterpret_cast<wz::WzFileManager*>(ptr);
  JniUtfString path(env, basePath);
  wz::WzImage* img = manager->LoadDataWzHotfixFile(
      path.c_str(), static_cast<wz::WzMapleVersion>(mapVersion));
  return reinterpret_cast<jlong>(img);
}

}  // extern "C"

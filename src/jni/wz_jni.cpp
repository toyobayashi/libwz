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
#define JNI_FUNC(cls, method) Java_com_toyobayashi_libwz_##cls##_##method

static const char* GetStringUTF(JNIEnv* env, jstring str) {
  if (!str) return nullptr;
  return env->GetStringUTFChars(str, nullptr);
}

static void ReleaseStringUTF(JNIEnv* env, jstring str, const char* chars) {
  if (str && chars) env->ReleaseStringUTFChars(str, chars);
}

static void CheckErrorAndThrow(JNIEnv* env) {
  wz_error_code err = wz_get_last_error();
  if (err != WZ_ERROR_NONE) {
    const char* msg = wz_get_last_error_message();
    jclass exClass = env->FindClass("java/lang/RuntimeException");
    if (exClass) {
      env->ThrowNew(exClass, msg ? msg : "libwz error");
    }
  }
}

extern "C" {

// ==================== WzFile ====================

JNIEXPORT jlong JNICALL JNI_FUNC(WzFile, nativeOpen)(
    JNIEnv* env, jclass, jstring filePath, jshort gameVersion, jint version) {
  const char* path = GetStringUTF(env, filePath);
  wz_file f =
      wz_open_file(path, gameVersion, static_cast<wz_maple_version>(version));
  ReleaseStringUTF(env, filePath, path);
  CheckErrorAndThrow(env);
  return reinterpret_cast<jlong>(f);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzFile, nativeOpenWithIv)(JNIEnv* env,
                                                           jclass,
                                                           jstring filePath,
                                                           jbyteArray iv) {
  const char* path = GetStringUTF(env, filePath);
  jbyte* ivBytes = env->GetByteArrayElements(iv, nullptr);
  wz_file f = wz_open_file_with_iv(path, reinterpret_cast<uint8_t*>(ivBytes));
  env->ReleaseByteArrayElements(iv, ivBytes, JNI_ABORT);
  ReleaseStringUTF(env, filePath, path);
  CheckErrorAndThrow(env);
  return reinterpret_cast<jlong>(f);
}

JNIEXPORT jint JNICALL JNI_FUNC(WzFile, nativeParseWzFile)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  jint result = static_cast<jint>(wz_parse(reinterpret_cast<wz_file>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT void JNICALL JNI_FUNC(WzFile,
                                nativeDispose)(JNIEnv*, jclass, jlong ptr) {
  wz_close_file(reinterpret_cast<wz_file>(ptr));
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzFile,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  jstring result =
      env->NewStringUTF(wz_file_name(reinterpret_cast<wz_file>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzFile, nativeFilePath)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  jstring result =
      env->NewStringUTF(wz_file_path(reinterpret_cast<wz_file>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jshort JNICALL JNI_FUNC(WzFile, nativeVersion)(JNIEnv* env,
                                                         jclass,
                                                         jlong ptr) {
  jshort result = wz_file_version(reinterpret_cast<wz_file>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzFile, nativeMapleVersion)(JNIEnv* env,
                                                            jclass,
                                                            jlong ptr) {
  jint result =
      static_cast<jint>(wz_file_maple_version(reinterpret_cast<wz_file>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzFile, nativeGetWzDirectory)(JNIEnv* env,
                                                               jclass,
                                                               jlong ptr) {
  jlong result = reinterpret_cast<jlong>(
      wz_file_get_wz_directory(reinterpret_cast<wz_file>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzFile, nativeIs64BitWzFile)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  jboolean result =
      wz_file_is_64bit(reinterpret_cast<wz_file>(ptr)) ? JNI_TRUE : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzFile, nativeIsUnloaded)(JNIEnv* env,
                                                              jclass,
                                                              jlong ptr) {
  jboolean result = wz_file_is_unloaded(reinterpret_cast<wz_file>(ptr))
                        ? JNI_TRUE
                        : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzFile, nativeVersionHash)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  jint result =
      static_cast<jint>(wz_file_version_hash(reinterpret_cast<wz_file>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzFile, nativeGetObjectFromPath)(
    JNIEnv* env, jclass, jlong ptr, jstring path, jboolean checkFirstDir) {
  const char* p = GetStringUTF(env, path);
  auto* obj = wz_file_get_object_from_path(
      reinterpret_cast<wz_file>(ptr), p, checkFirstDir);
  ReleaseStringUTF(env, path, p);
  CheckErrorAndThrow(env);
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
  jstring result =
      env->NewStringUTF(wz_dir_name(reinterpret_cast<wz_dir>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzDirectory, nativeCountImages)(JNIEnv* env,
                                                                jclass,
                                                                jlong ptr) {
  jint result = wz_dir_count_images(reinterpret_cast<wz_dir>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL
JNI_FUNC(WzDirectory, nativeCountDirectories)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_dir_count_directories(reinterpret_cast<wz_dir>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL
JNI_FUNC(WzDirectory, nativeCountImagesTotal)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_dir_count_images_total(reinterpret_cast<wz_dir>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory, nativeGetImage)(JNIEnv* env,
                                                              jclass,
                                                              jlong ptr,
                                                              jint index) {
  jlong result = reinterpret_cast<jlong>(
      wz_dir_get_image(reinterpret_cast<wz_dir>(ptr), index));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory, nativeGetImageByName)(
    JNIEnv* env, jclass, jlong ptr, jstring name) {
  const char* n = GetStringUTF(env, name);
  auto* img = wz_dir_get_image_by_name(reinterpret_cast<wz_dir>(ptr), n);
  ReleaseStringUTF(env, name, n);
  CheckErrorAndThrow(env);
  return reinterpret_cast<jlong>(img);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory, nativeGetDirectory)(JNIEnv* env,
                                                                  jclass,
                                                                  jlong ptr,
                                                                  jint index) {
  jlong result = reinterpret_cast<jlong>(
      wz_dir_get_directory(reinterpret_cast<wz_dir>(ptr), index));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory, nativeGetDirectoryByName)(
    JNIEnv* env, jclass, jlong ptr, jstring name) {
  const char* n = GetStringUTF(env, name);
  auto* dir = wz_dir_get_directory_by_name(reinterpret_cast<wz_dir>(ptr), n);
  ReleaseStringUTF(env, name, n);
  CheckErrorAndThrow(env);
  return reinterpret_cast<jlong>(dir);
}

JNIEXPORT jint JNICALL JNI_FUNC(WzDirectory, nativeBlockSize)(JNIEnv* env,
                                                              jclass,
                                                              jlong ptr) {
  jint result = wz_dir_block_size(reinterpret_cast<wz_dir>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzDirectory, nativeChecksum)(JNIEnv* env,
                                                             jclass,
                                                             jlong ptr) {
  jint result = wz_dir_checksum(reinterpret_cast<wz_dir>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzDirectory,
                                 nativeOffset)(JNIEnv* env, jclass, jlong ptr) {
  jlong result =
      static_cast<jlong>(wz_dir_offset(reinterpret_cast<wz_dir>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

// ==================== WzImage ====================

JNIEXPORT void JNICALL JNI_FUNC(WzImage,
                                nativeDispose)(JNIEnv*, jclass, jlong ptr) {
  auto* img = reinterpret_cast<wz::WzImage*>(ptr);
  delete img;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzImage,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  jstring result =
      env->NewStringUTF(wz_image_name(reinterpret_cast<wz_image>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzImage, nativeParsed)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  jboolean result = wz_image_is_parsed(reinterpret_cast<wz_image>(ptr))
                        ? JNI_TRUE
                        : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzImage, nativeChanged)(JNIEnv* env,
                                                            jclass,
                                                            jlong ptr) {
  jboolean result = wz_image_is_changed(reinterpret_cast<wz_image>(ptr))
                        ? JNI_TRUE
                        : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzImage, nativeBlockSize)(JNIEnv* env,
                                                          jclass,
                                                          jlong ptr) {
  jint result = wz_image_block_size(reinterpret_cast<wz_image>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzImage, nativeChecksum)(JNIEnv* env,
                                                         jclass,
                                                         jlong ptr) {
  jint result = wz_image_checksum(reinterpret_cast<wz_image>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImage,
                                 nativeOffset)(JNIEnv* env, jclass, jlong ptr) {
  jlong result =
      static_cast<jlong>(wz_image_offset(reinterpret_cast<wz_image>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzImage, nativeIsLuaWzImage)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  jboolean result = wz_image_is_lua_wz_image(reinterpret_cast<wz_image>(ptr))
                        ? JNI_TRUE
                        : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzImage, nativeCountProperties)(JNIEnv* env,
                                                                jclass,
                                                                jlong ptr) {
  jint result = wz_image_count_properties(reinterpret_cast<wz_image>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImage, nativeGetProperty)(JNIEnv* env,
                                                             jclass,
                                                             jlong ptr,
                                                             jint index) {
  jlong result = reinterpret_cast<jlong>(
      wz_image_get_property(reinterpret_cast<wz_image>(ptr), index));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImage, nativeGetFromPath)(JNIEnv* env,
                                                             jclass,
                                                             jlong ptr,
                                                             jstring path) {
  const char* p = GetStringUTF(env, path);
  auto* prop = wz_image_get_from_path(reinterpret_cast<wz_image>(ptr), p);
  ReleaseStringUTF(env, path, p);
  CheckErrorAndThrow(env);
  return reinterpret_cast<jlong>(prop);
}

JNIEXPORT void JNICALL JNI_FUNC(WzImage, nativeParseImage)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  wz_image_parse(reinterpret_cast<wz_image>(ptr));
  CheckErrorAndThrow(env);
}

// ==================== WzObject ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzObject, nativeObjectType)(JNIEnv* env,
                                                            jclass,
                                                            jlong ptr) {
  jint result =
      static_cast<jint>(wz_get_object_type(reinterpret_cast<wz_object>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzObject,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  jstring result =
      env->NewStringUTF(wz_object_name(reinterpret_cast<wz_object>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject,
                                 nativeParent)(JNIEnv* env, jclass, jlong ptr) {
  jlong result = reinterpret_cast<jlong>(
      wz_object_get_parent(reinterpret_cast<wz_object>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzObject, nativeFullPath)(JNIEnv* env,
                                                             jclass,
                                                             jlong ptr) {
  jstring result =
      env->NewStringUTF(wz_object_full_path(reinterpret_cast<wz_object>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject, nativeWzFileParent)(JNIEnv* env,
                                                               jclass,
                                                               jlong ptr) {
  jlong result = reinterpret_cast<jlong>(
      wz_object_get_wz_file_parent(reinterpret_cast<wz_object>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject,
                                 nativeGetTopMostWzDirectory)(JNIEnv* env,
                                                              jclass,
                                                              jlong ptr) {
  jlong result = reinterpret_cast<jlong>(
      wz_object_get_top_most_wz_directory(reinterpret_cast<wz_object>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject, nativeGetTopMostWzImage)(JNIEnv* env,
                                                                    jclass,
                                                                    jlong ptr) {
  jlong result = reinterpret_cast<jlong>(
      wz_object_get_top_most_wz_image(reinterpret_cast<wz_object>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzObject, nativeAt)(JNIEnv* env,
                                                     jclass,
                                                     jlong ptr,
                                                     jstring name) {
  const char* n = GetStringUTF(env, name);
  auto* obj = wz_object_at(reinterpret_cast<wz_object>(ptr), n);
  ReleaseStringUTF(env, name, n);
  CheckErrorAndThrow(env);
  return reinterpret_cast<jlong>(obj);
}

// ==================== WzImageProperty (was WzProperty) ====================

JNIEXPORT jstring JNICALL JNI_FUNC(WzImageProperty,
                                   nativeName)(JNIEnv* env, jclass, jlong ptr) {
  jstring result =
      env->NewStringUTF(wz_property_name(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL
JNI_FUNC(WzImageProperty, nativePropertyType)(JNIEnv* env, jclass, jlong ptr) {
  jint result = static_cast<jint>(
      wz_property_get_type(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzImageProperty,
                                nativeGetInt)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_property_get_int(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jshort JNICALL JNI_FUNC(WzImageProperty, nativeGetShort)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  jshort result = wz_property_get_short(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty, nativeGetLong)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  jlong result = wz_property_get_long(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jfloat JNICALL JNI_FUNC(WzImageProperty, nativeGetFloat)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  jfloat result = wz_property_get_float(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jdouble JNICALL
JNI_FUNC(WzImageProperty, nativeGetDouble)(JNIEnv* env, jclass, jlong ptr) {
  jdouble result = wz_property_get_double(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jstring JNICALL
JNI_FUNC(WzImageProperty, nativeGetString)(JNIEnv* env, jclass, jlong ptr) {
  jstring result = env->NewStringUTF(
      wz_property_get_string(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzImageProperty, nativeGetBytes)(JNIEnv* env, jclass, jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = wz_property_get_bytes(prop, nullptr, 0);
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    wz_property_get_bytes(prop, buf.data(), size);
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jint JNICALL
JNI_FUNC(WzImageProperty, nativeCountChildren)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_property_count_children(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty, nativeGetChild)(JNIEnv* env,
                                                                  jclass,
                                                                  jlong ptr,
                                                                  jint index) {
  jlong result = reinterpret_cast<jlong>(
      wz_property_get_child(reinterpret_cast<wz_property>(ptr), index));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty, nativeGetChildByName)(
    JNIEnv* env, jclass, jlong ptr, jstring name) {
  const char* n = GetStringUTF(env, name);
  auto* child =
      wz_property_get_child_by_name(reinterpret_cast<wz_property>(ptr), n);
  ReleaseStringUTF(env, name, n);
  CheckErrorAndThrow(env);
  return reinterpret_cast<jlong>(child);
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzImageProperty, nativeGetFromPath)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  const char* p = GetStringUTF(env, path);
  auto* prop = wz_property_get_from_path(reinterpret_cast<wz_property>(ptr), p);
  ReleaseStringUTF(env, path, p);
  CheckErrorAndThrow(env);
  return reinterpret_cast<jlong>(prop);
}

// ==================== WzPropertyFactory ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzPropertyFactory,
                                nativePropertyType)(JNIEnv* env,
                                                    jclass,
                                                    jlong ptr) {
  jint result = static_cast<jint>(
      wz_property_get_type(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

// ==================== Scalar Properties ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzIntProperty, nativeGetValue)(JNIEnv* env,
                                                               jclass,
                                                               jlong ptr) {
  jint result = wz_int_get_value(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT void JNICALL JNI_FUNC(WzIntProperty, nativeSetValue)(JNIEnv* env,
                                                               jclass,
                                                               jlong ptr,
                                                               jint value) {
  wz_int_set_value(reinterpret_cast<wz_property>(ptr), value);
  CheckErrorAndThrow(env);
}

JNIEXPORT jshort JNICALL JNI_FUNC(WzShortProperty, nativeGetValue)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  jshort result = wz_short_get_value(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jlong JNICALL JNI_FUNC(WzLongProperty, nativeGetValue)(JNIEnv* env,
                                                                 jclass,
                                                                 jlong ptr) {
  jlong result = wz_long_get_value(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jfloat JNICALL JNI_FUNC(WzFloatProperty, nativeGetValue)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  jfloat result = wz_float_get_value(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jdouble JNICALL
JNI_FUNC(WzDoubleProperty, nativeGetValue)(JNIEnv* env, jclass, jlong ptr) {
  jdouble result = wz_double_get_value(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jstring JNICALL
JNI_FUNC(WzStringProperty, nativeGetValue)(JNIEnv* env, jclass, jlong ptr) {
  jstring result = env->NewStringUTF(
      wz_string_get_value(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzStringProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  const char* p = GetStringUTF(env, path);
  int result = wz_string_save_to_file(reinterpret_cast<wz_property>(ptr), p);
  ReleaseStringUTF(env, path, p);
  CheckErrorAndThrow(env);
  return result ? JNI_TRUE : JNI_FALSE;
}

// ==================== WzPngProperty ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzPngProperty,
                                nativeWidth)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_png_width(reinterpret_cast<wz_png_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzPngProperty,
                                nativeHeight)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_png_height(reinterpret_cast<wz_png_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzPngProperty,
                                nativeFormat)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_png_format(reinterpret_cast<wz_png_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL
JNI_FUNC(WzPngProperty, nativeListWzUsed)(JNIEnv* env, jclass, jlong ptr) {
  jboolean result =
      wz_png_is_list_wz_used(reinterpret_cast<wz_png_property>(ptr))
          ? JNI_TRUE
          : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzPngProperty, nativeGetImage)(JNIEnv* env, jclass, jlong ptr) {
  auto* png = reinterpret_cast<wz_png_property>(ptr);
  size_t size = wz_png_get_image(png, nullptr, 0);
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    wz_png_get_image(png, buf.data(), size);
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
  size_t size = wz_png_get_compressed_bytes(png, nullptr, 0);
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    wz_png_get_compressed_bytes(png, buf.data(), size);
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzPngProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring filePath) {
  const char* path = GetStringUTF(env, filePath);
  int result =
      wz_png_save_to_file(reinterpret_cast<wz_png_property>(ptr), path);
  ReleaseStringUTF(env, filePath, path);
  CheckErrorAndThrow(env);
  return result ? JNI_TRUE : JNI_FALSE;
}

// ==================== WzCanvasProperty ====================

JNIEXPORT jlong JNICALL
JNI_FUNC(WzCanvasProperty, nativePngProperty)(JNIEnv* env, jclass, jlong ptr) {
  jlong result = reinterpret_cast<jlong>(
      wz_canvas_get_png(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzCanvasProperty,
                                    nativeContainsInlink)(JNIEnv* env,
                                                          jclass,
                                                          jlong ptr) {
  jboolean result =
      wz_canvas_contains_inlink(reinterpret_cast<wz_property>(ptr)) ? JNI_TRUE
                                                                    : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzCanvasProperty,
                                    nativeContainsOutlink)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  jboolean result =
      wz_canvas_contains_outlink(reinterpret_cast<wz_property>(ptr))
          ? JNI_TRUE
          : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzCanvasProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  const char* p = GetStringUTF(env, path);
  int result = wz_canvas_save_to_file(reinterpret_cast<wz_property>(ptr), p);
  ReleaseStringUTF(env, path, p);
  CheckErrorAndThrow(env);
  return result ? JNI_TRUE : JNI_FALSE;
}

// ==================== WzUOLProperty ====================

JNIEXPORT jstring JNICALL JNI_FUNC(WzUOLProperty, nativeGetValue)(JNIEnv* env,
                                                                  jclass,
                                                                  jlong ptr) {
  jstring result =
      env->NewStringUTF(wz_uol_get_value(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

// ==================== Property Link Resolution ====================

JNIEXPORT jlong JNICALL
JNI_FUNC(WzImageProperty, nativeGetLinkedWzImageProperty)(JNIEnv* env,
                                                          jclass,
                                                          jlong ptr) {
  jlong result = reinterpret_cast<jlong>(
      wz_property_get_linked_wz_image_property(
          reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

// ==================== WzVectorProperty ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzVectorProperty,
                                nativeGetX)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_vector_get_x(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzVectorProperty,
                                nativeGetY)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_vector_get_y(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

// ==================== WzLuaProperty ====================

JNIEXPORT jbyteArray JNICALL JNI_FUNC(WzLuaProperty, nativeGetData)(JNIEnv* env,
                                                                    jclass,
                                                                    jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = wz_lua_get_data(prop, nullptr, 0);
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    wz_lua_get_data(prop, buf.data(), size);
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jstring JNICALL JNI_FUNC(WzLuaProperty, nativeGetString)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  jstring result =
      env->NewStringUTF(wz_lua_get_string(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzLuaProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring filePath) {
  const char* path = GetStringUTF(env, filePath);
  int result = wz_lua_save_to_file(reinterpret_cast<wz_property>(ptr), path);
  ReleaseStringUTF(env, filePath, path);
  CheckErrorAndThrow(env);
  return result ? JNI_TRUE : JNI_FALSE;
}

// ==================== WzBinaryProperty ====================

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzBinaryProperty, nativeGetBytes)(JNIEnv* env, jclass, jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = wz_binary_get_data(prop, nullptr, 0);
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    wz_binary_get_data(prop, buf.data(), size);
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
  size_t size = wz_binary_get_wav_playback(prop, nullptr, 0);
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    wz_binary_get_wav_playback(prop, buf.data(), size);
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzBinaryProperty,
                                nativeLength)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_binary_get_length(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzBinaryProperty, nativeFrequency)(JNIEnv* env,
                                                                   jclass,
                                                                   jlong ptr) {
  jint result = wz_binary_get_frequency(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzBinaryProperty,
                                nativeType)(JNIEnv* env, jclass, jlong ptr) {
  jint result =
      static_cast<jint>(wz_binary_get_type(reinterpret_cast<wz_property>(ptr)));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzBinaryProperty,
                                    nativeHeaderEncrypted)(JNIEnv* env,
                                                           jclass,
                                                           jlong ptr) {
  jboolean result =
      wz_binary_is_header_encrypted(reinterpret_cast<wz_property>(ptr))
          ? JNI_TRUE
          : JNI_FALSE;
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzBinaryProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring path) {
  const char* p = GetStringUTF(env, path);
  int result = wz_binary_save_to_file(reinterpret_cast<wz_property>(ptr), p);
  ReleaseStringUTF(env, path, p);
  CheckErrorAndThrow(env);
  return result ? JNI_TRUE : JNI_FALSE;
}

// ==================== WzRawDataProperty ====================

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzRawDataProperty, nativeGetBytes)(JNIEnv* env, jclass, jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = wz_rawdata_get_data(prop, nullptr, 0);
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    wz_rawdata_get_data(prop, buf.data(), size);
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jint JNICALL JNI_FUNC(WzRawDataProperty,
                                nativeType)(JNIEnv* env, jclass, jlong ptr) {
  jint result = wz_rawdata_get_type(reinterpret_cast<wz_property>(ptr));
  CheckErrorAndThrow(env);
  return result;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzRawDataProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring filePath) {
  const char* path = GetStringUTF(env, filePath);
  int result =
      wz_rawdata_save_to_file(reinterpret_cast<wz_property>(ptr), path);
  ReleaseStringUTF(env, filePath, path);
  CheckErrorAndThrow(env);
  return result ? JNI_TRUE : JNI_FALSE;
}

// ==================== WzVideoProperty ====================

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzVideoProperty, nativeGetBytes)(JNIEnv* env, jclass, jlong ptr) {
  auto* prop = reinterpret_cast<wz_property>(ptr);
  size_t size = wz_video_get_data(prop, nullptr, 0);
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(static_cast<jsize>(size));
  if (size > 0) {
    std::vector<uint8_t> buf(size);
    wz_video_get_data(prop, buf.data(), size);
    env->SetByteArrayRegion(
        arr, 0, static_cast<jsize>(size), reinterpret_cast<jbyte*>(buf.data()));
  }
  return arr;
}

JNIEXPORT jboolean JNICALL JNI_FUNC(WzVideoProperty, nativeSaveToFile)(
    JNIEnv* env, jclass, jlong ptr, jstring filePath) {
  const char* path = GetStringUTF(env, filePath);
  int result = wz_video_save_to_file(reinterpret_cast<wz_property>(ptr), path);
  ReleaseStringUTF(env, filePath, path);
  CheckErrorAndThrow(env);
  return result ? JNI_TRUE : JNI_FALSE;
}

// ==================== Utility ====================

JNIEXPORT jint JNICALL JNI_FUNC(WzTool, nativeDetectMapleVersion)(
    JNIEnv* env, jclass, jstring filePath, jshortArray outVersion) {
  const char* path = GetStringUTF(env, filePath);
  int16_t ver = 0;
  jint result = static_cast<jint>(wz_detect_maple_version(path, &ver));
  ReleaseStringUTF(env, filePath, path);
  CheckErrorAndThrow(env);
  if (outVersion) {
    env->SetShortArrayRegion(outVersion, 0, 1, &ver);
  }
  return result;
}

JNIEXPORT jbyteArray JNICALL
JNI_FUNC(WzTool, nativeGetIvForVersion)(JNIEnv* env, jclass, jint version) {
  const uint8_t* iv =
      wz_get_iv_for_version(static_cast<wz_maple_version>(version));
  CheckErrorAndThrow(env);
  jbyteArray arr = env->NewByteArray(4);
  env->SetByteArrayRegion(arr, 0, 4, reinterpret_cast<const jbyte*>(iv));
  return arr;
}

// ==================== WzFileManager ====================

JNIEXPORT jlong JNICALL JNI_FUNC(WzFileManager, nativeCreate)(
    JNIEnv* env, jclass, jstring directory, jboolean isStandalone) {
  const char* dir = GetStringUTF(env, directory);
  auto* manager = new wz::WzFileManager(dir, isStandalone);
  ReleaseStringUTF(env, directory, dir);
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
  const char* key = GetStringUTF(env, baseName);
  const auto& map = manager->GetWzFilesList();
  auto it = map.find(key);
  ReleaseStringUTF(env, baseName, key);
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
  const char* name = GetStringUTF(env, baseName);
  wz::WzFile* file =
      manager->LoadWzFile(name, static_cast<wz::WzMapleVersion>(mapVersion));
  ReleaseStringUTF(env, baseName, name);
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
  const char* path = GetStringUTF(env, basePath);
  wz::WzImage* img = manager->LoadDataWzHotfixFile(
      path, static_cast<wz::WzMapleVersion>(mapVersion));
  ReleaseStringUTF(env, basePath, path);
  return reinterpret_cast<jlong>(img);
}

}  // extern "C"

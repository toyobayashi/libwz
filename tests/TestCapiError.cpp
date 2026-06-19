#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include "wz/wz_api.h"

namespace {

std::filesystem::path TempPath(const std::string& name) {
  return std::filesystem::temp_directory_path() / name;
}

wz_error_code LastErrorCode() {
  const wz_last_error_info* info = nullptr;
  if (wz_get_last_error_info(&info) != WZ_ERROR_NONE || !info) {
    return WZ_ERROR_NOT_IMPLEMENTED;
  }
  return info->code;
}

}  // namespace

TEST(CapiError, OpenFileRejectsNullPath) {
  wz_file file = nullptr;
  EXPECT_EQ(wz_open_file(nullptr, 0, WZ_GMS, &file), WZ_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(file, nullptr);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_INVALID_ARGUMENT);
}

TEST(CapiError, SuccessfulCallClearsPreviousError) {
  const char* name = nullptr;
  EXPECT_EQ(wz_file_name(nullptr, &name), WZ_ERROR_NULL_HANDLE);
  EXPECT_EQ(name, nullptr);
  EXPECT_NE(LastErrorCode(), WZ_ERROR_NONE);

  wz_file file = nullptr;
  ASSERT_EQ(wz_open_file("dummy.wz", 0, WZ_GMS, &file), WZ_ERROR_NONE);
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_NONE);

  name = nullptr;
  ASSERT_EQ(wz_file_name(file, &name), WZ_ERROR_NONE);
  ASSERT_NE(name, nullptr);
  EXPECT_STREQ(name, "dummy.wz");
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_NONE);

  wz_close_file(file);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_NONE);
}

TEST(CapiEditing, CreateEditReadAndRemoveImageProperty) {
  wz_file file = nullptr;
  ASSERT_EQ(wz_create_file(95, WZ_GMS, &file), WZ_ERROR_NONE);
  ASSERT_NE(file, nullptr);

  wz_dir root = nullptr;
  ASSERT_EQ(wz_file_get_wz_directory(file, &root), WZ_ERROR_NONE);
  ASSERT_NE(root, nullptr);

  wz_image image = nullptr;
  ASSERT_EQ(wz_dir_create_image(root, "test.img", &image), WZ_ERROR_NONE);
  ASSERT_NE(image, nullptr);

  wz_property prop = nullptr;
  ASSERT_EQ(wz_property_create_int("count", 42, &prop), WZ_ERROR_NONE);
  ASSERT_NE(prop, nullptr);
  ASSERT_EQ(wz_image_add_property(image, prop), WZ_ERROR_NONE);

  ASSERT_EQ(wz_int_set_value(prop, 43), WZ_ERROR_NONE);
  int32_t value = 0;
  ASSERT_EQ(wz_int_get_value(prop, &value), WZ_ERROR_NONE);
  EXPECT_EQ(value, 43);

  int count = -1;
  ASSERT_EQ(wz_image_count_properties(image, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 1);

  ASSERT_EQ(wz_image_remove_property(image, prop), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_count_properties(image, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 0);

  wz_close_file(file);
}

TEST(CapiEditing, DuplicateAddAndRenameMapCoreInvalidArgument) {
  wz_file file = nullptr;
  ASSERT_EQ(wz_create_file(95, WZ_GMS, &file), WZ_ERROR_NONE);

  wz_dir root = nullptr;
  ASSERT_EQ(wz_file_get_wz_directory(file, &root), WZ_ERROR_NONE);

  wz_image first = nullptr;
  ASSERT_EQ(wz_dir_create_image(root, "first.img", &first), WZ_ERROR_NONE);
  wz_image duplicate = nullptr;
  EXPECT_EQ(wz_dir_create_image(root, "FIRST.IMG", &duplicate),
            WZ_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(duplicate, nullptr);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_INVALID_ARGUMENT);

  wz_image second = nullptr;
  ASSERT_EQ(wz_dir_create_image(root, "second.img", &second), WZ_ERROR_NONE);
  EXPECT_EQ(
      wz_object_set_name(reinterpret_cast<wz_object>(second), "FIRST.IMG"),
      WZ_ERROR_INVALID_ARGUMENT);
  const char* name = nullptr;
  ASSERT_EQ(wz_image_name(second, &name), WZ_ERROR_NONE);
  EXPECT_STREQ(name, "second.img");

  wz_close_file(file);
}

TEST(CapiEditing, SaveInMemoryFileToDiskAndReopen) {
  const auto path = TempPath("libwz_capi_editing_save.wz");
  std::error_code ec;
  std::filesystem::remove(path, ec);

  wz_file file = nullptr;
  ASSERT_EQ(wz_create_file(95, WZ_GMS, &file), WZ_ERROR_NONE);
  wz_dir root = nullptr;
  ASSERT_EQ(wz_file_get_wz_directory(file, &root), WZ_ERROR_NONE);
  wz_image image = nullptr;
  ASSERT_EQ(wz_dir_create_image(root, "test.img", &image), WZ_ERROR_NONE);
  wz_property prop = nullptr;
  ASSERT_EQ(wz_property_create_int("count", 7, &prop), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(image, prop), WZ_ERROR_NONE);

  ASSERT_EQ(wz_file_save_to_disk(file, path.string().c_str()), WZ_ERROR_NONE);
  wz_close_file(file);

  wz_file reopened = nullptr;
  ASSERT_EQ(wz_open_file(path.string().c_str(), 95, WZ_GMS, &reopened),
            WZ_ERROR_NONE);
  ASSERT_NE(reopened, nullptr);
  wz_parse_status status = WZ_PARSE_FAILED_UNKNOWN;
  ASSERT_EQ(wz_parse(reopened, &status), WZ_ERROR_NONE);
  EXPECT_EQ(status, WZ_PARSE_SUCCESS);
  wz_close_file(reopened);

  std::filesystem::remove(path, ec);
}

TEST(CapiError, UtilityCallClearsPreviousError) {
  wz_file file = nullptr;
  EXPECT_EQ(wz_open_file_with_iv(nullptr, nullptr, &file),
            WZ_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(file, nullptr);
  EXPECT_NE(LastErrorCode(), WZ_ERROR_NONE);

  const uint8_t* iv = nullptr;
  ASSERT_EQ(wz_get_iv_for_version(WZ_GMS, &iv), WZ_ERROR_NONE);
  ASSERT_NE(iv, nullptr);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_NONE);
}

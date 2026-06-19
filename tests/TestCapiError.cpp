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

TEST(CapiEditing, FailedAddKeepsCallerOwnedPropertyUsable) {
  wz_file file = nullptr;
  ASSERT_EQ(wz_create_file(95, WZ_GMS, &file), WZ_ERROR_NONE);
  wz_dir root = nullptr;
  ASSERT_EQ(wz_file_get_wz_directory(file, &root), WZ_ERROR_NONE);
  wz_image image = nullptr;
  ASSERT_EQ(wz_dir_create_image(root, "test.img", &image), WZ_ERROR_NONE);

  wz_property first = nullptr;
  wz_property duplicate = nullptr;
  ASSERT_EQ(wz_property_create_int("count", 1, &first), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_create_int("COUNT", 2, &duplicate), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(image, first), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(image, duplicate), WZ_ERROR_INVALID_ARGUMENT);

  ASSERT_EQ(wz_int_set_value(duplicate, 3), WZ_ERROR_NONE);
  int32_t value = 0;
  ASSERT_EQ(wz_int_get_value(duplicate, &value), WZ_ERROR_NONE);
  EXPECT_EQ(value, 3);
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(duplicate)),
            WZ_ERROR_NONE);

  wz_property parent = nullptr;
  wz_property child = nullptr;
  wz_property duplicate_child = nullptr;
  ASSERT_EQ(wz_property_create_sub("parent", &parent), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_create_int("child", 4, &child), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_create_int("CHILD", 5, &duplicate_child),
            WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(image, parent), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_add_child(parent, child), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_add_child(parent, duplicate_child),
            WZ_ERROR_INVALID_ARGUMENT);

  ASSERT_EQ(wz_int_set_value(duplicate_child, 6), WZ_ERROR_NONE);
  ASSERT_EQ(wz_int_get_value(duplicate_child, &value), WZ_ERROR_NONE);
  EXPECT_EQ(value, 6);
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(duplicate_child)),
            WZ_ERROR_NONE);

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

TEST(CapiEditing, DirectoryCreateAndRemoveObjects) {
  wz_file file = nullptr;
  ASSERT_EQ(wz_create_file(95, WZ_GMS, &file), WZ_ERROR_NONE);
  wz_dir root = nullptr;
  ASSERT_EQ(wz_file_get_wz_directory(file, &root), WZ_ERROR_NONE);

  wz_dir child_dir = nullptr;
  ASSERT_EQ(wz_dir_create_directory(root, "Character", &child_dir),
            WZ_ERROR_NONE);
  wz_image child_image = nullptr;
  ASSERT_EQ(wz_dir_create_image(root, "item.img", &child_image), WZ_ERROR_NONE);

  int count = -1;
  ASSERT_EQ(wz_dir_count_directories(root, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 1);
  ASSERT_EQ(wz_dir_count_images(root, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 1);

  ASSERT_EQ(wz_dir_remove_image(root, child_image), WZ_ERROR_NONE);
  ASSERT_EQ(wz_dir_count_images(root, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 0);

  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(child_dir)),
            WZ_ERROR_NONE);
  ASSERT_EQ(wz_dir_count_directories(root, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 0);

  wz_close_file(file);
}

TEST(CapiEditing, RemoveFromWrongParentReportsNotFound) {
  wz_file file = nullptr;
  ASSERT_EQ(wz_create_file(95, WZ_GMS, &file), WZ_ERROR_NONE);
  wz_dir root = nullptr;
  ASSERT_EQ(wz_file_get_wz_directory(file, &root), WZ_ERROR_NONE);

  wz_dir first_dir = nullptr;
  wz_dir second_dir = nullptr;
  ASSERT_EQ(wz_dir_create_directory(root, "first", &first_dir), WZ_ERROR_NONE);
  ASSERT_EQ(wz_dir_create_directory(root, "second", &second_dir),
            WZ_ERROR_NONE);

  wz_dir nested_dir = nullptr;
  ASSERT_EQ(wz_dir_create_directory(first_dir, "nested", &nested_dir),
            WZ_ERROR_NONE);
  EXPECT_EQ(wz_dir_remove_directory(second_dir, nested_dir),
            WZ_ERROR_NOT_FOUND);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_NOT_FOUND);

  wz_image nested_image = nullptr;
  ASSERT_EQ(wz_dir_create_image(first_dir, "test.img", &nested_image),
            WZ_ERROR_NONE);
  EXPECT_EQ(wz_dir_remove_image(second_dir, nested_image), WZ_ERROR_NOT_FOUND);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_NOT_FOUND);

  wz_image other_image = nullptr;
  ASSERT_EQ(wz_dir_create_image(second_dir, "other.img", &other_image),
            WZ_ERROR_NONE);
  wz_property prop = nullptr;
  ASSERT_EQ(wz_property_create_int("count", 1, &prop), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(nested_image, prop), WZ_ERROR_NONE);
  EXPECT_EQ(wz_image_remove_property(other_image, prop), WZ_ERROR_NOT_FOUND);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_NOT_FOUND);

  wz_property first_parent = nullptr;
  wz_property second_parent = nullptr;
  wz_property child = nullptr;
  ASSERT_EQ(wz_property_create_sub("firstParent", &first_parent),
            WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_create_sub("secondParent", &second_parent),
            WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_create_int("child", 2, &child), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(nested_image, first_parent), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(nested_image, second_parent), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_add_child(first_parent, child), WZ_ERROR_NONE);
  EXPECT_EQ(wz_property_remove_child(second_parent, child), WZ_ERROR_NOT_FOUND);
  EXPECT_EQ(LastErrorCode(), WZ_ERROR_NOT_FOUND);

  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(child)),
            WZ_ERROR_NONE);
  int count = -1;
  ASSERT_EQ(wz_property_count_children(first_parent, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 0);

  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(prop)), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_count_properties(nested_image, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 2);

  wz_close_file(file);
}

TEST(CapiEditing, ImageAndPropertyChildContainersCanBeCleared) {
  wz_file file = nullptr;
  ASSERT_EQ(wz_create_file(95, WZ_GMS, &file), WZ_ERROR_NONE);
  wz_dir root = nullptr;
  ASSERT_EQ(wz_file_get_wz_directory(file, &root), WZ_ERROR_NONE);
  wz_image image = nullptr;
  ASSERT_EQ(wz_dir_create_image(root, "test.img", &image), WZ_ERROR_NONE);

  wz_property first = nullptr;
  wz_property second = nullptr;
  ASSERT_EQ(wz_property_create_int("first", 1, &first), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_create_int("second", 2, &second), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(image, first), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(image, second), WZ_ERROR_NONE);

  int count = -1;
  ASSERT_EQ(wz_image_count_properties(image, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 2);
  ASSERT_EQ(wz_image_clear_properties(image), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_count_properties(image, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 0);

  wz_property parent = nullptr;
  wz_property child = nullptr;
  ASSERT_EQ(wz_property_create_sub("info", &parent), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_create_string("name", "alpha", &child), WZ_ERROR_NONE);
  ASSERT_EQ(wz_image_add_property(image, parent), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_add_child(parent, child), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_count_children(parent, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 1);
  ASSERT_EQ(wz_property_clear_children(parent), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_count_children(parent, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 0);

  wz_property child_to_remove = nullptr;
  ASSERT_EQ(wz_property_create_int("count", 3, &child_to_remove),
            WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_add_child(parent, child_to_remove), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_remove_child(parent, child_to_remove), WZ_ERROR_NONE);
  ASSERT_EQ(wz_property_count_children(parent, &count), WZ_ERROR_NONE);
  EXPECT_EQ(count, 0);

  wz_close_file(file);
}

TEST(CapiEditing, PropertyConstructorsAndSettersRoundTripValues) {
  wz_property null_prop = nullptr;
  ASSERT_EQ(wz_property_create_null("nil", &null_prop), WZ_ERROR_NONE);
  wz_property_type type = WZ_PROP_INT;
  ASSERT_EQ(wz_property_get_type(null_prop, &type), WZ_ERROR_NONE);
  EXPECT_EQ(type, WZ_PROP_NULL);
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(null_prop)),
            WZ_ERROR_NONE);

  wz_property short_prop = nullptr;
  ASSERT_EQ(wz_property_create_short("short", 1, &short_prop), WZ_ERROR_NONE);
  ASSERT_EQ(wz_short_set_value(short_prop, 2), WZ_ERROR_NONE);
  int16_t short_value = 0;
  ASSERT_EQ(wz_short_get_value(short_prop, &short_value), WZ_ERROR_NONE);
  EXPECT_EQ(short_value, 2);
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(short_prop)),
            WZ_ERROR_NONE);

  wz_property long_prop = nullptr;
  ASSERT_EQ(wz_property_create_long("long", 3, &long_prop), WZ_ERROR_NONE);
  ASSERT_EQ(wz_long_set_value(long_prop, 4), WZ_ERROR_NONE);
  int64_t long_value = 0;
  ASSERT_EQ(wz_long_get_value(long_prop, &long_value), WZ_ERROR_NONE);
  EXPECT_EQ(long_value, 4);
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(long_prop)),
            WZ_ERROR_NONE);

  wz_property float_prop = nullptr;
  ASSERT_EQ(wz_property_create_float("float", 1.25f, &float_prop),
            WZ_ERROR_NONE);
  ASSERT_EQ(wz_float_set_value(float_prop, 2.5f), WZ_ERROR_NONE);
  float float_value = 0;
  ASSERT_EQ(wz_float_get_value(float_prop, &float_value), WZ_ERROR_NONE);
  EXPECT_FLOAT_EQ(float_value, 2.5f);
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(float_prop)),
            WZ_ERROR_NONE);

  wz_property double_prop = nullptr;
  ASSERT_EQ(wz_property_create_double("double", 1.5, &double_prop),
            WZ_ERROR_NONE);
  ASSERT_EQ(wz_double_set_value(double_prop, 3.25), WZ_ERROR_NONE);
  double double_value = 0;
  ASSERT_EQ(wz_double_get_value(double_prop, &double_value), WZ_ERROR_NONE);
  EXPECT_DOUBLE_EQ(double_value, 3.25);
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(double_prop)),
            WZ_ERROR_NONE);

  wz_property string_prop = nullptr;
  ASSERT_EQ(wz_property_create_string("string", "a", &string_prop),
            WZ_ERROR_NONE);
  ASSERT_EQ(wz_string_set_value(string_prop, "b"), WZ_ERROR_NONE);
  const char* string_value = nullptr;
  ASSERT_EQ(wz_string_get_value(string_prop, &string_value), WZ_ERROR_NONE);
  EXPECT_STREQ(string_value, "b");
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(string_prop)),
            WZ_ERROR_NONE);

  wz_property vector_prop = nullptr;
  ASSERT_EQ(wz_property_create_vector("vector", 5, 6, &vector_prop),
            WZ_ERROR_NONE);
  int32_t vector_value = 0;
  ASSERT_EQ(wz_vector_get_x(vector_prop, &vector_value), WZ_ERROR_NONE);
  EXPECT_EQ(vector_value, 5);
  ASSERT_EQ(wz_vector_get_y(vector_prop, &vector_value), WZ_ERROR_NONE);
  EXPECT_EQ(vector_value, 6);
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(vector_prop)),
            WZ_ERROR_NONE);

  wz_property uol_prop = nullptr;
  ASSERT_EQ(wz_property_create_uol("link", "../a", &uol_prop), WZ_ERROR_NONE);
  ASSERT_EQ(wz_uol_set_value(uol_prop, "../b"), WZ_ERROR_NONE);
  const char* uol_value = nullptr;
  ASSERT_EQ(wz_uol_get_value(uol_prop, &uol_value), WZ_ERROR_NONE);
  EXPECT_STREQ(uol_value, "../b");
  ASSERT_EQ(wz_object_remove(reinterpret_cast<wz_object>(uol_prop)),
            WZ_ERROR_NONE);
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

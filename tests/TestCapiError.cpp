#include <gtest/gtest.h>
#include "wz/wz_api.h"

namespace {

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

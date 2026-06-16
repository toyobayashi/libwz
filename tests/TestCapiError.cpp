#include <gtest/gtest.h>
#include "wz/wz_api.h"

TEST(CapiError, OpenFileRejectsNullPath) {
  wz_clear_error();

  EXPECT_EQ(wz_open_file(nullptr, 0, WZ_GMS), nullptr);
  EXPECT_EQ(wz_get_last_error(), WZ_ERROR_INVALID_ARGUMENT);
}

TEST(CapiError, SuccessfulCallClearsPreviousError) {
  wz_clear_error();

  EXPECT_EQ(wz_file_name(nullptr), nullptr);
  EXPECT_NE(wz_get_last_error(), WZ_ERROR_NONE);

  wz_file file = wz_open_file("dummy.wz", 0, WZ_GMS);
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(wz_get_last_error(), WZ_ERROR_NONE);

  const char* name = wz_file_name(file);
  ASSERT_NE(name, nullptr);
  EXPECT_STREQ(name, "dummy.wz");
  EXPECT_EQ(wz_get_last_error(), WZ_ERROR_NONE);

  wz_close_file(file);
  EXPECT_EQ(wz_get_last_error(), WZ_ERROR_NONE);
}

TEST(CapiError, UtilityCallClearsPreviousError) {
  wz_clear_error();

  EXPECT_EQ(wz_open_file_with_iv(nullptr, nullptr), nullptr);
  EXPECT_NE(wz_get_last_error(), WZ_ERROR_NONE);

  const uint8_t* iv = wz_get_iv_for_version(WZ_GMS);
  ASSERT_NE(iv, nullptr);
  EXPECT_EQ(wz_get_last_error(), WZ_ERROR_NONE);
}

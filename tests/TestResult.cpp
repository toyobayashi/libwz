#include <gtest/gtest.h>

#include <expected>
#include <string>

#include "wz/Result.h"

TEST(Result, UsesStdExpectedValueApi) {
  wz::Result<std::string> result("ok");

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result);
  EXPECT_EQ(result.value(), "ok");
  EXPECT_EQ(*result, "ok");
}

TEST(Result, UsesStdExpectedErrorApi) {
  wz::Result<std::string> result =
      std::unexpected(wz::Error::ParseError("bad wz"));

  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error().code(), wz::ErrorCode::ParseError);
  EXPECT_EQ(result.error().message(), "bad wz");
}

TEST(Result, SupportsStdExpectedVoidApi) {
  wz::Result<void> ok;
  EXPECT_TRUE(ok.has_value());
  ok.value();

  wz::Result<void> err = std::unexpected(wz::Error::IoError("missing file"));
  ASSERT_FALSE(err.has_value());
  EXPECT_EQ(err.error().code(), wz::ErrorCode::IoError);
  EXPECT_EQ(err.error().message(), "missing file");
}

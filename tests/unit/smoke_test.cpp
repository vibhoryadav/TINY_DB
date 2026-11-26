#include <gtest/gtest.h>

#include "tinydb/build_info.hpp"

namespace {

TEST(SmokeTest, ProjectConstantsMatchSpecification) {
  EXPECT_EQ(tinydb::BuildInfo::kProjectName, "TinyDB");
  EXPECT_EQ(tinydb::BuildInfo::kPageSizeBytes, 4096U);
  EXPECT_FALSE(tinydb::GetVersionBanner().empty());
}

}  // namespace

// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_timezone.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace privacy_cef {

TEST(PrivacyTimeZoneTest, AcceptsKnownIds) {
  EXPECT_TRUE(IsValidTimeZoneId("America/Los_Angeles"));
  EXPECT_TRUE(IsValidTimeZoneId("Europe/Berlin"));
  EXPECT_TRUE(IsValidTimeZoneId("Asia/Shanghai"));
  EXPECT_TRUE(IsValidTimeZoneId("UTC"));
  EXPECT_TRUE(IsValidTimeZoneId("GMT"));
}

TEST(PrivacyTimeZoneTest, RejectsUnknownIds) {
  // ICU silently resolves unknown IDs to a GMT-like zone, so these must be
  // rejected explicitly instead of falling back to UTC.
  EXPECT_FALSE(IsValidTimeZoneId(""));
  EXPECT_FALSE(IsValidTimeZoneId("Not/AZone"));
  EXPECT_FALSE(IsValidTimeZoneId("Mars/Olympus"));
}

TEST(PrivacyTimeZoneTest, SetProcessTimeZoneAppliesOnlyValidIds) {
  const std::string original = GetProcessTimeZoneId();
  ASSERT_FALSE(original.empty());

  EXPECT_FALSE(SetProcessTimeZone("Not/AZone"));
  EXPECT_EQ(original, GetProcessTimeZoneId());

  ASSERT_TRUE(SetProcessTimeZone("America/Los_Angeles"));
  EXPECT_EQ("America/Los_Angeles", GetProcessTimeZoneId());

  ASSERT_TRUE(SetProcessTimeZone("Europe/Berlin"));
  EXPECT_EQ("Europe/Berlin", GetProcessTimeZoneId());

  // Restore so that test ordering cannot affect other suites.
  ASSERT_TRUE(SetProcessTimeZone(original));
  EXPECT_EQ(original, GetProcessTimeZoneId());
}

}  // namespace privacy_cef

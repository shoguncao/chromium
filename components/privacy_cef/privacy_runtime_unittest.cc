// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_runtime.h"

#include <array>

#include "testing/gtest/include/gtest/gtest.h"

namespace privacy_cef {
namespace {

PrivacyProfile FarblingProfile() {
  PrivacyProfile profile;
  profile.profile_id = "stable-profile";
  profile.canvas_mode = CanvasMode::kFarble;
  profile.canvas_algorithm = "brave-derived";
  profile.canvas_algorithm_version = 1;
  for (size_t index = 0; index < profile.master_seed.size(); ++index) {
    profile.master_seed[index] = static_cast<uint8_t>(index);
  }
  return profile;
}

class PrivacyRuntimeTest : public testing::Test {
 protected:
  void TearDown() override { PrivacyRuntime::GetInstance().ResetForTesting(); }
};

TEST_F(PrivacyRuntimeTest, DoesNothingUntilBrowserDeliversProfile) {
  std::array<uint8_t, 8> pixels = {10, 20, 30, 255, 40, 50, 60, 255};
  const auto original = pixels;
  EXPECT_EQ(
      PrivacyRuntime::GetInstance().ProtectCanvasPixels("example.com", pixels),
      CanvasProtectionResult::kRuntimeNotConfigured);
  EXPECT_EQ(pixels, original);
}

TEST_F(PrivacyRuntimeTest, IsStableAcrossEquivalentReads) {
  PrivacyRuntime::GetInstance().SetProfile(FarblingProfile());
  std::array<uint8_t, 16> first = {1, 2, 3, 255, 4,  5,  6,  255,
                                   7, 8, 9, 255, 10, 11, 12, 255};
  auto second = first;
  EXPECT_EQ(
      PrivacyRuntime::GetInstance().ProtectCanvasPixels("example.com", first),
      CanvasProtectionResult::kFarbled);
  EXPECT_EQ(
      PrivacyRuntime::GetInstance().ProtectCanvasPixels("example.com", second),
      CanvasProtectionResult::kFarbled);
  EXPECT_EQ(first, second);
}

TEST_F(PrivacyRuntimeTest, SeparatesTopLevelSites) {
  PrivacyRuntime::GetInstance().SetProfile(FarblingProfile());
  std::array<uint8_t, 64> first{};
  for (size_t index = 0; index < first.size(); ++index) {
    first[index] = static_cast<uint8_t>(index);
  }
  auto second = first;
  PrivacyRuntime::GetInstance().ProtectCanvasPixels("example.com", first);
  PrivacyRuntime::GetInstance().ProtectCanvasPixels("github.com", second);
  EXPECT_NE(first, second);
}

}  // namespace
}  // namespace privacy_cef

// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_profile.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace privacy_cef {
namespace {

constexpr char kValidProfile[] = R"JSON({
  "schemaVersion": 1,
  "profileId": "mac-standard-01",
  "displayName": "Mac Standard 1",
  "masterSeed": "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=",
  "preset": "balanced",
  "locale": {"language":"en-US","languages":["en-US","en"],"timezone":"America/Los_Angeles"},
  "hardware": {"cpuCores":8,"memoryGB":8,"touchClass":"none"},
  "display": {"width":1728,"height":1117,"deviceScaleFactor":2,"colorDepth":24,"colorGamut":"p3"},
  "audit": {"mode":"summary","retentionDays":7,"maxFileSizeMB":20},
  "protections": {
    "canvas":{"mode":"farble","algorithm":"brave-derived","algorithmVersion":1},
    "webgl":"standardize-and-farble","audio":"farble","fonts":"standardize",
    "geometry":"environment-only","storage":"bucket","speech":"standardize",
    "webrtc":"no-local-ip","webgpu":"disabled"
  }
})JSON";

}  // namespace

TEST(PrivacyProfileTest, ParsesValidProfile) {
  std::string error;
  std::optional<PrivacyProfile> profile =
      PrivacyProfile::Parse(kValidProfile, &error);
  ASSERT_TRUE(profile) << error;
  EXPECT_EQ(profile->profile_id, "mac-standard-01");
  EXPECT_EQ(profile->master_seed.size(), 32u);
  EXPECT_EQ(profile->canvas_mode, CanvasMode::kFarble);
  EXPECT_EQ(profile->canvas_algorithm_version, 1);
  EXPECT_EQ(profile->audit_mode, AuditMode::kSummary);
}

TEST(PrivacyProfileTest, RejectsSeedWithWrongSize) {
  std::string json = kValidProfile;
  const size_t seed_start =
      json.find("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
  ASSERT_NE(seed_start, std::string::npos);
  json.replace(seed_start, 44, "YQ==");
  std::string error;
  EXPECT_FALSE(PrivacyProfile::Parse(json, &error));
  EXPECT_EQ(error, "masterSeed must contain exactly 32 bytes");
}

TEST(PrivacyProfileTest, RejectsUnknownCanvasAlgorithmVersion) {
  std::string json = kValidProfile;
  const size_t version = json.find("\"algorithmVersion\":1");
  ASSERT_NE(version, std::string::npos);
  json.replace(version, 20, "\"algorithmVersion\":2");
  std::string error;
  EXPECT_FALSE(PrivacyProfile::Parse(json, &error));
  EXPECT_EQ(error, "unsupported Canvas farbling algorithm");
}

}  // namespace privacy_cef

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
  EXPECT_EQ(profile->webgl_mode, WebGlMode::kStandardizeAndFarble);
  EXPECT_EQ(profile->audio_mode, AudioMode::kFarble);
  EXPECT_EQ(profile->fonts_mode, FontsMode::kStandardize);
  EXPECT_EQ(profile->geometry_mode, GeometryMode::kEnvironmentOnly);
  EXPECT_EQ(profile->storage_mode, StorageMode::kBucket);
  EXPECT_EQ(profile->speech_mode, SpeechMode::kStandardize);
  EXPECT_EQ(profile->webrtc_mode, WebRtcMode::kNoLocalIp);
  EXPECT_EQ(profile->webgpu_mode, WebGpuMode::kDisabled);
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

TEST(PrivacyProfileTest, RendererPayloadRoundTripsThroughValidation) {
  std::string error;
  std::optional<PrivacyProfile> profile =
      PrivacyProfile::Parse(kValidProfile, &error);
  ASSERT_TRUE(profile) << error;

  std::optional<PrivacyProfile> round_trip =
      PrivacyProfile::Parse(profile->SerializeForRenderer(), &error);
  ASSERT_TRUE(round_trip) << error;
  EXPECT_EQ(round_trip->profile_id, profile->profile_id);
  EXPECT_EQ(round_trip->master_seed, profile->master_seed);
  EXPECT_EQ(round_trip->canvas_mode, profile->canvas_mode);
  EXPECT_EQ(round_trip->canvas_algorithm, profile->canvas_algorithm);
  EXPECT_EQ(round_trip->languages, profile->languages);
  EXPECT_EQ(round_trip->webgl_mode, profile->webgl_mode);
  EXPECT_EQ(round_trip->audio_mode, profile->audio_mode);
  EXPECT_EQ(round_trip->fonts_mode, profile->fonts_mode);
  EXPECT_EQ(round_trip->geometry_mode, profile->geometry_mode);
  EXPECT_EQ(round_trip->storage_mode, profile->storage_mode);
  EXPECT_EQ(round_trip->speech_mode, profile->speech_mode);
  EXPECT_EQ(round_trip->webrtc_mode, profile->webrtc_mode);
  EXPECT_EQ(round_trip->webgpu_mode, profile->webgpu_mode);
}

TEST(PrivacyProfileTest, RejectsUnknownWebGlMode) {
  std::string json = kValidProfile;
  constexpr std::string_view kWebGlMode =
      "\"webgl\":\"standardize-and-farble\"";
  const size_t mode = json.find(kWebGlMode);
  ASSERT_NE(mode, std::string::npos);
  json.replace(mode, kWebGlMode.size(), "\"webgl\":\"invented\"");
  std::string error;
  EXPECT_FALSE(PrivacyProfile::Parse(json, &error));
  EXPECT_EQ(error, "unsupported WebGL mode");
}

}  // namespace privacy_cef

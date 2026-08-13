// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/audio_farbling.h"

#include <array>

#include "testing/gtest/include/gtest/gtest.h"

namespace privacy_cef {
namespace {

TEST(AudioFarblingHelperTest, BalancedMatchesBraveMultiplication) {
  std::array<float, 3> samples = {0.25f, -0.5f, 1.0f};
  AudioFarblingHelper(0.9995, 123, false).FarbleAudioChannel(samples);
  EXPECT_FLOAT_EQ(samples[0], 0.25f * 0.9995f);
  EXPECT_FLOAT_EQ(samples[1], -0.5f * 0.9995f);
  EXPECT_FLOAT_EQ(samples[2], 0.9995f);
}

TEST(AudioFarblingHelperTest, MaximumIsDeterministicAndInputIndependent) {
  std::array<float, 4> first = {-1.0f, -0.5f, 0.5f, 1.0f};
  std::array<float, 4> second = {7.0f, 8.0f, 9.0f, 10.0f};
  const AudioFarblingHelper helper(1.0, 0x123456789abcdef0ULL, true);
  helper.FarbleAudioChannel(first);
  helper.FarbleAudioChannel(second);
  EXPECT_EQ(first, second);
  for (float sample : first) {
    EXPECT_GE(sample, 0.0f);
    EXPECT_LE(sample, 0.1f);
  }
}

TEST(AudioFarblingHelperTest, TimeDomainOutputIsStable) {
  std::array<float, 8> input = {-1.0f, -0.5f, 0.0f, 0.5f,
                                1.0f,  0.5f,  0.0f, -0.5f};
  std::array<float, 4> first{};
  std::array<float, 4> second{};
  const AudioFarblingHelper helper(0.9999, 42, false);
  helper.FarbleFloatTimeDomainData(input, first, first.size(), 6, 4);
  helper.FarbleFloatTimeDomainData(input, second, second.size(), 6, 4);
  EXPECT_EQ(first, second);
}

}  // namespace
}  // namespace privacy_cef

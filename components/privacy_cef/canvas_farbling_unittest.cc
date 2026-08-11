// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/privacy_cef/canvas_farbling.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace privacy_cef {

TEST(CanvasFarblingTest, IsStableAndDoesNotModifyAlpha) {
  PrivacySeed site_seed;
  site_seed.fill(0x2a);
  std::vector<uint8_t> original(64 * 64 * 4);
  for (size_t index = 0; index < original.size(); ++index) {
    original[index] = static_cast<uint8_t>(index);
  }
  std::vector<uint8_t> first = original;
  std::vector<uint8_t> second = original;
  FarbleCanvasPixels(site_seed, first);
  FarbleCanvasPixels(site_seed, second);
  EXPECT_EQ(first, second);
  EXPECT_NE(first, original);
  for (size_t index = 3; index < original.size(); index += 4) {
    EXPECT_EQ(first[index], original[index]);
  }
}

TEST(CanvasFarblingTest, ChangesEachRgbChannelByAtMostOneBit) {
  PrivacySeed site_seed;
  site_seed.fill(0x5c);
  std::vector<uint8_t> original(32 * 32 * 4, 0xaa);
  std::vector<uint8_t> farbled = original;
  FarbleCanvasPixels(site_seed, farbled);
  for (size_t index = 0; index < original.size(); ++index) {
    if (index % 4 == 3) {
      EXPECT_EQ(farbled[index], original[index]);
    } else {
      EXPECT_TRUE(farbled[index] == original[index] ||
                  farbled[index] == (original[index] ^ 0x1));
    }
  }
}

TEST(CanvasFarblingTest, SeparatesSiteSeeds) {
  PrivacySeed first_seed;
  PrivacySeed second_seed;
  first_seed.fill(0x01);
  second_seed.fill(0x02);
  std::vector<uint8_t> first(64 * 64 * 4, 0x44);
  std::vector<uint8_t> second = first;
  FarbleCanvasPixels(first_seed, first);
  FarbleCanvasPixels(second_seed, second);
  EXPECT_NE(first, second);
}

TEST(CanvasFarblingTest, IgnoresMalformedPixelSpan) {
  PrivacySeed site_seed;
  site_seed.fill(0x2a);
  std::vector<uint8_t> malformed = {1, 2, 3};
  const std::vector<uint8_t> original = malformed;
  FarbleCanvasPixels(site_seed, malformed);
  EXPECT_EQ(malformed, original);
}

}  // namespace privacy_cef

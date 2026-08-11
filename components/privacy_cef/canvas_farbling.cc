// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/privacy_cef/canvas_farbling.h"

#include <cstdint>

#include "base/containers/span.h"
#include "base/numerics/byte_conversions.h"
#include "crypto/hmac.h"

namespace privacy_cef {
namespace {

uint64_t NextLfsrValue(uint64_t value) {
  constexpr uint64_t kZero = 0;
  return (value >> 1) |
         (((value << 62) ^ (value << 61)) & (~(~kZero << 63) << 62));
}

}  // namespace

void FarbleCanvasPixels(const PrivacySeed& site_seed,
                        base::span<uint8_t> rgba_pixels) {
  if (rgba_pixels.empty() || rgba_pixels.size() % 4 != 0) {
    return;
  }

  const size_t pixel_count = rgba_pixels.size() / 4;
  const PrivacySeed canvas_key =
      crypto::hmac::SignSha256(site_seed, rgba_pixels);
  uint64_t value =
      base::U64FromNativeEndian(base::span(canvas_key).first<8u>());

  // Keep this loop byte-for-byte equivalent to Brave's balanced Canvas
  // farbling behavior. Every selected channel changes by at most one LSB.
  for (uint8_t key : canvas_key) {
    uint8_t bit = key;
    for (int index = 0; index < 16; ++index) {
      if (index % 8 == 0) {
        bit = key;
      }
      const uint8_t channel = value % 3;
      const uint64_t pixel_index = 4 * (value % pixel_count) + channel;
      rgba_pixels[pixel_index] ^= bit & 0x1;
      bit >>= 1;
      value = NextLfsrValue(value);
    }
  }
}

}  // namespace privacy_cef

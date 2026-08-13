/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Adapted from Brave Core commit 66867f5c43390a235672bfc3e091d02e84d6892c,
// third_party/blink/renderer/platform/brave_audio_farbling_helper.cc.

#include "components/privacy_cef/audio_farbling.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace privacy_cef {
namespace {

uint64_t NextLfsrValue(uint64_t value) {
  constexpr uint64_t kZero = 0;
  return (value >> 1) |
         (((value << 62) ^ (value << 61)) & (~(~kZero << 63) << 62));
}

float MaximumValue(uint64_t* value) {
  *value = NextLfsrValue(*value);
  return static_cast<float>(
      (*value / static_cast<double>(std::numeric_limits<uint64_t>::max())) /
      10.0);
}

float LinearToDecibels(float linear) {
  return 20.0f * std::log10(linear);
}

}  // namespace

AudioFarblingHelper::AudioFarblingHelper(double fudge_factor,
                                         uint64_t seed,
                                         bool maximum)
    : fudge_factor_(fudge_factor), seed_(seed), maximum_(maximum) {}

void AudioFarblingHelper::FarbleAudioChannel(
    base::span<float> destination) const {
  if (maximum_) {
    uint64_t value = seed_;
    for (float& sample : destination) {
      sample = MaximumValue(&value);
    }
    return;
  }
  for (float& sample : destination) {
    sample *= fudge_factor_;
  }
}

void AudioFarblingHelper::FarbleFloatTimeDomainData(
    base::span<const float> input_buffer,
    base::span<float> destination,
    size_t length,
    unsigned write_index,
    unsigned fft_size) const {
  uint64_t value = seed_;
  for (size_t index = 0; index < length; ++index) {
    destination[index] =
        maximum_
            ? MaximumValue(&value)
            : fudge_factor_ * input_buffer[(index + write_index - fft_size +
                                            input_buffer.size()) %
                                           input_buffer.size()];
  }
}

void AudioFarblingHelper::FarbleByteTimeDomainData(
    base::span<const float> input_buffer,
    base::span<uint8_t> destination,
    size_t length,
    unsigned write_index,
    unsigned fft_size) const {
  uint64_t value = seed_;
  for (size_t index = 0; index < length; ++index) {
    const float sample =
        maximum_
            ? MaximumValue(&value)
            : fudge_factor_ * input_buffer[(index + write_index - fft_size +
                                            input_buffer.size()) %
                                           input_buffer.size()];
    destination[index] =
        static_cast<uint8_t>(std::clamp(128.0 * (sample + 1.0), 0.0, 255.0));
  }
}

void AudioFarblingHelper::FarbleConvertToByteData(
    base::span<const float> source,
    base::span<uint8_t> destination,
    size_t length,
    double min_decibels,
    double range_scale_factor) const {
  uint64_t value = seed_;
  for (size_t index = 0; index < length; ++index) {
    const float linear =
        maximum_ ? MaximumValue(&value) : fudge_factor_ * source[index];
    const double scaled =
        255.0 * (LinearToDecibels(linear) - min_decibels) * range_scale_factor;
    destination[index] = static_cast<uint8_t>(std::clamp(scaled, 0.0, 255.0));
  }
}

void AudioFarblingHelper::FarbleConvertFloatToDb(base::span<const float> source,
                                                 base::span<float> destination,
                                                 size_t length) const {
  uint64_t value = seed_;
  for (size_t index = 0; index < length; ++index) {
    const float linear =
        maximum_ ? MaximumValue(&value) : fudge_factor_ * source[index];
    destination[index] = static_cast<float>(LinearToDecibels(linear));
  }
}

}  // namespace privacy_cef

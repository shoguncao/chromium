/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Adapted from Brave Core commit 66867f5c43390a235672bfc3e091d02e84d6892c,
// third_party/blink/renderer/platform/brave_audio_farbling_helper.h.

#ifndef COMPONENTS_PRIVACY_CEF_AUDIO_FARBLING_H_
#define COMPONENTS_PRIVACY_CEF_AUDIO_FARBLING_H_

#include <stddef.h>
#include <stdint.h>

#include "base/containers/span.h"

namespace privacy_cef {

// Port of BraveAudioFarblingHelper from Brave Core commit
// 66867f5c43390a235672bfc3e091d02e84d6892c (MPL-2.0). The caller supplies
// Privacy CEF's persistent, top-level-site-derived parameters.
class AudioFarblingHelper final {
 public:
  AudioFarblingHelper(double fudge_factor, uint64_t seed, bool maximum);

  void FarbleAudioChannel(base::span<float> destination) const;
  void FarbleFloatTimeDomainData(base::span<const float> input_buffer,
                                 base::span<float> destination,
                                 size_t length,
                                 unsigned write_index,
                                 unsigned fft_size) const;
  void FarbleByteTimeDomainData(base::span<const float> input_buffer,
                                base::span<uint8_t> destination,
                                size_t length,
                                unsigned write_index,
                                unsigned fft_size) const;
  void FarbleConvertToByteData(base::span<const float> source,
                               base::span<uint8_t> destination,
                               size_t length,
                               double min_decibels,
                               double range_scale_factor) const;
  void FarbleConvertFloatToDb(base::span<const float> source,
                              base::span<float> destination,
                              size_t length) const;

 private:
  double fudge_factor_;
  uint64_t seed_;
  bool maximum_;
};

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_AUDIO_FARBLING_H_

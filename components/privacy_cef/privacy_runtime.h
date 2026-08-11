// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_RUNTIME_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_RUNTIME_H_

#include <string_view>

#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "components/privacy_cef/privacy_profile.h"

namespace privacy_cef {

enum class CanvasProtectionResult {
  kRuntimeNotConfigured,
  kDisabled,
  kFarbled,
};

// Renderer-process runtime state. The browser process must deliver a validated
// profile over IPC before Blink handles protected page APIs. This class never
// reads the profile file and never logs the master seed.
class PrivacyRuntime {
 public:
  static PrivacyRuntime& GetInstance();

  PrivacyRuntime(const PrivacyRuntime&) = delete;
  PrivacyRuntime& operator=(const PrivacyRuntime&) = delete;

  void SetProfile(PrivacyProfile profile);
  bool IsConfigured() const;

  CanvasProtectionResult ProtectCanvasPixels(
      std::string_view top_level_site,
      base::span<uint8_t> rgba_pixels) const;

  void ResetForTesting();

 private:
  friend class base::NoDestructor<PrivacyRuntime>;

  PrivacyRuntime();
  ~PrivacyRuntime();

  class State;
  State& state() const;
};

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_PRIVACY_RUNTIME_H_

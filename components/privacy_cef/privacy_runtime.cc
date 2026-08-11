// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_runtime.h"

#include <optional>
#include <utility>

#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "components/privacy_cef/canvas_farbling.h"
#include "components/privacy_cef/privacy_seed.h"

namespace privacy_cef {

class PrivacyRuntime::State {
 public:
  mutable base::Lock lock;
  std::optional<PrivacyProfile> profile GUARDED_BY(lock);
};

PrivacyRuntime& PrivacyRuntime::GetInstance() {
  static base::NoDestructor<PrivacyRuntime> instance;
  return *instance;
}

PrivacyRuntime::PrivacyRuntime() = default;
PrivacyRuntime::~PrivacyRuntime() = default;

PrivacyRuntime::State& PrivacyRuntime::state() const {
  static base::NoDestructor<State> state;
  return *state;
}

void PrivacyRuntime::SetProfile(PrivacyProfile profile) {
  base::AutoLock lock(state().lock);
  state().profile = std::move(profile);
}

bool PrivacyRuntime::IsConfigured() const {
  base::AutoLock lock(state().lock);
  return state().profile.has_value();
}

CanvasProtectionResult PrivacyRuntime::ProtectCanvasPixels(
    std::string_view top_level_site,
    base::span<uint8_t> rgba_pixels) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return CanvasProtectionResult::kRuntimeNotConfigured;
    }
    profile = *state().profile;
  }

  if (profile.canvas_mode != CanvasMode::kFarble || top_level_site.empty() ||
      rgba_pixels.empty() || rgba_pixels.size() % 4 != 0) {
    return CanvasProtectionResult::kDisabled;
  }

  FarbleCanvasPixels(DeriveSiteSeed(profile, top_level_site), rgba_pixels);
  return CanvasProtectionResult::kFarbled;
}

void PrivacyRuntime::ResetForTesting() {
  base::AutoLock lock(state().lock);
  state().profile.reset();
}

}  // namespace privacy_cef

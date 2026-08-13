// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_runtime.h"

#include <limits>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/numerics/byte_conversions.h"
#include "base/process/process.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "components/privacy_cef/canvas_farbling.h"
#include "components/privacy_cef/privacy_seed.h"
#include "crypto/hmac.h"
#include "third_party/abseil-cpp/absl/random/random.h"

namespace privacy_cef {
namespace {

uint64_t NextLfsrValue(uint64_t value) {
  constexpr uint64_t kZero = 0;
  return (value >> 1) |
         (((value << 62) ^ (value << 61)) & (~(~kZero << 63) << 62));
}

}  // namespace

class PrivacyRuntime::State {
 public:
  mutable base::Lock lock;
  std::optional<PrivacyProfile> profile GUARDED_BY(lock);
  scoped_refptr<base::SequencedTaskRunner> audit_task_runner GUARDED_BY(lock);
  base::RepeatingCallback<void(PrivacyAuditEvent)> audit_callback
      GUARDED_BY(lock);
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

void PrivacyRuntime::SetAuditCallback(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    base::RepeatingCallback<void(PrivacyAuditEvent)> callback) {
  base::AutoLock lock(state().lock);
  state().audit_task_runner = std::move(task_runner);
  state().audit_callback = std::move(callback);
}

bool PrivacyRuntime::IsConfigured() const {
  base::AutoLock lock(state().lock);
  return state().profile.has_value();
}

CanvasProtectionResult PrivacyRuntime::ProtectCanvasPixels(
    std::string_view top_level_site,
    base::span<uint8_t> rgba_pixels,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return CanvasProtectionResult::kRuntimeNotConfigured;
    }
    profile = *state().profile;
  }

  CanvasProtectionResult result = CanvasProtectionResult::kDisabled;
  if (profile.canvas_mode != CanvasMode::kFarble || top_level_site.empty() ||
      rgba_pixels.empty() || rgba_pixels.size() % 4 != 0) {
    result = CanvasProtectionResult::kDisabled;
  } else {
    FarbleCanvasPixels(DeriveSiteSeed(profile, top_level_site), rgba_pixels);
    result = CanvasProtectionResult::kFarbled;
  }

  PostAuditEvent(profile, "canvas", profile.canvas_algorithm_version,
                 result == CanvasProtectionResult::kFarbled ? "farbled" : "off",
                 std::move(audit_context), top_level_site);

  return result;
}

WebGlMode PrivacyRuntime::GetWebGlMode() const {
  base::AutoLock lock(state().lock);
  return state().profile ? state().profile->webgl_mode : WebGlMode::kOff;
}

std::string PrivacyRuntime::WebGlDebugString(
    std::string_view top_level_site,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return {};
    }
    profile = *state().profile;
  }
  const bool standardized =
      profile.webgl_mode == WebGlMode::kStandardize ||
      profile.webgl_mode == WebGlMode::kStandardizeAndFarble;
  PostAuditEvent(profile, "webgl", 1, standardized ? "standardized" : "off",
                 std::move(audit_context), top_level_site);
  return standardized ? "Brave" : std::string();
}

std::string PrivacyRuntime::WebGlRandomString(
    std::string_view top_level_site,
    std::string_view seed_label,
    size_t length,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return {};
    }
    profile = *state().profile;
  }
  if (profile.webgl_mode != WebGlMode::kBlock || top_level_site.empty()) {
    PostAuditEvent(profile, "webgl", 1, "off", std::move(audit_context),
                   top_level_site);
    return {};
  }
  constexpr std::string_view kLetters =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  const PrivacySeed site_seed = DeriveSiteSeed(profile, top_level_site);
  const PrivacySeed key =
      crypto::hmac::SignSha256(site_seed, base::as_byte_span(seed_label));
  uint64_t value = base::U64FromNativeEndian(base::span(key).first<8u>());
  std::string result;
  result.reserve(length);
  for (size_t index = 0; index < length; ++index) {
    result.push_back(kLetters[value % kLetters.size()]);
    value = NextLfsrValue(value);
  }
  PostAuditEvent(profile, "webgl", 1, "blocked", std::move(audit_context),
                 top_level_site);
  return result;
}

size_t PrivacyRuntime::WebGlFakeExtensionIndex(
    std::string_view top_level_site,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return 0;
    }
    profile = *state().profile;
  }
  if (profile.webgl_mode != WebGlMode::kStandardizeAndFarble ||
      top_level_site.empty()) {
    const char* decision = profile.webgl_mode == WebGlMode::kStandardize
                               ? "standardized"
                           : profile.webgl_mode == WebGlMode::kBlock ? "blocked"
                                                                     : "off";
    PostAuditEvent(profile, "webgl", 1, decision, std::move(audit_context),
                   top_level_site);
    return 0;
  }
  const PrivacySeed seed = DeriveSiteSeed(profile, top_level_site);
  const size_t index =
      base::U64FromNativeEndian(base::span(seed).first<8u>()) % 21u;
  PostAuditEvent(profile, "webgl", 1, "farbled", std::move(audit_context),
                 top_level_site);
  return index;
}

int64_t PrivacyRuntime::FarbleWebGlInteger(
    std::string_view top_level_site,
    int64_t value,
    int discard,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return value;
    }
    profile = *state().profile;
  }
  if (profile.webgl_mode != WebGlMode::kStandardizeAndFarble ||
      top_level_site.empty() || value <= 0) {
    const char* decision = profile.webgl_mode == WebGlMode::kStandardize
                               ? "standardized"
                           : profile.webgl_mode == WebGlMode::kBlock ? "blocked"
                                                                     : "off";
    PostAuditEvent(profile, "webgl", 1, decision, std::move(audit_context),
                   top_level_site);
    return value;
  }
  const PrivacySeed site_seed = DeriveSiteSeed(profile, top_level_site);
  const uint64_t high =
      base::U64FromNativeEndian(base::span(site_seed).subspan<0u, 8u>());
  const uint64_t low =
      base::U64FromNativeEndian(base::span(site_seed).subspan<8u, 8u>());
  absl::random_internal::randen_engine<uint64_t> prng(high ^ low);
  prng.discard(discard);
  const int64_t farbled = prng() % 2 != 0 ? value - 1 : value;
  PostAuditEvent(profile, "webgl", 1, "farbled", std::move(audit_context),
                 top_level_site);
  return farbled;
}

WebGlProtectionResult PrivacyRuntime::ProtectWebGlPixels(
    std::string_view top_level_site,
    base::span<uint8_t> rgba_pixels,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return WebGlProtectionResult::kRuntimeNotConfigured;
    }
    profile = *state().profile;
  }
  WebGlProtectionResult result = WebGlProtectionResult::kDisabled;
  if (profile.webgl_mode == WebGlMode::kStandardize ||
      profile.webgl_mode == WebGlMode::kStandardizeAndFarble) {
    // Brave balanced WebGL protection intentionally leaves readPixels output
    // unchanged. Do not invent a pixel mutation algorithm for this boundary.
    result = WebGlProtectionResult::kStandardized;
  }
  PostAuditEvent(profile, "webgl", 1,
                 result == WebGlProtectionResult::kFarbled ? "farbled"
                 : result == WebGlProtectionResult::kStandardized
                     ? (profile.webgl_mode == WebGlMode::kStandardizeAndFarble
                            ? "native-balanced"
                            : "standardized")
                     : "off",
                 std::move(audit_context), top_level_site);
  return result;
}

void PrivacyRuntime::RecordWebGlAccess(
    std::string_view top_level_site,
    std::string policy_decision,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return;
    }
    profile = *state().profile;
  }
  PostAuditEvent(profile, "webgl", 1, std::move(policy_decision),
                 std::move(audit_context), top_level_site);
}

AudioMode PrivacyRuntime::GetAudioMode() const {
  base::AutoLock lock(state().lock);
  return state().profile ? state().profile->audio_mode : AudioMode::kOff;
}

std::optional<AudioFarblingParameters>
PrivacyRuntime::GetAudioFarblingParameters(
    std::string_view top_level_site) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return std::nullopt;
    }
    profile = *state().profile;
  }
  if (profile.audio_mode == AudioMode::kOff || top_level_site.empty()) {
    return std::nullopt;
  }
  const PrivacySeed site_seed = DeriveSiteSeed(profile, top_level_site);
  const uint64_t high =
      base::U64FromNativeEndian(base::span(site_seed).subspan<0u, 8u>());
  const uint64_t low =
      base::U64FromNativeEndian(base::span(site_seed).subspan<8u, 8u>());
  return AudioFarblingParameters{
      .fudge_factor =
          0.999 +
          ((high / static_cast<double>(std::numeric_limits<uint64_t>::max())) /
           1000.0),
      .seed = low,
      .maximum = profile.audio_mode == AudioMode::kBlock,
  };
}

bool PrivacyRuntime::ProtectAudioChannel(
    std::string_view top_level_site,
    base::span<float> samples,
    PrivacyAuditContext audit_context) const {
  const auto parameters = GetAudioFarblingParameters(top_level_site);
  if (parameters && !samples.empty()) {
    AudioFarblingHelper(parameters->fudge_factor, parameters->seed,
                        parameters->maximum)
        .FarbleAudioChannel(samples);
  }
  RecordAudioAccess(top_level_site, std::move(audit_context));
  return parameters.has_value();
}

void PrivacyRuntime::RecordAudioAccess(
    std::string_view top_level_site,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return;
    }
    profile = *state().profile;
  }
  const char* decision = profile.audio_mode == AudioMode::kFarble  ? "farbled"
                         : profile.audio_mode == AudioMode::kBlock ? "blocked"
                                                                   : "off";
  PostAuditEvent(profile, "audio", 1, decision, std::move(audit_context),
                 top_level_site);
}

WebGpuMode PrivacyRuntime::GetWebGpuMode() const {
  base::AutoLock lock(state().lock);
  return state().profile ? state().profile->webgpu_mode : WebGpuMode::kOff;
}

FontsMode PrivacyRuntime::GetFontsMode() const {
  base::AutoLock lock(state().lock);
  return state().profile ? state().profile->fonts_mode : FontsMode::kOff;
}

NavigatorMode PrivacyRuntime::GetNavigatorMode() const {
  base::AutoLock lock(state().lock);
  return state().profile ? state().profile->navigator_mode
                         : NavigatorMode::kOff;
}

std::optional<PrivacyRuntime::DisplayProfile>
PrivacyRuntime::GetDisplayProfile() const {
  base::AutoLock lock(state().lock);
  if (!state().profile) {
    return std::nullopt;
  }
  return DisplayProfile{state().profile->screen_width,
                        state().profile->screen_height,
                        state().profile->device_scale_factor,
                        state().profile->color_depth,
                        state().profile->color_gamut};
}

void PrivacyRuntime::RecordWebGpuAccess(
    std::string_view top_level_site,
    std::string policy_decision,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return;
    }
    profile = *state().profile;
  }
  PostAuditEvent(profile, "webgpu", 1, std::move(policy_decision),
                 std::move(audit_context), top_level_site);
}

std::optional<int> PrivacyRuntime::GetProfileCpuCores() const {
  base::AutoLock lock(state().lock);
  return state().profile ? std::optional(state().profile->cpu_cores)
                         : std::nullopt;
}

std::optional<int> PrivacyRuntime::GetProfileMemoryGb() const {
  base::AutoLock lock(state().lock);
  return state().profile
             ? std::optional(state().profile->navigator_device_memory_gb)
                         : std::nullopt;
}

void PrivacyRuntime::RecordNavigatorHardwareAccess(
    std::string_view top_level_site,
    PrivacyAuditContext audit_context) const {
  PrivacyProfile profile;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return;
    }
    profile = *state().profile;
  }
  PostAuditEvent(profile, "navigator", 1, "profiled", std::move(audit_context),
                 top_level_site);
}

void PrivacyRuntime::PostAuditEvent(const PrivacyProfile& profile,
                                    std::string category,
                                    int algorithm_version,
                                    std::string policy_decision,
                                    PrivacyAuditContext audit_context,
                                    std::string_view top_level_site) const {
  scoped_refptr<base::SequencedTaskRunner> audit_task_runner;
  base::RepeatingCallback<void(PrivacyAuditEvent)> audit_callback;
  {
    base::AutoLock lock(state().lock);
    audit_task_runner = state().audit_task_runner;
    audit_callback = state().audit_callback;
  }
  if (!audit_task_runner || !audit_callback || audit_context.api.empty()) {
    return;
  }
  PrivacyAuditEvent event;
  event.process_type = "renderer";
  event.process_id = base::Process::Current().Pid();
  event.thread_id =
      base::PlatformThread::CurrentId().truncate_to_int32_for_display_only();
  event.context_type = std::move(audit_context.context_type);
  event.frame_id = std::move(audit_context.frame_id);
  event.worker_id = std::move(audit_context.worker_id);
  event.top_level_site = std::string(top_level_site);
  event.frame_origin = std::move(audit_context.frame_origin);
  event.category = std::move(category);
  event.api = std::move(audit_context.api);
  event.profile_id = profile.profile_id;
  event.algorithm_version = algorithm_version;
  event.policy_decision = std::move(policy_decision);
  event.outcome = "success";
  audit_task_runner->PostTask(FROM_HERE,
                              base::BindOnce(audit_callback, std::move(event)));
}

void PrivacyRuntime::ResetForTesting() {
  base::AutoLock lock(state().lock);
  state().profile.reset();
  state().audit_task_runner.reset();
  state().audit_callback.Reset();
}

}  // namespace privacy_cef

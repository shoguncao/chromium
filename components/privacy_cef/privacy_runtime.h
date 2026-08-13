// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_RUNTIME_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_RUNTIME_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "components/privacy_cef/audio_farbling.h"
#include "components/privacy_cef/privacy_audit_service.h"
#include "components/privacy_cef/privacy_profile.h"

namespace privacy_cef {

enum class CanvasProtectionResult {
  kRuntimeNotConfigured,
  kDisabled,
  kFarbled,
};

enum class WebGlProtectionResult {
  kRuntimeNotConfigured,
  kDisabled,
  kStandardized,
  kFarbled,
};

struct AudioFarblingParameters {
  double fudge_factor = 1.0;
  uint64_t seed = 0;
  bool maximum = false;
};

struct PrivacyAuditContext {
  std::string api;
  std::string context_type;
  std::string frame_id;
  std::string worker_id;
  std::string frame_origin;
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
  void SetAuditCallback(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      base::RepeatingCallback<void(PrivacyAuditEvent)> callback);
  bool IsConfigured() const;

  CanvasProtectionResult ProtectCanvasPixels(
      std::string_view top_level_site,
      base::span<uint8_t> rgba_pixels,
      PrivacyAuditContext audit_context = {}) const;

  WebGlMode GetWebGlMode() const;
  std::string WebGlDebugString(std::string_view top_level_site,
                               PrivacyAuditContext audit_context = {}) const;
  std::string WebGlRandomString(std::string_view top_level_site,
                                std::string_view seed_label,
                                size_t length,
                                PrivacyAuditContext audit_context = {}) const;
  size_t WebGlFakeExtensionIndex(std::string_view top_level_site,
                                 PrivacyAuditContext audit_context = {}) const;
  int64_t FarbleWebGlInteger(std::string_view top_level_site,
                             int64_t value,
                             int discard,
                             PrivacyAuditContext audit_context = {}) const;
  WebGlProtectionResult ProtectWebGlPixels(
      std::string_view top_level_site,
      base::span<uint8_t> rgba_pixels,
      PrivacyAuditContext audit_context = {}) const;
  void RecordWebGlAccess(std::string_view top_level_site,
                         std::string policy_decision,
                         PrivacyAuditContext audit_context) const;

  AudioMode GetAudioMode() const;
  std::optional<AudioFarblingParameters> GetAudioFarblingParameters(
      std::string_view top_level_site) const;
  bool ProtectAudioChannel(std::string_view top_level_site,
                           base::span<float> samples,
                           PrivacyAuditContext audit_context = {}) const;
  void RecordAudioAccess(std::string_view top_level_site,
                         PrivacyAuditContext audit_context) const;

  WebGpuMode GetWebGpuMode() const;
  void RecordWebGpuAccess(std::string_view top_level_site,
                          std::string policy_decision,
                          PrivacyAuditContext audit_context) const;

  void ResetForTesting();

 private:
  friend class base::NoDestructor<PrivacyRuntime>;

  PrivacyRuntime();
  ~PrivacyRuntime();

  class State;
  State& state() const;
  void PostAuditEvent(const PrivacyProfile& profile,
                      std::string category,
                      int algorithm_version,
                      std::string policy_decision,
                      PrivacyAuditContext audit_context,
                      std::string_view top_level_site) const;
};

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_PRIVACY_RUNTIME_H_

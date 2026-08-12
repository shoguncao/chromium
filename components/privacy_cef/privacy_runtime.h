// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_RUNTIME_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_RUNTIME_H_

#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "components/privacy_cef/privacy_audit_service.h"
#include "components/privacy_cef/privacy_profile.h"

namespace privacy_cef {

enum class CanvasProtectionResult {
  kRuntimeNotConfigured,
  kDisabled,
  kFarbled,
};

struct CanvasAuditContext {
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
      CanvasAuditContext audit_context = {}) const;

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

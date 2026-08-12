// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_runtime.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/process/process.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "components/privacy_cef/canvas_farbling.h"
#include "components/privacy_cef/privacy_seed.h"

namespace privacy_cef {

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
    CanvasAuditContext audit_context) const {
  PrivacyProfile profile;
  scoped_refptr<base::SequencedTaskRunner> audit_task_runner;
  base::RepeatingCallback<void(PrivacyAuditEvent)> audit_callback;
  {
    base::AutoLock lock(state().lock);
    if (!state().profile) {
      return CanvasProtectionResult::kRuntimeNotConfigured;
    }
    profile = *state().profile;
    audit_task_runner = state().audit_task_runner;
    audit_callback = state().audit_callback;
  }

  CanvasProtectionResult result = CanvasProtectionResult::kDisabled;
  if (profile.canvas_mode != CanvasMode::kFarble || top_level_site.empty() ||
      rgba_pixels.empty() || rgba_pixels.size() % 4 != 0) {
    result = CanvasProtectionResult::kDisabled;
  } else {
    FarbleCanvasPixels(DeriveSiteSeed(profile, top_level_site), rgba_pixels);
    result = CanvasProtectionResult::kFarbled;
  }

  if (audit_task_runner && audit_callback && !audit_context.api.empty()) {
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
    event.category = "canvas";
    event.api = std::move(audit_context.api);
    event.profile_id = profile.profile_id;
    event.algorithm_version = profile.canvas_algorithm_version;
    event.policy_decision =
        result == CanvasProtectionResult::kFarbled ? "farbled" : "off";
    event.outcome = "success";
    audit_task_runner->PostTask(
        FROM_HERE, base::BindOnce(audit_callback, std::move(event)));
  }

  return result;
}

void PrivacyRuntime::ResetForTesting() {
  base::AutoLock lock(state().lock);
  state().profile.reset();
  state().audit_task_runner.reset();
  state().audit_callback.Reset();
}

}  // namespace privacy_cef

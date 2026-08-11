// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_audit_service.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "base/values.h"

namespace privacy_cef {

PrivacyAuditEvent::PrivacyAuditEvent() = default;
PrivacyAuditEvent::PrivacyAuditEvent(const PrivacyAuditEvent&) = default;
PrivacyAuditEvent& PrivacyAuditEvent::operator=(const PrivacyAuditEvent&) =
    default;
PrivacyAuditEvent::PrivacyAuditEvent(PrivacyAuditEvent&&) = default;
PrivacyAuditEvent& PrivacyAuditEvent::operator=(PrivacyAuditEvent&&) = default;
PrivacyAuditEvent::~PrivacyAuditEvent() = default;

PrivacyAuditService::PrivacyAuditService(base::FilePath path, AuditMode mode)
    : path_(std::move(path)), mode_(mode) {}

PrivacyAuditService::~PrivacyAuditService() = default;

bool PrivacyAuditService::Record(PrivacyAuditEvent event) {
  if (mode_ == AuditMode::kOff) {
    return true;
  }

  base::AutoLock guard(lock_);
  base::DictValue value;
  value.Set("sequence", static_cast<double>(next_sequence_++));
  value.Set("timestampMs", base::Time::Now().InMillisecondsFSinceUnixEpoch());
  value.Set("processType", std::move(event.process_type));
  value.Set("processId", event.process_id);
  value.Set("threadId", event.thread_id);
  value.Set("frameId", std::move(event.frame_id));
  value.Set("workerId", std::move(event.worker_id));
  value.Set("topFrameOrigin", std::move(event.top_frame_origin));
  value.Set("frameOrigin", std::move(event.frame_origin));
  value.Set("category", std::move(event.category));
  value.Set("api", std::move(event.api));
  value.Set("profileId", std::move(event.profile_id));
  value.Set("algorithmVersion", event.algorithm_version);
  value.Set("policyDecision", std::move(event.policy_decision));
  value.Set("outcome", std::move(event.outcome));
  value.Set("error", std::move(event.error));
  std::string line;
  if (!base::JSONWriter::Write(value, &line) ||
      !base::AppendToFile(path_, line + "\n")) {
    ++failed_events_;
    return false;
  }
  ++written_events_;
  return true;
}

uint64_t PrivacyAuditService::written_events() const {
  base::AutoLock guard(lock_);
  return written_events_;
}

uint64_t PrivacyAuditService::failed_events() const {
  base::AutoLock guard(lock_);
  return failed_events_;
}

}  // namespace privacy_cef

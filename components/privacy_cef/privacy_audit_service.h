// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_AUDIT_SERVICE_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_AUDIT_SERVICE_H_

#include <cstdint>
#include <string>

#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "components/privacy_cef/privacy_profile.h"

namespace privacy_cef {

struct PrivacyAuditEvent {
  PrivacyAuditEvent();
  PrivacyAuditEvent(const PrivacyAuditEvent&);
  PrivacyAuditEvent& operator=(const PrivacyAuditEvent&);
  PrivacyAuditEvent(PrivacyAuditEvent&&);
  PrivacyAuditEvent& operator=(PrivacyAuditEvent&&);
  ~PrivacyAuditEvent();

  std::string process_type;
  int process_id = 0;
  int thread_id = 0;
  std::string frame_id;
  std::string worker_id;
  std::string top_frame_origin;
  std::string frame_origin;
  std::string category;
  std::string api;
  std::string profile_id;
  int algorithm_version = 0;
  std::string policy_decision;
  std::string outcome;
  std::string error;
};

// Browser-process-only JSONL writer. Renderer/GPU/worker code must report over
// IPC and must not instantiate this class directly.
class PrivacyAuditService {
 public:
  PrivacyAuditService(base::FilePath path, AuditMode mode);
  ~PrivacyAuditService();

  PrivacyAuditService(const PrivacyAuditService&) = delete;
  PrivacyAuditService& operator=(const PrivacyAuditService&) = delete;

  bool Record(PrivacyAuditEvent event);
  uint64_t written_events() const;
  uint64_t failed_events() const;

 private:
  const base::FilePath path_;
  const AuditMode mode_;
  mutable base::Lock lock_;
  uint64_t next_sequence_ GUARDED_BY(lock_) = 1;
  uint64_t written_events_ GUARDED_BY(lock_) = 0;
  uint64_t failed_events_ GUARDED_BY(lock_) = 0;
};

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_PRIVACY_AUDIT_SERVICE_H_

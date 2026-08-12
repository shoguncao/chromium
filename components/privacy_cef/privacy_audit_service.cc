// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_audit_service.h"

#include <algorithm>
#include <utility>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"

namespace privacy_cef {

PrivacyAuditEvent::PrivacyAuditEvent() = default;
PrivacyAuditEvent::PrivacyAuditEvent(const PrivacyAuditEvent&) = default;
PrivacyAuditEvent& PrivacyAuditEvent::operator=(const PrivacyAuditEvent&) =
    default;
PrivacyAuditEvent::PrivacyAuditEvent(PrivacyAuditEvent&&) = default;
PrivacyAuditEvent& PrivacyAuditEvent::operator=(PrivacyAuditEvent&&) = default;
PrivacyAuditEvent::~PrivacyAuditEvent() = default;

PrivacyAuditService::PrivacyAuditService(base::FilePath path,
                                         AuditMode mode,
                                         int retention_days,
                                         int max_file_size_mb)
    : path_(std::move(path)),
      mode_(mode),
      retention_days_(std::clamp(retention_days, 1, 30)),
      max_file_size_bytes_(
          static_cast<int64_t>(std::clamp(max_file_size_mb, 1, 100)) * 1024 *
          1024) {
  if (mode_ != AuditMode::kOff && !path_.empty()) {
    base::CreateDirectory(path_.DirName());
    base::AutoLock guard(lock_);
    RemoveExpiredFilesLocked();
  }
}

PrivacyAuditService::~PrivacyAuditService() = default;

bool PrivacyAuditService::Record(PrivacyAuditEvent event) {
  if (mode_ == AuditMode::kOff) {
    return true;
  }

  base::AutoLock guard(lock_);
  if (mode_ == AuditMode::kSummary) {
    const std::string key = base::StrCat(
        {event.process_type, "\n", event.context_type, "\n",
         event.top_level_site, "\n", event.frame_origin, "\n", event.category,
         "\n", event.api, "\n", event.policy_decision, "\n", event.outcome});
    ++summary_counts_[key];
    summary_events_[key] = std::move(event);
    if (!RewriteSummaryLocked()) {
      ++failed_events_;
      LOG(ERROR) << "Privacy CEF audit summary write failed: " << path_;
      return false;
    }
    ++written_events_;
    return true;
  }

  base::DictValue value;
  value.Set("sequence", static_cast<double>(next_sequence_++));
  value.Set("timestampMs", base::Time::Now().InMillisecondsFSinceUnixEpoch());
  value.Set("eventType", "access");
  value.Set("processType", event.process_type);
  value.Set("processId", event.process_id);
  value.Set("threadId", event.thread_id);
  value.Set("contextType", event.context_type);
  value.Set("frameId", event.frame_id);
  value.Set("workerId", event.worker_id);
  value.Set("topLevelSite", event.top_level_site);
  value.Set("frameOrigin", event.frame_origin);
  value.Set("category", event.category);
  value.Set("api", event.api);
  value.Set("profileId", event.profile_id);
  value.Set("algorithmVersion", event.algorithm_version);
  value.Set("policyDecision", event.policy_decision);
  value.Set("outcome", event.outcome);
  value.Set("error", event.error);
  std::string line;
  if (!base::JSONWriter::Write(value, &line) ||
      !AppendLineLocked(std::move(line))) {
    ++failed_events_;
    LOG(ERROR) << "Privacy CEF audit write failed: " << path_;
    return false;
  }
  ++written_events_;
  return true;
}

bool PrivacyAuditService::RewriteSummaryLocked() {
  std::string contents;
  uint64_t sequence = 1;
  for (const auto& [key, event] : summary_events_) {
    base::DictValue value;
    value.Set("sequence", static_cast<double>(sequence++));
    value.Set("timestampMs", base::Time::Now().InMillisecondsFSinceUnixEpoch());
    value.Set("eventType", "summary");
    value.Set("processType", event.process_type);
    value.Set("processId", 0);
    value.Set("threadId", 0);
    value.Set("contextType", event.context_type);
    value.Set("frameId", "");
    value.Set("workerId", "");
    value.Set("topLevelSite", event.top_level_site);
    value.Set("frameOrigin", event.frame_origin);
    value.Set("category", event.category);
    value.Set("api", event.api);
    value.Set("profileId", event.profile_id);
    value.Set("algorithmVersion", event.algorithm_version);
    value.Set("policyDecision", event.policy_decision);
    value.Set("outcome", event.outcome);
    value.Set("error", event.error.empty() ? "" : "redacted-error");
    value.Set("count", static_cast<double>(summary_counts_[key]));
    std::string line;
    if (!base::JSONWriter::Write(value, &line)) {
      return false;
    }
    contents.append(line);
    contents.push_back('\n');
  }
  if (dropped_events_ > 0) {
    base::DictValue value;
    value.Set("sequence", static_cast<double>(sequence));
    value.Set("timestampMs", base::Time::Now().InMillisecondsFSinceUnixEpoch());
    value.Set("eventType", "health");
    value.Set("category", "audit");
    value.Set("api", "PrivacyAudit.drop");
    value.Set("outcome", "dropped");
    value.Set("reason", dropped_reason_);
    value.Set("droppedEvents", static_cast<double>(dropped_events_));
    std::string line;
    if (!base::JSONWriter::Write(value, &line)) {
      return false;
    }
    contents.append(line);
    contents.push_back('\n');
  }
  if (static_cast<int64_t>(contents.size()) > max_file_size_bytes_ ||
      !base::WriteFile(path_, contents)) {
    return false;
  }
#if BUILDFLAG(IS_POSIX)
  return base::SetPosixFilePermissions(path_, 0600);
#else
  return true;
#endif
}

void PrivacyAuditService::RecordDroppedEvents(std::string reason,
                                              uint64_t count) {
  if (mode_ == AuditMode::kOff) {
    return;
  }
  base::AutoLock guard(lock_);
  dropped_events_ += count;
  dropped_reason_ = reason;
  if (mode_ == AuditMode::kSummary) {
    if (!RewriteSummaryLocked()) {
      ++failed_events_;
      LOG(ERROR) << "Privacy CEF audit dropped-event write failed: " << path_;
    } else {
      ++written_events_;
    }
    return;
  }
  base::DictValue value;
  value.Set("sequence", static_cast<double>(next_sequence_++));
  value.Set("timestampMs", base::Time::Now().InMillisecondsFSinceUnixEpoch());
  value.Set("eventType", "health");
  value.Set("category", "audit");
  value.Set("api", "PrivacyAudit.drop");
  value.Set("outcome", "dropped");
  value.Set("reason", std::move(reason));
  value.Set("droppedEvents", static_cast<double>(dropped_events_));
  value.Set("droppedInBatch", static_cast<double>(count));
  std::string line;
  if (!base::JSONWriter::Write(value, &line) ||
      !AppendLineLocked(std::move(line))) {
    ++failed_events_;
    LOG(ERROR) << "Privacy CEF audit dropped-event write failed: " << path_;
  } else {
    ++written_events_;
  }
}

bool PrivacyAuditService::AppendLineLocked(std::string line) {
  line.push_back('\n');
  if (!RotateIfNeededLocked(line.size())) {
    return false;
  }
  if (!base::PathExists(path_)) {
    if (!base::WriteFile(path_, line)) {
      return false;
    }
#if BUILDFLAG(IS_POSIX)
    if (!base::SetPosixFilePermissions(path_, 0600)) {
      return false;
    }
#endif
    return true;
  }
  return base::AppendToFile(path_, line);
}

bool PrivacyAuditService::RotateIfNeededLocked(size_t next_line_size) {
  const std::optional<int64_t> current_size = base::GetFileSize(path_);
  if (!current_size || *current_size + static_cast<int64_t>(next_line_size) <=
                           max_file_size_bytes_) {
    return true;
  }
  constexpr int kMaximumRotatedFiles = 32;
  base::DeleteFile(
      path_.AddExtensionASCII(base::NumberToString(kMaximumRotatedFiles)));
  for (int index = kMaximumRotatedFiles - 1; index >= 1; --index) {
    const base::FilePath from =
        path_.AddExtensionASCII(base::NumberToString(index));
    const base::FilePath to =
        path_.AddExtensionASCII(base::NumberToString(index + 1));
    if (base::PathExists(from) && !base::ReplaceFile(from, to, nullptr)) {
      return false;
    }
  }
  if (base::PathExists(path_) &&
      !base::ReplaceFile(path_, path_.AddExtensionASCII("1"), nullptr)) {
    return false;
  }
  RemoveExpiredFilesLocked();
  return true;
}

void PrivacyAuditService::RemoveExpiredFilesLocked() {
  const base::Time cutoff = base::Time::Now() - base::Days(retention_days_);
  base::FileEnumerator files(path_.DirName(), false,
                             base::FileEnumerator::FILES,
                             path_.BaseName().value() + FILE_PATH_LITERAL("*"));
  for (base::FilePath file = files.Next(); !file.empty(); file = files.Next()) {
    base::File::Info info;
    if (base::GetFileInfo(file, &info) && info.last_modified < cutoff) {
      base::DeleteFile(file);
    }
  }
}

uint64_t PrivacyAuditService::written_events() const {
  base::AutoLock guard(lock_);
  return written_events_;
}

uint64_t PrivacyAuditService::failed_events() const {
  base::AutoLock guard(lock_);
  return failed_events_;
}

uint64_t PrivacyAuditService::dropped_events() const {
  base::AutoLock guard(lock_);
  return dropped_events_;
}

}  // namespace privacy_cef

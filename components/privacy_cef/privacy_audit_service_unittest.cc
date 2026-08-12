// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_audit_service.h"

#include <algorithm>
#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace privacy_cef {
namespace {

PrivacyAuditEvent TestEvent() {
  PrivacyAuditEvent event;
  event.process_type = "renderer";
  event.process_id = 42;
  event.thread_id = 7;
  event.context_type = "worker";
  event.top_level_site = "https://example.test";
  event.frame_origin = "https://frame.test";
  event.category = "canvas";
  event.api = "OffscreenCanvas.convertToBlob";
  event.profile_id = "profile-a";
  event.algorithm_version = 1;
  event.policy_decision = "farbled";
  event.outcome = "success";
  return event;
}

TEST(PrivacyAuditServiceTest, OffDoesNotCreateFile) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  const base::FilePath path = temp.GetPath().AppendASCII("audit.jsonl");
  PrivacyAuditService service(path, AuditMode::kOff, 7, 1);
  EXPECT_TRUE(service.Record(TestEvent()));
  EXPECT_FALSE(base::PathExists(path));
}

TEST(PrivacyAuditServiceTest, FullWritesStructuredEventWithoutSeed) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  const base::FilePath path = temp.GetPath().AppendASCII("audit.jsonl");
  PrivacyAuditService service(path, AuditMode::kFull, 7, 1);
  EXPECT_TRUE(service.Record(TestEvent()));
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(path, &contents));
  EXPECT_NE(contents.find("\"eventType\":\"access\""), std::string::npos);
  EXPECT_NE(contents.find("\"api\":\"OffscreenCanvas.convertToBlob\""),
            std::string::npos);
  EXPECT_EQ(contents.find("masterSeed"), std::string::npos);
#if BUILDFLAG(IS_POSIX)
  int permissions = 0;
  ASSERT_TRUE(base::GetPosixFilePermissions(path, &permissions));
  EXPECT_EQ(permissions, 0600);
#endif
}

TEST(PrivacyAuditServiceTest, SummaryWritesIncreasingAggregateCount) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  const base::FilePath path = temp.GetPath().AppendASCII("audit.jsonl");
  PrivacyAuditService service(path, AuditMode::kSummary, 7, 1);
  EXPECT_TRUE(service.Record(TestEvent()));
  EXPECT_TRUE(service.Record(TestEvent()));
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(path, &contents));
  EXPECT_NE(contents.find("\"eventType\":\"summary\""), std::string::npos);
  EXPECT_NE(contents.find("\"count\":2.0"), std::string::npos);
  EXPECT_EQ(std::count(contents.begin(), contents.end(), '\n'), 1);
}

TEST(PrivacyAuditServiceTest, TracksDroppedEvents) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  const base::FilePath path = temp.GetPath().AppendASCII("audit.jsonl");
  PrivacyAuditService service(path, AuditMode::kFull, 7, 1);
  service.RecordDroppedEvents("invalid-event", 3);
  EXPECT_EQ(service.dropped_events(), 3u);
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(path, &contents));
  EXPECT_NE(contents.find("\"droppedEvents\":3.0"), std::string::npos);
  EXPECT_NE(contents.find("\"droppedInBatch\":3.0"), std::string::npos);
  EXPECT_NE(contents.find("\"eventType\":\"health\""), std::string::npos);
}

TEST(PrivacyAuditServiceTest, SummaryRetainsDroppedEventHealth) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  const base::FilePath path = temp.GetPath().AppendASCII("audit.jsonl");
  PrivacyAuditService service(path, AuditMode::kSummary, 7, 1);
  EXPECT_TRUE(service.Record(TestEvent()));
  service.RecordDroppedEvents("queue-overflow", 2);
  EXPECT_TRUE(service.Record(TestEvent()));
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(path, &contents));
  EXPECT_NE(contents.find("\"eventType\":\"health\""), std::string::npos);
  EXPECT_NE(contents.find("\"droppedEvents\":2.0"), std::string::npos);
}

TEST(PrivacyAuditServiceTest, RotatesAtConfiguredSize) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  const base::FilePath path = temp.GetPath().AppendASCII("audit.jsonl");
  PrivacyAuditService service(path, AuditMode::kFull, 7, 1);
  PrivacyAuditEvent event = TestEvent();
  event.frame_id.assign(1800, 'f');
  for (int i = 0; i < 700; ++i) {
    ASSERT_TRUE(service.Record(event));
  }
  EXPECT_TRUE(base::PathExists(path));
  EXPECT_TRUE(base::PathExists(path.AddExtensionASCII("1")));
}

TEST(PrivacyAuditServiceTest, RemovesExpiredRotatedFiles) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  const base::FilePath path = temp.GetPath().AppendASCII("audit.jsonl");
  const base::FilePath expired = path.AddExtensionASCII("1");
  ASSERT_TRUE(base::WriteFile(expired, "old"));
  const base::Time old = base::Time::Now() - base::Days(10);
  ASSERT_TRUE(base::TouchFile(expired, old, old));
  PrivacyAuditService service(path, AuditMode::kFull, 7, 1);
  EXPECT_FALSE(base::PathExists(expired));
}

TEST(PrivacyAuditServiceTest, ReportsWriteFailure) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  const base::FilePath directory_path =
      temp.GetPath().AppendASCII("audit.jsonl");
  ASSERT_TRUE(base::CreateDirectory(directory_path));
  PrivacyAuditService service(directory_path, AuditMode::kFull, 7, 1);
  EXPECT_FALSE(service.Record(TestEvent()));
  EXPECT_EQ(service.failed_events(), 1u);
}

}  // namespace
}  // namespace privacy_cef

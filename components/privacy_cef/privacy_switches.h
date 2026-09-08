// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_SWITCHES_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_SWITCHES_H_

namespace privacy_cef::switches {

extern const char kPrivacyProfilePath[];
extern const char kPrivacyAuditMode[];
extern const char kPrivacyAuditLogPath[];

// Egress IP environment overrides. Read by the browser process only: command
// line switches are not propagated to renderers, the resolved values travel
// over the existing NewRenderThreadInfo IPC instead.
// --privacy-timezone=America/Los_Angeles
extern const char kPrivacyTimezone[];
// --privacy-accept-language=en-US,en
extern const char kPrivacyAcceptLanguage[];

}  // namespace privacy_cef::switches

#endif  // COMPONENTS_PRIVACY_CEF_PRIVACY_SWITCHES_H_

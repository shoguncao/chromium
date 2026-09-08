// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_TIMEZONE_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_TIMEZONE_H_

#include <string>
#include <string_view>

namespace privacy_cef {

// Returns true when |iana_id| resolves to a real ICU time zone.
//
// ICU never fails on an unknown ID: TimeZone::createTimeZone() silently
// returns the "Etc/Unknown"/GMT fallback. Callers must therefore use this
// helper instead of trusting createTimeZone() and never fall back to UTC when
// the requested ID is invalid.
bool IsValidTimeZoneId(std::string_view iana_id);

// Sets the default time zone of the current process to |iana_id|.
//
// Covers both consumers of "the current time zone":
//   * ICU (V8 Date/Intl, WTF date math, Blink) via icu::TimeZone::adoptDefault
//   * base::Time::LocalExplode()/localtime_r(), which read the TZ environment
//     variable on POSIX and never look at ICU (see base/time/time_exploded_posix.cc).
//
// Returns false and leaves the process untouched when |iana_id| is invalid;
// callers must log and skip instead of applying a silent fallback.
bool SetProcessTimeZone(std::string_view iana_id);

// Returns the ICU default time zone ID of the current process.
std::string GetProcessTimeZoneId();

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_PRIVACY_TIMEZONE_H_

// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_timezone.h"

#include <memory>
#include <string>
#include <string_view>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "third_party/icu/source/common/unicode/unistr.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

#if BUILDFLAG(IS_POSIX)
#include <stdlib.h>
#include <time.h>
#endif

namespace privacy_cef {
namespace {

std::string ToUtf8(const icu::UnicodeString& value) {
  std::string out;
  value.toUTF8String(out);
  return out;
}

bool EqualsIgnoringCase(std::string_view a, std::string_view b) {
  return base::EqualsCaseInsensitiveASCII(a, b);
}

}  // namespace

bool IsValidTimeZoneId(std::string_view iana_id) {
  if (iana_id.empty()) {
    return false;
  }
  const std::string requested(iana_id);
  std::unique_ptr<icu::TimeZone> zone(
      icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(requested)));
  if (!zone) {
    return false;
  }
  icu::UnicodeString resolved_ustring;
  zone->getID(resolved_ustring);
  const std::string resolved = ToUtf8(resolved_ustring);
  if (resolved.empty()) {
    return false;
  }
  // ICU resolves anything it does not know to "Etc/Unknown" (or GMT). Treat
  // those as invalid unless they were explicitly requested.
  if (EqualsIgnoringCase(resolved, "Etc/Unknown")) {
    return false;
  }
  if (EqualsIgnoringCase(resolved, "GMT") &&
      !EqualsIgnoringCase(requested, "GMT")) {
    return false;
  }
  return true;
}

bool SetProcessTimeZone(std::string_view iana_id) {
  if (!IsValidTimeZoneId(iana_id)) {
    LOG(ERROR) << "[privacy_cef][env] timezone rejected id=" << iana_id;
    return false;
  }

  const std::string requested(iana_id);
  std::unique_ptr<icu::TimeZone> zone(
      icu::TimeZone::createTimeZone(icu::UnicodeString::fromUTF8(requested)));
  if (!zone) {
    LOG(ERROR) << "[privacy_cef][env] timezone create failed id=" << requested;
    return false;
  }
  icu::TimeZone::adoptDefault(zone.release());

#if BUILDFLAG(IS_POSIX)
  // base::Time::LocalExplode() and localtime_r() read TZ, not ICU.
  setenv("TZ", requested.c_str(), 1);
  tzset();
#endif

  return true;
}

std::string GetProcessTimeZoneId() {
  std::unique_ptr<icu::TimeZone> zone(icu::TimeZone::createDefault());
  if (!zone) {
    return std::string();
  }
  icu::UnicodeString id;
  zone->getID(id);
  return ToUtf8(id);
}

}  // namespace privacy_cef

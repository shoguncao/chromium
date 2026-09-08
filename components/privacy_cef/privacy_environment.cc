// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_environment.h"

#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "components/privacy_cef/privacy_timezone.h"

namespace privacy_cef {
namespace {

void SetError(std::string* error, const std::string& message) {
  if (error) {
    *error = message;
  }
}

// Accepts "en", "en-US", "zh-Hans-CN". Rejects empty and malformed values.
bool IsValidLocale(std::string_view locale) {
  if (locale.empty() || locale.size() > 64) {
    return false;
  }
  for (const std::string_view part : base::SplitStringPiece(
           locale, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
    if (part.empty() || part.size() > 8) {
      return false;
    }
    for (const char c : part) {
      if (!base::IsAsciiAlpha(c) && !base::IsAsciiDigit(c)) {
        return false;
      }
    }
  }
  return true;
}

std::string RegionFromLocale(std::string_view locale) {
  const size_t pos = locale.rfind('-');
  if (pos == std::string_view::npos) {
    return std::string();
  }
  const std::string region(locale.substr(pos + 1));
  // A 2 letter or 3 digit subtag is a region; a script such as "Hans" is not.
  if (region.size() == 2 ||
      (region.size() == 3 && base::IsAsciiDigit(region[0]))) {
    return base::ToUpperASCII(region);
  }
  return std::string();
}

}  // namespace

std::optional<IpEnvironment> IpEnvironment::Parse(const base::DictValue& root,
                                                  std::string* error) {
  const base::DictValue* environment = root.FindDict("ipEnvironment");
  if (!environment) {
    return IpEnvironment();
  }

  IpEnvironment result;
  result.configured = true;

  const std::string* timezone = environment->FindString("timezone");
  if (!timezone || timezone->empty()) {
    SetError(error, "ipEnvironment.timezone is required");
    return std::nullopt;
  }
  if (!IsValidTimeZoneId(*timezone)) {
    SetError(error,
             "ipEnvironment.timezone is not a valid IANA ID: " + *timezone);
    return std::nullopt;
  }
  result.timezone = *timezone;

  const std::string* locale = environment->FindString("locale");
  if (!locale || locale->empty()) {
    SetError(error, "ipEnvironment.locale is required");
    return std::nullopt;
  }
  if (!IsValidLocale(*locale)) {
    SetError(error, "ipEnvironment.locale is malformed: " + *locale);
    return std::nullopt;
  }
  result.locale = *locale;

  if (const base::ListValue* languages = environment->FindList("languages")) {
    for (const base::Value& item : *languages) {
      if (!item.is_string() || item.GetString().empty()) {
        SetError(error, "ipEnvironment.languages must only contain strings");
        return std::nullopt;
      }
      result.languages.push_back(item.GetString());
    }
    if (result.languages.empty()) {
      SetError(error, "ipEnvironment.languages must not be empty");
      return std::nullopt;
    }
    if (!base::EqualsCaseInsensitiveASCII(result.languages.front(),
                                          result.locale)) {
      SetError(error,
               "ipEnvironment.languages[0] must equal ipEnvironment.locale");
      return std::nullopt;
    }
  }

  if (const std::string* region = environment->FindString("region")) {
    result.region = *region;
  }

  if (const base::DictValue* geo = environment->FindDict("geo")) {
    const std::string* mode = geo->FindString("mode");
    if (!mode) {
      SetError(error, "ipEnvironment.geo.mode is required");
      return std::nullopt;
    }
    if (*mode == "spoof") {
      result.geo_mode = GeoMode::kSpoof;
      const std::optional<double> latitude = geo->FindDouble("latitude");
      const std::optional<double> longitude = geo->FindDouble("longitude");
      const std::optional<double> accuracy = geo->FindDouble("accuracy");
      if (!latitude || !longitude || !accuracy) {
        SetError(error,
                 "ipEnvironment.geo requires latitude, longitude and accuracy "
                 "in spoof mode");
        return std::nullopt;
      }
      if (!std::isfinite(*latitude) || *latitude < -90.0 || *latitude > 90.0) {
        SetError(error, "ipEnvironment.geo.latitude is out of range");
        return std::nullopt;
      }
      if (!std::isfinite(*longitude) || *longitude < -180.0 ||
          *longitude > 180.0) {
        SetError(error, "ipEnvironment.geo.longitude is out of range");
        return std::nullopt;
      }
      if (!std::isfinite(*accuracy) || *accuracy <= 0.0) {
        SetError(error, "ipEnvironment.geo.accuracy must be positive");
        return std::nullopt;
      }
      result.geo_latitude = *latitude;
      result.geo_longitude = *longitude;
      result.geo_accuracy = *accuracy;
    } else if (*mode == "block") {
      result.geo_mode = GeoMode::kBlock;
    } else {
      SetError(error, "ipEnvironment.geo.mode is invalid: " + *mode);
      return std::nullopt;
    }
  }

  if (const base::DictValue* webrtc = environment->FindDict("webrtc")) {
    const std::string* handling = webrtc->FindString("ipHandling");
    if (handling) {
      if (*handling == "default") {
        result.webrtc_ip_handling = WebrtcIpHandling::kDefault;
      } else if (*handling == "default_public_interface_only") {
        result.webrtc_ip_handling =
            WebrtcIpHandling::kDefaultPublicInterfaceOnly;
      } else if (*handling == "disable_non_proxied_udp") {
        result.webrtc_ip_handling = WebrtcIpHandling::kDisableNonProxiedUdp;
      } else {
        SetError(error,
                 "ipEnvironment.webrtc.ipHandling is invalid: " + *handling);
        return std::nullopt;
      }
    }
  }

  result.FillDefaults();
  return result;
}

void IpEnvironment::FillDefaults() {
  if (!configured) {
    return;
  }
  if (languages.empty()) {
    languages.push_back(locale);
  }
  if (region.empty()) {
    region = RegionFromLocale(locale);
  }
  if (webrtc_ip_handling == WebrtcIpHandling::kUnset) {
    // Fail closed: without this, ICE candidates reveal the real egress IP and
    // every other spoofed value becomes pointless.
    webrtc_ip_handling = WebrtcIpHandling::kDisableNonProxiedUdp;
  }
}

std::string IpEnvironment::languages_csv() const {
  std::string out;
  for (const std::string& language : languages) {
    if (!out.empty()) {
      out += ",";
    }
    out += language;
  }
  return out;
}

std::string IpEnvironment::accept_languages_header() const {
  // Mirrors the weighting Chromium derives from the accept languages pref.
  std::string out;
  double quality = 1.0;
  for (const std::string& language : languages) {
    if (!out.empty()) {
      out += ",";
    }
    if (quality >= 1.0) {
      out += language;
    } else {
      out += base::StringPrintf("%s;q=%.1f", language.c_str(), quality);
    }
    quality -= 0.1;
    if (quality < 0.1) {
      quality = 0.1;
    }
  }
  return out;
}

base::DictValue IpEnvironment::ToDict() const {
  base::DictValue dict;
  if (!configured) {
    return dict;
  }
  dict.Set("timezone", timezone);
  dict.Set("locale", locale);
  base::ListValue languages_value;
  for (const std::string& language : languages) {
    languages_value.Append(language);
  }
  dict.Set("languages", std::move(languages_value));
  if (!region.empty()) {
    dict.Set("region", region);
  }
  if (geo_mode != GeoMode::kUnset) {
    base::DictValue geo;
    geo.Set("mode", geo_mode == GeoMode::kSpoof ? "spoof" : "block");
    if (geo_mode == GeoMode::kSpoof) {
      geo.Set("latitude", geo_latitude);
      geo.Set("longitude", geo_longitude);
      geo.Set("accuracy", geo_accuracy);
    }
    dict.Set("geo", std::move(geo));
  }
  const std::string handling =
      ToWebRtcIpHandlingPolicyValue(webrtc_ip_handling);
  if (!handling.empty()) {
    base::DictValue webrtc;
    webrtc.Set("ipHandling", handling);
    dict.Set("webrtc", std::move(webrtc));
  }
  return dict;
}

bool SetProcessLocale(std::string_view locale) {
  if (locale.empty()) {
    LOG(ERROR) << "[privacy_cef][env] locale rejected (empty)";
    return false;
  }
  base::i18n::SetICUDefaultLocale(locale);
  return true;
}

namespace {

IpEnvironment& MutableProcessEnvironment() {
  static base::NoDestructor<IpEnvironment> environment;
  return *environment;
}

}  // namespace

const IpEnvironment& GetProcessEnvironment() {
  return MutableProcessEnvironment();
}

void SetProcessEnvironment(IpEnvironment environment) {
  // Written once by the browser process at startup; every consumer must then
  // observe the same egress identity for the lifetime of the process.
  MutableProcessEnvironment() = std::move(environment);
}

std::string ToWebRtcIpHandlingPolicyValue(WebrtcIpHandling handling) {
  switch (handling) {
    case WebrtcIpHandling::kDefault:
      return "default";
    case WebrtcIpHandling::kDefaultPublicInterfaceOnly:
      return "default_public_interface_only";
    case WebrtcIpHandling::kDisableNonProxiedUdp:
      return "disable_non_proxied_udp";
    case WebrtcIpHandling::kUnset:
      return std::string();
  }
  return std::string();
}

}  // namespace privacy_cef

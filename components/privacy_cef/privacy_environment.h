// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_ENVIRONMENT_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_ENVIRONMENT_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/values.h"
#include "components/privacy_cef/privacy_timezone.h"

namespace privacy_cef {

// How navigator.geolocation should behave.
enum class GeoMode {
  kUnset,  // No ipEnvironment configured: leave the platform behaviour alone.
  kSpoof,  // Report a fixed position tied to the egress IP.
  kBlock,  // Report kPositionUnavailable without touching the platform stack.
};

// Mirrors blink::mojom::WebRtcIpHandlingPolicy values used by CEF.
enum class WebrtcIpHandling {
  kUnset,
  kDefault,
  kDefaultPublicInterfaceOnly,
  // Strongest: no STUN, no host candidates, nothing that can reveal the real
  // egress IP unless it goes through the proxy.
  kDisableNonProxiedUdp,
};

// Everything that must agree with the egress IP. Values come from the privacy
// profile (|ipEnvironment|) and may be overridden by command line switches.
struct IpEnvironment {
  bool configured = false;

  // IANA time zone ID, e.g. "America/Los_Angeles". Never a fixed UTC offset:
  // daylight saving time must stay correct.
  std::string timezone;

  // Primary locale, e.g. "en-US". Drives Intl, toLocaleString() and the
  // primary entry of the Accept-Language header.
  std::string locale;
  std::vector<std::string> languages;
  std::string region;

  GeoMode geo_mode = GeoMode::kUnset;
  double geo_latitude = 0.0;
  double geo_longitude = 0.0;
  double geo_accuracy = 0.0;

  WebrtcIpHandling webrtc_ip_handling = WebrtcIpHandling::kUnset;

  // Parses the optional top-level "ipEnvironment" object.
  //   * absent  -> returns a default instance with |configured| == false
  //   * present but invalid -> returns nullopt and fills |error|
  static std::optional<IpEnvironment> Parse(const base::DictValue& root,
                                            std::string* error);

  // Fills derived defaults (languages from locale, region, WebRTC policy).
  void FillDefaults();

  // "en-US,en;q=0.9" - value of the Accept-Language header and of
  // RendererPreferences::accept_languages (navigator.language/languages).
  std::string accept_languages_header() const;

  // "en-US,en" - plain ordered list.
  std::string languages_csv() const;

  base::DictValue ToDict() const;
};

// Applies |locale| to the ICU default locale of the current process, which is
// what V8 uses for Intl/DateTimeFormat/NumberFormat. Returns false (and leaves
// the process untouched) when |locale| cannot be parsed by ICU.
bool SetProcessLocale(std::string_view locale);

// Process wide environment, resolved once by the browser process at startup
// and read by every place that has to apply it (browser prefs, WebContents
// renderer preferences, renderer process startup). Storing it here keeps CEF
// from depending on //content inside this component.
const IpEnvironment& GetProcessEnvironment();
void SetProcessEnvironment(IpEnvironment environment);

// Returns the blink::mojom::WebRtcIpHandlingPolicy wire value, e.g.
// "disable_non_proxied_udp". Empty when unset.
std::string ToWebRtcIpHandlingPolicyValue(WebrtcIpHandling handling);

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_PRIVACY_ENVIRONMENT_H_

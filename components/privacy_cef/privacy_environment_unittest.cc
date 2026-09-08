// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_environment.h"

#include <string>

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace privacy_cef {
namespace {

base::DictValue MakeEnvironment(const std::string& timezone,
                                const std::string& locale) {
  base::DictValue environment;
  environment.Set("timezone", timezone);
  environment.Set("locale", locale);
  return environment;
}

}  // namespace

TEST(PrivacyEnvironmentTest, AbsentEnvironmentIsUnconfigured) {
  base::DictValue root;
  std::string error;
  std::optional<IpEnvironment> environment = IpEnvironment::Parse(root, &error);
  ASSERT_TRUE(environment.has_value());
  EXPECT_FALSE(environment->configured);
  EXPECT_TRUE(error.empty());
}

TEST(PrivacyEnvironmentTest, ParsesValidEnvironmentAndFillsDefaults) {
  base::DictValue root;
  root.Set("ipEnvironment",
           MakeEnvironment("America/Los_Angeles", "en-US"));

  std::string error;
  std::optional<IpEnvironment> environment = IpEnvironment::Parse(root, &error);
  ASSERT_TRUE(environment.has_value()) << error;
  EXPECT_TRUE(environment->configured);
  EXPECT_EQ("America/Los_Angeles", environment->timezone);
  EXPECT_EQ("en-US", environment->locale);
  ASSERT_EQ(1u, environment->languages.size());
  EXPECT_EQ("en-US", environment->languages[0]);
  EXPECT_EQ("US", environment->region);
  EXPECT_EQ(WebrtcIpHandling::kDisableNonProxiedUdp,
            environment->webrtc_ip_handling);
  EXPECT_EQ("en-US", environment->accept_languages_header());
}

TEST(PrivacyEnvironmentTest, RejectsUnknownTimeZone) {
  base::DictValue root;
  root.Set("ipEnvironment", MakeEnvironment("Not/AZone", "en-US"));

  std::string error;
  std::optional<IpEnvironment> environment = IpEnvironment::Parse(root, &error);
  EXPECT_FALSE(environment.has_value());
  EXPECT_FALSE(error.empty());
}

TEST(PrivacyEnvironmentTest, RejectsMissingLocale) {
  base::DictValue root;
  base::DictValue environment;
  environment.Set("timezone", "America/Los_Angeles");
  root.Set("ipEnvironment", std::move(environment));

  std::string error;
  EXPECT_FALSE(IpEnvironment::Parse(root, &error).has_value());
  EXPECT_FALSE(error.empty());
}

TEST(PrivacyEnvironmentTest, RejectsLanguagesWithoutPrimaryLocale) {
  base::DictValue root;
  base::DictValue environment = MakeEnvironment("America/Los_Angeles", "en-US");
  base::ListValue languages;
  languages.Append("de-DE");
  languages.Append("en-US");
  environment.Set("languages", std::move(languages));
  root.Set("ipEnvironment", std::move(environment));

  std::string error;
  EXPECT_FALSE(IpEnvironment::Parse(root, &error).has_value());
  EXPECT_FALSE(error.empty());
}

TEST(PrivacyEnvironmentTest, RejectsSpoofGeoWithoutCoordinates) {
  base::DictValue root;
  base::DictValue environment = MakeEnvironment("America/Los_Angeles", "en-US");
  base::DictValue geo;
  geo.Set("mode", "spoof");
  environment.Set("geo", std::move(geo));
  root.Set("ipEnvironment", std::move(environment));

  std::string error;
  EXPECT_FALSE(IpEnvironment::Parse(root, &error).has_value());
  EXPECT_FALSE(error.empty());
}

TEST(PrivacyEnvironmentTest, RejectsOutOfRangeCoordinates) {
  base::DictValue root;
  base::DictValue environment = MakeEnvironment("America/Los_Angeles", "en-US");
  base::DictValue geo;
  geo.Set("mode", "spoof");
  geo.Set("latitude", 200.0);
  geo.Set("longitude", 0.0);
  geo.Set("accuracy", 50.0);
  environment.Set("geo", std::move(geo));
  root.Set("ipEnvironment", std::move(environment));

  std::string error;
  EXPECT_FALSE(IpEnvironment::Parse(root, &error).has_value());
}

TEST(PrivacyEnvironmentTest, AcceptLanguagesHeaderWeightsFallbacks) {
  IpEnvironment environment;
  environment.configured = true;
  environment.locale = "en-US";
  environment.languages = {"en-US", "en"};
  environment.FillDefaults();
  EXPECT_EQ("en-US,en;q=0.9", environment.accept_languages_header());
  EXPECT_EQ("en-US,en", environment.languages_csv());
}

TEST(PrivacyEnvironmentTest, RoundTripsThroughDict) {
  base::DictValue root;
  base::DictValue environment = MakeEnvironment("Europe/Berlin", "de-DE");
  base::DictValue geo;
  geo.Set("mode", "spoof");
  geo.Set("latitude", 52.52);
  geo.Set("longitude", 13.405);
  geo.Set("accuracy", 25.0);
  environment.Set("geo", std::move(geo));
  root.Set("ipEnvironment", std::move(environment));

  std::string error;
  std::optional<IpEnvironment> parsed = IpEnvironment::Parse(root, &error);
  ASSERT_TRUE(parsed.has_value()) << error;
  EXPECT_EQ(GeoMode::kSpoof, parsed->geo_mode);
  EXPECT_DOUBLE_EQ(52.52, parsed->geo_latitude);

  base::DictValue second;
  second.Set("ipEnvironment", parsed->ToDict());
  std::optional<IpEnvironment> reparsed =
      IpEnvironment::Parse(second, &error);
  ASSERT_TRUE(reparsed.has_value()) << error;
  EXPECT_EQ(parsed->timezone, reparsed->timezone);
  EXPECT_EQ(parsed->locale, reparsed->locale);
  EXPECT_EQ(parsed->languages, reparsed->languages);
  EXPECT_EQ(parsed->region, reparsed->region);
  EXPECT_EQ(parsed->geo_mode, reparsed->geo_mode);
  EXPECT_DOUBLE_EQ(parsed->geo_latitude, reparsed->geo_latitude);
  EXPECT_EQ(parsed->webrtc_ip_handling, reparsed->webrtc_ip_handling);
}

}  // namespace privacy_cef

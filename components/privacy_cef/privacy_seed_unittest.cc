// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_seed.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace privacy_cef {

TEST(PrivacySeedTest, StableForSameProfileSiteAndOperation) {
  PrivacyProfile profile;
  profile.master_seed.fill(0x2a);
  PrivacySeed first_site = DeriveSiteSeed(profile, "github.com");
  PrivacySeed second_site = DeriveSiteSeed(profile, "github.com");
  EXPECT_EQ(first_site, second_site);
  EXPECT_EQ(DeriveOperationSeed(first_site, "canvas", 1, "pixels-a"),
            DeriveOperationSeed(second_site, "canvas", 1, "pixels-a"));
}

TEST(PrivacySeedTest, SeparatesSitesInputsAndAlgorithmVersions) {
  PrivacyProfile profile;
  profile.master_seed.fill(0x2a);
  PrivacySeed github = DeriveSiteSeed(profile, "github.com");
  PrivacySeed example = DeriveSiteSeed(profile, "example.com");
  EXPECT_NE(github, example);
  EXPECT_NE(DeriveOperationSeed(github, "canvas", 1, "pixels-a"),
            DeriveOperationSeed(github, "canvas", 1, "pixels-b"));
  EXPECT_NE(DeriveOperationSeed(github, "canvas", 1, "pixels-a"),
            DeriveOperationSeed(github, "canvas", 2, "pixels-a"));
}

}  // namespace privacy_cef

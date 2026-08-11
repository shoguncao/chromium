// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_SEED_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_SEED_H_

#include <array>
#include <cstdint>
#include <string_view>

#include "components/privacy_cef/privacy_profile.h"

namespace privacy_cef {

using PrivacySeed = std::array<uint8_t, 32>;

PrivacySeed DeriveSiteSeed(const PrivacyProfile& profile,
                           std::string_view top_level_site);
PrivacySeed DeriveOperationSeed(const PrivacySeed& site_seed,
                                std::string_view algorithm,
                                int algorithm_version,
                                std::string_view input_digest);

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_PRIVACY_SEED_H_

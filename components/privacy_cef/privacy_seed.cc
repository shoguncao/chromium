// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_seed.h"

#include <string>

#include "base/containers/span.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "crypto/hmac.h"

namespace privacy_cef {

PrivacySeed DeriveSiteSeed(const PrivacyProfile& profile,
                           std::string_view top_level_site) {
  return crypto::hmac::SignSha256(profile.master_seed,
                                  base::as_byte_span(top_level_site));
}

PrivacySeed DeriveOperationSeed(const PrivacySeed& site_seed,
                                std::string_view algorithm,
                                int algorithm_version,
                                std::string_view input_digest) {
  const std::string context =
      base::StrCat({algorithm, "\n", base::NumberToString(algorithm_version),
                    "\n", input_digest});
  return crypto::hmac::SignSha256(site_seed, base::as_byte_span(context));
}

}  // namespace privacy_cef

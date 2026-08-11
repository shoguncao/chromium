// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef COMPONENTS_PRIVACY_CEF_CANVAS_FARBLING_H_
#define COMPONENTS_PRIVACY_CEF_CANVAS_FARBLING_H_

#include "base/containers/span.h"
#include "components/privacy_cef/privacy_seed.h"

namespace privacy_cef {

// Deterministically perturbs RGB least-significant bits in an RGBA pixel span.
// The algorithm is ported from BraveSessionCache::PerturbPixelsInternal at
// brave/brave-core commit 66867f5c43390a235672bfc3e091d02e84d6892c.
// Alpha bytes are never modified.
void FarbleCanvasPixels(const PrivacySeed& site_seed,
                        base::span<uint8_t> rgba_pixels);

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_CANVAS_FARBLING_H_

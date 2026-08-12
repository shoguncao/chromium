/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_PRIVACY_WEBGL_EXTENSION_HANDLER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_PRIVACY_WEBGL_EXTENSION_HANDLER_H_

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

struct PrivacyWebGLFakeExtension {
  String name;
  String script_object_name;
};

// Privacy CEF adaptation of Brave Core's WebGLFarbledExtensionHandler at
// commit 66867f5c43390a235672bfc3e091d02e84d6892c.
class MODULES_EXPORT PrivacyWebGLExtensionHandler {
 public:
  static const PrivacyWebGLFakeExtension& Get(size_t index);
  static bool IsFakeExtensionName(const String& name);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_PRIVACY_WEBGL_EXTENSION_HANDLER_H_

// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/navigator_device_memory.h"

#include "components/privacy_cef/privacy_runtime.h"
#include "third_party/blink/public/common/device_memory/approximated_device_memory.h"
#include "third_party/blink/public/mojom/use_counter/metrics/web_feature.mojom-shared.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/html/canvas/privacy_canvas_context.h"

namespace blink {

float NavigatorDeviceMemory::deviceMemory(ScriptState* script_state) const {
  ExecutionContext* context = ExecutionContext::From(script_state);
  const auto configured =
      privacy_cef::PrivacyRuntime::GetInstance().GetProfileMemoryGb();
  if (configured) {
    privacy_cef::PrivacyRuntime::GetInstance().RecordNavigatorHardwareAccess(
        PrivacyCanvasTopLevelSite(context),
        PrivacyExecutionAuditContext(context, "Navigator.deviceMemory"));
    return static_cast<float>(*configured);
  }
  return ApproximatedDeviceMemory::GetApproximatedDeviceMemory();
}

}  // namespace blink

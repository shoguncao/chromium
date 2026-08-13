// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/navigator_concurrent_hardware.h"

#include "base/system/sys_info.h"
#include "components/privacy_cef/privacy_runtime.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/html/canvas/privacy_canvas_context.h"

namespace blink {

unsigned NavigatorConcurrentHardware::hardwareConcurrency(
    ScriptState* script_state) const {
  ExecutionContext* context = ExecutionContext::From(script_state);
  const auto configured =
      privacy_cef::PrivacyRuntime::GetInstance().GetProfileCpuCores();
  if (configured) {
    privacy_cef::PrivacyRuntime::GetInstance().RecordNavigatorHardwareAccess(
        PrivacyCanvasTopLevelSite(context),
        PrivacyExecutionAuditContext(context, "Navigator.hardwareConcurrency"));
    return static_cast<unsigned>(*configured);
  }
  return static_cast<unsigned>(base::SysInfo::NumberOfProcessors());
}

}  // namespace blink

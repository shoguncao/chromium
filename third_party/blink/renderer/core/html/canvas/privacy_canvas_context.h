// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_PRIVACY_CANVAS_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_PRIVACY_CANVAS_CONTEXT_H_

#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/threading/platform_thread.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/network/blink_schemeful_site.h"
#include "third_party/blink/renderer/platform/storage/blink_storage_key.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {

// Brave derives a domain-scoped farbling token before Canvas data reaches
// Blink. Privacy CEF instead derives it locally from the persistent profile,
// so every Canvas export path must supply the same top-level schemeful site.
// This also keeps cross-origin frames and their dedicated workers in the
// top-level page's privacy partition.
inline std::string PrivacyCanvasTopLevelSite(ExecutionContext* context) {
  if (!context) {
    return {};
  }

  if (auto* window = DynamicTo<LocalDOMWindow>(context)) {
    return window->GetStorageKey().GetTopLevelSite().Serialize().Utf8();
  }

  if (auto* worker = DynamicTo<WorkerGlobalScope>(context)) {
    if (const SecurityOrigin* origin =
            worker->top_level_frame_security_origin()) {
      return BlinkSchemefulSite(origin->IsolatedCopy()).Serialize().Utf8();
    }
  }

  const SecurityOrigin* origin = context->GetSecurityOrigin();
  if (!origin || origin->IsOpaque()) {
    return {};
  }
  return BlinkSchemefulSite(origin->IsolatedCopy()).Serialize().Utf8();
}

inline privacy_cef::PrivacyAuditContext PrivacyExecutionAuditContext(
    ExecutionContext* context,
    std::string api) {
  privacy_cef::PrivacyAuditContext audit;
  audit.api = std::move(api);
  if (!context) {
    return audit;
  }
  audit.context_type =
      DynamicTo<WorkerGlobalScope>(context) ? "worker" : "frame";
  if (auto* window = DynamicTo<LocalDOMWindow>(context)) {
    if (LocalFrame* frame = window->GetFrame()) {
      audit.frame_id = frame->GetDevToolsFrameToken().ToString();
    }
  } else if (DynamicTo<WorkerGlobalScope>(context)) {
    audit.worker_id =
        base::NumberToString(base::PlatformThread::CurrentId().raw());
  }
  if (const SecurityOrigin* origin = context->GetSecurityOrigin()) {
    audit.frame_origin = origin->ToString().Utf8();
  }
  return audit;
}

inline privacy_cef::PrivacyAuditContext PrivacyCanvasAuditContext(
    ExecutionContext* context,
    std::string api) {
  return PrivacyExecutionAuditContext(context, std::move(api));
}

inline privacy_cef::PrivacyAuditContext PrivacyWebGlAuditContext(
    ExecutionContext* context,
    std::string api) {
  return PrivacyExecutionAuditContext(context, std::move(api));
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_PRIVACY_CANVAS_CONTEXT_H_

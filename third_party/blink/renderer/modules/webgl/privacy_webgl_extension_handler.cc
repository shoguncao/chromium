/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/webgl/privacy_webgl_extension_handler.h"

#include <array>

#include "base/no_destructor.h"
#include "third_party/blink/renderer/platform/wtf/text/ascii_ctype.h"

namespace blink {
namespace {

constexpr size_t kFakeExtensionsSize = 21;

const std::array<PrivacyWebGLFakeExtension, kFakeExtensionsSize>&
GetFakeExtensions() {
  // Kept byte-for-byte equivalent to Brave Core's fake extension table at
  // commit 66867f5c43390a235672bfc3e091d02e84d6892c.
  static const base::NoDestructor<
      std::array<PrivacyWebGLFakeExtension, kFakeExtensionsSize>>
      fake_values({{
          {"EXT_texture_sampler", "ExtTextureSampler"},
          {"EXT_texture_compressor", "ExtTextureCompressor"},
          {"EXT_texture_blender", "ExtTextureBlender"},
          {"EXT_expanded_sampler", "ExtExpandedSampler"},
          {"EXT_expanded_compressor", "ExtExpandedCompressor"},
          {"EXT_expanded_blender", "ExtExpandedBlender"},
          {"EXT_polygon_sampler", "ExtPolygonSampler"},
          {"EXT_polygon_compressor", "ExtPolygonCompressor"},
          {"EXT_polygon_blender", "ExtPolygonBlender"},
          {"EXT_circle_sampler", "ExtCircleSampler"},
          {"EXT_circle_compressor", "ExtCircleCompressor"},
          {"EXT_circle_blender", "ExtCircleBlender"},
          {"EXT_triangle_sampler", "ExtTriangleSampler"},
          {"EXT_triangle_compressor", "ExtTriangleCompressor"},
          {"EXT_triangle_blender", "ExtTriangleBlender"},
          {"EXT_blend_sampler", "ExtBlendSampler"},
          {"EXT_blend_compressor", "ExtBlendCompressor"},
          {"EXT_blend_blender", "ExtBlendBlender"},
          {"EXT_draw_sampler", "ExtDrawSampler"},
          {"EXT_draw_compressor", "ExtDrawCompressor"},
          {"EXT_draw_blender", "ExtDrawBlender"},
      }});
  return *fake_values;
}

}  // namespace

const PrivacyWebGLFakeExtension& PrivacyWebGLExtensionHandler::Get(
    size_t index) {
  const auto& extensions = GetFakeExtensions();
  return extensions[index % extensions.size()];
}

bool PrivacyWebGLExtensionHandler::IsFakeExtensionName(const String& name) {
  for (const auto& extension : GetFakeExtensions()) {
    if (EqualIgnoringAsciiCase(extension.name, name)) {
      return true;
    }
  }
  return false;
}

}  // namespace blink

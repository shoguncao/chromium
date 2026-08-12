// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRIVACY_CEF_PRIVACY_PROFILE_H_
#define COMPONENTS_PRIVACY_CEF_PRIVACY_PROFILE_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"

namespace privacy_cef {

enum class AuditMode { kOff, kSummary, kFull };
enum class CanvasMode { kOff, kFarble, kBlock };
enum class WebGlMode { kOff, kStandardize, kStandardizeAndFarble, kBlock };
enum class AudioMode { kOff, kFarble, kBlock };
enum class FontsMode { kOff, kStandardize };
enum class GeometryMode { kOff, kEnvironmentOnly };
enum class StorageMode { kOff, kBucket };
enum class SpeechMode { kOff, kStandardize, kBlock };
enum class WebRtcMode { kOff, kNoLocalIp, kBlock };
enum class WebGpuMode { kOff, kStandardize, kDisabled };

struct PrivacyProfile {
  static constexpr int kSupportedSchemaVersion = 1;
  static constexpr int kMasterSeedBytes = 32;

  PrivacyProfile();
  PrivacyProfile(const PrivacyProfile&);
  PrivacyProfile& operator=(const PrivacyProfile&);
  PrivacyProfile(PrivacyProfile&&);
  PrivacyProfile& operator=(PrivacyProfile&&);
  ~PrivacyProfile();

  int schema_version = 0;
  std::string profile_id;
  std::string display_name;
  std::array<uint8_t, kMasterSeedBytes> master_seed{};
  std::string preset;
  std::string language;
  std::vector<std::string> languages;
  std::string timezone;
  int cpu_cores = 0;
  int memory_gb = 0;
  int screen_width = 0;
  int screen_height = 0;
  int device_scale_factor = 0;
  int color_depth = 0;
  std::string color_gamut;
  AuditMode audit_mode = AuditMode::kOff;
  int audit_retention_days = 0;
  int audit_max_file_size_mb = 0;
  CanvasMode canvas_mode = CanvasMode::kOff;
  std::string canvas_algorithm;
  int canvas_algorithm_version = 0;
  WebGlMode webgl_mode = WebGlMode::kOff;
  AudioMode audio_mode = AudioMode::kOff;
  FontsMode fonts_mode = FontsMode::kOff;
  GeometryMode geometry_mode = GeometryMode::kOff;
  StorageMode storage_mode = StorageMode::kOff;
  SpeechMode speech_mode = SpeechMode::kOff;
  WebRtcMode webrtc_mode = WebRtcMode::kOff;
  WebGpuMode webgpu_mode = WebGpuMode::kOff;

  static std::optional<PrivacyProfile> LoadFromFile(const base::FilePath& path,
                                                    std::string* error);
  static std::optional<PrivacyProfile> Parse(std::string_view json,
                                             std::string* error);

  // Canonical Browser-to-Renderer IPC payload. It contains the master seed
  // and must never be logged or exposed to page JavaScript.
  std::string SerializeForRenderer() const;
};

}  // namespace privacy_cef

#endif  // COMPONENTS_PRIVACY_CEF_PRIVACY_PROFILE_H_

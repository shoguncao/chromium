// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_profile.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "components/privacy_cef/privacy_environment.h"

namespace privacy_cef {
namespace {

bool Fail(std::string_view message, std::string* error) {
  if (error) {
    *error = message;
  }
  return false;
}

const base::DictValue* RequiredDict(const base::DictValue& parent,
                                    std::string_view key,
                                    std::string* error) {
  const base::DictValue* value = parent.FindDict(key);
  if (!value) {
    Fail(std::string("missing object: ") + std::string(key), error);
  }
  return value;
}

const std::string* RequiredString(const base::DictValue& parent,
                                  std::string_view key,
                                  std::string* error) {
  const std::string* value = parent.FindString(key);
  if (!value || value->empty()) {
    Fail(std::string("missing string: ") + std::string(key), error);
    return nullptr;
  }
  return value;
}

std::optional<int> RequiredInt(const base::DictValue& parent,
                               std::string_view key,
                               std::string* error) {
  std::optional<int> value = parent.FindInt(key);
  if (!value) {
    Fail(std::string("missing integer: ") + std::string(key), error);
  }
  return value;
}

std::string_view ToString(WebGlMode mode) {
  switch (mode) {
    case WebGlMode::kOff:
      return "off";
    case WebGlMode::kStandardize:
      return "standardize";
    case WebGlMode::kStandardizeAndFarble:
      return "standardize-and-farble";
    case WebGlMode::kBlock:
      return "block";
  }
}

std::string_view ToString(AudioMode mode) {
  switch (mode) {
    case AudioMode::kOff:
      return "off";
    case AudioMode::kFarble:
      return "farble";
    case AudioMode::kBlock:
      return "block";
  }
}

std::string_view ToString(FontsMode mode) {
  switch (mode) {
    case FontsMode::kOff:
      return "off";
    case FontsMode::kStandardize:
      return "standardize";
  }
}

std::string_view ToString(GeometryMode mode) {
  switch (mode) {
    case GeometryMode::kOff:
      return "off";
    case GeometryMode::kEnvironmentOnly:
      return "environment-only";
  }
}

std::string_view ToString(StorageMode mode) {
  switch (mode) {
    case StorageMode::kOff:
      return "off";
    case StorageMode::kBucket:
      return "bucket";
  }
}

std::string_view ToString(SpeechMode mode) {
  switch (mode) {
    case SpeechMode::kOff:
      return "off";
    case SpeechMode::kStandardize:
      return "standardize";
    case SpeechMode::kBlock:
      return "block";
  }
}

std::string_view ToString(WebRtcMode mode) {
  switch (mode) {
    case WebRtcMode::kOff:
      return "off";
    case WebRtcMode::kNoLocalIp:
      return "no-local-ip";
    case WebRtcMode::kBlock:
      return "block";
  }
}

std::string_view ToString(WebGpuMode mode) {
  switch (mode) {
    case WebGpuMode::kOff:
      return "off";
    case WebGpuMode::kStandardize:
      return "standardize";
    case WebGpuMode::kDisabled:
      return "disabled";
  }
}

}  // namespace

PrivacyProfile::PrivacyProfile() = default;
PrivacyProfile::PrivacyProfile(const PrivacyProfile&) = default;
PrivacyProfile& PrivacyProfile::operator=(const PrivacyProfile&) = default;
PrivacyProfile::PrivacyProfile(PrivacyProfile&&) = default;
PrivacyProfile& PrivacyProfile::operator=(PrivacyProfile&&) = default;
PrivacyProfile::~PrivacyProfile() = default;

std::string PrivacyProfile::SerializeForRenderer() const {
  base::DictValue locale;
  locale.Set("language", language);
  base::ListValue language_list;
  for (const std::string& item : languages) {
    language_list.Append(item);
  }
  locale.Set("languages", std::move(language_list));
  locale.Set("timezone", timezone);

  base::DictValue hardware;
  hardware.Set("cpuCores", cpu_cores);
  hardware.Set("physicalMemoryGB", physical_memory_gb);
  hardware.Set("navigatorDeviceMemoryGB", navigator_device_memory_gb);

  base::DictValue display;
  display.Set("width", screen_width);
  display.Set("height", screen_height);
  display.Set("deviceScaleFactor", device_scale_factor);
  display.Set("colorDepth", color_depth);
  display.Set("colorGamut", color_gamut);

  std::string audit_mode_name;
  switch (this->audit_mode) {
    case AuditMode::kOff:
      audit_mode_name = "off";
      break;
    case AuditMode::kSummary:
      audit_mode_name = "summary";
      break;
    case AuditMode::kFull:
      audit_mode_name = "full";
      break;
  }
  base::DictValue audit;
  audit.Set("mode", audit_mode_name);
  audit.Set("retentionDays", audit_retention_days);
  audit.Set("maxFileSizeMB", audit_max_file_size_mb);

  std::string canvas_mode_name;
  switch (this->canvas_mode) {
    case CanvasMode::kOff:
      canvas_mode_name = "off";
      break;
    case CanvasMode::kFarble:
      canvas_mode_name = "farble";
      break;
    case CanvasMode::kBlock:
      canvas_mode_name = "block";
      break;
  }
  base::DictValue canvas;
  canvas.Set("mode", canvas_mode_name);
  canvas.Set("algorithm", canvas_algorithm);
  canvas.Set("algorithmVersion", canvas_algorithm_version);
  base::DictValue protections;
  protections.Set("canvas", std::move(canvas));
  protections.Set("webgl", ToString(webgl_mode));
  protections.Set("audio", ToString(audio_mode));
  protections.Set("fonts", ToString(fonts_mode));
  protections.Set("geometry", ToString(geometry_mode));
  protections.Set("storage", ToString(storage_mode));
  protections.Set("speech", ToString(speech_mode));
  protections.Set("webrtc", ToString(webrtc_mode));
  protections.Set("webgpu", ToString(webgpu_mode));
  protections.Set("navigator",
                  navigator_mode == NavigatorMode::kStandardize
                      ? "standardize"
                      : "off");

  base::DictValue identity;
  identity.Set("mode", identity_mode);
  base::DictValue network_fingerprint;
  network_fingerprint.Set("mode", network_fingerprint_mode);

  base::DictValue root;
  root.Set("schemaVersion", schema_version);
  root.Set("profileId", profile_id);
  root.Set("displayName", display_name);
  root.Set("masterSeed", base::Base64Encode(master_seed));
  root.Set("preset", preset);
  root.Set("identity", std::move(identity));
  root.Set("networkFingerprint", std::move(network_fingerprint));
  root.Set("locale", std::move(locale));
  if (ip_environment.configured) {
    root.Set("ipEnvironment", ip_environment.ToDict());
  }
  root.Set("hardware", std::move(hardware));
  root.Set("display", std::move(display));
  root.Set("audit", std::move(audit));
  root.Set("protections", std::move(protections));

  std::string json;
  CHECK(base::JSONWriter::Write(root, &json));
  return json;
}

std::optional<PrivacyProfile> PrivacyProfile::LoadFromFile(
    const base::FilePath& path,
    std::string* error) {
  std::string json;
  if (!base::ReadFileToString(path, &json)) {
    Fail("unable to read profile file", error);
    return std::nullopt;
  }
  return Parse(json, error);
}

std::optional<PrivacyProfile> PrivacyProfile::Parse(std::string_view json,
                                                    std::string* error) {
  std::optional<base::DictValue> root =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  if (!root) {
    Fail("invalid JSON", error);
    return std::nullopt;
  }

  PrivacyProfile profile;
  std::optional<int> schema_version =
      RequiredInt(*root, "schemaVersion", error);
  const std::string* profile_id = RequiredString(*root, "profileId", error);
  const std::string* display_name = RequiredString(*root, "displayName", error);
  const std::string* encoded_seed = RequiredString(*root, "masterSeed", error);
  const std::string* preset = RequiredString(*root, "preset", error);
  if (!schema_version || !profile_id || !display_name || !encoded_seed ||
      !preset) {
    return std::nullopt;
  }
  if (*schema_version != kSupportedSchemaVersion) {
    Fail("unsupported schemaVersion", error);
    return std::nullopt;
  }
  std::optional<std::vector<uint8_t>> decoded_seed =
      base::Base64Decode(*encoded_seed);
  if (!decoded_seed || decoded_seed->size() != kMasterSeedBytes) {
    Fail("masterSeed must contain exactly 32 bytes", error);
    return std::nullopt;
  }

  const base::DictValue* locale = RequiredDict(*root, "locale", error);
  const base::DictValue* identity = RequiredDict(*root, "identity", error);
  const base::DictValue* network_fingerprint =
      RequiredDict(*root, "networkFingerprint", error);
  const base::DictValue* hardware = RequiredDict(*root, "hardware", error);
  const base::DictValue* display = RequiredDict(*root, "display", error);
  const base::DictValue* audit = RequiredDict(*root, "audit", error);
  const base::DictValue* protections =
      RequiredDict(*root, "protections", error);
  if (!locale || !identity || !network_fingerprint || !hardware || !display ||
      !audit || !protections) {
    return std::nullopt;
  }
  const base::DictValue* canvas = RequiredDict(*protections, "canvas", error);
  if (!canvas) {
    return std::nullopt;
  }

  const std::string* language = RequiredString(*locale, "language", error);
  const std::string* identity_mode = RequiredString(*identity, "mode", error);
  const std::string* network_fingerprint_mode =
      RequiredString(*network_fingerprint, "mode", error);
  const base::ListValue* languages = locale->FindList("languages");
  const std::string* timezone = RequiredString(*locale, "timezone", error);
  std::optional<int> cpu_cores = RequiredInt(*hardware, "cpuCores", error);
  std::optional<int> physical_memory_gb =
      RequiredInt(*hardware, "physicalMemoryGB", error);
  std::optional<int> navigator_device_memory_gb =
      RequiredInt(*hardware, "navigatorDeviceMemoryGB", error);
  std::optional<int> width = RequiredInt(*display, "width", error);
  std::optional<int> height = RequiredInt(*display, "height", error);
  std::optional<int> scale = RequiredInt(*display, "deviceScaleFactor", error);
  std::optional<int> depth = RequiredInt(*display, "colorDepth", error);
  const std::string* gamut = RequiredString(*display, "colorGamut", error);
  const std::string* audit_mode = RequiredString(*audit, "mode", error);
  std::optional<int> retention = RequiredInt(*audit, "retentionDays", error);
  std::optional<int> max_size = RequiredInt(*audit, "maxFileSizeMB", error);
  const std::string* canvas_mode = RequiredString(*canvas, "mode", error);
  const std::string* canvas_algorithm =
      RequiredString(*canvas, "algorithm", error);
  std::optional<int> canvas_version =
      RequiredInt(*canvas, "algorithmVersion", error);
  const std::string* webgl_mode = RequiredString(*protections, "webgl", error);
  const std::string* audio_mode = RequiredString(*protections, "audio", error);
  const std::string* fonts_mode = RequiredString(*protections, "fonts", error);
  const std::string* geometry_mode =
      RequiredString(*protections, "geometry", error);
  const std::string* storage_mode =
      RequiredString(*protections, "storage", error);
  const std::string* speech_mode =
      RequiredString(*protections, "speech", error);
  const std::string* webrtc_mode =
      RequiredString(*protections, "webrtc", error);
  const std::string* webgpu_mode =
      RequiredString(*protections, "webgpu", error);
  const std::string* navigator_mode =
      RequiredString(*protections, "navigator", error);
  if (!language || !identity_mode || !network_fingerprint_mode || !languages ||
      languages->empty() || !timezone ||
      !cpu_cores || !physical_memory_gb || !navigator_device_memory_gb ||
      !width || !height || !scale || !depth ||
      !gamut || !audit_mode || !retention || !max_size || !canvas_mode ||
      !canvas_algorithm || !canvas_version || !webgl_mode || !audio_mode ||
      !fonts_mode || !geometry_mode || !storage_mode || !speech_mode ||
      !webrtc_mode || !webgpu_mode || !navigator_mode) {
    return std::nullopt;
  }

  constexpr int kCpuCoreBuckets[] = {8, 10, 12, 16};
  constexpr int kPhysicalMemoryGbBuckets[] = {8, 16, 32};
  constexpr int kNavigatorMemoryGbBuckets[] = {8, 16, 32};
  if (std::ranges::find(kCpuCoreBuckets, *cpu_cores) ==
          std::ranges::end(kCpuCoreBuckets) ||
      std::ranges::find(kPhysicalMemoryGbBuckets, *physical_memory_gb) ==
          std::ranges::end(kPhysicalMemoryGbBuckets) ||
      std::ranges::find(kNavigatorMemoryGbBuckets,
                        *navigator_device_memory_gb) ==
          std::ranges::end(kNavigatorMemoryGbBuckets) ||
      *width <= 0 || *height <= 0 || (*scale != 1 && *scale != 2) ||
      *retention < 1 || *retention > 30 || *max_size < 1 || *max_size > 100) {
    Fail("profile contains an out-of-range numeric value", error);
    return std::nullopt;
  }

  if (*audit_mode == "off") {
    profile.audit_mode = AuditMode::kOff;
  } else if (*audit_mode == "summary") {
    profile.audit_mode = AuditMode::kSummary;
  } else if (*audit_mode == "full") {
    profile.audit_mode = AuditMode::kFull;
  } else {
    Fail("unsupported audit mode", error);
    return std::nullopt;
  }
  if (*canvas_mode == "off") {
    profile.canvas_mode = CanvasMode::kOff;
  } else if (*canvas_mode == "farble") {
    profile.canvas_mode = CanvasMode::kFarble;
  } else if (*canvas_mode == "block") {
    profile.canvas_mode = CanvasMode::kBlock;
  } else {
    Fail("unsupported canvas mode", error);
    return std::nullopt;
  }
  if (profile.canvas_mode == CanvasMode::kFarble &&
      (*canvas_algorithm != "brave-derived" || *canvas_version != 1)) {
    Fail("unsupported Canvas farbling algorithm", error);
    return std::nullopt;
  }
  if (*webgl_mode == "off") {
    profile.webgl_mode = WebGlMode::kOff;
  } else if (*webgl_mode == "standardize") {
    profile.webgl_mode = WebGlMode::kStandardize;
  } else if (*webgl_mode == "standardize-and-farble") {
    profile.webgl_mode = WebGlMode::kStandardizeAndFarble;
  } else if (*webgl_mode == "block") {
    profile.webgl_mode = WebGlMode::kBlock;
  } else {
    Fail("unsupported WebGL mode", error);
    return std::nullopt;
  }
  if (*audio_mode == "off") {
    profile.audio_mode = AudioMode::kOff;
  } else if (*audio_mode == "farble") {
    profile.audio_mode = AudioMode::kFarble;
  } else if (*audio_mode == "block") {
    profile.audio_mode = AudioMode::kBlock;
  } else {
    Fail("unsupported Web Audio mode", error);
    return std::nullopt;
  }
  if (*fonts_mode == "off") {
    profile.fonts_mode = FontsMode::kOff;
  } else if (*fonts_mode == "standardize") {
    profile.fonts_mode = FontsMode::kStandardize;
  } else {
    Fail("unsupported Fonts mode", error);
    return std::nullopt;
  }
  if (*geometry_mode == "off") {
    profile.geometry_mode = GeometryMode::kOff;
  } else if (*geometry_mode == "environment-only") {
    profile.geometry_mode = GeometryMode::kEnvironmentOnly;
  } else {
    Fail("unsupported Geometry mode", error);
    return std::nullopt;
  }
  if (*storage_mode == "off") {
    profile.storage_mode = StorageMode::kOff;
  } else if (*storage_mode == "bucket") {
    profile.storage_mode = StorageMode::kBucket;
  } else {
    Fail("unsupported Storage mode", error);
    return std::nullopt;
  }
  if (*speech_mode == "off") {
    profile.speech_mode = SpeechMode::kOff;
  } else if (*speech_mode == "standardize") {
    profile.speech_mode = SpeechMode::kStandardize;
  } else if (*speech_mode == "block") {
    profile.speech_mode = SpeechMode::kBlock;
  } else {
    Fail("unsupported Speech mode", error);
    return std::nullopt;
  }
  if (*webrtc_mode == "off") {
    profile.webrtc_mode = WebRtcMode::kOff;
  } else if (*webrtc_mode == "no-local-ip") {
    profile.webrtc_mode = WebRtcMode::kNoLocalIp;
  } else if (*webrtc_mode == "block") {
    profile.webrtc_mode = WebRtcMode::kBlock;
  } else {
    Fail("unsupported WebRTC mode", error);
    return std::nullopt;
  }
  if (*webgpu_mode == "off") {
    profile.webgpu_mode = WebGpuMode::kOff;
  } else if (*webgpu_mode == "standardize") {
    profile.webgpu_mode = WebGpuMode::kStandardize;
  } else if (*webgpu_mode == "disabled") {
    profile.webgpu_mode = WebGpuMode::kDisabled;
  } else {
    Fail("unsupported WebGPU mode", error);
    return std::nullopt;
  }
  if (*navigator_mode == "off") {
    profile.navigator_mode = NavigatorMode::kOff;
  } else if (*navigator_mode == "standardize") {
    profile.navigator_mode = NavigatorMode::kStandardize;
  } else {
    Fail("unsupported Navigator mode", error);
    return std::nullopt;
  }
  if (*identity_mode != "engine-consistent" ||
      *network_fingerprint_mode != "chromium-sdk-default") {
    Fail("unsupported identity or network fingerprint mode", error);
    return std::nullopt;
  }

  profile.schema_version = *schema_version;
  profile.profile_id = *profile_id;
  profile.display_name = *display_name;
  std::ranges::copy(*decoded_seed, profile.master_seed.begin());
  profile.preset = *preset;
  profile.identity_mode = *identity_mode;
  profile.network_fingerprint_mode = *network_fingerprint_mode;
  profile.language = *language;
  for (const base::Value& item : *languages) {
    if (!item.is_string() || item.GetString().empty()) {
      Fail("languages must only contain non-empty strings", error);
      return std::nullopt;
    }
    profile.languages.push_back(item.GetString());
  }
  if (profile.languages.front() != profile.language) {
    Fail("primary language must be first in languages", error);
    return std::nullopt;
  }
  // Parsed last so that a malformed ipEnvironment rejects the whole profile
  // instead of silently falling back to platform values.
  std::optional<IpEnvironment> ip_environment =
      IpEnvironment::Parse(*root, error);
  if (!ip_environment) {
    return std::nullopt;
  }
  profile.ip_environment = *ip_environment;
  profile.timezone = *timezone;
  profile.cpu_cores = *cpu_cores;
  profile.physical_memory_gb = *physical_memory_gb;
  profile.navigator_device_memory_gb = *navigator_device_memory_gb;
  profile.screen_width = *width;
  profile.screen_height = *height;
  profile.device_scale_factor = *scale;
  profile.color_depth = *depth;
  profile.color_gamut = *gamut;
  profile.audit_retention_days = *retention;
  profile.audit_max_file_size_mb = *max_size;
  profile.canvas_algorithm = *canvas_algorithm;
  profile.canvas_algorithm_version = *canvas_version;
  return profile;
}

}  // namespace privacy_cef

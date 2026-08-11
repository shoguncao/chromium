// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/privacy_cef/privacy_profile.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/values.h"

namespace privacy_cef {
namespace {

bool Fail(std::string_view message, std::string* error) {
  if (error) {
    *error = message;
  }
  return false;
}

const base::Value::Dict* RequiredDict(const base::Value::Dict& parent,
                                      std::string_view key,
                                      std::string* error) {
  const base::Value::Dict* value = parent.FindDict(key);
  if (!value) {
    Fail(std::string("missing object: ") + std::string(key), error);
  }
  return value;
}

const std::string* RequiredString(const base::Value::Dict& parent,
                                  std::string_view key,
                                  std::string* error) {
  const std::string* value = parent.FindString(key);
  if (!value || value->empty()) {
    Fail(std::string("missing string: ") + std::string(key), error);
    return nullptr;
  }
  return value;
}

std::optional<int> RequiredInt(const base::Value::Dict& parent,
                               std::string_view key,
                               std::string* error) {
  std::optional<int> value = parent.FindInt(key);
  if (!value) {
    Fail(std::string("missing integer: ") + std::string(key), error);
  }
  return value;
}

}  // namespace

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
  std::optional<base::Value::Dict> root =
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

  const base::Value::Dict* locale = RequiredDict(*root, "locale", error);
  const base::Value::Dict* hardware = RequiredDict(*root, "hardware", error);
  const base::Value::Dict* display = RequiredDict(*root, "display", error);
  const base::Value::Dict* audit = RequiredDict(*root, "audit", error);
  const base::Value::Dict* protections =
      RequiredDict(*root, "protections", error);
  if (!locale || !hardware || !display || !audit || !protections) {
    return std::nullopt;
  }
  const base::Value::Dict* canvas = RequiredDict(*protections, "canvas", error);
  if (!canvas) {
    return std::nullopt;
  }

  const std::string* language = RequiredString(*locale, "language", error);
  const base::Value::List* languages = locale->FindList("languages");
  const std::string* timezone = RequiredString(*locale, "timezone", error);
  std::optional<int> cpu_cores = RequiredInt(*hardware, "cpuCores", error);
  std::optional<int> memory_gb = RequiredInt(*hardware, "memoryGB", error);
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
  if (!language || !languages || languages->empty() || !timezone ||
      !cpu_cores || !memory_gb || !width || !height || !scale || !depth ||
      !gamut || !audit_mode || !retention || !max_size || !canvas_mode ||
      !canvas_algorithm || !canvas_version) {
    return std::nullopt;
  }

  if (*cpu_cores <= 0 || *memory_gb <= 0 || *width <= 0 || *height <= 0 ||
      (*scale != 1 && *scale != 2) || *retention < 1 || *retention > 30 ||
      *max_size < 1 || *max_size > 100) {
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

  profile.schema_version = *schema_version;
  profile.profile_id = *profile_id;
  profile.display_name = *display_name;
  std::ranges::copy(*decoded_seed, profile.master_seed.begin());
  profile.preset = *preset;
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
  profile.timezone = *timezone;
  profile.cpu_cores = *cpu_cores;
  profile.memory_gb = *memory_gb;
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

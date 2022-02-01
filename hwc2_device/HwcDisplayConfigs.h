/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HWC2_DEVICE_HWC_DISPLAY_CONFIGS_H
#define ANDROID_HWC2_DEVICE_HWC_DISPLAY_CONFIGS_H

#include <hardware/hwcomposer2.h>

#include <map>

#include "drm/DrmMode.h"

namespace android {

class DrmConnector;

struct HwcDisplayConfig {
  int id{};
  int group_id{};
  DrmMode mode;
  bool disabled{};

  bool IsInterlaced() const {
    return (mode.flags() & DRM_MODE_FLAG_INTERLACE) != 0;
  }
};

struct HwcDisplayConfigs {
  HWC2::Error Update(DrmConnector &conn);

  std::map<int /*config_id*/, struct HwcDisplayConfig> hwc_configs;

  int active_config_id = 0;
  int preferred_config_id = 0;

  int last_config_id = 1;

  uint32_t mm_width = 0;
  uint32_t mm_height = 0;
};

}  // namespace android

#endif
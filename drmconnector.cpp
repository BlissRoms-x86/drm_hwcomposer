/*
 * Copyright (C) 2015 The Android Open Source Project
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

#define LOG_TAG "hwc-drm-connector"

#include "drmconnector.h"
#include "drmresources.h"

#include <errno.h>
#include <stdint.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <xf86drmMode.h>

namespace android {

DrmConnector::DrmConnector(DrmResources *drm, drmModeConnectorPtr c,
                           DrmEncoder *current_encoder,
                           std::vector<DrmEncoder *> &possible_encoders)
    : drm_(drm),
      id_(c->connector_id),
      encoder_(current_encoder),
      display_(-1),
      type_(c->connector_type),
      state_(c->connection),
      mm_width_(c->mmWidth),
      mm_height_(c->mmHeight),
      possible_encoders_(possible_encoders) {
}

int DrmConnector::Init() {
  int ret = drm_->GetConnectorProperty(*this, "DPMS", &dpms_property_);
  if (ret) {
    ALOGE("Could not get DPMS property\n");
    return ret;
  }
  ret = drm_->GetConnectorProperty(*this, "CRTC_ID", &crtc_id_property_);
  if (ret) {
    ALOGE("Could not get CRTC_ID property\n");
    return ret;
  }
  return 0;
}

uint32_t DrmConnector::id() const {
  return id_;
}

int DrmConnector::display() const {
  return display_;
}

void DrmConnector::set_display(int display) {
  display_ = display;
}

bool DrmConnector::internal() const {
  return type_ == DRM_MODE_CONNECTOR_LVDS || type_ == DRM_MODE_CONNECTOR_eDP ||
         type_ == DRM_MODE_CONNECTOR_DSI || type_ == DRM_MODE_CONNECTOR_VIRTUAL;
}

bool DrmConnector::external() const {
  return type_ == DRM_MODE_CONNECTOR_HDMIA || type_ == DRM_MODE_CONNECTOR_DisplayPort ||
         type_ == DRM_MODE_CONNECTOR_DVID || type_ == DRM_MODE_CONNECTOR_DVII ||
         type_ == DRM_MODE_CONNECTOR_VGA;
}

bool DrmConnector::valid_type() const {
  return internal() || external();
}

int DrmConnector::UpdateModes() {
  char value[PROPERTY_VALUE_MAX];
  uint32_t xres = 0, yres = 0, rate = 0;
  if (property_get("debug.drm.mode.force", value, NULL)) {
    // parse <xres>x<yres>[@<refreshrate>]
    if (sscanf(value, "%dx%d@%d", &xres, &yres, &rate) != 3) {
      rate = 0;
      if (sscanf(value, "%dx%d", &xres, &yres) != 2) {
        xres = yres = 0;
      }
    }
    ALOGI_IF(xres && yres, "force mode to %dx%d@%dHz", xres, yres, rate);
  }

  int fd = drm_->fd();

  drmModeConnectorPtr c = drmModeGetConnector(fd, id_);
  if (!c) {
    ALOGE("Failed to get connector %d", id_);
    return -ENODEV;
  }

  state_ = c->connection;

  std::vector<DrmMode> new_modes;
  for (int i = 0; i < c->count_modes; ++i) {
    bool exists = false;
    for (const DrmMode &mode : modes_) {
      if (mode == c->modes[i]) {
        new_modes.push_back(mode);
        exists = true;
        break;
      }
    }
    if (exists)
      continue;

    DrmMode m(&c->modes[i]);
    if (xres && yres) {
      if (m.h_display() != xres || m.v_display() != yres ||
            (rate && uint32_t(m.v_refresh()) != rate))
        continue;
    }
    m.set_id(drm_->next_mode_id());
    new_modes.push_back(m);
    ALOGD("add new mode %dx%d@%.1f id %d for display %d", m.h_display(), m.v_display(), m.v_refresh(), m.id(), display_);
  }
  modes_.swap(new_modes);
  return 0;
}

const DrmMode &DrmConnector::active_mode() const {
  return active_mode_;
}

void DrmConnector::set_active_mode(const DrmMode &mode) {
  active_mode_ = mode;
}

const DrmProperty &DrmConnector::dpms_property() const {
  return dpms_property_;
}

const DrmProperty &DrmConnector::crtc_id_property() const {
  return crtc_id_property_;
}

DrmEncoder *DrmConnector::encoder() const {
  return encoder_;
}

void DrmConnector::set_encoder(DrmEncoder *encoder) {
  encoder_ = encoder;
}

drmModeConnection DrmConnector::state() const {
  return state_;
}

uint32_t DrmConnector::mm_width() const {
  return mm_width_;
}

uint32_t DrmConnector::mm_height() const {
  return mm_height_;
}
}

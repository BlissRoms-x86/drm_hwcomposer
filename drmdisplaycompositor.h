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

#ifndef ANDROID_DRM_DISPLAY_COMPOSITOR_H_
#define ANDROID_DRM_DISPLAY_COMPOSITOR_H_

#include "drm_hwcomposer.h"
#include "drmcomposition.h"
#include "drmcompositorworker.h"
#include "drmframebuffer.h"
#include "seperate_rects.h"

#include <pthread.h>
#include <memory>
#include <queue>
#include <sstream>
#include <tuple>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

#define DRM_DISPLAY_BUFFERS 2

namespace android {

class GLWorkerCompositor;

class SquashState {
 public:
  static const unsigned kHistoryLength = 6;
  static const unsigned kMaxLayers = 64;

  struct Region {
    DrmHwcRect<int> rect;
    std::bitset<kMaxLayers> layer_refs;
    std::bitset<kHistoryLength> change_history;
    bool squashed = false;
  };

  bool is_stable(int region_index) const {
    return valid_history_ >= kHistoryLength &&
           regions_[region_index].change_history.none();
  }

  const std::vector<Region> &regions() const {
    return regions_;
  }

  void Init(DrmHwcLayer *layers, size_t num_layers);
  void GenerateHistory(DrmHwcLayer *layers,
                       std::vector<bool> &changed_regions) const;
  void StableRegionsWithMarginalHistory(
      const std::vector<bool> &changed_regions,
      std::vector<bool> &stable_regions) const;
  void RecordHistory(DrmHwcLayer *layers,
                     const std::vector<bool> &changed_regions);
  void RecordSquashed(const std::vector<bool> &squashed_regions);

  void Dump(std::ostringstream *out) const;

 private:
  size_t generation_number_ = 0;
  unsigned valid_history_ = 0;
  std::vector<buffer_handle_t> last_handles_;

  std::vector<Region> regions_;
};

class DrmDisplayCompositor {
 public:
  DrmDisplayCompositor();
  ~DrmDisplayCompositor();

  int Init(DrmResources *drm, int display);

  std::unique_ptr<DrmDisplayComposition> CreateComposition() const;
  int QueueComposition(std::unique_ptr<DrmDisplayComposition> composition);
  int Composite();
  void Dump(std::ostringstream *out) const;

  std::tuple<uint32_t, uint32_t, int> GetActiveModeResolution();

  bool HaveQueuedComposites() const;

  SquashState *squash_state() {
    return NULL;
  }

 private:
  DrmDisplayCompositor(const DrmDisplayCompositor &) = delete;

  // We'll wait for acquire fences to fire for kAcquireWaitTimeoutMs,
  // kAcquireWaitTries times, logging a warning in between.
  static const int kAcquireWaitTries = 5;
  static const int kAcquireWaitTimeoutMs = 100;

  int PrepareFramebuffer(DrmFramebuffer &fb,
                         DrmDisplayComposition *display_comp);
  int ApplyPreComposite(DrmDisplayComposition *display_comp);
  int ApplyFrame(DrmDisplayComposition *display_comp);
  int ApplyDpms(DrmDisplayComposition *display_comp);
  int DisablePlanes(DrmDisplayComposition *display_comp);

  DrmResources *drm_;
  int display_;

  DrmCompositorWorker worker_;

  std::queue<std::unique_ptr<DrmDisplayComposition>> composite_queue_;
  std::unique_ptr<DrmDisplayComposition> active_composition_;

  bool initialized_;
  bool active_;

  DrmMode next_mode_;
  bool needs_modeset_;

  int framebuffer_index_;
  DrmFramebuffer framebuffers_[DRM_DISPLAY_BUFFERS];
  std::unique_ptr<GLWorkerCompositor> pre_compositor_;

  // mutable since we need to acquire in HaveQueuedComposites
  mutable pthread_mutex_t lock_;

  // State tracking progress since our last Dump(). These are mutable since
  // we need to reset them on every Dump() call.
  mutable uint64_t dump_frames_composited_;
  mutable uint64_t dump_last_timestamp_ns_;
};
}

#endif  // ANDROID_DRM_DISPLAY_COMPOSITOR_H_

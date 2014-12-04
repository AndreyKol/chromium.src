// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_H_
#define CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "cc/base/swap_promise.h"
#include "cc/base/swap_promise_monitor.h"
#include "cc/input/top_controls_state.h"
#include "cc/trees/layer_tree_host_client.h"
#include "cc/trees/layer_tree_host_single_thread_client.h"
#include "cc/trees/layer_tree_settings.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/WebLayerTreeView.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/rect.h"

namespace ui {
struct LatencyInfo;
}

namespace cc {
class CopyOutputRequest;
class InputHandler;
class Layer;
class LayerTreeHost;
}

namespace content {
class RenderWidget;

class CONTENT_EXPORT RenderWidgetCompositor
    : NON_EXPORTED_BASE(public blink::WebLayerTreeView),
      NON_EXPORTED_BASE(public cc::LayerTreeHostClient),
      NON_EXPORTED_BASE(public cc::LayerTreeHostSingleThreadClient) {
 public:
  // Attempt to construct and initialize a compositor instance for the widget
  // with the given settings. Returns NULL if initialization fails.
  static scoped_ptr<RenderWidgetCompositor> Create(RenderWidget* widget,
                                                   bool threaded);

  virtual ~RenderWidgetCompositor();

  const base::WeakPtr<cc::InputHandler>& GetInputHandler();
  bool BeginMainFrameRequested() const;
  void SetNeedsDisplayOnAllLayers();
  void SetRasterizeOnlyVisibleContent();
  void UpdateTopControlsState(cc::TopControlsState constraints,
                              cc::TopControlsState current,
                              bool animate);
  void SetTopControlsShrinkBlinkSize(bool shrink);
  void SetTopControlsHeight(float height);
  void SetNeedsRedrawRect(gfx::Rect damage_rect);
  // Like setNeedsRedraw but forces the frame to be drawn, without early-outs.
  // Redraw will be forced after the next commit
  void SetNeedsForcedRedraw();
  // Calling CreateLatencyInfoSwapPromiseMonitor() to get a scoped
  // LatencyInfoSwapPromiseMonitor. During the life time of the
  // LatencyInfoSwapPromiseMonitor, if SetNeedsCommit() or SetNeedsUpdateLayer()
  // is called on LayerTreeHost, the original latency info will be turned
  // into a LatencyInfoSwapPromise.
  scoped_ptr<cc::SwapPromiseMonitor> CreateLatencyInfoSwapPromiseMonitor(
      ui::LatencyInfo* latency);
  // Calling QueueSwapPromise() to directly queue a SwapPromise into
  // LayerTreeHost.
  void QueueSwapPromise(scoped_ptr<cc::SwapPromise> swap_promise);
  int GetLayerTreeId() const;
  int GetSourceFrameNumber() const;
  void SetNeedsCommit();
  void NotifyInputThrottledUntilCommit();
  const cc::Layer* GetRootLayer() const;
  int ScheduleMicroBenchmark(
      const std::string& name,
      scoped_ptr<base::Value> value,
      const base::Callback<void(scoped_ptr<base::Value>)>& callback);
  bool SendMessageToMicroBenchmark(int id, scoped_ptr<base::Value> value);

  // WebLayerTreeView implementation.
  virtual void setSurfaceReady();
  virtual void setRootLayer(const blink::WebLayer& layer);
  virtual void clearRootLayer();
  virtual void setViewportSize(
      const blink::WebSize& unused_deprecated,
      const blink::WebSize& device_viewport_size);
  virtual void setViewportSize(const blink::WebSize& device_viewport_size);
  virtual blink::WebSize layoutViewportSize() const;
  virtual blink::WebSize deviceViewportSize() const;
  virtual blink::WebFloatPoint adjustEventPointForPinchZoom(
      const blink::WebFloatPoint& point) const;
  virtual void setDeviceScaleFactor(float device_scale);
  virtual float deviceScaleFactor() const;
  virtual void setBackgroundColor(blink::WebColor color);
  virtual void setHasTransparentBackground(bool transparent);
  virtual void setOverhangBitmap(const SkBitmap& bitmap);
  virtual void setVisible(bool visible);
  virtual void setPageScaleFactorAndLimits(float page_scale_factor,
                                           float minimum,
                                           float maximum);
  virtual void startPageScaleAnimation(const blink::WebPoint& destination,
                                       bool use_anchor,
                                       float new_page_scale,
                                       double duration_sec);
  virtual void heuristicsForGpuRasterizationUpdated(bool matches_heuristics);
  virtual void setNeedsAnimate();
  virtual bool commitRequested() const;
  virtual void didStopFlinging();
  virtual void compositeAndReadbackAsync(
      blink::WebCompositeAndReadbackAsyncCallback* callback);
  virtual void finishAllRendering();
  virtual void setDeferCommits(bool defer_commits);
  virtual void registerForAnimations(blink::WebLayer* layer);
  virtual void registerViewportLayers(
      const blink::WebLayer* overscrollElasticityLayer,
      const blink::WebLayer* pageScaleLayer,
      const blink::WebLayer* innerViewportScrollLayer,
      const blink::WebLayer* outerViewportScrollLayer) override;
  virtual void clearViewportLayers() override;
  virtual void registerSelection(const blink::WebSelectionBound& start,
                                 const blink::WebSelectionBound& end) override;
  virtual void clearSelection() override;
  virtual void setShowFPSCounter(bool show);
  virtual void setShowPaintRects(bool show);
  virtual void setShowDebugBorders(bool show);
  virtual void setContinuousPaintingEnabled(bool enabled);
  virtual void setShowScrollBottleneckRects(bool show);
  virtual void setTopControlsContentOffset(float);

  // cc::LayerTreeHostClient implementation.
  void WillBeginMainFrame(int frame_id) override;
  void DidBeginMainFrame() override;
  void BeginMainFrame(const cc::BeginFrameArgs& args) override;
  void Layout() override;
  void ApplyViewportDeltas(const gfx::Vector2d& inner_delta,
                           const gfx::Vector2d& outer_delta,
                           float page_scale,
                           float top_controls_delta) override;
  void ApplyViewportDeltas(const gfx::Vector2d& scroll_delta,
                           float page_scale,
                           float top_controls_delta) override;
  void RequestNewOutputSurface(bool fallback) override;
  void DidInitializeOutputSurface() override;
  void WillCommit() override;
  void DidCommit() override;
  void DidCommitAndDrawFrame() override;
  void DidCompleteSwapBuffers() override;
  void RateLimitSharedMainThreadContext() override;

  // cc::LayerTreeHostSingleThreadClient implementation.
  void ScheduleAnimation() override;
  void DidPostSwapBuffers() override;
  void DidAbortSwapBuffers() override;

 private:
  RenderWidgetCompositor(RenderWidget* widget, bool threaded);

  void Initialize(cc::LayerTreeSettings settings);

  bool threaded_;
  RenderWidget* widget_;
  scoped_ptr<cc::LayerTreeHost> layer_tree_host_;

  scoped_ptr<cc::CopyOutputRequest> temporary_copy_output_request_;

  bool send_v8_idle_notification_after_commit_;
  base::TimeTicks begin_main_frame_time_;
  // The time interval between BeginMainFrame calls, provided by the scheduler.
  base::TimeDelta begin_main_frame_interval_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_GPU_RENDER_WIDGET_COMPOSITOR_H_

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/event_dispatch_callback.h"

namespace ui {

KeyEventParams::KeyEventParams(int device_id, unsigned int code, bool down)
    : device_id(device_id), code(code), down(down) {
}

KeyEventParams::KeyEventParams(const KeyEventParams& other) = default;

KeyEventParams::~KeyEventParams() {
}

MouseMoveEventParams::MouseMoveEventParams(int device_id,
                                           const gfx::PointF& location)
    : device_id(device_id), location(location) {
}

MouseMoveEventParams::MouseMoveEventParams(const MouseMoveEventParams& other) =
    default;

MouseMoveEventParams::~MouseMoveEventParams() {
}

MouseButtonEventParams::MouseButtonEventParams(int device_id,
                                               const gfx::PointF& location,
                                               unsigned int button,
                                               bool down,
                                               bool allow_remap)
    : device_id(device_id),
      location(location),
      button(button),
      down(down),
      allow_remap(allow_remap) {
}

MouseButtonEventParams::MouseButtonEventParams(
    const MouseButtonEventParams& other) = default;

MouseButtonEventParams::~MouseButtonEventParams() {
}

MouseWheelEventParams::MouseWheelEventParams(int device_id,
                                             const gfx::PointF& location,
                                             const gfx::Vector2d& delta)
    : device_id(device_id), location(location), delta(delta) {
}

MouseWheelEventParams::MouseWheelEventParams(
    const MouseWheelEventParams& other) = default;

MouseWheelEventParams::~MouseWheelEventParams() {
}

ScrollEventParams::ScrollEventParams(int device_id,
                                     EventType type,
                                     const gfx::PointF location,
                                     const gfx::Vector2dF delta,
                                     const gfx::Vector2dF ordinal_delta,
                                     int finger_count,
                                     const base::TimeDelta timestamp)
    : device_id(device_id),
      type(type),
      location(location),
      delta(delta),
      ordinal_delta(ordinal_delta),
      finger_count(finger_count),
      timestamp(timestamp) {
}

ScrollEventParams::ScrollEventParams(const ScrollEventParams& other) = default;

ScrollEventParams::~ScrollEventParams() {
}

TouchEventParams::TouchEventParams(int device_id,
                                   int touch_id,
                                   EventType type,
                                   const gfx::PointF& location,
                                   const gfx::Vector2dF& radii,
                                   float pressure,
                                   const base::TimeDelta& timestamp)
    : device_id(device_id),
      touch_id(touch_id),
      type(type),
      location(location),
      radii(radii),
      pressure(pressure),
      timestamp(timestamp) {
}

TouchEventParams::TouchEventParams(const TouchEventParams& other) = default;

TouchEventParams::~TouchEventParams() {
}

}  // namspace ui

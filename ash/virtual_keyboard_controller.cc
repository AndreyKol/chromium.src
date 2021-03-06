// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/virtual_keyboard_controller.h"

#include <vector>

#include "ash/shell.h"
#include "ash/wm/maximize_mode/maximize_mode_controller.h"
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/keyboard_device.h"
#include "ui/events/devices/touchscreen_device.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_util.h"

namespace ash {
namespace {

// Checks whether smart deployment is enabled.
bool IsSmartVirtualKeyboardEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      keyboard::switches::kEnableAutoVirtualKeyboard);
}

}  // namespace

VirtualKeyboardController::VirtualKeyboardController()
    : has_external_keyboard_(false),
      has_internal_keyboard_(false),
      has_touchscreen_(false),
      ignore_external_keyboard_(false) {
  Shell::GetInstance()->AddShellObserver(this);
  ui::DeviceDataManager::GetInstance()->AddObserver(this);
  UpdateDevices();
}

VirtualKeyboardController::~VirtualKeyboardController() {
  Shell::GetInstance()->RemoveShellObserver(this);
  ui::DeviceDataManager::GetInstance()->RemoveObserver(this);
}

void VirtualKeyboardController::OnMaximizeModeStarted() {
  if (!IsSmartVirtualKeyboardEnabled())
    SetKeyboardEnabled(true);
}

void VirtualKeyboardController::OnMaximizeModeEnded() {
  if (!IsSmartVirtualKeyboardEnabled())
    SetKeyboardEnabled(false);
}

void VirtualKeyboardController::OnTouchscreenDeviceConfigurationChanged() {
  UpdateDevices();
}

void VirtualKeyboardController::OnKeyboardDeviceConfigurationChanged() {
  UpdateDevices();
}

void VirtualKeyboardController::OnMouseDeviceConfigurationChanged() {
}

void VirtualKeyboardController::OnTouchpadDeviceConfigurationChanged() {
}

void VirtualKeyboardController::ToggleIgnoreExternalKeyboard() {
  ignore_external_keyboard_ = !ignore_external_keyboard_;
  UpdateKeyboardEnabled();
}

void VirtualKeyboardController::UpdateDevices() {
  ui::DeviceDataManager* device_data_manager =
      ui::DeviceDataManager::GetInstance();

  // Checks for touchscreens.
  has_touchscreen_ = device_data_manager->touchscreen_devices().size() > 0;

  // Checks for keyboards.
  has_external_keyboard_ = false;
  has_internal_keyboard_ = false;
  for (const ui::KeyboardDevice& device :
       device_data_manager->keyboard_devices()) {
    if (has_internal_keyboard_ && has_external_keyboard_)
      break;
    ui::InputDeviceType type = device.type;
    if (type == ui::InputDeviceType::INPUT_DEVICE_INTERNAL)
      has_internal_keyboard_ = true;
    if (type == ui::InputDeviceType::INPUT_DEVICE_EXTERNAL)
      has_external_keyboard_ = true;
  }
  // Update keyboard state.
  UpdateKeyboardEnabled();
}

void VirtualKeyboardController::UpdateKeyboardEnabled() {
  if (!IsSmartVirtualKeyboardEnabled()) {
    SetKeyboardEnabled(Shell::GetInstance()
                           ->maximize_mode_controller()
                           ->IsMaximizeModeWindowManagerEnabled());
    return;
  }
  SetKeyboardEnabled(!has_internal_keyboard_ && has_touchscreen_ &&
                     (!has_external_keyboard_ || ignore_external_keyboard_));
  ash::Shell::GetInstance()
      ->system_tray_notifier()
      ->NotifyVirtualKeyboardSuppressionChanged(!has_internal_keyboard_ &&
                                                has_touchscreen_ &&
                                                has_external_keyboard_);
}

void VirtualKeyboardController::SetKeyboardEnabled(bool enabled) {
  keyboard::SetTouchKeyboardEnabled(enabled);
  if (enabled) {
    Shell::GetInstance()->CreateKeyboard();
  } else {
    if (!keyboard::IsKeyboardEnabled())
      Shell::GetInstance()->DeactivateKeyboard();
  }
}

}  // namespace ash

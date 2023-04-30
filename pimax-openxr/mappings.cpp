// MIT License
//
// Copyright(c) 2022-2023 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "pch.h"

#include "log.h"
#include "runtime.h"
#include "utils.h"

namespace {
    std::string rreplace(const std::string& str, const std::string& from, const std::string& to) {
        std::string copy(str);
        const size_t start_pos = str.rfind(from);
        copy.replace(start_pos, from.length(), to);

        return copy;
    }
} // namespace

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    void OpenXrRuntime::initializeRemappingTables() {
        // 1:1 mappings.
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/htc/vive_controller", "/interaction_profiles/htc/vive_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                return mapPathToViveControllerInputState(xrAction, getXrPath(binding), source);
            });
        m_controllerMappingTable.insert_or_assign(std::make_pair("/interaction_profiles/valve/index_controller",
                                                                 "/interaction_profiles/valve/index_controller"),
                                                  [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                                                      return mapPathToIndexControllerInputState(
                                                          xrAction, getXrPath(binding), source);
                                                  });
        m_controllerMappingTable.insert_or_assign(std::make_pair("/interaction_profiles/oculus/touch_controller",
                                                                 "/interaction_profiles/oculus/touch_controller"),
                                                  [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                                                      return mapPathToCrystalControllerInputState(
                                                          xrAction, getXrPath(binding), source);
                                                  });
        m_controllerMappingTable.insert_or_assign(std::make_pair("/interaction_profiles/khr/simple_controller",
                                                                 "/interaction_profiles/khr/simple_controller"),
                                                  [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                                                      return mapPathToSimpleControllerInputState(
                                                          xrAction, getXrPath(binding), source);
                                                  });

        // Virtual mappings to Vive controller.
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/oculus/touch_controller",
                           "/interaction_profiles/htc/vive_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapOculusTouchControllerToViveController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToViveControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/microsoft/motion_controller",
                           "/interaction_profiles/htc/vive_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapMicrosoftMotionControllerToViveController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToViveControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/valve/index_controller", "/interaction_profiles/htc/vive_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapIndexControllerToViveController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToViveControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/khr/simple_controller", "/interaction_profiles/htc/vive_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapSimpleControllerToViveController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToViveControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });

        // Virtual mappings to Index controller.
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/oculus/touch_controller",
                           "/interaction_profiles/valve/index_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapOculusTouchControllerToIndexController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToIndexControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/microsoft/motion_controller",
                           "/interaction_profiles/valve/index_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapMicrosoftMotionControllerToIndexController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToIndexControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/htc/vive_controller", "/interaction_profiles/valve/index_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapViveControllerToIndexController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToIndexControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/khr/simple_controller",
                           "/interaction_profiles/valve/index_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapSimpleControllerToIndexController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToIndexControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });

        // Virtual mappings to Crystal controller.
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/valve/index_controller",
                           "/interaction_profiles/oculus/touch_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapIndexControllerToCrystalController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToCrystalControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/htc/vive_controller",
                           "/interaction_profiles/oculus/touch_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapViveControllerToCrystalController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToCrystalControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/microsoft/motion_controller",
                           "/interaction_profiles/oculus/touch_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapMicrosoftMotionControllerToCrystalController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToCrystalControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/khr/simple_controller",
                           "/interaction_profiles/oculus/touch_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapSimpleControllerToCrystalController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToCrystalControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });

        // Virtual mappings to Simple controller.
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/oculus/touch_controller",
                           "/interaction_profiles/khr/simple_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapOculusTouchControllerToSimpleController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToSimpleControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/htc/vive_controller", "/interaction_profiles/khr/simple_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapViveControllerToSimpleController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToSimpleControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/valve/index_controller",
                           "/interaction_profiles/khr/simple_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapIndexControllerToSimpleController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToSimpleControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });
        m_controllerMappingTable.insert_or_assign(
            std::make_pair("/interaction_profiles/microsoft/motion_controller",
                           "/interaction_profiles/khr/simple_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapMicrosoftMotionControllerToSimpleController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToSimpleControllerInputState(xrAction, remapped.value(), source);
                }
                return false;
            });

        // Functions for validating paths.
        m_controllerValidPathsTable.insert_or_assign(
            "/interaction_profiles/khr/simple_controller",
            [&](const std::string& path) { return getSimpleControllerLocalizedSourceName(path) != "<Unknown>"; });
        m_controllerValidPathsTable.insert_or_assign(
            "/interaction_profiles/htc/vive_controller",
            [&](const std::string& path) { return getViveControllerLocalizedSourceName(path) != "<Unknown>"; });
        m_controllerValidPathsTable.insert_or_assign(
            "/interaction_profiles/valve/index_controller",
            [&](const std::string& path) { return getIndexControllerLocalizedSourceName(path) != "<Unknown>"; });
        m_controllerValidPathsTable.insert_or_assign(
            "/interaction_profiles/oculus/touch_controller",
            [&](const std::string& path) { return getCrystalControllerLocalizedSourceName(path) != "<Unknown>"; });
        m_controllerValidPathsTable.insert_or_assign(
            "/interaction_profiles/microsoft/motion_controller", [&](const std::string& path) {
                if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu") ||
                    endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
                    endsWith(path, "/input/squeeze/force") || endsWith(path, "/input/squeeze") ||
                    endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                    endsWith(path, "/input/trigger") || endsWith(path, "/input/thumbstick") ||
                    endsWith(path, "/input/thumbstick/x") || endsWith(path, "/input/thumbstick/y") ||
                    endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick/force") ||
                    endsWith(path, "/input/thumbstick/touch") || endsWith(path, "/input/trackpad") ||
                    endsWith(path, "/input/trackpad/x") || endsWith(path, "/input/trackpad/y") ||
                    endsWith(path, "/input/trackpad/click") || endsWith(path, "/input/trackpad/force") ||
                    endsWith(path, "/input/trackpad/touch") || endsWith(path, "/input/grip/pose") ||
                    endsWith(path, "/input/aim/pose") || endsWith(path, "/output/haptic")) {
                    return true;
                }
                return false;
            });
        m_controllerValidPathsTable.insert_or_assign(
            "/interaction_profiles/google/daydream_controller", [](const std::string& path) {
                if (endsWith(path, "/input/select/click") || endsWith(path, "/input/select") ||
                    endsWith(path, "/input/trackpad") || endsWith(path, "/input/trackpad/x") ||
                    endsWith(path, "/input/trackpad/y") || endsWith(path, "/input/trackpad/click") ||
                    endsWith(path, "/input/trackpad/force") || endsWith(path, "/input/trackpad/touch") ||
                    endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose")) {
                    return true;
                }
                return false;
            });
        m_controllerValidPathsTable.insert_or_assign("/interaction_profiles/htc/vive_pro", [](const std::string& path) {
            if (path == "/user/head/input/system/click" || path == "/user/head/input/system" ||
                path == "/user/head/input/volume_up/click" || path == "/user/head/input/volume_up" ||
                path == "/user/head/input/volume_down/click" || path == "/user/head/input/volume_down" ||
                path == "/user/head/input/mute_mic/click" || path == "/user/head/input/mute_mic") {
                return true;
            }
            return false;
        });
        m_controllerValidPathsTable.insert_or_assign(
            "/interaction_profiles/microsoft/xbox_controller", [](const std::string& path) {
                if (path == "/user/gamepad/input/menu/click" || path == "/user/gamepad/input/menu" ||
                    path == "/user/gamepad/input/view/click" || path == "/user/gamepad/input/view" ||
                    path == "/user/gamepad/input/a/click" || path == "/user/gamepad/input/a" ||
                    path == "/user/gamepad/input/b/click" || path == "/user/gamepad/input/b" ||
                    path == "/user/gamepad/input/x/click" || path == "/user/gamepad/input/x" ||
                    path == "/user/gamepad/input/y/click" || path == "/user/gamepad/input/y" ||
                    path == "/user/gamepad/input/dpad_down/click" || path == "/user/gamepad/input/dpad_down" ||
                    path == "/user/gamepad/input/dpad_right/click" || path == "/user/gamepad/input/dpad_right" ||
                    path == "/user/gamepad/input/dpad_up/click" || path == "/user/gamepad/input/dpad_up" ||
                    path == "/user/gamepad/input/dpad_left/click" || path == "/user/gamepad/input/dpad_left" ||
                    path == "/user/gamepad/input/shoulder_left/click" || path == "/user/gamepad/input/shoulder_left" ||
                    path == "/user/gamepad/input/shoulder_right/click" ||
                    path == "/user/gamepad/input/shoulder_right" || path == "/user/gamepad/input/trigger_left/click" ||
                    path == "/user/gamepad/input/trigger_left/value" ||
                    path == "/user/gamepad/input/trigger_left/force" || path == "/user/gamepad/input/trigger_left" ||
                    path == "/user/gamepad/input/trigger_right/click" ||
                    path == "/user/gamepad/input/trigger_right/value" ||
                    path == "/user/gamepad/input/trigger_right/force" || path == "/user/gamepad/input/trigger_right" ||
                    path == "/user/gamepad/input/thumbstick_left" || path == "/user/gamepad/input/thumbstick_left/x" ||
                    path == "/user/gamepad/input/thumbstick_left/y" ||
                    path == "/user/gamepad/input/thumbstick_left/click" ||
                    path == "/user/gamepad/input/thumbstick_left/force" ||
                    path == "/user/gamepad/input/thumbstick_right" ||
                    path == "/user/gamepad/input/thumbstick_right/x" ||
                    path == "/user/gamepad/input/thumbstick_right/y" ||
                    path == "/user/gamepad/input/thumbstick_right/click" ||
                    path == "/user/gamepad/input/thumbstick_right/force" ||
                    path == "/user/gamepad/output/haptic_left" || path == "/user/gamepad/output/haptic_right" ||
                    path == "/user/gamepad/output/haptic_left_trigger" ||
                    path == "/user/gamepad/output/haptic_right_trigger") {
                    return true;
                }
                return false;
            });
        m_controllerValidPathsTable.insert_or_assign(
            "/interaction_profiles/oculus/go_controller", [](const std::string& path) {
                if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system") ||
                    endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger") ||
                    endsWith(path, "/input/back/click") || endsWith(path, "/input/back") ||
                    endsWith(path, "/input/trackpad") || endsWith(path, "/input/trackpad/x") ||
                    endsWith(path, "/input/trackpad/y") || endsWith(path, "/input/trackpad/click") ||
                    endsWith(path, "/input/trackpad/force") || endsWith(path, "/input/trackpad/touch") ||
                    endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose")) {
                    return true;
                }
                return false;
            });
    }

    bool OpenXrRuntime::mapPathToViveControllerInputState(const Action& xrAction,
                                                          const std::string& path,
                                                          ActionSource& source) const {
        source.buttonMap = nullptr;
        source.floatValue = nullptr;
        source.vector2fValue = nullptr;

        if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/force") ||
                   endsWith(path, "/input/squeeze")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_Grip;
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_ApplicationMenu;
        } else if (endsWith(path, "/input/trigger/click") ||
                   (xrAction.type == XR_ACTION_TYPE_BOOLEAN_INPUT && endsWith(path, "/input/trigger"))) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/trigger/value") ||
                   (xrAction.type == XR_ACTION_TYPE_FLOAT_INPUT && endsWith(path, "/input/trigger"))) {
            source.floatValue = m_cachedInputState.Trigger;
        } else if (endsWith(path, "/input/trackpad")) {
            source.vector2fValue = m_cachedInputState.TouchPad;
            source.vector2fIndex = -1;
        } else if (endsWith(path, "/input/trackpad/x")) {
            source.vector2fValue = m_cachedInputState.TouchPad;
            source.vector2fIndex = 0;
        } else if (endsWith(path, "/input/trackpad/y")) {
            source.vector2fValue = m_cachedInputState.TouchPad;
            source.vector2fIndex = 1;
        } else if (endsWith(path, "/input/trackpad/click") || endsWith(path, "/input/trackpad/force") ||
                   endsWith(path, "/input/trackpad")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_TouchPad;
        } else if (endsWith(path, "/input/trackpad/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_TouchPad;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            // Do nothing.
        } else {
            // No possible binding.
            return false;
        }

        source.realPath = path;

        return true;
    }

    bool OpenXrRuntime::mapPathToIndexControllerInputState(const Action& xrAction,
                                                           const std::string& path,
                                                           ActionSource& source) const {
        source.buttonMap = nullptr;
        source.floatValue = nullptr;
        source.vector2fValue = nullptr;

        if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/system/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/a/click") || endsWith(path, "/input/a")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_A;
        } else if (endsWith(path, "/input/a/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_A;
        } else if (endsWith(path, "/input/b/click") || endsWith(path, "/input/b")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_B;
        } else if (endsWith(path, "/input/b/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_B;
        } else if (endsWith(path, "/input/squeeze/value") || endsWith(path, "/input/squeeze/click") ||
                   endsWith(path, "/input/squeeze")) {
            // We use the floatValue for squeeze/click since the threshhold for HandButtons seems too high.
            source.floatValue = m_cachedInputState.Grip;
        } else if (endsWith(path, "/input/squeeze/force")) {
            source.floatValue = m_cachedInputState.GripForce;
        } else if (endsWith(path, "/input/trigger/click") ||
                   (xrAction.type == XR_ACTION_TYPE_BOOLEAN_INPUT && endsWith(path, "/input/trigger"))) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/trigger/value") ||
                   (xrAction.type == XR_ACTION_TYPE_FLOAT_INPUT && endsWith(path, "/input/trigger"))) {
            source.floatValue = m_cachedInputState.Trigger;
        } else if (endsWith(path, "/input/trigger/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/thumbstick")) {
            source.vector2fValue = m_cachedInputState.JoyStick;
            source.vector2fIndex = -1;
        } else if (endsWith(path, "/input/thumbstick/x")) {
            source.vector2fValue = m_cachedInputState.JoyStick;
            source.vector2fIndex = 0;
        } else if (endsWith(path, "/input/thumbstick/y")) {
            source.vector2fValue = m_cachedInputState.JoyStick;
            source.vector2fIndex = 1;
        } else if (endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_JoyStick;
        } else if (endsWith(path, "/input/thumbstick/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_JoyStick;
        } else if (endsWith(path, "/input/trackpad")) {
            source.vector2fValue = m_cachedInputState.TouchPad;
            source.vector2fIndex = -1;
        } else if (endsWith(path, "/input/trackpad/x")) {
            source.vector2fValue = m_cachedInputState.TouchPad;
            source.vector2fIndex = 0;
        } else if (endsWith(path, "/input/trackpad/y")) {
            source.vector2fValue = m_cachedInputState.TouchPad;
            source.vector2fIndex = 1;
        } else if (endsWith(path, "/input/trackpad/force")) {
            source.floatValue = m_cachedInputState.TouchPadForce;
        } else if (endsWith(path, "/input/trackpad/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_TouchPad;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            // Do nothing.
        } else {
            // No possible binding.
            return false;
        }

        source.realPath = path;

        return true;
    }

    bool OpenXrRuntime::mapPathToCrystalControllerInputState(const Action& xrAction,
                                                             const std::string& path,
                                                             ActionSource& source) const {
        source.buttonMap = nullptr;
        source.floatValue = nullptr;
        source.vector2fValue = nullptr;

        if (path == "/user/hand/left/input/x/click" || path == "/user/hand/left/input/x") {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_A;
        } else if (path == "/user/hand/left/input/x/touch") {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_A;
        } else if (path == "/user/hand/left/input/y/click" || path == "/user/hand/left/input/y") {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_B;
        } else if (path == "/user/hand/left/input/y/touch") {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_B;
        } else if (path == "/user/hand/left/input/menu/click" || path == "/user/hand/left/menu/system") {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_ApplicationMenu;
        } else if (path == "/user/hand/right/input/a/click" || path == "/user/hand/right/input/a") {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_A;
        } else if (path == "/user/hand/right/input/a/touch") {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_A;
        } else if (path == "/user/hand/right/input/b/click" || path == "/user/hand/right/input/b") {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_B;
        } else if (path == "/user/hand/right/input/b/touch") {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_B;
        } else if (path == "/user/hand/right/input/system/click" || path == "/user/hand/right/input/system") {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/squeeze/value") || endsWith(path, "/input/squeeze/click") ||
                   endsWith(path, "/input/squeeze")) {
            // We use the floatValue for squeeze/click since the threshhold for HandButtons seems too high.
            source.floatValue = m_cachedInputState.Grip;
        } else if (endsWith(path, "/input/squeeze/force")) {
            source.floatValue = m_cachedInputState.GripForce;
        } else if (endsWith(path, "/input/trigger/click") ||
                   (xrAction.type == XR_ACTION_TYPE_BOOLEAN_INPUT && endsWith(path, "/input/trigger"))) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/trigger/value") ||
                   (xrAction.type == XR_ACTION_TYPE_FLOAT_INPUT && endsWith(path, "/input/trigger"))) {
            source.floatValue = m_cachedInputState.Trigger;
        } else if (endsWith(path, "/input/trigger/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/thumbstick")) {
            source.vector2fValue = m_cachedInputState.JoyStick;
            source.vector2fIndex = -1;
        } else if (endsWith(path, "/input/thumbstick/x")) {
            source.vector2fValue = m_cachedInputState.JoyStick;
            source.vector2fIndex = 0;
        } else if (endsWith(path, "/input/thumbstick/y")) {
            source.vector2fValue = m_cachedInputState.JoyStick;
            source.vector2fIndex = 1;
        } else if (endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_JoyStick;
        } else if (endsWith(path, "/input/thumbstick/touch")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_JoyStick;
        } else if (endsWith(path, "/input/thumbrest/touch") || endsWith(path, "/input/thumbrest")) {
            source.buttonMap = m_cachedInputState.HandTouches;
            source.buttonType = pvrButton_TouchPad;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            // Do nothing.
        } else {
            // No possible binding.
            return false;
        }

        source.realPath = path;

        return true;
    }

    bool OpenXrRuntime::mapPathToSimpleControllerInputState(const Action& xrAction,
                                                            const std::string& path,
                                                            ActionSource& source) const {
        source.buttonMap = nullptr;
        source.floatValue = nullptr;
        source.vector2fValue = nullptr;

        if (endsWith(path, "/input/select/click") || endsWith(path, "/input/select")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            source.buttonMap = m_cachedInputState.HandButtons;
            source.buttonType = pvrButton_ApplicationMenu;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            // Do nothing.
        } else {
            // No possible binding.
            return false;
        }

        source.realPath = path;

        return true;
    }

    std::string OpenXrRuntime::getViveControllerLocalizedSourceName(const std::string& path) const {
        if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system")) {
            return "System Button";
        } else if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/force") ||
                   endsWith(path, "/input/squeeze")) {
            return "Grip Press";
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return "Menu Button";
        } else if (endsWith(path, "/input/trigger/click")) {
            return "Trigger Press";
        } else if (endsWith(path, "/input/trigger/value") || endsWith(path, "/input/trigger")) {
            return "Trigger";
        } else if (endsWith(path, "/input/trackpad")) {
            return "Trackpad";
        } else if (endsWith(path, "/input/trackpad/x")) {
            return "Trackpad X axis";
        } else if (endsWith(path, "/input/trackpad/y")) {
            return "Trackpad Y axis";
        } else if (endsWith(path, "/input/trackpad/click") || endsWith(path, "/input/trackpad/force") ||
                   endsWith(path, "/input/trackpad")) {
            return "Trackpad Press";
        } else if (endsWith(path, "/input/trackpad/touch")) {
            return "Trackpad Touch";
        } else if (endsWith(path, "/input/grip/pose")) {
            return "Grip Pose";
        } else if (endsWith(path, "/input/aim/pose")) {
            return "Aim Pose";
        } else if (endsWith(path, "/output/haptic")) {
            return "Haptics";
        }

        return "<Unknown>";
    }

    std::string OpenXrRuntime::getIndexControllerLocalizedSourceName(const std::string& path) const {
        if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system")) {
            return "System Button";
        } else if (endsWith(path, "/input/system/touch")) {
            return "System Touch";
        } else if (endsWith(path, "/input/a/click") || endsWith(path, "/input/a")) {
            return "A Button";
        } else if (endsWith(path, "/input/a/touch")) {
            return "A Touch";
        } else if (endsWith(path, "/input/b/click") || endsWith(path, "/input/b")) {
            return "B Button";
        } else if (endsWith(path, "/input/b/touch")) {
            return "B Touch";
        } else if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
                   endsWith(path, "/input/squeeze")) {
            return "Grip";
        } else if (endsWith(path, "/input/squeeze/force")) {
            return "Grip Force";
        } else if (endsWith(path, "/input/trigger/click")) {
            return "Trigger Press";
        } else if (endsWith(path, "/input/trigger/value") || endsWith(path, "/input/trigger")) {
            return "Trigger";
        } else if (endsWith(path, "/input/trigger/touch")) {
            return "Trigger Touch";
        } else if (endsWith(path, "/input/thumbstick")) {
            return "Joystick";
        } else if (endsWith(path, "/input/thumbstick/x")) {
            return "Joystic X axis";
        } else if (endsWith(path, "/input/thumbstick/y")) {
            return "Joystick Y axis";
        } else if (endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick")) {
            return "Joystick Press";
        } else if (endsWith(path, "/input/thumbstick/touch")) {
            return "Joystick Touch";
        } else if (endsWith(path, "/input/trackpad")) {
            return "Trackpad";
        } else if (endsWith(path, "/input/trackpad/x")) {
            return "Trackpad X axis";
        } else if (endsWith(path, "/input/trackpad/y")) {
            return "Trackpad Y axis";
        } else if (endsWith(path, "/input/trackpad/force")) {
            return "Trackpad Force";
        } else if (endsWith(path, "/input/trackpad/touch")) {
            return "Trackpad Touch";
        } else if (endsWith(path, "/input/grip/pose")) {
            return "Grip Pose";
        } else if (endsWith(path, "/input/aim/pose")) {
            return "Aim Pose";
        } else if (endsWith(path, "/output/haptic")) {
            return "Haptics";
        }

        return "<Unknown>";
    }

    std::string OpenXrRuntime::getCrystalControllerLocalizedSourceName(const std::string& path) const {
        if (path == "/user/hand/left/input/x/click" || path == "/user/hand/left/input/x") {
            return "X Button";
        } else if (path == "/user/hand/left/input/x/touch") {
            return "X Touch";
        } else if (path == "/user/hand/left/input/y/click" || path == "/user/hand/left/input/y") {
            return "Y Button";
        } else if (path == "/user/hand/left/input/y/touch") {
            return "Y Touch";
        } else if (path == "/user/hand/left/input/menu/click" || path == "/user/hand/left/input/menu") {
            return "Menu Button";
        } else if (path == "/user/hand/right/input/a/click" || path == "/user/hand/right/input/a") {
            return "A Button";
        } else if (path == "/user/hand/right/input/a/touch") {
            return "A Touch";
        } else if (path == "/user/hand/right/input/b/click" || path == "/user/hand/right/input/b") {
            return "B Button";
        } else if (path == "/user/hand/right/input/b/touch") {
            return "B Touch";
        } else if (path == "/user/hand/right/input/system/click" || path == "/user/hand/right/input/system") {
            return "System Button";
        } else if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
                   endsWith(path, "/input/squeeze")) {
            return "Grip";
        } else if (endsWith(path, "/input/squeeze/force")) {
            return "Grip Force";
        } else if (endsWith(path, "/input/trigger/click")) {
            return "Trigger Press";
        } else if (endsWith(path, "/input/trigger/value") || endsWith(path, "/input/trigger")) {
            return "Trigger";
        } else if (endsWith(path, "/input/trigger/touch")) {
            return "Trigger Touch";
        } else if (endsWith(path, "/input/thumbstick")) {
            return "Joystick";
        } else if (endsWith(path, "/input/thumbstick/x")) {
            return "Joystic X axis";
        } else if (endsWith(path, "/input/thumbstick/y")) {
            return "Joystick Y axis";
        } else if (endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick")) {
            return "Joystick Press";
        } else if (endsWith(path, "/input/thumbstick/touch")) {
            return "Joystick Touch";
        } else if (endsWith(path, "/input/thumbrest/touch") || endsWith(path, "/input/thumbrest")) {
            return "Thumbrest Touch";
        } else if (endsWith(path, "/input/grip/pose")) {
            return "Grip Pose";
        } else if (endsWith(path, "/input/aim/pose")) {
            return "Aim Pose";
        } else if (endsWith(path, "/output/haptic")) {
            return "Haptics";
        }

        return "<Unknown>";
    }

    std::string OpenXrRuntime::getSimpleControllerLocalizedSourceName(const std::string& path) const {
        if (endsWith(path, "/input/select/click") || endsWith(path, "/input/select")) {
            return "Trigger Press";
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return "Menu Button";
        } else if (endsWith(path, "/input/grip/pose")) {
            return "Grip Pose";
        } else if (endsWith(path, "/input/aim/pose")) {
            return "Aim Pose";
        } else if (endsWith(path, "/output/haptic")) {
            return "Haptics";
        }

        return "<Unknown>";
    }

    std::optional<std::string> OpenXrRuntime::remapSimpleControllerToViveController(const std::string& path) const {
        if (endsWith(path, "/input/select/click") || endsWith(path, "/input/select")) {
            return rreplace(path, "/input/select", "/input/trigger");
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string>
    OpenXrRuntime::remapOculusTouchControllerToViveController(const std::string& path) const {
        if (endsWith(path, "/input/thumbstick/x") || endsWith(path, "/input/thumbstick/y") ||
            endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick/touch") ||
            endsWith(path, "/input/thumbstick")) {
            return rreplace(path, "/input/thumbstick", "/input/trackpad");
        } else if (endsWith(path, "/input/squeeze/value")) {
            return rreplace(path, "/input/squeeze/value", "/input/squeeze/click");
        } else if (endsWith(path, "/input/squeeze/force")) {
            return rreplace(path, "/input/squeeze/force", "/input/squeeze/click");
        } else if (path == "/user/hand/right/input/a/click" || path == "/user/hand/right/input/a") {
            return "/user/hand/right/input/menu/click";
        } else if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system") ||
                   endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu") ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze") ||
                   endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                   endsWith(path, "/input/trigger")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string>
    OpenXrRuntime::remapMicrosoftMotionControllerToViveController(const std::string& path) const {
        if (endsWith(path, "/input/squeeze/value")) {
            return rreplace(path, "/input/squeeze/value", "/input/squeeze/click");
        } else if (endsWith(path, "/input/squeeze/force")) {
            return rreplace(path, "/input/squeeze/force", "/input/squeeze/click");
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu") ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze") ||
                   endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                   endsWith(path, "/input/trigger") || endsWith(path, "/input/trackpad/x") ||
                   endsWith(path, "/input/trackpad/y") || endsWith(path, "/input/trackpad/click") ||
                   endsWith(path, "/input/trackpad/force") || endsWith(path, "/input/trackpad/touch") ||
                   endsWith(path, "/input/trackpad")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string> OpenXrRuntime::remapIndexControllerToViveController(const std::string& path) const {
        if (endsWith(path, "/input/a/click") || endsWith(path, "/input/a")) {
            return rreplace(path, "/input/a", "/input/menu");
        } else if (endsWith(path, "/input/thumbstick/x") || endsWith(path, "/input/thumbstick/y") ||
                   endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick/force") ||
                   endsWith(path, "/input/thumbstick/touch") || endsWith(path, "/input/thumbstick")) {
            return rreplace(path, "/input/thumbstick", "/input/trackpad");
        } else if (endsWith(path, "/input/squeeze/value")) {
            return rreplace(path, "/input/squeeze/value", "/input/squeeze/click");
        } else if (endsWith(path, "/input/squeeze/force")) {
            return rreplace(path, "/input/squeeze/force", "/input/squeeze/click");
        } else if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system") ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze") ||
                   endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                   endsWith(path, "/input/trigger/touch") || endsWith(path, "/input/trigger")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string> OpenXrRuntime::remapSimpleControllerToIndexController(const std::string& path) const {
        if (endsWith(path, "/input/select/click") || endsWith(path, "/input/select")) {
            return rreplace(path, "/input/select", "/input/trigger");
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return rreplace(path, "/input/menu", "/input/a");
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string>
    OpenXrRuntime::remapOculusTouchControllerToIndexController(const std::string& path) const {
        if (path == "/user/hand/left/input/x/click" || path == "/user/hand/left/input/x/touch" ||
            path == "/user/hand/left/input/x") {
            return rreplace(path, "/input/x", "/input/a");
        } else if (path == "/user/hand/left/input/y/click" || path == "/user/hand/left/input/y/touch" ||
                   path == "/user/hand/left/input/y") {
            return rreplace(path, "/input/y", "/input/b");
        } else if (endsWith(path, "/input/thumbrest/touch") || endsWith(path, "/input/thumbrest")) {
            return rreplace(path, "/input/thumbrest", "/input/trackpad");
        } else if (path == "/user/hand/right/input/a/click" || path == "/user/hand/right/input/a/touch" ||
                   path == "/user/hand/right/input/a" || path == "/user/hand/right/input/b/click" ||
                   path == "/user/hand/right/input/b/touch" || path == "/user/hand/right/input/b" ||
                   path == "/user/hand/right/input/system/click" || path == "/user/hand/right/input/system" ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
                   endsWith(path, "/input/squeeze/force") || endsWith(path, "/input/squeeze") ||
                   endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                   endsWith(path, "/input/trigger") || endsWith(path, "/input/thumbstick/x") ||
                   endsWith(path, "/input/thumbstick/y") || endsWith(path, "/input/thumbstick/click") ||
                   endsWith(path, "/input/thumbstick/touch") || endsWith(path, "/input/thumbstick")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string>
    OpenXrRuntime::remapMicrosoftMotionControllerToIndexController(const std::string& path) const {
        if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return rreplace(path, "/input/menu", "/input/a");
        } else if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
                   endsWith(path, "/input/squeeze/force") || endsWith(path, "/input/squeeze") ||
                   endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                   endsWith(path, "/input/trigger") || endsWith(path, "/input/trackpad/x") ||
                   endsWith(path, "/input/trackpad/y") || endsWith(path, "/input/trackpad/click") ||
                   endsWith(path, "/input/trackpad/force") || endsWith(path, "/input/trackpad/touch") ||
                   endsWith(path, "/input/trackpad") || endsWith(path, "/input/thumbstick/x") ||
                   endsWith(path, "/input/thumbstick/y") || endsWith(path, "/input/thumbstick/click") ||
                   endsWith(path, "/input/thumbstick/touch") || endsWith(path, "/input/thumbstick")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string> OpenXrRuntime::remapViveControllerToIndexController(const std::string& path) const {
        if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return rreplace(path, "/input/menu", "/input/a");
        } else if (endsWith(path, "/input/trackpad/x") || endsWith(path, "/input/trackpad/y") ||
                   endsWith(path, "/input/trackpad/click") || endsWith(path, "/input/trackpad/force") ||
                   endsWith(path, "/input/trackpad/touch") || endsWith(path, "/input/trackpad")) {
            return rreplace(path, "/input/trackpad", "/input/thumbstick");
        } else if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system") ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/force") ||
                   endsWith(path, "/input/squeeze") || endsWith(path, "/input/trigger/click") ||
                   endsWith(path, "/input/trigger/value") || endsWith(path, "/input/trigger/touch") ||
                   endsWith(path, "/input/trigger")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string> OpenXrRuntime::remapSimpleControllerToCrystalController(const std::string& path) const {
        if (endsWith(path, "/input/select/click") || endsWith(path, "/input/select")) {
            return rreplace(path, "/input/select", "/input/trigger");
        } else if (path == "/user/hand/right/input/menu/click" || path == "/user/hand/right/input/menu") {
            return rreplace(path, "/input/menu", "/input/a");
        } else if (path == "/user/hand/left/input/menu/click" || path == "/user/hand/left/input/menu") {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string>
    OpenXrRuntime::remapMicrosoftMotionControllerToCrystalController(const std::string& path) const {
        if (path == "/user/hand/right/input/menu/click" || path == "/user/hand/right/input/menu") {
            return rreplace(path, "/input/menu", "/input/a");
        } else if (path == "/user/hand/left/input/menu/click" || path == "/user/hand/left/input/menu" ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
                   endsWith(path, "/input/squeeze/force") || endsWith(path, "/input/squeeze") ||
                   endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                   endsWith(path, "/input/trigger") || endsWith(path, "/input/trackpad") ||
                   endsWith(path, "/input/thumbstick/x") || endsWith(path, "/input/thumbstick/y") ||
                   endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick/touch") ||
                   endsWith(path, "/input/thumbstick")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string> OpenXrRuntime::remapViveControllerToCrystalController(const std::string& path) const {
        if (path == "/user/hand/right/input/menu/click" || path == "/user/hand/right/input/menu") {
            return rreplace(path, "/input/menu", "/input/a");
        } else if (endsWith(path, "/input/trackpad/x") || endsWith(path, "/input/trackpad/y") ||
                   endsWith(path, "/input/trackpad/click") || endsWith(path, "/input/trackpad/force") ||
                   endsWith(path, "/input/trackpad/touch") || endsWith(path, "/input/trackpad")) {
            return rreplace(path, "/input/trackpad", "/input/thumbstick");
        } else if (path == "/user/hand/right/input/system/click" || path == "/user/hand/right/input/system" ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/force") ||
                   endsWith(path, "/input/squeeze") || path == "/user/hand/left/input/menu/click" ||
                   path == "/user/hand/left/input/menu" || endsWith(path, "/input/trigger/click") ||
                   endsWith(path, "/input/trigger/value") || endsWith(path, "/input/trigger")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string> OpenXrRuntime::remapIndexControllerToCrystalController(const std::string& path) const {
        if (path == "/user/hand/left/input/a/click" || path == "/user/hand/left/input/a/touch" ||
            path == "/user/hand/left/input/a") {
            return rreplace(path, "/input/a", "/input/x");
        } else if (path == "/user/hand/left/input/b/click" || path == "/user/hand/left/input/b/touch" ||
                   path == "/user/hand/left/input/b") {
            return rreplace(path, "/input/b", "/input/y");
        } else if (endsWith(path, "/input/trackpad/touch")) {
            return rreplace(path, "/input/trackpad", "/input/thumbrest");
        } else if (path == "/user/hand/right/input/a/click" || path == "/user/hand/right/input/a/touch" ||
                   path == "/user/hand/right/input/a" || path == "/user/hand/right/input/b/click" ||
                   path == "/user/hand/right/input/b/touch" || path == "/user/hand/right/input/b" ||
                   path == "/user/hand/right/input/system/click" || path == "/user/hand/right/input/system" ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
                   endsWith(path, "/input/squeeze/force") || endsWith(path, "/input/squeeze") ||
                   endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                   endsWith(path, "/input/trigger") || endsWith(path, "/input/thumbstick/x") ||
                   endsWith(path, "/input/thumbstick/y") || endsWith(path, "/input/thumbstick/click") ||
                   endsWith(path, "/input/thumbstick/touch") || endsWith(path, "/input/thumbstick")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string>
    OpenXrRuntime::remapOculusTouchControllerToSimpleController(const std::string& path) const {
        if (endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger")) {
            return rreplace(path, "/input/trigger", "/input/select");
        } else if (endsWith(path, "/input/trigger/value")) {
            return rreplace(path, "/input/trigger/value", "/input/select/click");
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string>
    OpenXrRuntime::remapMicrosoftMotionControllerToSimpleController(const std::string& path) const {
        if (endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger")) {
            return rreplace(path, "/input/trigger", "/input/select");
        } else if (endsWith(path, "/input/trigger/value")) {
            return rreplace(path, "/input/trigger/value", "/input/select/click");
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string> OpenXrRuntime::remapViveControllerToSimpleController(const std::string& path) const {
        if (endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger")) {
            return rreplace(path, "/input/trigger", "/input/select");
        } else if (endsWith(path, "/input/trigger/value")) {
            return rreplace(path, "/input/trigger/value", "/input/select/click");
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            return path;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

    std::optional<std::string> OpenXrRuntime::remapIndexControllerToSimpleController(const std::string& path) const {
        if (endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger")) {
            return rreplace(path, "/input/trigger", "/input/select");
        } else if (endsWith(path, "/input/trigger/value")) {
            return rreplace(path, "/input/trigger/value", "/input/select/click");
        } else if (endsWith(path, "/input/a/click") || endsWith(path, "/input/a")) {
            return rreplace(path, "/input/a", "/input/menu");
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            return path;
        }

        // No possible binding.
        return {};
    }

} // namespace pimax_openxr

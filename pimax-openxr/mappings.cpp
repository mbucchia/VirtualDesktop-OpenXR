// MIT License
//
// Copyright(c) 2022 Matthieu Bucchianeri
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
            std::make_pair("/interaction_profiles/khr/simple_controller",
                           "/interaction_profiles/valve/index_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapSimpleControllerToIndexController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToIndexControllerInputState(xrAction, remapped.value(), source);
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
            std::make_pair("/interaction_profiles/microsoft/motion_controller",
                           "/interaction_profiles/khr/simple_controller"),
            [&](const Action& xrAction, XrPath binding, ActionSource& source) {
                const auto remapped = remapMicrosoftMotionControllerToSimpleController(getXrPath(binding));
                if (remapped.has_value()) {
                    return mapPathToSimpleControllerInputState(xrAction, remapped.value(), source);
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
        } else if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze")) {
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
        } else if (endsWith(path, "/input/trackpad/click") || endsWith(path, "/input/trackpad")) {
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
        } else if (endsWith(path, "/input/squeeze/value") || endsWith(path, "/input/squeeze")) {
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

        return true;
    }

    std::string OpenXrRuntime::getViveControllerLocalizedSourceName(const std::string& path) const {
        if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system")) {
            return "System Button";
        } else if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze")) {
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
        } else if (endsWith(path, "/input/trackpad/click") || endsWith(path, "/input/trackpad")) {
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
        } else if (endsWith(path, "/input/squeeze/value") || endsWith(path, "/input/squeeze")) {
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
        if (endsWith(path, "/input/thumbstick") || endsWith(path, "/input/thumbstick/x") ||
            endsWith(path, "/input/thumbstick/y") || endsWith(path, "/input/thumbstick/click") ||
            endsWith(path, "/input/thumbstick/touch")) {
            return rreplace(path, "/input/thumbstick", "/input/trackpad");
        } else if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system") ||
                   endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu") ||
                   endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
                   endsWith(path, "/input/squeeze") || endsWith(path, "/input/trigger/click") ||
                   endsWith(path, "/input/trigger/value") || endsWith(path, "/input/trigger")) {
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
        if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu") ||
            endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
            endsWith(path, "/input/squeeze") || endsWith(path, "/input/trigger/click") ||
            endsWith(path, "/input/trigger/value") || endsWith(path, "/input/trigger") ||
            endsWith(path, "/input/trackpad") || endsWith(path, "/input/trackpad/x") ||
            endsWith(path, "/input/trackpad/y") || endsWith(path, "/input/trackpad/click") ||
            endsWith(path, "/input/trackpad/touch")) {
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
        if (endsWith(path, "/input/x/click") || endsWith(path, "/input/x")) {
            return rreplace(path, "/input/x", "/input/a");
        } else if (endsWith(path, "/input/y/click") || endsWith(path, "/input/y")) {
            return rreplace(path, "/input/y", "/input/b");
        } else if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system") ||
                   endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu") ||
                   endsWith(path, "/input/a/click") || endsWith(path, "/input/a") || endsWith(path, "/input/b/click") ||
                   endsWith(path, "/input/b") || endsWith(path, "/input/squeeze/click") ||
                   endsWith(path, "/input/squeeze/value") || endsWith(path, "/input/squeeze") ||
                   endsWith(path, "/input/trigger/click") || endsWith(path, "/input/trigger/value") ||
                   endsWith(path, "/input/trigger") || endsWith(path, "/input/thumbstick") ||
                   endsWith(path, "/input/thumbstick/x") || endsWith(path, "/input/thumbstick/y") ||
                   endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick/touch")) {
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
        if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze/value") ||
            endsWith(path, "/input/squeeze") || endsWith(path, "/input/trigger/click") ||
            endsWith(path, "/input/trigger/value") || endsWith(path, "/input/trigger") ||
            endsWith(path, "/input/trackpad") || endsWith(path, "/input/trackpad/x") ||
            endsWith(path, "/input/trackpad/y") || endsWith(path, "/input/trackpad/click") ||
            endsWith(path, "/input/trackpad/touch") || endsWith(path, "/input/thumbstick") ||
            endsWith(path, "/input/thumbstick/x") || endsWith(path, "/input/thumbstick/y") ||
            endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick/touch")) {
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

} // namespace pimax_openxr
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

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    void OpenXrRuntime::mapPathToViveControllerInputState(Action& xrAction, XrPath binding) const {
        const auto path = getXrPath(binding);

        xrAction.buttonMap = nullptr;
        xrAction.floatValue = nullptr;
        xrAction.vector2fValue = nullptr;

        if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/squeeze/click") || endsWith(path, "/input/squeeze")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_Grip;
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_ApplicationMenu;
        } else if (endsWith(path, "/input/trigger/click") ||
                   (xrAction.type == XR_ACTION_TYPE_BOOLEAN_INPUT && endsWith(path, "/input/trigger"))) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/trigger/value") ||
                   (xrAction.type == XR_ACTION_TYPE_FLOAT_INPUT && endsWith(path, "/input/trigger"))) {
            xrAction.floatValue = m_cachedInputState.Trigger;
        } else if (endsWith(path, "/input/trackpad")) {
            xrAction.vector2fValue = m_cachedInputState.TouchPad;
            xrAction.vector2fIndex = -1;
        } else if (endsWith(path, "/input/trackpad/x")) {
            xrAction.vector2fValue = m_cachedInputState.TouchPad;
            xrAction.vector2fIndex = 0;
        } else if (endsWith(path, "/input/trackpad/y")) {
            xrAction.vector2fValue = m_cachedInputState.TouchPad;
            xrAction.vector2fIndex = 1;
        } else if (endsWith(path, "/input/trackpad/click") || endsWith(path, "/input/trackpad")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_TouchPad;
        } else if (endsWith(path, "/input/trackpad/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_TouchPad;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            // Do nothing.
        } else {
            // No possible binding.
            return;
        }

        xrAction.path = path;
    }

    void OpenXrRuntime::mapPathToIndexControllerInputState(Action& xrAction, XrPath binding) const {
        const auto path = getXrPath(binding);

        xrAction.buttonMap = nullptr;
        xrAction.floatValue = nullptr;
        xrAction.vector2fValue = nullptr;

        if (endsWith(path, "/input/system/click") || endsWith(path, "/input/system")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/system/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/a/click") || endsWith(path, "/input/a")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_A;
        } else if (endsWith(path, "/input/a/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_A;
        } else if (endsWith(path, "/input/b/click") || endsWith(path, "/input/b")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_B;
        } else if (endsWith(path, "/input/b/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_B;
        } else if (endsWith(path, "/input/squeeze/value") || endsWith(path, "/input/squeeze")) {
            xrAction.floatValue = m_cachedInputState.Grip;
        } else if (endsWith(path, "/input/squeeze/force")) {
            xrAction.floatValue = m_cachedInputState.GripForce;
        } else if (endsWith(path, "/input/trigger/click") ||
                   (xrAction.type == XR_ACTION_TYPE_BOOLEAN_INPUT && endsWith(path, "/input/trigger"))) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/trigger/value") ||
                   (xrAction.type == XR_ACTION_TYPE_FLOAT_INPUT && endsWith(path, "/input/trigger"))) {
            xrAction.floatValue = m_cachedInputState.Trigger;
        } else if (endsWith(path, "/input/trigger/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/thumbstick")) {
            xrAction.vector2fValue = m_cachedInputState.JoyStick;
            xrAction.vector2fIndex = -1;
        } else if (endsWith(path, "/input/thumbstick/x")) {
            xrAction.vector2fValue = m_cachedInputState.JoyStick;
            xrAction.vector2fIndex = 0;
        } else if (endsWith(path, "/input/thumbstick/y")) {
            xrAction.vector2fValue = m_cachedInputState.JoyStick;
            xrAction.vector2fIndex = 1;
        } else if (endsWith(path, "/input/thumbstick/click") || endsWith(path, "/input/thumbstick")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_JoyStick;
        } else if (endsWith(path, "/input/thumbstick/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_JoyStick;
        } else if (endsWith(path, "/input/trackpad")) {
            xrAction.vector2fValue = m_cachedInputState.TouchPad;
            xrAction.vector2fIndex = -1;
        } else if (endsWith(path, "/input/trackpad/x")) {
            xrAction.vector2fValue = m_cachedInputState.TouchPad;
            xrAction.vector2fIndex = 0;
        } else if (endsWith(path, "/input/trackpad/y")) {
            xrAction.vector2fValue = m_cachedInputState.TouchPad;
            xrAction.vector2fIndex = 1;
        } else if (endsWith(path, "/input/trackpad/force")) {
            xrAction.floatValue = m_cachedInputState.TouchPadForce;
        } else if (endsWith(path, "/input/trackpad/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_TouchPad;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            // Do nothing.
        } else {
            // No possible binding.
            return;
        }

        xrAction.path = path;
    }

    void OpenXrRuntime::mapPathToSimpleControllerInputState(Action& xrAction, XrPath binding) const {
        const auto path = getXrPath(binding);

        xrAction.buttonMap = nullptr;
        xrAction.floatValue = nullptr;
        xrAction.vector2fValue = nullptr;

        if (endsWith(path, "/input/select/click") || endsWith(path, "/input/select")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/menu/click") || endsWith(path, "/input/menu")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_ApplicationMenu;
        } else if (endsWith(path, "/input/grip/pose") || endsWith(path, "/input/aim/pose") ||
                   endsWith(path, "/output/haptic")) {
            // Do nothing.
        } else {
            // No possible binding.
            return;
        }

        xrAction.path = path;
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

} // namespace pimax_openxr
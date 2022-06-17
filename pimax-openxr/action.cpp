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

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrStringToPath
    XrResult OpenXrRuntime::xrStringToPath(XrInstance instance, const char* pathString, XrPath* path) {
        TraceLoggingWrite(g_traceProvider, "xrStringToPath", TLXArg(instance, "Instance"), TLArg(pathString, "String"));

        if (instance != XR_NULL_PATH && (!m_instanceCreated || instance != (XrInstance)1)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        std::string_view str(pathString);

        bool found = false;
        for (auto entry : m_strings) {
            if (entry.second == str) {
                *path = entry.first;
                found = true;
                break;
            }
        }

        if (!found) {
            *path = (XrPath)++m_stringIndex;
            m_strings.insert_or_assign(*path, str);
        }

        TraceLoggingWrite(g_traceProvider, "xrStringToPath", TLArg(*path, "Path"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrPathToString
    XrResult OpenXrRuntime::xrPathToString(
        XrInstance instance, XrPath path, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer) {
        TraceLoggingWrite(g_traceProvider,
                          "xrPathToString",
                          TLXArg(instance, "Instance"),
                          TLArg(path, "Path"),
                          TLArg(bufferCapacityInput, "BufferCapacityInput"));

        if (instance != XR_NULL_PATH && (!m_instanceCreated || instance != (XrInstance)1)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        const auto it = m_strings.find(path);
        if (it == m_strings.cend()) {
            return XR_ERROR_PATH_INVALID;
        }

        const auto& str = it->second;
        if (bufferCapacityInput && bufferCapacityInput < str.length()) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *bufferCountOutput = (uint32_t)str.length();
        TraceLoggingWrite(g_traceProvider, "xrPathToString", TLArg(*bufferCountOutput, "BufferCountOutput"));

        if (bufferCapacityInput) {
            sprintf_s(buffer, bufferCapacityInput, "%s", str.c_str());
            TraceLoggingWrite(g_traceProvider, "xrPathToString", TLArg(buffer, "String"));
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateActionSet
    XrResult OpenXrRuntime::xrCreateActionSet(XrInstance instance,
                                              const XrActionSetCreateInfo* createInfo,
                                              XrActionSet* actionSet) {
        if (createInfo->type != XR_TYPE_ACTION_SET_CREATE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateActionSet",
                          TLXArg(instance, "Instance"),
                          TLArg(createInfo->actionSetName, "Name"),
                          TLArg(createInfo->localizedActionSetName, "LocalizedName"),
                          TLArg(createInfo->priority, "Priority"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // TODO: We do not support the notion of priority.

        *actionSet = (XrActionSet)++m_actionSetIndex;

        // Maintain a list of known actionsets for validation.
        m_actionSets.insert(*actionSet);

        TraceLoggingWrite(g_traceProvider, "xrCreateActionSet", TLXArg(*actionSet, "ActionSet"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroyActionSet
    XrResult OpenXrRuntime::xrDestroyActionSet(XrActionSet actionSet) {
        TraceLoggingWrite(g_traceProvider, "xrDestroyActionSet", TLXArg(actionSet, "ActionSet"));

        if (!m_actionSets.count(actionSet)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        m_actionSets.erase(actionSet);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateAction
    XrResult OpenXrRuntime::xrCreateAction(XrActionSet actionSet,
                                           const XrActionCreateInfo* createInfo,
                                           XrAction* action) {
        if (createInfo->type != XR_TYPE_ACTION_CREATE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateAction",
                          TLXArg(actionSet, "ActionSet"),
                          TLArg(createInfo->actionName, "Name"),
                          TLArg(createInfo->localizedActionName, "LocalizedName"),
                          TLArg(xr::ToCString(createInfo->actionType), "Type"));
        for (uint32_t i = 0; i < createInfo->countSubactionPaths; i++) {
            TraceLoggingWrite(g_traceProvider,
                              "xrCreateAction",
                              TLArg(getXrPath(createInfo->subactionPaths[i]).c_str(), "SubactionPath"));
        }

        if (!m_actionSets.count(actionSet)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // Create the internal struct.
        Action& xrAction = *new Action;
        xrAction.actionSet = actionSet;

        // TODO: We do nothing about subActionPaths validation, or actionType.

        *action = (XrAction)&xrAction;

        // Maintain a list of known actionsets for validation.
        m_actions.insert(*action);

        TraceLoggingWrite(g_traceProvider, "xrCreateAction", TLXArg(*action, "Action"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroyAction
    XrResult OpenXrRuntime::xrDestroyAction(XrAction action) {
        TraceLoggingWrite(g_traceProvider, "xrDestroyAction", TLXArg(action, "Action"));

        if (action != (XrAction)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrSuggestInteractionProfileBindings
    XrResult OpenXrRuntime::xrSuggestInteractionProfileBindings(
        XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) {
        if (suggestedBindings->type != XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrSuggestInteractionProfileBindings",
                          TLXArg(instance, "Instance"),
                          TLArg(getXrPath(suggestedBindings->interactionProfile).c_str(), "InteractionProfile"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
            TraceLoggingWrite(g_traceProvider,
                              "xrSuggestInteractionProfileBindings",
                              TLXArg(suggestedBindings->suggestedBindings[i].action, "Action"),
                              TLArg(getXrPath(suggestedBindings->suggestedBindings[i].binding).c_str(), "Path"));
        }

        if (m_activeActionSets.size()) {
            return XR_ERROR_ACTIONSETS_ALREADY_ATTACHED;
        }

        std::vector<XrActionSuggestedBinding> bindings;
        for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
            bindings.push_back(suggestedBindings->suggestedBindings[i]);
        }

        m_suggestedBindings.insert_or_assign(getXrPath(suggestedBindings->interactionProfile), bindings);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrAttachSessionActionSets
    XrResult OpenXrRuntime::xrAttachSessionActionSets(XrSession session,
                                                      const XrSessionActionSetsAttachInfo* attachInfo) {
        if (attachInfo->type != XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrAttachSessionActionSets", TLXArg(session, "Session"));
        for (uint32_t i = 0; i < attachInfo->countActionSets; i++) {
            TraceLoggingWrite(
                g_traceProvider, "xrAttachSessionActionSets", TLXArg(attachInfo->actionSets[i], "ActionSet"));
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (m_activeActionSets.size()) {
            return XR_ERROR_ACTIONSETS_ALREADY_ATTACHED;
        }

        for (uint32_t i = 0; i < attachInfo->countActionSets; i++) {
            if (!m_actionSets.count(attachInfo->actionSets[i])) {
                return XR_ERROR_HANDLE_INVALID;
            }

            m_activeActionSets.insert(attachInfo->actionSets[i]);
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetCurrentInteractionProfile
    XrResult OpenXrRuntime::xrGetCurrentInteractionProfile(XrSession session,
                                                           XrPath topLevelUserPath,
                                                           XrInteractionProfileState* interactionProfile) {
        if (interactionProfile->type != XR_TYPE_INTERACTION_PROFILE_STATE) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetCurrentInteractionProfile",
                          TLXArg(session, "Session"),
                          TLArg(getXrPath(topLevelUserPath).c_str(), "TopLevelUserPath"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // If no side is specified, we use left.
        const int side = max(0, getActionSide(getXrPath(topLevelUserPath)));
        interactionProfile->interactionProfile = m_currentInteractionProfile[side];

        TraceLoggingWrite(g_traceProvider,
                          "xrGetCurrentInteractionProfile",
                          TLArg(getXrPath(interactionProfile->interactionProfile).c_str(), "InteractionProfile"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetActionStateBoolean
    XrResult OpenXrRuntime::xrGetActionStateBoolean(XrSession session,
                                                    const XrActionStateGetInfo* getInfo,
                                                    XrActionStateBoolean* state) {
        if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_BOOLEAN) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetActionStateBoolean",
                          TLXArg(session, "Session"),
                          TLXArg(getInfo->action, "Action"),
                          TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_actions.count(getInfo->action)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Action& xrAction = *(Action*)getInfo->action;

        if (!m_activeActionSets.count(xrAction.actionSet)) {
            return XR_ERROR_ACTIONSET_NOT_ATTACHED;
        }

        state->isActive = XR_FALSE;
        state->currentState = xrAction.lastBoolValue;

        if (!xrAction.path.empty()) {
            if (xrAction.buttonMap == nullptr && xrAction.floatValue == nullptr) {
                return XR_ERROR_ACTION_TYPE_MISMATCH;
            }

            const std::string fullPath = getActionPath(xrAction, getInfo->subactionPath);
            const int side = getActionSide(fullPath);
            if (side < 0) {
                return XR_ERROR_PATH_INVALID;
            }

            state->isActive = m_isControllerActive[side];
            if (state->isActive && m_frameLatchedActionSets.count(xrAction.actionSet)) {
                if (xrAction.buttonMap) {
                    state->currentState = xrAction.buttonMap[side] & xrAction.buttonType;
                } else {
                    state->currentState = xrAction.floatValue[side] > 0.99f;
                }
            }
        }

        state->changedSinceLastSync = !!state->currentState != xrAction.lastBoolValue;
        state->lastChangeTime = state->changedSinceLastSync ? pvrTimeToXrTime(m_cachedInputState.TimeInSeconds)
                                                            : xrAction.lastBoolValueChangedTime;

        xrAction.lastBoolValue = state->currentState;
        xrAction.lastBoolValueChangedTime = state->lastChangeTime;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetActionStateBoolean",
                          TLArg(!!state->isActive, "Active"),
                          TLArg(!!state->currentState, "CurrentState"),
                          TLArg(!!state->changedSinceLastSync, "ChangedSinceLastSync"),
                          TLArg(state->lastChangeTime, "LastChangeTime"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetActionStateFloat
    XrResult OpenXrRuntime::xrGetActionStateFloat(XrSession session,
                                                  const XrActionStateGetInfo* getInfo,
                                                  XrActionStateFloat* state) {
        if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_FLOAT) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetActionStateFloat",
                          TLXArg(session, "Session"),
                          TLXArg(getInfo->action, "Action"),
                          TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_actions.count(getInfo->action)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Action& xrAction = *(Action*)getInfo->action;

        if (!m_activeActionSets.count(xrAction.actionSet)) {
            return XR_ERROR_ACTIONSET_NOT_ATTACHED;
        }

        state->isActive = XR_FALSE;
        state->currentState = xrAction.lastFloatValue;

        if (!xrAction.path.empty()) {
            if (xrAction.floatValue == nullptr && (xrAction.vector2fValue == nullptr || xrAction.vector2fIndex == -1) &&
                xrAction.buttonMap == nullptr) {
                return XR_ERROR_ACTION_TYPE_MISMATCH;
            }

            const std::string fullPath = getActionPath(xrAction, getInfo->subactionPath);
            const int side = getActionSide(fullPath);
            if (side < 0) {
                return XR_ERROR_PATH_INVALID;
            }

            state->isActive = m_isControllerActive[side];
            if (state->isActive && m_frameLatchedActionSets.count(xrAction.actionSet)) {
                if (xrAction.floatValue) {
                    state->currentState = xrAction.floatValue[side];
                } else if (xrAction.buttonMap) {
                    state->currentState = xrAction.buttonMap[side] & xrAction.buttonType ? 1.f : 0.f;
                } else {
                    state->currentState =
                        xrAction.vector2fIndex == 0 ? xrAction.vector2fValue[side].x : xrAction.vector2fValue[side].y;
                }
            }
        }

        state->changedSinceLastSync = state->currentState != xrAction.lastFloatValue;
        state->lastChangeTime = state->changedSinceLastSync ? pvrTimeToXrTime(m_cachedInputState.TimeInSeconds)
                                                            : xrAction.lastFloatValueChangedTime;

        xrAction.lastFloatValue = state->currentState;
        xrAction.lastFloatValueChangedTime = state->lastChangeTime;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetActionStateFloat",
                          TLArg(!!state->isActive, "Active"),
                          TLArg(state->currentState, "CurrentState"),
                          TLArg(!!state->changedSinceLastSync, "ChangedSinceLastSync"),
                          TLArg(state->lastChangeTime, "LastChangeTime"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetActionStateVector2f
    XrResult OpenXrRuntime::xrGetActionStateVector2f(XrSession session,
                                                     const XrActionStateGetInfo* getInfo,
                                                     XrActionStateVector2f* state) {
        if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_VECTOR2F) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetActionStateVector2f",
                          TLXArg(session, "Session"),
                          TLXArg(getInfo->action, "Action"),
                          TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_actions.count(getInfo->action)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Action& xrAction = *(Action*)getInfo->action;

        if (!m_activeActionSets.count(xrAction.actionSet)) {
            return XR_ERROR_ACTIONSET_NOT_ATTACHED;
        }

        state->isActive = XR_FALSE;
        state->currentState = xrAction.lastVector2fValue;

        if (!xrAction.path.empty()) {
            if (xrAction.vector2fValue == nullptr) {
                return XR_ERROR_ACTION_TYPE_MISMATCH;
            }

            const std::string fullPath = getActionPath(xrAction, getInfo->subactionPath);
            const int side = getActionSide(fullPath);
            if (side < 0) {
                return XR_ERROR_PATH_INVALID;
            }

            state->isActive = m_isControllerActive[side];
            if (state->isActive && m_frameLatchedActionSets.count(xrAction.actionSet)) {
                state->currentState.x = xrAction.vector2fValue[side].x;
                state->currentState.y = xrAction.vector2fValue[side].y;
            }
        }

        state->changedSinceLastSync = state->currentState.x != xrAction.lastVector2fValue.x ||
                                      state->currentState.y != xrAction.lastVector2fValue.y;
        state->lastChangeTime = state->changedSinceLastSync ? pvrTimeToXrTime(m_cachedInputState.TimeInSeconds)
                                                            : xrAction.lastVector2fValueChangedTime;

        xrAction.lastVector2fValue = state->currentState;
        xrAction.lastVector2fValueChangedTime = state->lastChangeTime;

        TraceLoggingWrite(
            g_traceProvider,
            "xrGetActionStateVector2f",
            TLArg(!!state->isActive, "Active"),
            TLArg(fmt::format("{}, {}", state->currentState.x, state->currentState.y).c_str(), "CurrentState"),
            TLArg(!!state->changedSinceLastSync, "ChangedSinceLastSync"),
            TLArg(state->lastChangeTime, "LastChangeTime"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetActionStatePose
    XrResult OpenXrRuntime::xrGetActionStatePose(XrSession session,
                                                 const XrActionStateGetInfo* getInfo,
                                                 XrActionStatePose* state) {
        if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_POSE) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetActionStatePose",
                          TLXArg(session, "Session"),
                          TLXArg(getInfo->action, "Action"),
                          TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_actions.count(getInfo->action)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Action& xrAction = *(Action*)getInfo->action;

        if (!m_activeActionSets.count(xrAction.actionSet)) {
            return XR_ERROR_ACTIONSET_NOT_ATTACHED;
        }

        state->isActive = XR_FALSE;
        if (!xrAction.path.empty()) {
            if (xrAction.buttonMap || xrAction.floatValue || xrAction.vector2fValue) {
                return XR_ERROR_ACTION_TYPE_MISMATCH;
            }

            const std::string fullPath = getActionPath(xrAction, getInfo->subactionPath);
            const int side = getActionSide(fullPath);
            if (side < 0) {
                return XR_ERROR_PATH_INVALID;
            }

            state->isActive = m_isControllerActive[side];
        }

        TraceLoggingWrite(g_traceProvider, "xrGetActionStatePose", TLArg(!!state->isActive, "Active"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrSyncActions
    XrResult OpenXrRuntime::xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
        if (syncInfo->type != XR_TYPE_ACTIONS_SYNC_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrSyncActions", TLXArg(session, "Session"));
        for (uint32_t i = 0; i < syncInfo->countActiveActionSets; i++) {
            TraceLoggingWrite(g_traceProvider,
                              "xrSyncActions",
                              TLXArg(syncInfo->activeActionSets[i].actionSet, "ActionSet"),
                              TLArg(syncInfo->activeActionSets[i].subactionPath, "SubactionPath"));
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        for (uint32_t i = 0; i < syncInfo->countActiveActionSets; i++) {
            if (!m_activeActionSets.count(syncInfo->activeActionSets[i].actionSet)) {
                return XR_ERROR_ACTIONSET_NOT_ATTACHED;
            }

            m_frameLatchedActionSets.insert(syncInfo->activeActionSets[i].actionSet);

            // TODO: We do nothing with subActionPath.
        }

        // Latch the state of all inputs, and we will let the further calls to xrGetActionState*() do the triage.
        CHECK_PVRCMD(pvr_getInputState(m_pvrSession, &m_cachedInputState));
        for (uint32_t side = 0; side < 2; side++) {
            TraceLoggingWrite(
                g_traceProvider,
                "PVR_InputState",
                TLArg(side == 0 ? "Left" : "Right", "Side"),
                TLArg(m_cachedInputState.TimeInSeconds, "TimeInSeconds"),
                TLArg(m_cachedInputState.HandButtons[side], "ButtonPress"),
                TLArg(m_cachedInputState.HandTouches[side], "ButtonTouches"),
                TLArg(m_cachedInputState.Trigger[side], "Trigger"),
                TLArg(m_cachedInputState.Grip[side], "Grip"),
                TLArg(m_cachedInputState.GripForce[side], "GripForce"),
                TLArg(fmt::format("{}, {}", m_cachedInputState.JoyStick[side].x, m_cachedInputState.JoyStick[side].y)
                          .c_str(),
                      "Joystick"),
                TLArg(fmt::format("{}, {}", m_cachedInputState.TouchPad[side].x, m_cachedInputState.TouchPad[side].y)
                          .c_str(),
                      "Touchpad"),
                TLArg(m_cachedInputState.TouchPadForce[side], "TouchpadForce"),
                TLArg(m_cachedInputState.fingerIndex[side], "IndexFinger"),
                TLArg(m_cachedInputState.fingerMiddle[side], "MiddleFinger"),
                TLArg(m_cachedInputState.fingerRing[side], "RingFinger"),
                TLArg(m_cachedInputState.fingerPinky[side], "PinkyFinger"));

            const auto lastControllerType = m_cachedControllerType[side];
            const int size = pvr_getTrackedDeviceStringProperty(m_pvrSession,
                                                                side == 0 ? pvrTrackedDevice_LeftController
                                                                          : pvrTrackedDevice_RightController,
                                                                pvrTrackedDeviceProp_ControllerType_String,
                                                                nullptr,
                                                                0);
            m_isControllerActive[side] = size > 0;
            if (m_isControllerActive[side]) {
                m_cachedControllerType[side].resize(size, 0);
                pvr_getTrackedDeviceStringProperty(m_pvrSession,
                                                   side == 0 ? pvrTrackedDevice_LeftController
                                                             : pvrTrackedDevice_RightController,
                                                   pvrTrackedDeviceProp_ControllerType_String,
                                                   m_cachedControllerType[side].data(),
                                                   (int)m_cachedControllerType[side].size());
            } else {
                m_cachedControllerType[side].clear();
            }

            if (lastControllerType != m_cachedControllerType[side]) {
                rebindControllerActions(side);
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateBoundSourcesForAction
    XrResult OpenXrRuntime::xrEnumerateBoundSourcesForAction(XrSession session,
                                                             const XrBoundSourcesForActionEnumerateInfo* enumerateInfo,
                                                             uint32_t sourceCapacityInput,
                                                             uint32_t* sourceCountOutput,
                                                             XrPath* sources) {
        if (enumerateInfo->type != XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateBoundSourcesForAction",
                          TLXArg(session, "Session"),
                          TLXArg(enumerateInfo->action, "Action"),
                          TLArg(sourceCapacityInput, "SourceCapacityInput"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        *sourceCountOutput = 0;
        TraceLoggingWrite(
            g_traceProvider, "xrEnumerateBoundSourcesForAction", TLArg(*sourceCountOutput, "SourceCountOutput"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetInputSourceLocalizedName
    XrResult OpenXrRuntime::xrGetInputSourceLocalizedName(XrSession session,
                                                          const XrInputSourceLocalizedNameGetInfo* getInfo,
                                                          uint32_t bufferCapacityInput,
                                                          uint32_t* bufferCountOutput,
                                                          char* buffer) {
        if (getInfo->type != XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetInputSourceLocalizedName",
                          TLXArg(session, "Session"),
                          TLArg(getXrPath(getInfo->sourcePath).c_str(), "SourcePath"),
                          TLArg(getInfo->whichComponents, "WhichComponents"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        *bufferCountOutput = 0;
        TraceLoggingWrite(
            g_traceProvider, "xrGetInputSourceLocalizedName", TLArg(*bufferCountOutput, "BufferCountOutput"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrApplyHapticFeedback
    XrResult OpenXrRuntime::xrApplyHapticFeedback(XrSession session,
                                                  const XrHapticActionInfo* hapticActionInfo,
                                                  const XrHapticBaseHeader* hapticFeedback) {
        if (hapticActionInfo->type != XR_TYPE_HAPTIC_ACTION_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrApplyHapticFeedback",
                          TLXArg(session, "Session"),
                          TLXArg(hapticActionInfo->action, "Action"),
                          TLArg(getXrPath(hapticActionInfo->subactionPath).c_str(), "SubactionPath"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_actions.count(hapticActionInfo->action)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Action& xrAction = *(Action*)hapticActionInfo->action;

        if (!m_activeActionSets.count(xrAction.actionSet)) {
            return XR_ERROR_ACTIONSET_NOT_ATTACHED;
        }

        if (!xrAction.path.empty()) {
            const std::string fullPath = getActionPath(xrAction, hapticActionInfo->subactionPath);

            if (!endsWith(fullPath, "/output/haptic")) {
                return XR_ERROR_ACTION_TYPE_MISMATCH;
            }

            const int side = getActionSide(fullPath);
            if (side < 0) {
                return XR_ERROR_PATH_INVALID;
            }

            const XrHapticBaseHeader* entry = reinterpret_cast<const XrHapticBaseHeader*>(hapticFeedback);
            while (entry) {
                if (entry->type == XR_TYPE_HAPTIC_VIBRATION) {
                    const XrHapticVibration* vibration = reinterpret_cast<const XrHapticVibration*>(entry);

                    // TODO: What to do with frequency/duration?
                    CHECK_PVRCMD(pvr_triggerHapticPulse(m_pvrSession,
                                                        side == 0 ? pvrTrackedDevice_LeftController
                                                                  : pvrTrackedDevice_RightController,
                                                        vibration->amplitude));
                    break;
                }

                entry = reinterpret_cast<const XrHapticBaseHeader*>(entry->next);
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrStopHapticFeedback
    XrResult OpenXrRuntime::xrStopHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo) {
        if (hapticActionInfo->type != XR_TYPE_HAPTIC_ACTION_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrStopHapticFeedback",
                          TLXArg(session, "Session"),
                          TLXArg(hapticActionInfo->action, "Action"),
                          TLArg(getXrPath(hapticActionInfo->subactionPath).c_str(), "SubactionPath"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_actions.count(hapticActionInfo->action)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Action& xrAction = *(Action*)hapticActionInfo->action;

        if (!m_activeActionSets.count(xrAction.actionSet)) {
            return XR_ERROR_ACTIONSET_NOT_ATTACHED;
        }

        if (!xrAction.path.empty()) {
            const std::string fullPath = getActionPath(xrAction, hapticActionInfo->subactionPath);

            if (!endsWith(fullPath, "/output/haptic")) {
                return XR_ERROR_ACTION_TYPE_MISMATCH;
            }

            const int side = getActionSide(fullPath);
            if (side < 0) {
                return XR_ERROR_PATH_INVALID;
            }

            // Nothing to do here.
        }

        return XR_SUCCESS;
    }

    void OpenXrRuntime::rebindControllerActions(int side) {
        std::string preferredInteractionProfile;
        std::string actualInteractionProfile;

        std::function<void(Action&, XrPath)> mapping;

        // Identified the physical controller type.
        if (m_cachedControllerType[side] == "vive_controller") {
            preferredInteractionProfile = "/interaction_profiles/htc/vive_controller";
            mapping = [&](Action& xrAction, XrPath binding) { mapPathToViveControllerInputState(xrAction, binding); };
        } else if (m_cachedControllerType[side] == "index_controller") {
            preferredInteractionProfile = "/interaction_profiles/valve/index_controller";
            mapping = [&](Action& xrAction, XrPath binding) { mapPathToIndexControllerInputState(xrAction, binding); };
        } else {
            // Fallback to simple controller.
            preferredInteractionProfile = "/interaction_profiles/khr/simple_controller";
            mapping = [&](Action& xrAction, XrPath binding) { mapPathToSimpleControllerInputState(xrAction, binding); };
        }

        // Try to map with the preferred bindings.
        auto bindings = m_suggestedBindings.find(preferredInteractionProfile);
        if (bindings == m_suggestedBindings.cend()) {
            // Fallback to simple controller.
            preferredInteractionProfile = "/interaction_profiles/khr/simple_controller";
            bindings = m_suggestedBindings.find(preferredInteractionProfile);
        }

        if (bindings != m_suggestedBindings.cend()) {
            for (const auto& binding : bindings->second) {
                if (!m_actions.count(binding.action)) {
                    // TODO: I don't think we should allow xrDestroyAction() while it's in an active actionset?
                    continue;
                }

                Action& xrAction = *(Action*)binding.action;

                // Map to the PVR input state.
                mapping(xrAction, binding.binding);
            }

            actualInteractionProfile = preferredInteractionProfile;
        } else {
            // TODO: Create mappings for compatible bindings in case the application does not provide matching bindings.
        }

        CHECK_XRCMD(
            xrStringToPath(XR_NULL_HANDLE, actualInteractionProfile.c_str(), &m_currentInteractionProfile[side]));

        m_currentInteractionProfileDirty = true;
    }

    void OpenXrRuntime::mapPathToViveControllerInputState(Action& xrAction, XrPath binding) const {
        const auto path = getXrPath(binding);

        xrAction.buttonMap = nullptr;
        xrAction.floatValue = nullptr;
        xrAction.vector2fValue = nullptr;

        if (endsWith(path, "/input/system/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/squeeze/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_Grip;
        } else if (endsWith(path, "/input/menu/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_ApplicationMenu;
        } else if (endsWith(path, "/input/trigger/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/trigger/value")) {
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
        } else if (endsWith(path, "/input/trackpad/click")) {
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

        if (endsWith(path, "/input/system/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/system/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_System;
        } else if (endsWith(path, "/input/a/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_A;
        } else if (endsWith(path, "/input/a/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_A;
        } else if (endsWith(path, "/input/b/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_B;
        } else if (endsWith(path, "/input/b/touch")) {
            xrAction.buttonMap = m_cachedInputState.HandTouches;
            xrAction.buttonType = pvrButton_B;
        } else if (endsWith(path, "/input/squeeze/value")) {
            xrAction.floatValue = m_cachedInputState.Grip;
        } else if (endsWith(path, "/input/squeeze/force")) {
            xrAction.floatValue = m_cachedInputState.GripForce;
        } else if (endsWith(path, "/input/trigger/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/trigger/value")) {
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
        } else if (endsWith(path, "/input/thumbstick/click")) {
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

        if (endsWith(path, "/input/select/click")) {
            xrAction.buttonMap = m_cachedInputState.HandButtons;
            xrAction.buttonType = pvrButton_Trigger;
        } else if (endsWith(path, "/input/menu/click")) {
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

    std::string OpenXrRuntime::getXrPath(XrPath path) const {
        if (path == XR_NULL_PATH) {
            return "";
        }

        const auto it = m_strings.find(path);
        if (it == m_strings.cend()) {
            return "<unknown>";
        }

        return it->second;
    }

    std::string OpenXrRuntime::getActionPath(const Action& xrAction, XrPath subActionPath) const {
        std::string path;
        if (subActionPath != XR_NULL_PATH) {
            path = getXrPath(subActionPath);
        }

        if (!path.empty() && !endsWith(path, "/") && !startsWith(xrAction.path, "/")) {
            path += "/";
        }

        path += xrAction.path;

        return path;
    }

    int OpenXrRuntime::getActionSide(const std::string& fullPath) const {
        if (startsWith(fullPath, "/user/hand/left/")) {
            return 0;
        } else if (startsWith(fullPath, "/user/hand/right/")) {
            return 1;
        }

        return -1;
    }

} // namespace pimax_openxr

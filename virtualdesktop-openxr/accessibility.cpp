// MIT License
//
// Copyright(c) 2025 Microsoft Corp.
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

#include "accessibility.h"
#include "log.h"
#include "runtime.h"
#include "utils.h"

// Implements hooks to connect accessibility devices.

namespace {

    using namespace virtualdesktop_openxr;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    struct EmulatedControllerState {
        bool enabled = false;
    };

    class AccessibilityHelperImpl : public AccessibilityHelper {
      public:
        AccessibilityHelperImpl(ovrSession ovrSession) : m_ovrSession(ovrSession) {
            // TODO: For testing, until we have a config file.
            m_controllerState[0].enabled = m_controllerState[1].enabled = true;
        }

        bool IsControllerEmulated(xr::side_t side) const override {
            if (side >= xr::Side::Count) {
                return false;
            }

            return m_controllerState[side].enabled;
        }

        bool GetEmulatedDevicePose(xr::side_t side, double absTime, ovrPoseStatef* outDevicePose) override {
            if (side >= xr::Side::Count) {
                return false;
            }

            ZeroMemory(outDevicePose, sizeof(ovrPoseStatef));

            // TODO: Nothing here yet, just a quick demo of how to put the virtual controllers in front of the user.

            // Get the head pose.
            ovrPoseStatef state{};
            ovrTrackedDeviceType hmd = ovrTrackedDevice_HMD;
            const auto result = ovr_GetDevicePoses(m_ovrSession, &hmd, 1, absTime, &state);

            const auto headPose = ovrPoseToXrPose(state.ThePose);

            // Left or right 15cm, below 10cm, in front 35cm.
            const auto inFront =
                Pose::MakePose(XrVector3f{side == xr::Side::Left ? -0.15f : 0.15f, -0.1f, -0.35f}, XrVector3f{0, 0, 0});

            const auto transformedPose = inFront * headPose;

            outDevicePose->ThePose = xrPoseToOvrPose(transformedPose);
            outDevicePose->TimeInSeconds = absTime;

            return true;
        }

        bool GetEmulatedInputState(xr::side_t side, ovrInputState* outInputState) override {
            if (side >= xr::Side::Count) {
                return false;
            }

            // This structure holds the state for both controller buttons, but the caller will recombine the state
            // correctly.
            ZeroMemory(outInputState, sizeof(ovrInputState));

            // TODO: Nothing here yet.

            return true;
        }

      private:
        const ovrSession m_ovrSession;
        EmulatedControllerState m_controllerState[xr::Side::Count];
    };

} // namespace

namespace virtualdesktop_openxr {

    std::unique_ptr<AccessibilityHelper> CreateAccessibilityHelper(ovrSession ovrSession) {
        return std::make_unique<AccessibilityHelperImpl>(ovrSession);
    }

} // namespace virtualdesktop_openxr

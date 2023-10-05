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

// Implements the foundations of eye tracking needed for various extensions.

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    bool OpenXrRuntime::getEyeGaze(XrTime time, bool getStateOnly, XrVector3f& unitVector, double& sampleTime) const {
        if (!m_isEyeTrackingAvailable) {
            return false;
        }

        if (m_eyeTrackingType == EyeTracking::Simulated) {
            XrVector2f point{0.5f, 0.5f};

            // Use the mouse to simulate eye tracking.
            RECT rect;
            rect.left = 1;
            rect.right = 999;
            rect.top = 1;
            rect.bottom = 999;
            ClipCursor(&rect);

            POINT pt{};
            GetCursorPos(&pt);

            point = {(float)pt.x / 1000.f, (1000.f - pt.y) / 1000.f};
            sampleTime = pvr_getTimeSeconds(m_pvr);

            unitVector = Normalize({point.x - 0.5f, 0.5f - point.y, -0.35f});

        } else {
            return false;
        }

        return true;
    }

} // namespace virtualdesktop_openxr

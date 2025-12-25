// MIT License
//
// Copyright(c) 2025 Microsoft Corp.
// Initial implementation by Matthieu Bucchianeri, Jonas Holderman and Heather Kemp.
// Copyright(c) 2025 Matthieu Bucchianeri
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

#pragma once

namespace virtualdesktop_openxr {

    struct AccessibilityHelper {
        virtual ~AccessibilityHelper() = default;

        virtual bool IsControllerEmulated(xr::side_t side) const = 0;
        virtual bool GetEmulatedDevicePose(xr::side_t side, double absTime, ovrPoseStatef* outDevicePose) = 0;
        virtual bool GetEmulatedInputState(xr::side_t side, ovrInputState* outInputState) = 0;
        virtual void SendEmulatedHapticPulse(xr::side_t side, float frequency, float amplitude) = 0;

        virtual void SetOpenXrPoses(xr::side_t side, const XrPosef& rawToGrip, const XrPosef& rawToAim) = 0;
    };

    std::unique_ptr<AccessibilityHelper> CreateAccessibilityHelper(ovrSession ovrSession,
                                                                   const std::wstring& configPath,
                                                                   const std::string& applicationName,
                                                                   const std::string& exeName);

} // namespace virtualdesktop_openxr

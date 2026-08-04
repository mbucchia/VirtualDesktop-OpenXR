// MIT License
//
// Copyright(c) 2025-2026 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
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

#include "pch.h"

namespace ovrnull::driver {

    struct IDriver {
        virtual ~IDriver() = default;

        virtual void SetSubmissionDevice(ID3D11Device* device) = 0;

        virtual void* CreateSwapchain(const ovrTextureSwapChainDesc& ovrDesc) = 0;
        virtual void DestroySwapchain(void* swapchain) = 0;
        virtual ovrTextureSwapChainDesc GetSwapchainDesc(void* swapchain) const = 0;
        virtual ID3D11Texture2D* GetSwapchainImage(void* swapchain, int index) const = 0;
        virtual int GetSwapchainImageIndex(void* swapchain) const = 0;
        virtual bool CommitSwapchainImage(void* swapchain) const = 0;
        virtual void SetMirrorTexture(void* swapchain) = 0;

        virtual double GetPredictedDisplayTime(long long frameIndex) const = 0;
        virtual void WaitForVsync(long long frameIndex) = 0;
        virtual bool SubmitFrame(const std::vector<const ovrLayerHeader*>& layers) = 0;

        virtual std::string GetManufacturerName() const = 0;
        virtual std::string GetModelNumber() const = 0;
        virtual LUID GetAdapterLuid() const = 0;
        virtual float GetDisplayRate() const = 0;
        virtual ovrSizei GetRecommendedResolution() const = 0;
        virtual ovrFovPort GetEyeFov(ovrEyeType eye) const = 0;
        virtual ovrPosef GetEyePose(ovrEyeType eye) const = 0;
        virtual void SetStageTracking(bool useStage) = 0;
        virtual bool IsStageTracking() const = 0;
        virtual float GetEyeHeight() const = 0;
        virtual bool IsAswActive() const = 0;

        virtual ovrPoseStatef GetHmdPose(double absTime) const = 0;

        virtual bool HasController(ovrHandType side) const = 0;
        virtual ovrPoseStatef GetControllerPose(ovrHandType side, double absTime) const = 0;
        virtual ovrInputState GetControllerButtons() const = 0;
        virtual void SetControllerVibration(ovrHandType side, float frequency, float amplitude) = 0;
    };

    IDriver* CreateDriver();

} // namespace ovrnull::driver

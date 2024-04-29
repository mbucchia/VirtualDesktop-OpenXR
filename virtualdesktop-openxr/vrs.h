// MIT License
//
// Copyright(c) 2024 Matthieu Bucchianeri
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

#include <cmath>

namespace vrs {

    enum RateComponent { _1 = 1, _2 = 2, _4 = 4 };

    struct Rate {
        RateComponent X;
        RateComponent Y;
    };

    struct Parameters {
        // TODO: Make an ellipsis instead.
        float InnerRing;
        float OuterRing;
        Rate InnerRate;
        Rate MiddleRate;
        Rate OuterRate;
    };

    struct Resolution {
        UINT Width{0};
        UINT Height{0};
    };

    // Defines the criteria for enabling VRS for a given render pass.
    static bool IsViewportEligible(const Resolution& PresentResolution, const Resolution& ViewportResolution) {
        if (!ViewportResolution.Width || !ViewportResolution.Height) {
            return false;
        }

        const double targetAspectRatio = static_cast<double>(PresentResolution.Height) / PresentResolution.Width;
        const double viewportAspectRatio = static_cast<double>(ViewportResolution.Height) / ViewportResolution.Width;
        const double scaleOfTarget = static_cast<double>(ViewportResolution.Width) / PresentResolution.Width;

        return std::abs(targetAspectRatio - viewportAspectRatio) < 0.0001 &&
               scaleOfTarget >= 0.32; // DLSS/FSR "Ultra Performance" might render at 33% of the final resolution.
    }

    void InstallD3D11Hooks(ID3D11Device* device, const Resolution& presentResolution);
    void InstallD3D12Hooks(ID3D12Device* device, const Resolution& presentResolution);

    void UninstallD3D11Hooks();
    void UninstallD3D12Hooks();

    void SetStateD3D11(bool state, std::optional<Parameters> parameters = std::nullopt);
    void SetStateD3D12(bool state, std::optional<Parameters> parameters = std::nullopt);

    // Must be called periodically to perform clean up.
    void NewFrameD3D11();
    void NewFrameD3D12();

} // namespace vrs

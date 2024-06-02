// MIT License
//
// Copyright(c) 2022-2024 Matthieu Bucchianeri
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

// Clear or set the alpha channel and/or premultiply each component.

#include "Common.hlsli"

cbuffer config : register(b0)
{
    uint2 offset;
    uint2 dimension;
    bool ignoreAlpha;
    bool isPremultipliedAlpha;
    bool isSRGB;
    float smoothingArea;
};

RWTexture2D<unorm float4> inoutTexture : register(u0);

[numthreads(32, 32, 1)]
void main(uint2 pos : SV_DispatchThreadID)
{
    if (any(pos > dimension))
    {
        return;
    }

    uint2 surfacePos = offset + pos;

    float4 output = inoutTexture[surfacePos];

    // Apply transforms requested by OpenXR (part 1).
    if (ignoreAlpha)
    {
        output.a = 1;
    }

    if (smoothingArea || !isPremultipliedAlpha)
    {
        if (isSRGB)
        {
            output.rgb = FromSRGB(output.rgb);
        }

        if (isPremultipliedAlpha)
        {
            output = UnpremultiplyAlpha(output);
        }

        // Smooth the focus view (quad views only).
        if (smoothingArea)
        {
            float2 uv = float2(pos) / dimension;
            float2 s = smoothstep(float2(0, 0), float2(smoothingArea, smoothingArea), uv) -
                   smoothstep(float2(1, 1) - float2(smoothingArea, smoothingArea), float2(1, 1), uv);
            output.a *= max(0.5, s.x * s.y);
        }

        // OVR always expects premultiplied alpha.
        output = PremultiplyAlpha(output);

        if (isSRGB)
        {
            output.rgb = ToSRGB(output.rgb);
        }
    }

    inoutTexture[surfacePos] = output;
}

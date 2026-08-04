// MIT License
//
// Copyright(c) 2025-2026 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
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

#include "constantsbuffer.h"
#include "utils.hlsli"

cbuffer cb : register(b0)
{
    ConstantsBuffer cb;
};

SamplerState linearSampler : register(s0);

Texture2D<float4> sourceColor : register(t0);

float4 main(in float4 position : SV_Position, in float3 reprojectedCoord : TEXCOORD0) : SV_Target
{
    float2 reprojectedNdc = reprojectedCoord.xy / reprojectedCoord.z;
    if (any(abs(reprojectedNdc) > 1.0))
    {
        discard;
    }

    float2 texCoord = NdcToTexCoord(reprojectedNdc);
    float2 realTexCoord = ApplyImageRect(texCoord, cb.imageRectNormalized, cb.flipY);

    float4 color = sourceColor.SampleLevel(linearSampler, realTexCoord, 0);

    return PreMultiplyAlpha(color);
}

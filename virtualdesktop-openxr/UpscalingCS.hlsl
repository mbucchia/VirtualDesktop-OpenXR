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

// Perform FSR1 upscaling.

#include "Common.hlsli"

cbuffer config : register(b0)
{
    float2 topLeftNormalized;
    bool isSRGB;
    uint padding;
    uint4 const0; // FSR
    uint4 const1; // FSR
    uint4 const2; // FSR
    uint4 const3; // FSR
};

SamplerState linearClamp : register(s0);

Texture2D<float4> sourceTexture : register(t0);
RWTexture2D<float4> upscaledTexture : register(u0);

#define A_GPU 1
#define A_HLSL 1

#include <ffx_a.h>

#define FSR_EASU_F 1

AF4 FsrEasuRF(AF2 p)
{
    p += topLeftNormalized;
    return sourceTexture.GatherRed(linearClamp, p, int2(0, 0));
}
AF4 FsrEasuGF(AF2 p)
{
    p += topLeftNormalized;
    return sourceTexture.GatherGreen(linearClamp, p, int2(0, 0));
}
AF4 FsrEasuBF(AF2 p)
{
    p += topLeftNormalized;
    return sourceTexture.GatherBlue(linearClamp, p, int2(0, 0));
}

#include <ffx_fsr1.h>

void FsrStore(AU2 p, AF3 c)
{
    if (isSRGB)
    {
        c = ToSRGB(c);
    }

    // TODO: We don't passthrough alpha channel. This is typically OK since we should be layer 0.
    upscaledTexture[p] = AF4(c, 1);
}

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 wgid : SV_GroupID)
{
    // Do remapping of local xy in workgroup for a more PS-like swizzle pattern.
    AU2 gxy = ARmp8x8(tid.x) + AU2(wgid.x << 4u, wgid.y << 4u);

    AF3 c;

    FsrEasuF(c, gxy, const0, const1, const2, const3);
    FsrStore(gxy, c);
    gxy.x += 8u;

    FsrEasuF(c, gxy, const0, const1, const2, const3);
    FsrStore(gxy, c);
    gxy.y += 8u;

    FsrEasuF(c, gxy, const0, const1, const2, const3);
    FsrStore(gxy, c);
    gxy.x -= 8u;

    FsrEasuF(c, gxy, const0, const1, const2, const3);
    FsrStore(gxy, c);
}

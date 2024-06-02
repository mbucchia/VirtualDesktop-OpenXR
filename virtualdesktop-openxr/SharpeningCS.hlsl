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

// Apply CAS sharpening.

#include "Common.hlsli"

cbuffer config : register(b0)
{
    uint2 topLeft;
    bool isSRGB;
    uint padding;
    uint4 const0; // CAS
    uint4 const1; // CAS
};

Texture2D<float4> sourceTexture : register(t0);
RWTexture2D<float4> sharpenedTexture : register(u0);

#define A_GPU 1
#define A_HLSL 1

#include <ffx_a.h>

AF3 CasLoad(ASU2 p)
{
    p += topLeft.xy;
    return sourceTexture.Load(int3(p, 0)).rgb;
}

void CasInput(inout AF1 r, inout AF1 g, inout AF1 b)
{
}

#include <ffx_cas.h>

void CasStore(AU2 p, AF3 c)
{
    if (isSRGB)
    {
        c = ToSRGB(c);
    }

    // TODO: We don't passthrough alpha channel. This is typically OK since we should be layer 0.
    sharpenedTexture[p] = AF4(c, 1);
}

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 wgid : SV_GroupID)
{
    // Do remapping of local xy in workgroup for a more PS-like swizzle pattern.
    AU2 gxy = ARmp8x8(tid.x) + AU2(wgid.x << 4u, wgid.y << 4u);

    AF3 c;

    CasFilter(c.r, c.g, c.b, gxy, const0, const1, true /* noScaling */);
    CasStore(gxy, c);
    gxy.x += 8u;

    CasFilter(c.r, c.g, c.b, gxy, const0, const1, true /* noScaling */);
    CasStore(gxy, c);
    gxy.y += 8u;

    CasFilter(c.r, c.g, c.b, gxy, const0, const1, true /* noScaling */);
    CasStore(gxy, c);
    gxy.x -= 8u;

    CasFilter(c.r, c.g, c.b, gxy, const0, const1, true /* noScaling */);
    CasStore(gxy, c);
}

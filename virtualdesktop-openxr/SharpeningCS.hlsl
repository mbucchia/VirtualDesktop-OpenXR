// Apply CAS sharpening.
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
        // From https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli
        c = c < 0.0031308 ? 12.92 * c : 1.13005 * sqrt(c - 0.00228) - 0.13448 * c + 0.005719;
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

// Perform FSR1 upscaling.
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
        // From https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli
        c = c < 0.0031308 ? 12.92 * c : 1.13005 * sqrt(c - 0.00228) - 0.13448 * c + 0.005719;
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

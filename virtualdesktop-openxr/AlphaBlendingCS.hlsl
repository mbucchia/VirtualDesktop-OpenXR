// Clear or set the alpha channel and/or premultiply each component.

cbuffer config : register(b0)
{
    uint2 offset;
    uint2 dimension;
    bool ignoreAlpha;
    bool isUnpremultipliedAlpha;
    bool isSRGB;
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

    // Apply transforms requested by OpenXR.
    if (ignoreAlpha)
    {
        output.a = 1;
    }
    if (isUnpremultipliedAlpha)
    {
        float3 c = output.rgb;
        if (isSRGB)
        {
            // From https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli
            c = c < 0.04045 ? c / 12.92 : -7.43605 * c - 31.24297 * sqrt(-0.53792 * c + 1.279924) + 35.34864;
        }
        c *= output.a;
        if (isSRGB)
        {
            c = c < 0.0031308 ? 12.92 * c : 1.13005 * sqrt(c - 0.00228) - 0.13448 * c + 0.005719;
        }
        output.rgb = c;
    }

    inoutTexture[surfacePos] = output;
}

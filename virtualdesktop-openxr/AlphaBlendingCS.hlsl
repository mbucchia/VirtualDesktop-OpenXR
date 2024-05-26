// Clear or set the alpha channel and/or premultiply each component.

cbuffer config : register(b0)
{
    uint2 offset;
    uint2 dimension;
    bool ignoreAlpha;
    bool isUnpremultipliedAlpha;
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

    // Apply transforms requested by OpenXR.
    if (ignoreAlpha)
    {
        output.a = 1;
    }
    if (isUnpremultipliedAlpha)
    {
        output.rgb = output.rgb * output.a;
    }

    // Smooth the focus view (quad views only).
    if (smoothingArea)
    {
        float2 uv = float2(pos) / dimension;
        float2 s = smoothstep(float2(0, 0), float2(smoothingArea, smoothingArea), uv) -
                   smoothstep(float2(1, 1) - float2(smoothingArea, smoothingArea), float2(1, 1), uv);
        output.a *= max(0.5, s.x * s.y);
    }
    output.r = 1;

    inoutTexture[surfacePos] = output;
}

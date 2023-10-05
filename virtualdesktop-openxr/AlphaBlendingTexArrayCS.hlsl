// Clear or set the alpha channel and/or premultiply each component.

#include "AlphaBlending.hlsli"

cbuffer config : register(b0) {
    bool ignoreAlpha;
    bool isUnpremultipliedAlpha;
};

Texture2DArray in_texture : register(t0);
RWTexture2D<float4> out_texture : register(u0);

[numthreads(32, 32, 1)]
void main(uint2 pos : SV_DispatchThreadID) {
    uint width, height, size;
    in_texture.GetDimensions(width, height, size);
    out_texture[pos] = processAlpha(
        in_texture[float3(pos, 0)], pos, uint2(width, height), ignoreAlpha, isUnpremultipliedAlpha);
}

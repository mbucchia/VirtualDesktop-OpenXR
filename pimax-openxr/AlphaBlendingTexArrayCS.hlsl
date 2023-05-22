// Clear or set the alpha channel and/or premultiply each component.

#include "AlphaBlending.hlsli"

cbuffer config : register(b0) {
    int mode;
};

Texture2DArray in_texture : register(t0);
RWTexture2D<float4> out_texture : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 pos : SV_DispatchThreadID) {
    out_texture[pos] = processAlpha(in_texture[float3(pos, 0)], mode);
}

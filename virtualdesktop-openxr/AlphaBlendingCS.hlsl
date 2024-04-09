// Clear or set the alpha channel and/or premultiply each component.

#include "AlphaBlending.hlsli"

cbuffer config : register(b0) {
    bool ignoreAlpha;
    bool isUnpremultipliedAlpha;
};

RWTexture2D<unorm float4> inoutTexture : register(u0);

[numthreads(32, 32, 1)]
void main(uint2 pos : SV_DispatchThreadID) {
    inoutTexture[pos] = processAlpha(inoutTexture[pos], pos, ignoreAlpha, isUnpremultipliedAlpha);
}

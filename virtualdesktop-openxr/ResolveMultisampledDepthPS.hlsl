// A shader to resolve multisampled depth into single samples.
SamplerState sourceSampler : register(s0);

cbuffer config : register(b0)
{
    uint slice;
};

Texture2DMSArray<float> sourceDepthMS : register(t0);

float main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD0) : SV_Depth
{
    uint width, height, slices, samples;
    sourceDepthMS.GetDimensions(width, height, slices, samples);

    float depth = 0;
    for (uint i = 0; i < samples; ++i)
    {
        float sampleDepth = sourceDepthMS.Load(uint3(position.x, position.y, slice), i).x;
        depth = max(depth, sampleDepth);
    }
    return depth;
}

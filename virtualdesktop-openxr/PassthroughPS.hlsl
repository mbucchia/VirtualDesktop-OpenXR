// A simple passthrough shader (for SRGB color conversion).
SamplerState sourceSampler : register(s0);
Texture2D sourceTexture : register(t0);

float4 main(in float4 position : SV_POSITION, in float2 texcoord : TEXCOORD0) : SV_TARGET {
    return sourceTexture.Sample(sourceSampler, texcoord);
}

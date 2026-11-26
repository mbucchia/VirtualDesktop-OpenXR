// Green-screen key with linear premultiplied output.
// The input is desktop-encoded BGRA UNORM; the destination RTV is sRGB.
Texture2D captureTexture : register(t0);

float3 SrgbToLinear(float3 c) {
    return lerp(pow(abs((c + 0.055) / 1.055), 2.4), c / 12.92, step(c, 0.04045));
}

float4 main(float4 position : SV_POSITION) : SV_TARGET {
    float3 color = captureTexture.Load(int3(int2(position.xy), 0)).rgb;
    float strongestOther = max(color.r, color.b);
    float keyedAmount = smoothstep(0.08, 0.28, color.g - strongestOther) *
                        smoothstep(0.10, 0.32, color.g);
    float alpha = 1.0 - keyedAmount;
    color.g = lerp(color.g, min(color.g, strongestOther), keyedAmount);
    return float4(SrgbToLinear(color) * alpha, alpha);
}

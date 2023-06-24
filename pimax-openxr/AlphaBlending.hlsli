float4 processAlpha(float4 input, uint2 pos, uint2 widthHeight, bool ignoreAlpha, bool isUnpremultipliedAlpha, bool isFocusView) {
    float4 output = input;

    if (ignoreAlpha) {
        output.a = 1;
    }
    if (isFocusView) {
        float2 transitionArea = 0.1 * widthHeight;
        float2 s = smoothstep(float2(0, 0), transitionArea, pos) -
                   smoothstep(widthHeight - transitionArea, widthHeight, pos);
        output.a = max(0, s.x * s.y);
    }
    if (isUnpremultipliedAlpha) {
        output.rgb = output.rgb * output.a;
    }
    return output;
}

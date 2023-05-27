float4 processAlpha(float4 input, uint2 pos, uint2 widthHeight, int mode) {
    float4 output = input;
    if (mode & 1) {
        output.a = 1;
    }
    if (mode & 4) {
        float2 transitionArea = 0.1 * widthHeight;
        float2 s = smoothstep(float2(0, 0), transitionArea, pos) -
                   smoothstep(widthHeight - transitionArea, widthHeight, pos);
        output.a = max(0.5, s.x * s.y);
        // RGB need premultiplication (below).
    }
    if (mode & 6) {
        output.rgb = output.rgb * output.a;
    }
    return output;
}

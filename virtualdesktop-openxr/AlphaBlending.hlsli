float4 processAlpha(float4 input, uint2 pos, bool ignoreAlpha, bool isUnpremultipliedAlpha) {
    float4 output = input;

    if (ignoreAlpha) {
        output.a = 1;
    }
    if (isUnpremultipliedAlpha) {
        output.rgb = output.rgb * output.a;
    }
    return output;
}

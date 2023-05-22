float4 processAlpha(float4 input, int mode) {
    float4 output = input;
    if (mode & 1) {
        output.a = 1;
    }
    if (mode & 2) {
        output.rgb = output.rgb * output.a;
    }
    return output;
}

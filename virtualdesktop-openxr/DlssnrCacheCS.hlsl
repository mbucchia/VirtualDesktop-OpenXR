// MIT License
//
// Copyright(c) 2026 VirtualDesktop-OpenXR contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// DLSS-NR correction cache, reduced NR working resolution and debug overlay.
// Mode 0: store the NR correction (NR output - NR input).
// Mode 1: apply a stored correction to a new input (bilinear when NR ran at a reduced resolution).
// Mode 2: draw the outline of the foveated region (debug).
// Mode 3: downscale the foveated region to NR's working resolution.
// Mode 4: fade NR's output into the input over the region's edge (the transition width).

cbuffer config : register(b0)
{
    int2 srcOffset;   // Top-left of the foveated region in the input texture.
    int2 dstOffset;   // Top-left of the foveated region in the output texture.
    int2 size;        // Size of the region processed (full-size, or NR's working size for modes 0 and 3).
    int2 corrShift;   // Mode 1: full-size pixels to add to a pixel before reading the correction (head rotation).
    uint mode;
    uint inputIsLinear; // The input view decodes sRGB: re-encode so input and output values match.
    uint outputIsUnorm;
    uint borderWidth;
    float4 borderColor;
    float correctionScale; // NR working size / full size (1 at full resolution).
    int2 correctionSize;   // Size of the stored correction.
    uint padding; // Mode 4: the transition width (asfloat), a fraction of the region.
};

Texture2DArray<float4> inputTexture : register(t0);  // one slice
Texture2D<float4> outputSnapshot : register(t1);
RWTexture2D<float4> outputTexture : register(u0);
RWTexture2D<float4> correctionTexture : register(u1);

float3 LinearToSrgb(float3 color)
{
    color = saturate(color);
    const float3 low = color * 12.92;
    const float3 high = 1.055 * pow(color, 1.0 / 2.4) - 0.055;
    return lerp(high, low, step(color, 0.0031308));
}

float4 LoadInput(int2 pixel)
{
    float4 value = inputTexture.Load(int4(pixel, 0, 0));
    if (inputIsLinear) {
        value.rgb = LinearToSrgb(value.rgb);
    }
    return value;
}

float3 LoadCorrection(int2 texel)
{
    return correctionTexture[(uint2)clamp(texel, 0, correctionSize - 1)].rgb;
}

[numthreads(8, 8, 1)]
void main(uint2 id : SV_DispatchThreadID)
{
    if (id.x >= (uint)size.x || id.y >= (uint)size.y) {
        return;
    }

    const int2 dst = dstOffset + (int2)id;

    if (mode == 2) {
        if (id.x < borderWidth || id.y < borderWidth || id.x + borderWidth >= (uint)size.x ||
            id.y + borderWidth >= (uint)size.y) {
            outputTexture[dst] = borderColor;
        }
        return;
    }

    if (mode == 3) {
        // Average of the full-size pixels under this working pixel (up to 4x4).
        const float2 first = (float2)id / correctionScale;
        const float2 last = (float2)(id + 1) / correctionScale;
        const int2 begin = (int2)floor(first);
        const int2 end = min((int2)ceil(last), begin + 4);
        float4 sum = 0;
        float count = 0;
        for (int y = begin.y; y < end.y; y++) {
            for (int x = begin.x; x < end.x; x++) {
                sum += LoadInput(srcOffset + int2(x, y));
                count += 1;
            }
        }
        outputTexture[dst] = sum / max(count, 1.0);
        return;
    }

    const float4 src = LoadInput(srcOffset + (int2)id);

    if (mode == 4) {
        const float transition = asfloat(padding);
        const float2 uv = ((float2)id + 0.5) / (float2)size;
        const float2 edge = min(uv, 1.0 - uv);
        const float weight = smoothstep(0.0, transition, min(edge.x, edge.y));
        float4 result = lerp(src, outputTexture[dst], weight);
        if (outputIsUnorm) {
            result = saturate(result);
        }
        outputTexture[dst] = result;
        return;
    }

    if (mode == 0) {
        correctionTexture[id] = outputSnapshot.Load(int3(dst, 0)) - src;
    } else {
        // Where this pixel's content is in the stored correction.
        const float2 position = ((float2)id + 0.5 + (float2)corrShift) * correctionScale - 0.5;
        float3 correction = 0;
        if (all(position >= -0.5) && all(position <= (float2)correctionSize - 0.5)) {
            const int2 i0 = (int2)floor(position);
            const float2 f = position - (float2)i0;
            const float3 top = lerp(LoadCorrection(i0), LoadCorrection(i0 + int2(1, 0)), f.x);
            const float3 bottom = lerp(LoadCorrection(i0 + int2(0, 1)), LoadCorrection(i0 + int2(1, 1)), f.x);
            correction = lerp(top, bottom, f.y);
        }
        float4 result = float4(src.rgb + correction, src.a);
        if (outputIsUnorm) {
            result = saturate(result);
        }
        outputTexture[dst] = result;
    }
}

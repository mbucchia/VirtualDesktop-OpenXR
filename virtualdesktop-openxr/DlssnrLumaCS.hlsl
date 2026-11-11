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

// Optical flow input: the luminance of one eye's image at a quarter of its size,
// each texel the average of a 4x4 block.

#include "DlssnrMotion.hlsli"

Texture2DArray<float4> colorTexture : register(t0);  // one slice
RWTexture2D<float> luma : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 id : SV_DispatchThreadID)
{
    const int2 q = (int2)id;
    if (any(q >= Blocks())) {
        return;
    }
    float3 sum = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            const int2 pixel = min(q * 4 + int2(x, y), (int2)size - 1) + origin;
            sum += colorTexture.Load(int4(pixel, 0, 0)).rgb;
        }
    }
    const float value = dot(sum / 16.0, float3(0.2126, 0.7152, 0.0722));
    // A perceptual curve, so dark scenes still match.
    luma[q] = sqrt(max(value, 0.0));
}

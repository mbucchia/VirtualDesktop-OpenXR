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

// Optical flow at a quarter of the eye's image: each block starts from where head
// rotation alone moves it, then a search around that prediction in the previous
// luminance finds what locomotion, head translation and moving objects add.
// Output: full-size pixels from the current sample to its previous position.

#include "DlssnrMotion.hlsli"

RWTexture2D<float> currentLuma : register(u0);
RWTexture2D<float> previousLuma : register(u1);
RWTexture2D<float2> flow : register(u2);

float SampleCurrent(int2 q)
{
    return currentLuma[clamp(q, 0, Blocks() - 1)];
}

float SamplePrevious(int2 q)
{
    return previousLuma[clamp(q, 0, Blocks() - 1)];
}

// Sum of absolute differences of a 3x3 neighbourhood (mean removed, so a change
// of exposure does not move it). `step` 2 is a wider patch for the coarse search.
float Cost(int2 q, int2 r, float meanCurrent, int step)
{
    float meanPrevious = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            meanPrevious += SamplePrevious(r + step * int2(x, y));
        }
    }
    meanPrevious /= 9.0;
    float sum = 0;
    for (int y2 = -1; y2 <= 1; y2++) {
        for (int x2 = -1; x2 <= 1; x2++) {
            sum += abs((SampleCurrent(q + step * int2(x2, y2)) - meanCurrent) -
                       (SamplePrevious(r + step * int2(x2, y2)) - meanPrevious));
        }
    }
    return sum;
}

float Mean(int2 q, int step)
{
    float mean = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            mean += SampleCurrent(q + step * int2(x, y));
        }
    }
    return mean / 9.0;
}

[numthreads(8, 8, 1)]
void main(uint2 id : SV_DispatchThreadID)
{
    if (any((int2)id >= regionSize)) {
        return;
    }
    const int2 q = regionOrigin + (int2)id;
    if (any(q >= Blocks())) {
        return;
    }

    // The block's centre in full-size pixels, and where head rotation alone puts it
    // in the previous frame.
    const float2 centre = float2(q) * 4.0 + 2.0;
    const float2 uv = centre / size;
    const float3 direction =
        float3(lerp(currentTan.x, currentTan.y, uv.x), lerp(currentTan.z, currentTan.w, uv.y), -1.0);
    const float3 rotated = float3(dot(row0.xyz, direction), dot(row1.xyz, direction), dot(row2.xyz, direction));
    float2 predicted = 0;
    if (rotated.z < -1e-6) {
        const float2 tangent = rotated.xy / -rotated.z;
        const float2 previousUv = float2((tangent.x - previousTan.x) / (previousTan.y - previousTan.x),
                                         (previousTan.z - tangent.y) / (previousTan.z - previousTan.w));
        predicted = (previousUv - uv) * size;
    }

    const float meanCurrent = Mean(q, 1);
    // A flat block has nothing to match: keep the prediction.
    float detail = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            detail += abs(SampleCurrent(q + int2(x, y)) - meanCurrent);
        }
    }

    int2 start = q + (int2)round(predicted / 4.0);
    float2 motion = predicted;
    if (detail > 0.01 * 9.0) {
        // Coarse: steps of 4 texels up to 16 away (64 full-size pixels), for the large
        // displacements of locomotion; the fine search then refines it.
        const float wideMean = Mean(q, 2);
        const float coarseBase = Cost(q, start, wideMean, 2);
        float coarseBest = coarseBase;
        int2 coarse = 0;
        for (int cy = -16; cy <= 16; cy += 4) {
            for (int cx = -16; cx <= 16; cx += 4) {
                if (cx == 0 && cy == 0) {
                    continue;
                }
                const float c = Cost(q, start + int2(cx, cy), wideMean, 2) + penalty * length(float2(cx, cy));
                if (c < coarseBest) {
                    coarseBest = c;
                    coarse = int2(cx, cy);
                }
            }
        }

        const bool movedCoarse = any(coarse != 0) && coarseBest < coarseBase * 0.9;
        const float base = Cost(q, start, meanCurrent, 1);
        if (movedCoarse) {
            start += coarse;
        }
        float best = movedCoarse ? Cost(q, start, meanCurrent, 1) : base;
        int2 offset = 0;
        const int r = (int)radius;
        for (int fy = -r; fy <= r; fy++) {
            for (int fx = -r; fx <= r; fx++) {
                if (fx == 0 && fy == 0) {
                    continue;
                }
                const float c = Cost(q, start + int2(fx, fy), meanCurrent, 1) + penalty * length(float2(fx, fy));
                if (c < best) {
                    best = c;
                    offset = int2(fx, fy);
                }
            }
        }

        // Only a clearly better match moves the block off the prediction.
        if (best < base * 0.9) {
            const int2 m = start + offset;
            // Subpixel position from a parabola through the neighbouring costs.
            const float cx0 = Cost(q, m - int2(1, 0), meanCurrent, 1), cx1 = Cost(q, m + int2(1, 0), meanCurrent, 1);
            const float cy0 = Cost(q, m - int2(0, 1), meanCurrent, 1), cy1 = Cost(q, m + int2(0, 1), meanCurrent, 1);
            const float c0 = Cost(q, m, meanCurrent, 1);
            float2 sub = 0;
            const float dx = cx0 - 2.0 * c0 + cx1, dy = cy0 - 2.0 * c0 + cy1;
            if (dx > 1e-5) {
                sub.x = clamp(0.5 * (cx0 - cx1) / dx, -0.5, 0.5);
            }
            if (dy > 1e-5) {
                sub.y = clamp(0.5 * (cy0 - cy1) / dy, -0.5, 0.5);
            }
            motion = (float2(m - q) + sub) * 4.0;
        }
    }
    flow[q] = motion;
}

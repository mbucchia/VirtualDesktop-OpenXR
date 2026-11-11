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

// Shared by the DLSS-NR motion passes (luminance, optical flow, motion vectors).
// Must match DlssnrMotionConstants in dlssnr.cpp (32 root constants).

cbuffer config : register(b0)
{
    float4 currentTan;  // tan(left), tan(right), tan(up), tan(down) (left and down are negative)
    float4 previousTan;
    float4 row0;        // previous-view rotation * current-view rotation, by rows
    float4 row1;
    float4 row2;
    float2 size;        // the eye's image, full-size pixels
    float radius;       // flow: fine search radius, quarter-size texels
    float penalty;      // flow: cost per texel away from the prediction
    int2 origin;        // luma: the eye's image in the color texture; motion: the foveated region in the eye's image
    int2 regionSize;    // flow: blocks to compute; motion: size of the foveated region
    int2 regionOrigin;  // flow: first block to compute
    float2 shift;       // motion: added to every vector (the foveated region moved since the history)
    float scale;        // motion: NR working size / full size (vectors and grid at NR's working resolution)
    float3 padding;
};

int2 Blocks()
{
    return (int2)(((uint2)size + 3) / 4);
}

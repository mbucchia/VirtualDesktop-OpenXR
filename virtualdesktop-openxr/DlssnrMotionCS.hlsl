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

// Full-size motion vectors for the foveated region: bilinear between the flow
// blocks' centres, plus the shift of the region since NR's history.

#include "DlssnrMotion.hlsli"

RWTexture2D<float2> flow : register(u0);
RWTexture2D<float2> motion : register(u1);

float2 Flow(int2 q)
{
    return flow[clamp(q, 0, Blocks() - 1)];
}

[numthreads(8, 8, 1)]
void main(uint2 id : SV_DispatchThreadID)
{
    if (any((int2)id >= regionSize)) {
        return;
    }
    // regionSize is NR's working size: each output pixel covers 1/scale full-size pixels.
    const float2 local = float2(origin) + (float2(id) + 0.5) / scale;
    const float2 block = clamp(local / 4.0, 0.5, float2(Blocks()) - 0.5) - 0.5;
    const int2 i0 = (int2)floor(block);
    const float2 f = block - float2(i0);
    const float2 top = lerp(Flow(i0), Flow(i0 + int2(1, 0)), f.x);
    const float2 bottom = lerp(Flow(i0 + int2(0, 1)), Flow(i0 + int2(1, 1)), f.x);
    motion[id] = (lerp(top, bottom, f.y) + shift) * scale;
}

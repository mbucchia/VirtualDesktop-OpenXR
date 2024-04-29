// MIT License
//
// Copyright(c) 2024 Matthieu Bucchianeri
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

cbuffer config : register(b0)
{
    uint2 TopLeft;
    float CenterX;
    float CenterY;
    float InnerRing;
    float OuterRing;
    uint Rate1x1;
    uint RateMedium;
    uint RateLow;
    uint Slice;
    bool Additive;
};

RWTexture2D<uint> output : register(u0);
#ifndef NOARRAY
RWTexture2DArray<uint> outputArray : register(u1);
#endif

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 fromCenter = float2(DTid.x - CenterX, DTid.y - CenterY);
    float distance = sqrt(fromCenter.x * fromCenter.x + fromCenter.y * fromCenter.y);

    uint rate = Rate1x1;
    if (distance >= OuterRing)
    {
        rate = RateLow;
    }
    else if (distance >= InnerRing)
    {
        rate = RateMedium;
    }

    uint2 position = TopLeft + DTid.xy;
    if (!Additive)
    {
        output[position] = rate;
    }
    else
    {
        output[position] = max(output[position], rate);
    }
#ifndef NOARRAY
    outputArray[float3(position, Slice)] = rate;
#endif
}

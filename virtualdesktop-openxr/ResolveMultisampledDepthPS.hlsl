// MIT License
//
// Copyright(c) 2022-2024 Matthieu Bucchianeri
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

// A shader to resolve multisampled depth into single samples.
SamplerState sourceSampler : register(s0);

cbuffer config : register(b0)
{
    uint slice;
};

Texture2DMSArray<float> sourceDepthMS : register(t0);

float main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD0) : SV_Depth
{
    uint width, height, slices, samples;
    sourceDepthMS.GetDimensions(width, height, slices, samples);

    float depth = 0;
    for (uint i = 0; i < samples; ++i)
    {
        float sampleDepth = sourceDepthMS.Load(uint3(position.x, position.y, slice), i).x;
        depth = max(depth, sampleDepth);
    }
    return depth;
}

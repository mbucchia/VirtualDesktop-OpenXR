// Copyright(c) 2025-2026 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
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

#include "constantsbuffer.h"

cbuffer cb : register(b0)
{
    ConstantsBuffer cb;
};

float3 Reproject(float4 screenPos)
{
    float4 position = mul(screenPos, cb.reprojectionMatrix);
    position.z = 1.0;
    return position.xyz;
}

float4 main(in uint id : SV_VertexID, out float3 reprojectedCoord : TEXCOORD0) : SV_Position
{
    float2 texCoord = float2((id == 1) ? 2.0 : 0.0, (id == 2) ? 2.0 : 0.0);
    float4 position = float4(texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 1.0, 1.0);
    reprojectedCoord = Reproject(position);
    return position;
}

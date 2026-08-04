// MIT License
//
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

float2 ApplyImageRect(float2 texCoord, Rect rect, bool flipY)
{
    float2 coord = texCoord * rect.extent + rect.offset;
    if (flipY)
    {
        coord.y = 1 - coord.y;
    }
    return coord;
}

float4 PreMultiplyAlpha(float4 color)
{
    return float4(color.rgb * color.a, color.a);
}

float4 UnPreMultiplyAlpha(float4 color)
{
    if (color.a != 0)
    {
        return float4(color.rgb / color.a, color.a);
    }
    else
    {
        return 0;
    }
}

float2 NdcToTexCoord(float2 ndc)
{
    return ndc * float2(0.5f, -0.5f) + 0.5f;
}

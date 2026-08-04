// MIT License
//
// Copyright(c) 2025 Matthieu Bucchianeri
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

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winrt/base.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#include <wil/resource.h>

#include <dxgi1_6.h>
#include <d3d11.h>
#include <DirectXMath.h>

#include <intrin.h>
#include <timeapi.h>

#include <atomic>
#include <chrono>
#include <ctime>
#define _USE_MATH_DEFINES
#include <cmath>
#include <condition_variable>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include <TraceLoggingActivity.h>
#include <TraceLoggingProvider.h>

#include <OVR_CAPI.h>
#include <OVR_CAPI_Audio.h>
#include <OVR_CAPI_D3D.h>
#include <OVR_CAPI_GL.h>
#include <OVR_CAPI_Vk.h>
#include <OVR_CAPI_Keys.h>
#include <OVR_CAPI_Util.h>
#include <OVR_Math.h>

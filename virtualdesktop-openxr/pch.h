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
// The above copyright noticeand this permission notice shall be included in all
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

// Standard library.
#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <ctime>
#include <deque>
#include <intrin.h>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#pragma intrinsic(_ReturnAddress)

using namespace std::chrono_literals;

// Windows header files.
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <windows.h>
#include <unknwn.h>
#include <wrl.h>
#include <wil/registry.h>
#include <wil/resource.h>
#include <traceloggingactivity.h>
#include <traceloggingprovider.h>
#include <TlHelp32.h>

using Microsoft::WRL::ComPtr;

// Graphics APIs.
#include <d3d11_4.h>
#include <d3d12.h>
#include <dxgi1_2.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <GL/GL.h>
#include <GL/glext.h>
#include <GL/wglext.h>

// LibOVR.
#include <OVR_CAPI.h>
#include <OVR_CAPI_Audio.h>
#include <OVR_CAPI_D3D.h>
#include <OVR_Math.h>

// These values were added to LibOVR but never published by Meta.
static constexpr int ovrHmd_QuestPro = 22;
static constexpr int ovrHmd_Quest3 = 23;

// OpenXR + Windows-specific definitions.
#define XR_NO_PROTOTYPES
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#define XR_USE_GRAPHICS_API_VULKAN
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>

// Oculus extra headers.
#include "fb_face_tracking2.h"

// OpenXR loader interfaces.
#include <loader_interfaces.h>

// OpenXR utilities.
#include <XrError.h>
#include <XrMath.h>
#include <XrSide.h>
#include <XrStereoView.h>
#include <XrToString.h>
#include <ScopeGuard.h>

// This header is not compatible with XR_NO_PROTOTYPES... We make a couple of symbols for now.
#define xrStringToPath(...) XR_ERROR_RUNTIME_FAILURE
#define xrPathToString(...) XR_ERROR_RUNTIME_FAILURE
#include <XrString.h>
#undef xrStringToPath
#undef xrPathToString

// FMT formatter.
#define FMT_HEADER_ONLY
#include <fmt/format.h>

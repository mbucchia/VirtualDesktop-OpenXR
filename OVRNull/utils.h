// MIT License
//
// Copyright(c) 2024-2026 Matthieu Bucchianeri
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

namespace ovrnull::utils {

    extern const LARGE_INTEGER g_qpcFrequency;

    static inline double QpcToOvrTime(LARGE_INTEGER qpcTime) {
        return (double)qpcTime.QuadPart / (double)g_qpcFrequency.QuadPart;
    }

    static inline double QpcToOvrTime(int64_t qpcTime) {
        return (double)qpcTime / (double)g_qpcFrequency.QuadPart;
    }

    static inline LARGE_INTEGER OvrTimeToQpc(double ovrTime) {
        LARGE_INTEGER qpcTime;
        qpcTime.QuadPart = (LONGLONG)(ovrTime * g_qpcFrequency.QuadPart);
        return qpcTime;
    }

    static inline DirectX::XMMATRIX LoadOvrPose(const ovrPosef& pose) {
        const DirectX::XMVECTOR orientation = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)&pose.Orientation);
        const DirectX::XMVECTOR position = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&pose.Position);
        DirectX::XMMATRIX matrix = DirectX::XMMatrixRotationQuaternion(orientation);
        matrix.r[3] = DirectX::XMVectorAdd(matrix.r[3], position);
        return matrix;
    }

    static inline DirectX::XMMATRIX LoadInvertedOvrPose(const ovrPosef& pose) {
        const auto invertedPose = OVR::Posef(pose).Inverted();
        const DirectX::XMVECTOR orientation = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)&invertedPose.Rotation);
        const DirectX::XMVECTOR position = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&invertedPose.Translation);
        DirectX::XMMATRIX matrix = DirectX::XMMatrixRotationQuaternion(orientation);
        matrix.r[3] = DirectX::XMVectorAdd(matrix.r[3], position);
        return matrix;
    }

    static inline DirectX ::XMMATRIX LoadOvrProjection(const ovrFovPort& fov, float nearZ, float farZ) {
        const auto projection = ovrMatrix4f_Projection(fov, nearZ, farZ, 0);
        return DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)&projection);
    }

    static inline const char* ToString(ovrTextureType type) {
        switch (type) {
        case ovrTexture_2D:
            return "2D";
        case ovrTexture_Cube:
            return "Cube";
        default:
            return "Invalid";
        }
    }

    static inline const char* ToString(ovrTextureFormat format) {
        switch (format) {
        case OVR_FORMAT_R8G8B8A8_UNORM:
            return "R8G8B8A8UNorm";
        case OVR_FORMAT_R8G8B8A8_UNORM_SRGB:
            return "R8G8B8A8UNormSrgb";
        case OVR_FORMAT_B8G8R8A8_UNORM:
            return "B8G8R8A8Unorm";
        case OVR_FORMAT_B8G8R8A8_UNORM_SRGB:
            return "B8G8R8A8UnormSrgb";
        case OVR_FORMAT_B8G8R8X8_UNORM:
            return "B8G8R8X8Unorm";
        case OVR_FORMAT_B8G8R8X8_UNORM_SRGB:
            return "B8G8R8X8UnormSrgb";
        case OVR_FORMAT_R16G16B16A16_FLOAT:
            return "R16G16B16A16Float";
        case OVR_FORMAT_R11G11B10_FLOAT:
            return "R11G11B10Float";
        case OVR_FORMAT_D16_UNORM:
            return "D16";
        case OVR_FORMAT_D24_UNORM_S8_UINT:
            return "D24S8";
        case OVR_FORMAT_D32_FLOAT:
            return "D32";
        case OVR_FORMAT_D32_FLOAT_S8X24_UINT:
            return "D32S8X24";
        case OVR_FORMAT_BC1_UNORM:
            return "BC1Unorm";
        case OVR_FORMAT_BC1_UNORM_SRGB:
            return "BC1UnormSrgb";
        case OVR_FORMAT_BC2_UNORM:
            return "BC2Unorm";
        case OVR_FORMAT_BC2_UNORM_SRGB:
            return "BC2UnormSrgb";
        case OVR_FORMAT_BC3_UNORM:
            return "BC3Unorm";
        case OVR_FORMAT_BC3_UNORM_SRGB:
            return "BC3UnormSrgb";
        case OVR_FORMAT_BC6H_UF16:
            return "BC6HUF16";
        case OVR_FORMAT_BC6H_SF16:
            return "BC6HSF16";
        case OVR_FORMAT_BC7_UNORM:
            return "BC7Unorm";
        case OVR_FORMAT_BC7_UNORM_SRGB:
            return "BC7UnormSrgb";
        default:
            return "Invalid";
        }
    }

    static inline constexpr DXGI_FORMAT ToDxgiTextureFormat(ovrTextureFormat format) {
        switch (format) {
        case OVR_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case OVR_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case OVR_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case OVR_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case OVR_FORMAT_B8G8R8X8_UNORM:
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        case OVR_FORMAT_B8G8R8X8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case OVR_FORMAT_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case OVR_FORMAT_R11G11B10_FLOAT:
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case OVR_FORMAT_D16_UNORM:
            return DXGI_FORMAT_D16_UNORM;
        case OVR_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case OVR_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;
        case OVR_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        case OVR_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM;
        case OVR_FORMAT_BC1_UNORM_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case OVR_FORMAT_BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM;
        case OVR_FORMAT_BC2_UNORM_SRGB:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case OVR_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM;
        case OVR_FORMAT_BC3_UNORM_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case OVR_FORMAT_BC6H_UF16:
            return DXGI_FORMAT_BC6H_UF16;
        case OVR_FORMAT_BC6H_SF16:
            return DXGI_FORMAT_BC6H_SF16;
        case OVR_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM;
        case OVR_FORMAT_BC7_UNORM_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    static inline constexpr bool IsDepthFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D16_UNORM:
            return true;
        }

        return false;
    }

    static inline constexpr DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return DXGI_FORMAT_B8G8R8X8_TYPELESS;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32G8X24_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_TYPELESS;
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return DXGI_FORMAT_BC1_TYPELESS;
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return DXGI_FORMAT_BC2_TYPELESS;
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return DXGI_FORMAT_BC3_TYPELESS;
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
            return DXGI_FORMAT_BC6H_TYPELESS;
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return DXGI_FORMAT_BC7_TYPELESS;
        }

        return format;
    }

} // namespace ovrnull::utils

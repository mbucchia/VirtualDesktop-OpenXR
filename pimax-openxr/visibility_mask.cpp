// MIT License
//
// Copyright(c) 2022 Matthieu Bucchianeri
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

#include "pch.h"

#include "log.h"
#include "runtime.h"
#include "utils.h"

// Implements the necessary support for the XR_KHR_visibility_mask extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_visibility_mask

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;
    using namespace DirectX;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetVisibilityMaskKHR
    XrResult OpenXrRuntime::xrGetVisibilityMaskKHR(XrSession session,
                                                   XrViewConfigurationType viewConfigurationType,
                                                   uint32_t viewIndex,
                                                   XrVisibilityMaskTypeKHR visibilityMaskType,
                                                   XrVisibilityMaskKHR* visibilityMask) {
        if (visibilityMask->type != XR_TYPE_VISIBILITY_MASK_KHR) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetVisibilityMaskKHR",
                          TLXArg(session, "Session"),
                          TLArg(xr::ToCString(viewConfigurationType), "ViewConfigurationType"),
                          TLArg(viewIndex, "ViewIndex"),
                          TLArg(xr::ToCString(visibilityMaskType), "VisibilityMaskType"),
                          TLArg(visibilityMask->vertexCapacityInput, "VertexCapacityInput"),
                          TLArg(visibilityMask->indexCapacityInput, "IndexCapacityInput"));

        if (!m_isVisibilityMaskSupported) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        if (viewIndex >= xr::StereoView::Count) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        if (visibilityMaskType != XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR || !m_isVisibilityMaskEnabled) {
            // We only support the hidden area mesh and we don't return a mask with parallel projection.
            visibilityMask->vertexCountOutput = 0;
            visibilityMask->indexCountOutput = 0;
            return XR_SUCCESS;
        }

        const auto verticesCount =
            pvr_getEyeHiddenAreaMesh(m_pvrSession, !viewIndex ? pvrEye_Left : pvrEye_Right, nullptr, 0);
        TraceLoggingWrite(g_traceProvider, "PVR_EyeHiddenAreaMesh", TLArg(verticesCount, "VerticesCount"));

        if (visibilityMask->vertexCapacityInput == 0) {
            visibilityMask->vertexCountOutput = verticesCount;
            visibilityMask->indexCountOutput = verticesCount;
        } else if (visibilityMask->vertices && visibilityMask->indices) {
            if (visibilityMask->vertexCapacityInput < verticesCount ||
                visibilityMask->indexCapacityInput < verticesCount) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            static_assert(sizeof(XrVector2f) == sizeof(pvrVector2f));
            pvr_getEyeHiddenAreaMesh(m_pvrSession,
                                     !viewIndex ? pvrEye_Left : pvrEye_Right,
                                     (pvrVector2f*)visibilityMask->vertices,
                                     verticesCount);

            convertSteamVRToOpenXRHiddenMesh(
                m_cachedEyeInfo[viewIndex].Fov, visibilityMask->vertices, visibilityMask->indices, verticesCount);
        }

        return XR_SUCCESS;
    }

    void OpenXrRuntime::convertSteamVRToOpenXRHiddenMesh(const pvrFovPort& fov,
                                                         XrVector2f* vertices,
                                                         uint32_t* indices,
                                                         uint32_t count) const {
        const float b = -fov.DownTan;
        const float t = fov.UpTan;
        const float l = -fov.LeftTan;
        const float r = fov.RightTan;

        // z = -1, n = 1
        // pndcx = (2n/(r-l) * pvx - (r+l)/(r-l)) / -z => pvx = (pndcx + (r+l)/(r-l))/(2n/(r-l))
        // pndcy = (2n/(t-b) * pvy - (t+b)/(t-b)) / -z => pvy = (pndcy + (t+b)/(t-b))/(2n/(t-b))
        const float hSpanRcp = 1.0f / (r - l);
        const float vSpanRcp = 1.0f / (t - b);

        // (r+l)/(r-l)
        const float rplOverHSpan = (r + l) * hSpanRcp;
        const float tpbOverVSpan = (t + b) * vSpanRcp;

        const float halfHSpan = (r - l) * 0.5f;
        const float halfVSpan = (t - b) * 0.5f;

        // constTerm = (r+l)/(r-l)/(2n(r-l))
        const float hConstTerm = rplOverHSpan * halfHSpan;
        const float vConstTerm = tpbOverVSpan * halfVSpan;

        for (uint32_t i = 0; i < count; i++) {
            // Screen to NDC.
            XrVector2f ndc{(vertices[i].x - 0.5f) * 2.f, (vertices[i].y - 0.5f) * 2.f};

            // Project the vertex.
            XMStoreFloat2(reinterpret_cast<XMFLOAT2*>(&vertices[i]),
                          XMVectorMultiplyAdd(XMVECTORF32{{{ndc.x, ndc.y, 0.f, 0.f}}},
                                              XMVECTORF32{{{halfHSpan, halfVSpan, 0.f, 0.f}}},
                                              XMVECTORF32{{{hConstTerm, vConstTerm, 0.f, 0.f}}}));

            // Record the indices.
            indices[i] = i;
        }
    }

} // namespace pimax_openxr

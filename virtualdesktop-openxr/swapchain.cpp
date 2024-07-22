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

#include "pch.h"

#include "log.h"
#include "runtime.h"
#include "utils.h"

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateViewConfigurations
    XrResult OpenXrRuntime::xrEnumerateViewConfigurations(XrInstance instance,
                                                          XrSystemId systemId,
                                                          uint32_t viewConfigurationTypeCapacityInput,
                                                          uint32_t* viewConfigurationTypeCountOutput,
                                                          XrViewConfigurationType* viewConfigurationTypes) {
        std::vector<XrViewConfigurationType> types;
        if (has_XR_VARJO_quad_views) {
            // Push first to be advertised as the preferred view configuration type.
            types.push_back(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO);
        }
        types.push_back(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO);

        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateViewConfigurations",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"),
                          TLArg(viewConfigurationTypeCapacityInput, "ViewConfigurationTypeCapacityInput"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (viewConfigurationTypeCapacityInput && viewConfigurationTypeCapacityInput < types.size()) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *viewConfigurationTypeCountOutput = (uint32_t)types.size();
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateViewConfigurations",
                          TLArg(*viewConfigurationTypeCountOutput, "ViewConfigurationTypeCountOutput"));

        if (viewConfigurationTypeCapacityInput && viewConfigurationTypes) {
            for (uint32_t i = 0; i < *viewConfigurationTypeCountOutput; i++) {
                viewConfigurationTypes[i] = types[i];
                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateViewConfigurations",
                                  TLArg(xr::ToCString(viewConfigurationTypes[i]), "ViewConfigurationType"));
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetViewConfigurationProperties
    XrResult OpenXrRuntime::xrGetViewConfigurationProperties(XrInstance instance,
                                                             XrSystemId systemId,
                                                             XrViewConfigurationType viewConfigurationType,
                                                             XrViewConfigurationProperties* configurationProperties) {
        if (configurationProperties->type != XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetViewConfigurationProperties",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"),
                          TLArg(xr::ToCString(viewConfigurationType), "ViewConfigurationType"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO &&
            (!has_XR_VARJO_quad_views || viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO)) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        configurationProperties->viewConfigurationType = viewConfigurationType;
        configurationProperties->fovMutable = XR_TRUE;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetViewConfigurationProperties",
                          TLArg(xr::ToCString(configurationProperties->viewConfigurationType), "ViewConfigurationType"),
                          TLArg(!!configurationProperties->fovMutable, "FovMutable"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateViewConfigurationViews
    XrResult OpenXrRuntime::xrEnumerateViewConfigurationViews(XrInstance instance,
                                                              XrSystemId systemId,
                                                              XrViewConfigurationType viewConfigurationType,
                                                              uint32_t viewCapacityInput,
                                                              uint32_t* viewCountOutput,
                                                              XrViewConfigurationView* views) {
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateViewConfigurationViews",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"),
                          TLArg(viewCapacityInput, "ViewCapacityInput"),
                          TLArg(xr::ToCString(viewConfigurationType), "ViewConfigurationType"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO &&
            (!has_XR_VARJO_quad_views || viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO)) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        const uint32_t viewCount = viewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO
                                       ? xr::StereoView::Count
                                       : xr::QuadView::Count;
        if (viewCapacityInput && viewCapacityInput < viewCount) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *viewCountOutput = viewCount;
        TraceLoggingWrite(
            g_traceProvider, "xrEnumerateViewConfigurationViews", TLArg(*viewCountOutput, "ViewCountOutput"));

        if (viewCapacityInput && views) {
            // Override default to specify whether foveated rendering is desired when the application does
            // not specify.
            bool foveatedRenderingActive = m_preferFoveatedRendering;

            // When foveated rendering extension is active, look whether the application is requesting it
            // for the views. The spec is a little questionable and calls for each view to have the flag
            // specified. Here we check that at least one view has the flag on.
            if (has_XR_VARJO_foveated_rendering) {
                for (uint32_t i = 0; i < *viewCountOutput; i++) {
                    const XrFoveatedViewConfigurationViewVARJO* foveatedViewConfiguration =
                        reinterpret_cast<const XrFoveatedViewConfigurationViewVARJO*>(views[i].next);
                    while (foveatedViewConfiguration) {
                        if (foveatedViewConfiguration->type == XR_TYPE_FOVEATED_VIEW_CONFIGURATION_VIEW_VARJO) {
                            foveatedRenderingActive =
                                foveatedRenderingActive || foveatedViewConfiguration->foveatedRenderingActive;
                            break;
                        }
                        foveatedViewConfiguration = reinterpret_cast<const XrFoveatedViewConfigurationViewVARJO*>(
                            foveatedViewConfiguration->next);
                    }
                }

                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateViewConfigurationViews",
                                  TLArg(foveatedRenderingActive, "FoveatedRenderingActive"));
            }

            for (uint32_t i = 0; i < *viewCountOutput; i++) {
                if (views[i].type != XR_TYPE_VIEW_CONFIGURATION_VIEW) {
                    return XR_ERROR_VALIDATION_FAILURE;
                }

                // Lower the maximum on a low memory system.
                // Conformance testing is also create a number of very large textures, so we also lower the limit here.
                views[i].maxImageRectWidth = views[i].maxImageRectHeight =
                    !(m_isLowVideoMemorySystem || m_isConformanceTest) ? 16384 : 8192;

                // Per Direct3D 11 standard, "devices are required to support 4x MSAA for all render target formats, and
                // 8x MSAA for all render target formats except R32G32B32A32 formats.".
                // We could go and check every supported render target formats to find a possibly higher count, but we
                // do not bother.
                views[i].maxSwapchainSampleCount = 4;
                views[i].recommendedSwapchainSampleCount = 1;

                // When using quad views, we use 2 peripheral views with lower pixel densities, and 2 focus
                // views with higher pixel densities.
                uint32_t viewFovIndex = i;
                float pixelDensity = m_supersamplingFactor * m_upscalingMultiplier;
                if (viewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO) {
                    if (i < xr::StereoView::Count) {
                        pixelDensity = m_peripheralPixelDensity;
                    } else {
                        pixelDensity = m_supersamplingFactor;
                        if (foveatedRenderingActive) {
                            viewFovIndex = i + 2;
                        }
                    }
                }

                // Recommend the resolution with distortion accounted for.
                ovrFovPort fov;
                fov.UpTan = tan(m_cachedEyeFov[viewFovIndex].angleUp);
                fov.DownTan = tan(-m_cachedEyeFov[viewFovIndex].angleDown);
                fov.LeftTan = tan(-m_cachedEyeFov[viewFovIndex].angleLeft);
                fov.RightTan = tan(m_cachedEyeFov[viewFovIndex].angleRight);

                ovrSizei viewportSize = ovr_GetFovTextureSize(
                    m_ovrSession, (i % xr::StereoView::Count) == 0 ? ovrEye_Left : ovrEye_Right, fov, pixelDensity);
                if (viewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
                    viewportSize.w = (int)(viewportSize.w * m_fovTangentX);
                    viewportSize.h = (int)(viewportSize.h * m_fovTangentY);
                }

                views[i].recommendedImageRectWidth =
                    xr::math::AlignTo<4>(std::min((uint32_t)viewportSize.w, views[i].maxImageRectWidth));
                views[i].recommendedImageRectHeight =
                    xr::math::AlignTo<4>(std::min((uint32_t)viewportSize.h, views[i].maxImageRectHeight));

                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateViewConfigurationViews",
                                  TLArg(i, "ViewIndex"),
                                  TLArg(views[i].maxImageRectWidth, "MaxImageRectWidth"),
                                  TLArg(views[i].maxImageRectHeight, "MaxImageRectHeight"),
                                  TLArg(views[i].maxSwapchainSampleCount, "MaxSwapchainSampleCount"),
                                  TLArg(views[i].recommendedImageRectWidth, "RecommendedImageRectWidth"),
                                  TLArg(views[i].recommendedImageRectHeight, "RecommendedImageRectHeight"),
                                  TLArg(views[i].recommendedSwapchainSampleCount, "RecommendedSwapchainSampleCount"));
            }

            if (!m_loggedResolution) {
                if (viewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO) {
                    Log("Recommended peripheral resolution: %ux%u (%.3fx density)\n",
                        views[xr::StereoView::Left].recommendedImageRectWidth,
                        views[xr::StereoView::Left].recommendedImageRectHeight,
                        m_peripheralPixelDensity);
                    Log("Recommended focus resolution: %ux%u (%.3fx density)\n",
                        views[xr::QuadView::FocusLeft].recommendedImageRectWidth,
                        views[xr::QuadView::FocusLeft].recommendedImageRectHeight,
                        m_supersamplingFactor);
                } else {
                    Log("Recommended resolution: %ux%u (%.3f supersampling, %.3f upscaling, %.3f/%.3f tangents)\n",
                        views[xr::StereoView::Left].recommendedImageRectWidth,
                        views[xr::StereoView::Left].recommendedImageRectHeight,
                        m_supersamplingFactor,
                        1 / m_upscalingMultiplier,
                        m_fovTangentX,
                        m_fovTangentY);
                }
                m_loggedResolution = true;
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateSwapchainFormats
    XrResult OpenXrRuntime::xrEnumerateSwapchainFormats(XrSession session,
                                                        uint32_t formatCapacityInput,
                                                        uint32_t* formatCountOutput,
                                                        int64_t* formats) {
        // We match desirables formats from the ovrTextureFormat lists.
        std::vector<DXGI_FORMAT> d3dFormats;
        // Prefer SRGB formats.
        d3dFormats.push_back(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        d3dFormats.push_back(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
        d3dFormats.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
        d3dFormats.push_back(DXGI_FORMAT_B8G8R8A8_UNORM);
        d3dFormats.push_back(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
        d3dFormats.push_back(DXGI_FORMAT_B8G8R8X8_UNORM);
        d3dFormats.push_back(DXGI_FORMAT_R16G16B16A16_FLOAT);
        // Prefer 32-bit depth.
        d3dFormats.push_back(DXGI_FORMAT_D32_FLOAT);
        // Stencil formats are not shareable via NT HANDLE.
        if (!requireNTHandleSharing()) {
            d3dFormats.push_back(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
            d3dFormats.push_back(DXGI_FORMAT_D24_UNORM_S8_UINT);
        }
        d3dFormats.push_back(DXGI_FORMAT_D16_UNORM);

        std::vector<VkFormat> vkFormats;
        vkFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);
        vkFormats.push_back(VK_FORMAT_B8G8R8A8_SRGB);
        vkFormats.push_back(VK_FORMAT_R8G8B8A8_UNORM);
        vkFormats.push_back(VK_FORMAT_B8G8R8A8_UNORM);
        vkFormats.push_back(VK_FORMAT_R16G16B16A16_SFLOAT);
        vkFormats.push_back(VK_FORMAT_D32_SFLOAT);
        // Stencil formats are not shareable via NT HANDLE.
        if (!requireNTHandleSharing()) {
            vkFormats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
            vkFormats.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
        }
        vkFormats.push_back(VK_FORMAT_D16_UNORM);

        std::vector<GLenum> glFormats;
        // Prefer higher bit counts.
        glFormats.push_back(GL_RGBA16F);
        // Prefer SRGB formats.
        glFormats.push_back(GL_SRGB8_ALPHA8);
        glFormats.push_back(GL_RGBA8);
        // Prefer 32-bit depth.
        glFormats.push_back(GL_DEPTH_COMPONENT32F);
        // Stencil formats are not shareable via NT HANDLE.
        if (!requireNTHandleSharing()) {
            glFormats.push_back(GL_DEPTH32F_STENCIL8);
            glFormats.push_back(GL_DEPTH24_STENCIL8);
        }
        glFormats.push_back(GL_DEPTH_COMPONENT16);

        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateSwapchainFormats",
                          TLXArg(session, "Session"),
                          TLArg(formatCapacityInput, "FormatCapacityInput"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        const uint32_t count = m_isHeadless        ? 0u
                               : isVulkanSession() ? (uint32_t)vkFormats.size()
                               : isOpenGLSession() ? (uint32_t)glFormats.size()
                                                   : (uint32_t)d3dFormats.size();

        if (formatCapacityInput && formatCapacityInput < count) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *formatCountOutput = count;
        TraceLoggingWrite(
            g_traceProvider, "xrEnumerateSwapchainFormats", TLArg(*formatCountOutput, "FormatCountOutput"));

        if (formatCapacityInput && formats) {
            for (uint32_t i = 0; i < *formatCountOutput; i++) {
                if (isVulkanSession()) {
                    formats[i] = (int64_t)vkFormats[i];
                } else if (isOpenGLSession()) {
                    formats[i] = (int64_t)glFormats[i];
                } else {
                    formats[i] = (int64_t)d3dFormats[i];
                }
                TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainFormats", TLArg(formats[i], "Format"));
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateSwapchain
    XrResult OpenXrRuntime::xrCreateSwapchain(XrSession session,
                                              const XrSwapchainCreateInfo* createInfo,
                                              XrSwapchain* swapchain) {
        if (createInfo->type != XR_TYPE_SWAPCHAIN_CREATE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateSwapchain",
                          TLXArg(session, "Session"),
                          TLArg(createInfo->arraySize, "ArraySize"),
                          TLArg(createInfo->width, "Width"),
                          TLArg(createInfo->height, "Height"),
                          TLArg(createInfo->createFlags, "CreateFlags"),
                          TLArg(createInfo->format, "Format"),
                          TLArg(createInfo->faceCount, "FaceCount"),
                          TLArg(createInfo->mipCount, "MipCount"),
                          TLArg(createInfo->sampleCount, "SampleCount"),
                          TLArg(createInfo->usageFlags, "UsageFlags"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (m_isHeadless) {
            return XR_ERROR_FEATURE_UNSUPPORTED;
        }

        if (createInfo->faceCount != 1 && (!has_XR_KHR_composition_layer_cube || createInfo->faceCount != 6)) {
            return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
        }

        if (createInfo->faceCount == 6 && (createInfo->arraySize != 1 || createInfo->width != createInfo->height)) {
            return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
        }

        ovrTextureSwapChainDesc desc{};

        desc.Format = isVulkanSession()   ? vkToOvrTextureFormat((VkFormat)createInfo->format)
                      : isOpenGLSession() ? glToOvrTextureFormat((GLenum)createInfo->format)
                                          : dxgiToOvrTextureFormat((DXGI_FORMAT)createInfo->format);
        const auto dxgiFormatForSubmission = ovrToDxgiTextureFormat(desc.Format);
        if (desc.Format == OVR_FORMAT_UNKNOWN) {
            return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
        }
        desc.MiscFlags = ovrTextureMisc_DX_Typeless; // OpenXR requires to return typeless texures.

        // Request a swapchain from OVR.
        desc.Type = createInfo->faceCount != 6 ? ovrTexture_2D : ovrTexture_Cube;
        desc.StaticImage = !!(createInfo->createFlags & XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT);
        desc.ArraySize = createInfo->faceCount == 1 ? createInfo->arraySize : createInfo->faceCount;
        desc.Width = createInfo->width;
        desc.Height = createInfo->height;
        desc.MipLevels = createInfo->mipCount;
        if (desc.MipLevels > 1) {
            desc.MiscFlags |= ovrTextureMisc_AllowGenerateMips;
        }
        if (createInfo->createFlags & XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT) {
            desc.MiscFlags |= ovrTextureMisc_ProtectedContent;
        }
        desc.SampleCount = createInfo->sampleCount;

        if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            desc.BindFlags |= ovrTextureBind_DX_DepthStencil;
        } else {
            // Use the bits regardless of XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT or
            // XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT. We might run full quad shaders to pre-process swapchains.
            desc.BindFlags |= ovrTextureBind_DX_RenderTarget;
            if (desc.SampleCount == 1) {
                desc.BindFlags |= ovrTextureBind_DX_UnorderedAccess;
            }
        }
        if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) {
            desc.BindFlags |= ovrTextureBind_DX_UnorderedAccess;
        }

        ovrTextureSwapChain ovrSwapchain{};
        int length = 0;
        // If and only if the swapchain images are directly usable by LibOVR, we create an OVR swapchain. Otherwise, we
        // will create images ourselves.
        // - Our pre-processing shader does not support cubemaps.
        // - Our pre-processing shader does not support MSAA.
        // Additionally, OVR only uses KMT HANDLE, so if NT HANDLE are required, we must use our own images.
        if (desc.Type == ovrTexture_2D && desc.SampleCount == 1 && !m_forceSlowpathSwapchains &&
            !requireNTHandleSharing()) {
            if (desc.ArraySize > 1) {
                Log("Creating a swapchain with texture array\n");
            }
            CHECK_OVRCMD(ovr_CreateTextureSwapChainDX(m_ovrSession, m_ovrSubmissionDevice.Get(), &desc, &ovrSwapchain));
            CHECK_OVRCMD(ovr_GetTextureSwapChainLength(m_ovrSession, ovrSwapchain, &length));
        } else {
            Log("Creating a slow-path swapchain (reason: %d)\n",
                desc.Type != ovrTexture_2D ? 1
                : desc.SampleCount != 1    ? 2
                                           : 3);
            length = desc.StaticImage ? 1 : 3;
        }

        // Create the internal struct.
        Swapchain& xrSwapchain = *new Swapchain;
        xrSwapchain.appSwapchain.ovrSwapchain = ovrSwapchain;
        xrSwapchain.ovrSwapchainLength = length;
        xrSwapchain.ovrDesc = desc;
        xrSwapchain.xrDesc = *createInfo;
        xrSwapchain.dxgiFormatForSubmission = dxgiFormatForSubmission;

        *swapchain = (XrSwapchain)&xrSwapchain;

        // Maintain a list of known swapchains for validation and cleanup.
        {
            std::unique_lock lock(m_swapchainsMutex);

            m_swapchains.insert(*swapchain);
        }

        TraceLoggingWrite(g_traceProvider, "xrCreateSwapchain", TLXArg(*swapchain, "Swapchain"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroySwapchain
    XrResult OpenXrRuntime::xrDestroySwapchain(XrSwapchain swapchain) {
        TraceLoggingWrite(g_traceProvider, "xrDestroySwapchain", TLXArg(swapchain, "Swapchain"));

        std::unique_lock lock(m_swapchainsMutex);

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // Make sure there are no pending operations.
        if (isD3D12Session()) {
            flushD3D12CommandQueue();
        } else if (isVulkanSession()) {
            flushVulkanCommandQueue();
        } else if (isOpenGLSession()) {
            flushOpenGLContext();
        } else {
            flushD3D11Context();
        }
        if (m_useAsyncSubmission && !m_needStartAsyncSubmissionThread) {
            waitForAsyncSubmissionIdle();
        }
        flushSubmissionContext();

        Swapchain& xrSwapchain = *(Swapchain*)swapchain;

        if (!xrSwapchain.resolvedSlices.empty() && xrSwapchain.appSwapchain.ovrSwapchain &&
            xrSwapchain.resolvedSlices[0].ovrSwapchain != xrSwapchain.appSwapchain.ovrSwapchain) {
            ovr_DestroyTextureSwapChain(m_ovrSession, xrSwapchain.appSwapchain.ovrSwapchain);
        }
        while (!xrSwapchain.resolvedSlices.empty()) {
            auto ovrSwapchain = xrSwapchain.resolvedSlices.back().ovrSwapchain;
            if (ovrSwapchain) {
                ovr_DestroyTextureSwapChain(m_ovrSession, ovrSwapchain);
            }
            xrSwapchain.resolvedSlices.pop_back();
        }
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            if (xrSwapchain.stereoProjection[eye].ovrSwapchain) {
                ovr_DestroyTextureSwapChain(m_ovrSession, xrSwapchain.stereoProjection[eye].ovrSwapchain);
            }
        }

        cleanupSwapchainImagesVulkan(xrSwapchain);
        cleanupSwapchainImagesOpenGL(xrSwapchain);

        delete &xrSwapchain;
        m_swapchains.erase(swapchain);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateSwapchainImages
    XrResult OpenXrRuntime::xrEnumerateSwapchainImages(XrSwapchain swapchain,
                                                       uint32_t imageCapacityInput,
                                                       uint32_t* imageCountOutput,
                                                       XrSwapchainImageBaseHeader* images) {
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateSwapchainImages",
                          TLXArg(swapchain, "Swapchain"),
                          TLArg(imageCapacityInput, "ImageCapacityInput"));

        std::unique_lock lock(m_swapchainsMutex);

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)swapchain;

        int count = !xrSwapchain.ovrDesc.StaticImage ? xrSwapchain.ovrSwapchainLength : 1;

        if (imageCapacityInput && imageCapacityInput < (uint32_t)count) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *imageCountOutput = count;
        TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainImages", TLArg(*imageCountOutput, "ImageCountOutput"));

        if (imageCapacityInput && images) {
            if (isD3D12Session()) {
                XrSwapchainImageD3D12KHR* d3d12Images = reinterpret_cast<XrSwapchainImageD3D12KHR*>(images);
                return getSwapchainImagesD3D12(xrSwapchain, d3d12Images, *imageCountOutput);
            } else if (isVulkanSession()) {
                XrSwapchainImageVulkanKHR* vkImages = reinterpret_cast<XrSwapchainImageVulkanKHR*>(images);
                return getSwapchainImagesVulkan(xrSwapchain, vkImages, *imageCountOutput);
            } else if (isOpenGLSession()) {
                XrSwapchainImageOpenGLKHR* glImages = reinterpret_cast<XrSwapchainImageOpenGLKHR*>(images);
                return getSwapchainImagesOpenGL(xrSwapchain, glImages, *imageCountOutput);
            } else {
                XrSwapchainImageD3D11KHR* d3d11Images = reinterpret_cast<XrSwapchainImageD3D11KHR*>(images);
                return getSwapchainImagesD3D11(xrSwapchain, d3d11Images, *imageCountOutput);
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrAcquireSwapchainImage
    XrResult OpenXrRuntime::xrAcquireSwapchainImage(XrSwapchain swapchain,
                                                    const XrSwapchainImageAcquireInfo* acquireInfo,
                                                    uint32_t* index) {
        if (acquireInfo && acquireInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage", TLXArg(swapchain, "Swapchain"));

        std::unique_lock lock(m_swapchainsMutex);

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)swapchain;

        // Check that we can acquire an image.
        if ((xrSwapchain.frozen && !m_allowStaticSwapchainsReuse) ||
            xrSwapchain.acquiredIndices.size() == xrSwapchain.ovrSwapchainLength) {
            return XR_ERROR_CALL_ORDER_INVALID;
        }

        // We don't query the image index from OVR: this is because LibOVR producer/consumer model works much
        // differently than OpenXR. We maintain our own index and there is logic in preprocessSwapchainImage() to ensure
        // we pass the correct image to the compositor.
        const int imageIndex = xrSwapchain.nextIndex;
        xrSwapchain.acquiredIndices.push_back(imageIndex);
        xrSwapchain.frozen = xrSwapchain.ovrDesc.StaticImage;
        xrSwapchain.nextIndex = imageIndex + 1;
        if ((int)xrSwapchain.nextIndex >= xrSwapchain.ovrSwapchainLength) {
            xrSwapchain.nextIndex = 0;
        }
        *index = imageIndex;

        TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage", TLArg(*index, "Index"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrWaitSwapchainImage
    XrResult OpenXrRuntime::xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo) {
        if (waitInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrWaitSwapchainImage",
                          TLXArg(swapchain, "Swapchain"),
                          TLArg(waitInfo->timeout, "Timeout"));

        std::unique_lock lock(m_swapchainsMutex);

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)swapchain;

        // Check an image is acquired but not waited.
        if (xrSwapchain.acquiredIndices.empty() || xrSwapchain.acquiredIndices.front() == xrSwapchain.lastWaitedIndex) {
            return XR_ERROR_CALL_ORDER_INVALID;
        }

        // We assume that our frame timing in xrWaitFrame() guaranteed availability of the next image. No wait.
        xrSwapchain.lastWaitedIndex = xrSwapchain.acquiredIndices.front();

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrReleaseSwapchainImage
    XrResult OpenXrRuntime::xrReleaseSwapchainImage(XrSwapchain swapchain,
                                                    const XrSwapchainImageReleaseInfo* releaseInfo) {
        if (releaseInfo && releaseInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrReleaseSwapchainImage", TLXArg(swapchain, "Swapchain"));

        std::unique_lock lock(m_swapchainsMutex);

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)swapchain;

        // Check an image is acquired and waited.
        if (xrSwapchain.acquiredIndices.empty() || xrSwapchain.acquiredIndices.front() != xrSwapchain.lastWaitedIndex) {
            return XR_ERROR_CALL_ORDER_INVALID;
        }

        // Update the state of the swapchain.
        // We never commit images here: this is because LibOVR producer/consumer model works much differently than
        // OpenXR. We will perform swapchain commits in preprocessSwapchainImage().
        xrSwapchain.lastReleasedIndex = xrSwapchain.lastWaitedIndex;
        xrSwapchain.lastWaitedIndex = -1;
        xrSwapchain.dirty = true;
        xrSwapchain.acquiredIndices.pop_front();

        return XR_SUCCESS;
    }

} // namespace virtualdesktop_openxr

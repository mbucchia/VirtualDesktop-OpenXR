// MIT License
//
// Copyright(c) 2022-2023 Matthieu Bucchianeri
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

        if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        configurationProperties->viewConfigurationType = viewConfigurationType;
        configurationProperties->fovMutable = XR_FALSE;

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

        if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        if (viewCapacityInput && viewCapacityInput < xr::StereoView::Count) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *viewCountOutput = xr::StereoView::Count;
        TraceLoggingWrite(
            g_traceProvider, "xrEnumerateViewConfigurationViews", TLArg(*viewCountOutput, "ViewCountOutput"));

        if (viewCapacityInput && views) {
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
                // TODO: We do not support MSAA swapchains today, as they are incompatible with our alpha correction
                // shaders.
                views[i].maxSwapchainSampleCount = 1;
                views[i].recommendedSwapchainSampleCount = 1;

                // Recommend the resolution with distortion accounted for.
                ovrFovPort fov;
                fov.UpTan = tan(m_cachedEyeFov[i].angleUp);
                fov.DownTan = tan(-m_cachedEyeFov[i].angleDown);
                fov.LeftTan = tan(-m_cachedEyeFov[i].angleLeft);
                fov.RightTan = tan(m_cachedEyeFov[i].angleRight);

                const ovrSizei viewportSize =
                    ovr_GetFovTextureSize(m_ovrSession, i == 0 ? ovrEye_Left : ovrEye_Right, fov, 1.f);
                views[i].recommendedImageRectWidth = std::min((uint32_t)viewportSize.w, views[i].maxImageRectWidth);
                views[i].recommendedImageRectHeight = std::min((uint32_t)viewportSize.h, views[i].maxImageRectHeight);

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
                Log("Recommended resolution: %ux%u\n",
                    views[0].recommendedImageRectWidth,
                    views[0].recommendedImageRectHeight);
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
        static const DXGI_FORMAT d3dFormats[] = {
            // clang-format off
                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // Prefer SRGB formats.
                DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_FORMAT_B8G8R8A8_UNORM,
                DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
                DXGI_FORMAT_B8G8R8X8_UNORM,
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                DXGI_FORMAT_D32_FLOAT, // Prefer 32-bit depth.
                DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
                DXGI_FORMAT_D24_UNORM_S8_UINT,
                DXGI_FORMAT_D16_UNORM,
            // clang-format on
        };
        static const VkFormat vkFormats[] = {
            // clang-format off
                VK_FORMAT_R8G8B8A8_SRGB, // Prefer SRGB formats.
                VK_FORMAT_B8G8R8A8_SRGB,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_B8G8R8A8_UNORM,
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_FORMAT_D32_SFLOAT, // Prefer 32-bit depth.
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM,
            // clang-format on
        };
        static const GLenum glFormats[] = {
            // clang-format off
                GL_RGBA16F, // Prefer higher bit counts.
                GL_SRGB8_ALPHA8, // Prefer SRGB formats.
                GL_RGBA8,
                GL_DEPTH_COMPONENT32F, // Prefer 32-bit depth.
                GL_DEPTH32F_STENCIL8,
                GL_DEPTH24_STENCIL8,
                GL_DEPTH_COMPONENT16,
            // clang-format on
        };

        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateSwapchainFormats",
                          TLXArg(session, "Session"),
                          TLArg(formatCapacityInput, "FormatCapacityInput"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        const uint32_t count = m_isHeadless        ? 0
                               : isVulkanSession() ? ARRAYSIZE(vkFormats)
                               : isOpenGLSession() ? ARRAYSIZE(glFormats)
                                                   : ARRAYSIZE(d3dFormats);

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

        if (createInfo->faceCount != 1 && createInfo->faceCount != 6) {
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
            // Use the bit regardless of XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT. We might run full quad shaders to
            // pre-process swapchains (eg: alpha or SRGB).
            desc.BindFlags |= ovrTextureBind_DX_RenderTarget;
        }
        if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) {
            desc.BindFlags |= ovrTextureBind_DX_UnorderedAccess;
        }

        // There are situations in OVR where we cannot use the OVR swapchain alone:
        // - POVR does not let you submit a slice of a texture array and always reads from the first slice.
        //   To mitigate this, we will create several swapchains with ArraySize=1 and we will make copies during
        //   xrEndFrame().

        ovrTextureSwapChain ovrSwapchain{};
        CHECK_OVRCMD(ovr_CreateTextureSwapChainDX(m_ovrSession, m_ovrSubmissionDevice.Get(), &desc, &ovrSwapchain));

        // Create the internal struct.
        Swapchain& xrSwapchain = *new Swapchain;
        xrSwapchain.ovrSwapchain.push_back(ovrSwapchain);
        CHECK_OVRCMD(ovr_GetTextureSwapChainLength(m_ovrSession, ovrSwapchain, &xrSwapchain.ovrSwapchainLength));
        xrSwapchain.slices.push_back({});
        xrSwapchain.lastProcessedIndex.push_back(-1);
        xrSwapchain.imagesResourceView.push_back({});
        xrSwapchain.renderTargetView.push_back({});
        xrSwapchain.ovrDesc = desc;
        xrSwapchain.xrDesc = *createInfo;
        xrSwapchain.dxgiFormatForSubmission = dxgiFormatForSubmission;

        // Lazily-filled state.
        for (int i = 1; i < desc.ArraySize; i++) {
            xrSwapchain.ovrSwapchain.push_back(nullptr);
            xrSwapchain.slices.push_back({});
            xrSwapchain.lastProcessedIndex.push_back(-1);
            xrSwapchain.imagesResourceView.push_back({});
            xrSwapchain.renderTargetView.push_back({});
        }

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

        while (!xrSwapchain.ovrSwapchain.empty()) {
            auto ovrSwapchain = xrSwapchain.ovrSwapchain.back();
            if (ovrSwapchain) {
                ovr_DestroyTextureSwapChain(m_ovrSession, ovrSwapchain);
            }
            xrSwapchain.ovrSwapchain.pop_back();
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
        if (xrSwapchain.frozen || xrSwapchain.acquiredIndices.size() == xrSwapchain.ovrSwapchainLength) {
            return XR_ERROR_CALL_ORDER_INVALID;
        }

        // Query the image index from OVR.
        int imageIndex = xrSwapchain.nextIndex;
        if (xrSwapchain.acquiredIndices.empty()) {
            // "Re-synchronize" to the underlying swapchain. This should not be needed, but add robustness in case of a
            // bug.
            CHECK_OVRCMD(ovr_GetTextureSwapChainCurrentIndex(m_ovrSession, xrSwapchain.ovrSwapchain[0], &imageIndex));
        }

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

        // We will commit the texture to OVR during xrEndFrame() in order to handle texture arrays properly.
        xrSwapchain.lastReleasedIndex = xrSwapchain.lastWaitedIndex;
        xrSwapchain.lastWaitedIndex = -1;
        xrSwapchain.acquiredIndices.pop_front();

        return XR_SUCCESS;
    }

} // namespace virtualdesktop_openxr

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

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateViewConfigurations
    XrResult OpenXrRuntime::xrEnumerateViewConfigurations(XrInstance instance,
                                                          XrSystemId systemId,
                                                          uint32_t viewConfigurationTypeCapacityInput,
                                                          uint32_t* viewConfigurationTypeCountOutput,
                                                          XrViewConfigurationType* viewConfigurationTypes) {
        // We only support Stereo 3D.
        static const XrViewConfigurationType types[] = {XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};

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

        if (viewConfigurationTypeCapacityInput && viewConfigurationTypeCapacityInput < ARRAYSIZE(types)) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *viewConfigurationTypeCountOutput = ARRAYSIZE(types);
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateViewConfigurations",
                          TLArg(*viewConfigurationTypeCountOutput, "ViewConfigurationTypeCountOutput"));

        if (viewConfigurationTypes) {
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

        configurationProperties->viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        configurationProperties->fovMutable = XR_TRUE;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetViewConfigurationProperties",
                          TLArg(xr::ToCString(configurationProperties->viewConfigurationType), "ViewConfigurationType"),
                          TLArg(configurationProperties->fovMutable, "FovMutable"));

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

        if (views) {
            for (uint32_t i = 0; i < *viewCountOutput; i++) {
                if (views[i].type != XR_TYPE_VIEW_CONFIGURATION_VIEW) {
                    return XR_ERROR_VALIDATION_FAILURE;
                }

                views[i].maxImageRectWidth = 16384;
                views[i].maxImageRectHeight = 16384;

                // TODO: Do we support multisampling?
                views[i].recommendedSwapchainSampleCount = views[i].maxSwapchainSampleCount = 1;

                // Recommend the resolution with distortion accounted for.
                views[i].recommendedImageRectWidth = m_cachedEyeInfo[i].DistortedViewport.Size.w;
                views[i].recommendedImageRectHeight = m_cachedEyeInfo[i].DistortedViewport.Size.h;

                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateViewConfigurationViews",
                                  TLArg(views[i].maxImageRectWidth, "MaxImageRectWidth"),
                                  TLArg(views[i].maxImageRectHeight, "MaxImageRectHeight"),
                                  TLArg(views[i].maxSwapchainSampleCount, "MaxSwapchainSampleCount"),
                                  TLArg(views[i].recommendedImageRectWidth, "RecommendedImageRectWidth"),
                                  TLArg(views[i].recommendedImageRectHeight, "RecommendedImageRectHeight"),
                                  TLArg(views[i].recommendedSwapchainSampleCount, "RecommendedSwapchainSampleCount"));
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateSwapchainFormats
    XrResult OpenXrRuntime::xrEnumerateSwapchainFormats(XrSession session,
                                                        uint32_t formatCapacityInput,
                                                        uint32_t* formatCountOutput,
                                                        int64_t* formats) {
        // We match exactly what pvrTextureFormat lists for use.
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

                DXGI_FORMAT_BC1_UNORM,
                DXGI_FORMAT_BC1_UNORM_SRGB,
                DXGI_FORMAT_BC2_UNORM,
                DXGI_FORMAT_BC2_UNORM_SRGB,
                DXGI_FORMAT_BC3_UNORM,
                DXGI_FORMAT_BC3_UNORM_SRGB,
                DXGI_FORMAT_BC6H_UF16,
                DXGI_FORMAT_BC6H_SF16,
                DXGI_FORMAT_BC7_UNORM,
                DXGI_FORMAT_BC7_UNORM_SRGB,
                DXGI_FORMAT_R11G11B10_FLOAT,
            // clang-format on
        };
        static const VkFormat vkFormats[] = {
            // clang-format off
                VK_FORMAT_R8G8B8A8_SRGB, // Prefer SRGB formats.
                VK_FORMAT_B8G8R8A8_SRGB,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_B8G8R8A8_UNORM,
                VK_FORMAT_B8G8R8A8_SRGB,
                VK_FORMAT_B8G8R8A8_UNORM,
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_FORMAT_D32_SFLOAT, // Prefer 32-bit depth.
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM,

                VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
                VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
                VK_FORMAT_BC2_UNORM_BLOCK,
                VK_FORMAT_BC2_SRGB_BLOCK,
                VK_FORMAT_BC3_UNORM_BLOCK,
                VK_FORMAT_BC3_SRGB_BLOCK,
                VK_FORMAT_BC6H_UFLOAT_BLOCK,
                VK_FORMAT_BC6H_SFLOAT_BLOCK,
                VK_FORMAT_BC7_UNORM_BLOCK,
                VK_FORMAT_BC7_SRGB_BLOCK,
                VK_FORMAT_B10G11R11_UFLOAT_PACK32,
            // clang-format on
        };

        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateSwapchainFormats",
                          TLXArg(session, "Session"),
                          TLArg(formatCapacityInput, "FormatCapacityInput"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        const uint32_t count = isVulkanSession() ? ARRAYSIZE(vkFormats) : ARRAYSIZE(d3dFormats);

        if (formatCapacityInput && formatCapacityInput < count) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *formatCountOutput = count;
        TraceLoggingWrite(
            g_traceProvider, "xrEnumerateSwapchainFormats", TLArg(*formatCountOutput, "FormatCountOutput"));

        if (formats) {
            for (uint32_t i = 0; i < *formatCountOutput; i++) {
                if (isVulkanSession()) {
                    formats[i] = (int64_t)vkFormats[i];
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

        // We don't support cubemaps.
        if (createInfo->faceCount != 1) {
            return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
        }

        if (createInfo->createFlags & XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT) {
            return XR_ERROR_FEATURE_UNSUPPORTED;
        }

        pvrTextureSwapChainDesc desc{};

        desc.Format = isVulkanSession() ? vkToPvrTextureFormat((VkFormat)createInfo->format)
                                        : dxgiToPvrTextureFormat((DXGI_FORMAT)createInfo->format);
        if (desc.Format == PVR_FORMAT_UNKNOWN) {
            return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
        }
        desc.MiscFlags = pvrTextureMisc_DX_Typeless; // OpenXR requires to return typeless texures.

        // Request a swapchain from PVR.
        desc.Type = pvrTexture_2D;
        desc.StaticImage = !!(createInfo->createFlags & XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT);
        desc.ArraySize = createInfo->arraySize;
        desc.Width = createInfo->width;
        desc.Height = createInfo->height;
        desc.MipLevels = createInfo->mipCount;
        if (desc.MipLevels > 1) {
            desc.MiscFlags |= pvrTextureMisc_AllowGenerateMips;
        }
        desc.SampleCount = createInfo->sampleCount;

        if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) {
            desc.BindFlags |= pvrTextureBind_DX_RenderTarget;
        }
        if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            desc.BindFlags |= pvrTextureBind_DX_DepthStencil;
        }
        if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) {
            desc.BindFlags |= pvrTextureBind_DX_UnorderedAccess;
        }

        // There are 2 situations in PVR where we cannot use the PVR swapchain alone:
        // - PVR does not let you submit a slice of a texture array and always reads from the first slice.
        //   To mitigate this, we will create several swapchains with ArraySize=1 and we will make copies during
        //   xrEndFrame().
        //
        // - PVR does not like the D32_FLOAT_S8X24 format.
        //   To mitigate this, we will create a D32_FLOAT swapchain and perform a conversion during xrEndFrame().

        pvrTextureSwapChain pvrSwapchain{};
        bool needDepthResolve = false;
        if (desc.Format == PVR_FORMAT_D32_FLOAT_S8X24_UINT) {
            desc.Format = PVR_FORMAT_D32_FLOAT;
            needDepthResolve = true;
        }
        CHECK_PVRCMD(pvr_createTextureSwapChainDX(m_pvrSession, m_d3d11Device.Get(), &desc, &pvrSwapchain));

        // Create the internal struct.
        Swapchain& xrSwapchain = *new Swapchain;
        xrSwapchain.pvrSwapchain.push_back(pvrSwapchain);
        xrSwapchain.slices.push_back({});
        xrSwapchain.imagesResourceView.push_back({});
        xrSwapchain.pvrDesc = desc;
        xrSwapchain.xrDesc = *createInfo;
        xrSwapchain.needDepthResolve = needDepthResolve;

        // Lazily-filled state.
        for (int i = 1; i < desc.ArraySize; i++) {
            xrSwapchain.pvrSwapchain.push_back(nullptr);
            xrSwapchain.slices.push_back({});
            xrSwapchain.imagesResourceView.push_back({});
        }

        *swapchain = (XrSwapchain)&xrSwapchain;

        // Maintain a list of known swapchains for validation and cleanup.
        m_swapchains.insert(*swapchain);

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateSwapchain",
                          TLXArg(*swapchain, "Swapchain"),
                          TLArg(needDepthResolve, "NeedDepthResolve"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroySwapchain
    XrResult OpenXrRuntime::xrDestroySwapchain(XrSwapchain swapchain) {
        TraceLoggingWrite(g_traceProvider, "xrDestroySwapchain", TLXArg(swapchain, "Swapchain"));

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)swapchain;

        while (!xrSwapchain.pvrSwapchain.empty()) {
            auto pvrSwapchain = xrSwapchain.pvrSwapchain.back();
            if (pvrSwapchain) {
                pvr_destroyTextureSwapChain(m_pvrSession, pvrSwapchain);
            }
            xrSwapchain.pvrSwapchain.pop_back();
        }

        while (!xrSwapchain.vkImages.empty()) {
            m_vkDispatch.vkDestroyImage(m_vkDevice, xrSwapchain.vkImages.back(), m_vkAllocator);
            xrSwapchain.vkImages.pop_back();
        }

        while (!xrSwapchain.vkDeviceMemory.empty()) {
            m_vkDispatch.vkFreeMemory(m_vkDevice, xrSwapchain.vkDeviceMemory.back(), m_vkAllocator);
            xrSwapchain.vkDeviceMemory.pop_back();
        }

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

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)swapchain;

        int count = -1;
        CHECK_PVRCMD(pvr_getTextureSwapChainLength(m_pvrSession, xrSwapchain.pvrSwapchain[0], &count));

        if (imageCapacityInput && imageCapacityInput < (uint32_t)count) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *imageCountOutput = count;
        TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainImages", TLArg(*imageCountOutput, "ImageCountOutput"));

        if (images) {
            if (isD3D12Session()) {
                XrSwapchainImageD3D12KHR* d3d12Images = reinterpret_cast<XrSwapchainImageD3D12KHR*>(images);
                return getSwapchainImagesD3D12(xrSwapchain, d3d12Images, *imageCountOutput);
            } else if (isVulkanSession()) {
                XrSwapchainImageVulkanKHR* vkImages = reinterpret_cast<XrSwapchainImageVulkanKHR*>(images);
                return getSwapchainImagesVulkan(xrSwapchain, vkImages, *imageCountOutput);
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

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)swapchain;

        // Query the image index from PVR.
        int pvrIndex = -1;
        if (!xrSwapchain.needDepthResolve) {
            CHECK_PVRCMD(pvr_getTextureSwapChainCurrentIndex(m_pvrSession, xrSwapchain.pvrSwapchain[0], &pvrIndex));
        } else {
            pvrIndex = xrSwapchain.currentIndex++;
            if (xrSwapchain.currentIndex >= xrSwapchain.images.size()) {
                xrSwapchain.currentIndex = 0;
            }
        }

        *index = pvrIndex;

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

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // We assume that our frame timing in xrWaitFrame() guaranteed availability of the next image. No wait.

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrReleaseSwapchainImage
    XrResult OpenXrRuntime::xrReleaseSwapchainImage(XrSwapchain swapchain,
                                                    const XrSwapchainImageReleaseInfo* releaseInfo) {
        if (releaseInfo && releaseInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrReleaseSwapchainImage", TLXArg(swapchain, "Swapchain"));

        if (!m_swapchains.count(swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // We will commit the texture to PVR during xrEndFrame() in order to handle texture arrays properly. Nothing
        // to do here.

        return XR_SUCCESS;
    }

} // namespace pimax_openxr

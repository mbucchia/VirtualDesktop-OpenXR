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

// Implements the necessary support for the XR_KHR_openhl_enable extension:
// NOTE: PVR has native support for OpenGL, however it is buggy, therefore we implement support as interoperability to
//       D3D11 (like we do for Vulkan).
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_opengl_enable

namespace pimax_openxr {
    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetOpenGLGraphicsRequirementsKHR
    XrResult OpenXrRuntime::xrGetOpenGLGraphicsRequirementsKHR(XrInstance instance,
                                                               XrSystemId systemId,
                                                               XrGraphicsRequirementsOpenGLKHR* graphicsRequirements) {
        if (graphicsRequirements->type != XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetOpenGLGraphicsRequirementsKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (!m_isOpenGLSupported) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        // Get the display device LUID.
        fillDisplayDeviceInfo();

        // External objects require OpenGL 4.5.
        graphicsRequirements->minApiVersionSupported = XR_MAKE_VERSION(4, 0, 0);
        graphicsRequirements->maxApiVersionSupported = XR_MAKE_VERSION(5, 0, 0);

        TraceLoggingWrite(
            g_traceProvider,
            "xrGetOpenGLGraphicsRequirementsKHR",
            TLArg(xr::ToString(graphicsRequirements->minApiVersionSupported).c_str(), "MinApiVersionSupported"),
            TLArg(xr::ToString(graphicsRequirements->maxApiVersionSupported).c_str(), "MaxApiVersionSupported"));

        m_graphicsRequirementQueried = true;

        return XR_SUCCESS;
    }

    // Initialize all the resources needed for OpenGL interoperation with the D3D11 backend.
    XrResult OpenXrRuntime::initializeOpenGL(const XrGraphicsBindingOpenGLWin32KHR& glBindings) {
        // Gather function pointers for the OpenGL extensions we are going to use.
        initializeOpenGLDispatch();

        m_glContext.glDC = glBindings.hDC;
        m_glContext.glRC = glBindings.hGLRC;
        m_glContext.valid = true;

        GlContextSwitch context(m_glContext);

        // Check that this is the correct adapter for the HMD.
        LUID adapterLuid{};
        m_glDispatch.glGetUnsignedBytevEXT(GL_DEVICE_LUID_EXT, (GLubyte*)&adapterLuid);
        if (memcmp(&adapterLuid, &m_adapterLuid, sizeof(LUID))) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        ComPtr<IDXGIFactory1> dxgiFactory;
        CHECK_HRCMD(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter1> dxgiAdapter;
        for (UINT adapterIndex = 0;; adapterIndex++) {
            // EnumAdapters1 will fail with DXGI_ERROR_NOT_FOUND when there are no more adapters to
            // enumerate.
            CHECK_HRCMD(dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.ReleaseAndGetAddressOf()));

            DXGI_ADAPTER_DESC1 desc;
            CHECK_HRCMD(dxgiAdapter->GetDesc1(&desc));
            if (!memcmp(&desc.AdapterLuid, &m_adapterLuid, sizeof(LUID))) {
                std::string deviceName;
                const std::wstring wadapterDescription(desc.Description);
                std::transform(wadapterDescription.begin(),
                               wadapterDescription.end(),
                               std::back_inserter(deviceName),
                               [](wchar_t c) { return (char)c; });

                TraceLoggingWrite(g_traceProvider,
                                  "xrCreateSession",
                                  TLArg("OpenGL", "Api"),
                                  TLArg(deviceName.c_str(), "AdapterName"));
                Log("Using OpenGL on adapter: %s\n", deviceName.c_str());
                break;
            }
        }

        // Create the interop device that PVR will be using.
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> deviceContext;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        CHECK_HRCMD(D3D11CreateDevice(dxgiAdapter.Get(),
                                      D3D_DRIVER_TYPE_UNKNOWN,
                                      0,
                                      flags,
                                      &featureLevel,
                                      1,
                                      D3D11_SDK_VERSION,
                                      device.ReleaseAndGetAddressOf(),
                                      nullptr,
                                      deviceContext.ReleaseAndGetAddressOf()));

        ComPtr<ID3D11Device5> device5;
        CHECK_HRCMD(device->QueryInterface(m_d3d11Device.ReleaseAndGetAddressOf()));

        // Create the Direct3D 11 resources.
        XrGraphicsBindingD3D11KHR d3d11Bindings{};
        d3d11Bindings.device = device.Get();
        const auto result = initializeD3D11(d3d11Bindings, true);
        if (XR_FAILED(result)) {
            return result;
        }

        // Initialize common OpenGL resources.

        // We will use a shared fence to synchronize between the OpenGL context and the D3D11
        // context.
        m_glDispatch.glGenSemaphoresEXT(1, &m_glSemaphore);

        wil::unique_handle fenceHandle = nullptr;
        CHECK_HRCMD(m_d3d11Device->CreateFence(
            0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(m_d3d11Fence.ReleaseAndGetAddressOf())));
        CHECK_HRCMD(
            m_d3d11Fence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, m_fenceHandleForAMDWorkaround.put()));

        // On the OpenGL side, it is called a semaphore.
        m_glDispatch.glImportSemaphoreWin32HandleEXT(
            m_glSemaphore, GL_HANDLE_TYPE_D3D12_FENCE_EXT, m_fenceHandleForAMDWorkaround.get());

        return XR_SUCCESS;
    }

    // Initialize the function pointers for the OpenGL extensions.
    void OpenXrRuntime::initializeOpenGLDispatch() {
#define GL_GET_PTR(fun)                                                                                                \
    m_glDispatch.fun = reinterpret_cast<decltype(m_glDispatch.fun)>(wglGetProcAddress(#fun));                          \
    CHECK_MSG(m_glDispatch.fun, "OpenGL driver does not support " #fun);

        GL_GET_PTR(glGetUnsignedBytevEXT);
        GL_GET_PTR(glCreateTextures);
        GL_GET_PTR(glCreateMemoryObjectsEXT);
        GL_GET_PTR(glDeleteMemoryObjectsEXT);
        GL_GET_PTR(glTextureStorageMem2DEXT);
        GL_GET_PTR(glTextureStorageMem2DMultisampleEXT);
        GL_GET_PTR(glTextureStorageMem3DEXT);
        GL_GET_PTR(glTextureStorageMem3DMultisampleEXT);
        GL_GET_PTR(glGenSemaphoresEXT);
        GL_GET_PTR(glDeleteSemaphoresEXT);
        GL_GET_PTR(glSemaphoreParameterui64vEXT);
        GL_GET_PTR(glSignalSemaphoreEXT);
        GL_GET_PTR(glImportMemoryWin32HandleEXT);
        GL_GET_PTR(glImportSemaphoreWin32HandleEXT);

#undef GL_GET_PTR
    }

    void OpenXrRuntime::cleanupOpenGL() {
        if (m_glContext.valid) {
            GlContextSwitch context(m_glContext);

            glFinish();

            m_glDispatch.glDeleteSemaphoresEXT(1, &m_glSemaphore);
            m_fenceHandleForAMDWorkaround.reset();

            m_glContext.valid = false;
        }
    }

    bool OpenXrRuntime::isOpenGLSession() const {
        return m_glContext.valid;
    }

    // Retrieve the swapchain images for the application to use.
    XrResult OpenXrRuntime::getSwapchainImagesOpenGL(Swapchain& xrSwapchain,
                                                     XrSwapchainImageOpenGLKHR* glImages,
                                                     uint32_t count) {
        GlContextSwitch context(m_glContext);

        // Detect whether this is the first call for this swapchain.
        const bool initialized = !xrSwapchain.slices[0].empty();

        std::vector<XrSwapchainImageD3D11KHR> d3d11Images(count, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
        if (!initialized) {
            // Query the D3D11 textures.
            const auto result = getSwapchainImagesD3D11(xrSwapchain, d3d11Images.data(), count, true);
            if (XR_FAILED(result)) {
                return result;
            }
        }

        // Export each D3D11 texture to OpenGL.
        for (uint32_t i = 0; i < count; i++) {
            if (glImages[i].type != XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (!initialized) {
                HANDLE textureHandle;
                ComPtr<IDXGIResource1> dxgiResource;
                CHECK_HRCMD(
                    d3d11Images[i].texture->QueryInterface(IID_PPV_ARGS(dxgiResource.ReleaseAndGetAddressOf())));
                CHECK_HRCMD(dxgiResource->GetSharedHandle(&textureHandle));

                // Import the device memory from D3D.
                GLuint memory;
                m_glDispatch.glCreateMemoryObjectsEXT(1, &memory);
                xrSwapchain.glMemory.push_back(memory);

                const size_t bytePerPixels = glGetBytePerPixels((GLenum)xrSwapchain.xrDesc.format);

                // TODO: Not sure why we need to multiply by 2. Mipmapping?
                // https://stackoverflow.com/questions/71108346/how-to-use-glimportmemorywin32handleext-to-share-an-id3d11texture2d-keyedmutex-s
                m_glDispatch.glImportMemoryWin32HandleEXT(memory,
                                                          xrSwapchain.xrDesc.width * xrSwapchain.xrDesc.height *
                                                              xrSwapchain.xrDesc.sampleCount * bytePerPixels * 2,
                                                          GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT,
                                                          textureHandle);

                // Create the texture that the app will use.
                GLuint image;
                if (xrSwapchain.xrDesc.arraySize == 1) {
                    if (xrSwapchain.xrDesc.sampleCount == 1) {
                        m_glDispatch.glCreateTextures(GL_TEXTURE_2D, 1, &image);
                        m_glDispatch.glTextureStorageMem2DEXT(image,
                                                              xrSwapchain.xrDesc.mipCount,
                                                              (GLenum)xrSwapchain.xrDesc.format,
                                                              xrSwapchain.xrDesc.width,
                                                              xrSwapchain.xrDesc.height,
                                                              memory,
                                                              0);
                    } else {
                        m_glDispatch.glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &image);
                        m_glDispatch.glTextureStorageMem2DMultisampleEXT(image,
                                                                         xrSwapchain.xrDesc.sampleCount,
                                                                         (GLenum)xrSwapchain.xrDesc.format,
                                                                         xrSwapchain.xrDesc.width,
                                                                         xrSwapchain.xrDesc.height,
                                                                         GL_TRUE,
                                                                         memory,
                                                                         0);
                    }
                } else {
                    if (xrSwapchain.xrDesc.sampleCount == 1) {
                        m_glDispatch.glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &image);
                        m_glDispatch.glTextureStorageMem3DEXT(image,
                                                              xrSwapchain.xrDesc.mipCount,
                                                              (GLenum)xrSwapchain.xrDesc.format,
                                                              xrSwapchain.xrDesc.width,
                                                              xrSwapchain.xrDesc.height,
                                                              xrSwapchain.xrDesc.arraySize,
                                                              memory,
                                                              0);
                    } else {
                        m_glDispatch.glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, &image);
                        m_glDispatch.glTextureStorageMem3DMultisampleEXT(image,
                                                                         xrSwapchain.xrDesc.sampleCount,
                                                                         (GLenum)xrSwapchain.xrDesc.format,
                                                                         xrSwapchain.xrDesc.width,
                                                                         xrSwapchain.xrDesc.height,
                                                                         xrSwapchain.xrDesc.arraySize,
                                                                         GL_TRUE,
                                                                         memory,
                                                                         0);
                    }
                }
                xrSwapchain.glImages.push_back(image);
            }

            glImages[i].image = xrSwapchain.glImages[i];

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateSwapchainImages",
                              TLArg("OpenGL", "Api"),
                              TLArg(glImages[i].image, "Texture"));
        }

        return XR_SUCCESS;
    }

    // Flush any pending work.
    void OpenXrRuntime::flushOpenGLContext() {
        GlContextSwitch context(m_glContext);

        glFinish();
    }

    // Serialize commands from the OpenGL context to the D3D11 context used by PVR.
    void OpenXrRuntime::serializeOpenGLFrame() {
        GlContextSwitch context(m_glContext);

        m_fenceValue++;
        TraceLoggingWrite(g_traceProvider,
                          "xrEndFrame_Sync",
                          TLArg("OpenGL", "Api"),
                          TLArg(m_fenceValue, "FenceValue"),
                          TLArg(m_gpuTimerSynchronizationDuration[m_currentTimerIndex]->query(), "SyncDurationUs"),
                          TLArg(k_numGpuTimers - 1, "MeasurementLatency"));
        m_glDispatch.glSemaphoreParameterui64vEXT(m_glSemaphore, GL_D3D12_FENCE_VALUE_EXT, &m_fenceValue);
        m_glDispatch.glSignalSemaphoreEXT(m_glSemaphore, 0, nullptr, 0, nullptr, nullptr);
        glFlush();

        if (IsTraceEnabled()) {
            m_gpuTimerSynchronizationDuration[m_currentTimerIndex]->start();
        }
        CHECK_HRCMD(m_d3d11DeviceContext->Wait(m_d3d11Fence.Get(), m_fenceValue));
        if (IsTraceEnabled()) {
            m_gpuTimerSynchronizationDuration[m_currentTimerIndex]->stop();
        }
    }

} // namespace pimax_openxr

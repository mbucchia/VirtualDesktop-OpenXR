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

// Implements the necessary support for the XR_KHR_opengl_enable extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_opengl_enable

namespace virtualdesktop_openxr {
    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;

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

        if (!has_XR_KHR_opengl_enable) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

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
        if (!glBindings.hDC || !glBindings.hGLRC) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

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

        // Create the interop device and resources that OVR will be using.
        initializeSubmissionDevice("OpenGL");

        // We will use a shared fence to synchronize between the OpenGL context and the D3D11
        // context.
        CHECK_HRCMD(m_ovrSubmissionFence->CreateSharedHandle(
            nullptr, GENERIC_ALL, nullptr, m_fenceHandleForAMDWorkaround.put()));

        // On the OpenGL side, it is called a semaphore.
        m_glDispatch.glGenSemaphoresEXT(1, &m_glSemaphore);
        m_glDispatch.glImportSemaphoreWin32HandleEXT(
            m_glSemaphore, GL_HANDLE_TYPE_D3D12_FENCE_EXT, m_fenceHandleForAMDWorkaround.get());

        // Frame timers.
        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerApp[i] = std::make_unique<GlGpuTimer>(m_glDispatch, m_glContext);
        }

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
        GL_GET_PTR(glGenQueries);
        GL_GET_PTR(glDeleteQueries);
        GL_GET_PTR(glQueryCounter);
        GL_GET_PTR(glGetQueryObjectiv);
        GL_GET_PTR(glGetQueryObjectui64v);

#undef GL_GET_PTR
    }

    void OpenXrRuntime::cleanupOpenGL() {
        if (m_glContext.valid) {
            GlContextSwitch context(m_glContext);

            glFinish();

            for (uint32_t i = 0; i < k_numGpuTimers; i++) {
                m_gpuTimerApp[i].reset();
            }

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

        std::vector<HANDLE> textureHandles;
        if (!initialized) {
            // Query the swapchain textures.
            textureHandles = getSwapchainImages(xrSwapchain);
        }

        // Export each D3D11 texture to OpenGL.
        for (uint32_t i = 0; i < count; i++) {
            if (glImages[i].type != XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (!initialized) {
                // Import the device memory from D3D.
                GLuint memory;
                m_glDispatch.glCreateMemoryObjectsEXT(1, &memory);
                xrSwapchain.glMemory.push_back(memory);

                const size_t bytePerPixels = glGetBytePerPixels((GLenum)xrSwapchain.xrDesc.format);

                // TODO: Not sure why we need to multiply by 2. Mipmapping?
                // https://stackoverflow.com/questions/71108346/how-to-use-glimportmemorywin32handleext-to-share-an-id3d11texture2d-keyedmutex-s
                const auto memorySize = xrSwapchain.xrDesc.arraySize * xrSwapchain.xrDesc.width *
                                        xrSwapchain.xrDesc.height * xrSwapchain.xrDesc.sampleCount * bytePerPixels * 2;
                m_glDispatch.glImportMemoryWin32HandleEXT(
                    memory, memorySize, GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT, textureHandles[i]);

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

    // Serialize commands from the OpenGL context to the D3D11 context used by OVR.
    void OpenXrRuntime::serializeOpenGLFrame() {
        GlContextSwitch context(m_glContext);

        m_fenceValue++;
        TraceLoggingWrite(
            g_traceProvider, "xrEndFrame_Sync", TLArg("OpenGL", "Api"), TLArg(m_fenceValue, "FenceValue"));
        m_glDispatch.glSemaphoreParameterui64vEXT(m_glSemaphore, GL_D3D12_FENCE_VALUE_EXT, &m_fenceValue);
        m_glDispatch.glSignalSemaphoreEXT(m_glSemaphore, 0, nullptr, 0, nullptr, nullptr);
        glFlush();

        waitOnSubmissionDevice();
    }

} // namespace virtualdesktop_openxr

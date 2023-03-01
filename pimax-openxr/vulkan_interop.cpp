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

// Implements the necessary support for the XR_KHR_vulkan_enable and XR_KHR_vulkan_enable2 extensions:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_vulkan_enable
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_vulkan_enable2

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetVulkanInstanceExtensionsKHR
    XrResult OpenXrRuntime::xrGetVulkanInstanceExtensionsKHR(XrInstance instance,
                                                             XrSystemId systemId,
                                                             uint32_t bufferCapacityInput,
                                                             uint32_t* bufferCountOutput,
                                                             char* buffer) {
        static const std::string_view instanceExtensions =
            "VK_KHR_external_memory_capabilities VK_KHR_external_semaphore_capabilities "
            "VK_KHR_external_fence_capabilities "
            "VK_KHR_get_physical_device_properties2";

        TraceLoggingWrite(g_traceProvider,
                          "xrGetVulkanInstanceExtensionsKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"),
                          TLArg(bufferCapacityInput, "BufferCapacityInput"));

        // This function is used by our XR_KHR_vulkan_enable2 wrapper.
        if (!has_XR_KHR_vulkan_enable && !has_XR_KHR_vulkan_enable2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (bufferCapacityInput && bufferCapacityInput < instanceExtensions.size()) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *bufferCountOutput = (uint32_t)instanceExtensions.size() + 1;
        TraceLoggingWrite(
            g_traceProvider, "xrGetVulkanInstanceExtensionsKHR", TLArg(*bufferCountOutput, "BufferCountOutput"));

        if (bufferCapacityInput && buffer) {
            sprintf_s(buffer, bufferCapacityInput, "%s", instanceExtensions.data());
            TraceLoggingWrite(g_traceProvider, "xrGetVulkanInstanceExtensionsKHR", TLArg(buffer, "Extension"));
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetVulkanDeviceExtensionsKHR
    XrResult OpenXrRuntime::xrGetVulkanDeviceExtensionsKHR(XrInstance instance,
                                                           XrSystemId systemId,
                                                           uint32_t bufferCapacityInput,
                                                           uint32_t* bufferCountOutput,
                                                           char* buffer) {
        static const std::string_view deviceExtensions =
            "VK_KHR_dedicated_allocation VK_KHR_get_memory_requirements2 VK_KHR_bind_memory2 "
            "VK_KHR_external_memory "
            "VK_KHR_external_memory_win32 VK_KHR_timeline_semaphore "
            "VK_KHR_external_semaphore VK_KHR_external_semaphore_win32";

        TraceLoggingWrite(g_traceProvider,
                          "xrGetVulkanDeviceExtensionsKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"),
                          TLArg(bufferCapacityInput, "BufferCapacityInput"));

        // This function is used by our XR_KHR_vulkan_enable2 wrapper.
        if (!has_XR_KHR_vulkan_enable && !has_XR_KHR_vulkan_enable2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (bufferCapacityInput && bufferCapacityInput < deviceExtensions.size()) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *bufferCountOutput = (uint32_t)deviceExtensions.size() + 1;
        TraceLoggingWrite(
            g_traceProvider, "xrGetVulkanDeviceExtensionsKHR", TLArg(*bufferCountOutput, "BufferCountOutput"));

        if (bufferCapacityInput && buffer) {
            sprintf_s(buffer, bufferCapacityInput, "%s", deviceExtensions.data());
            TraceLoggingWrite(g_traceProvider, "xrGetVulkanDeviceExtensionsKHR", TLArg(buffer, "Extension"));
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetVulkanGraphicsDeviceKHR
    XrResult OpenXrRuntime::xrGetVulkanGraphicsDeviceKHR(XrInstance instance,
                                                         XrSystemId systemId,
                                                         VkInstance vkInstance,
                                                         VkPhysicalDevice* vkPhysicalDevice) {
        TraceLoggingWrite(g_traceProvider,
                          "xrGetVulkanGraphicsDeviceKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"),
                          TLPArg(vkInstance, "VkInstance"));

        // This function is used by our XR_KHR_vulkan_enable2 wrapper.
        if (!has_XR_KHR_vulkan_enable && !has_XR_KHR_vulkan_enable2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        // Get the display device LUID.
        fillDisplayDeviceInfo();

        uint32_t deviceCount = 0;
        CHECK_VKCMD(vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr));
        std::vector<VkPhysicalDevice> devices(deviceCount);
        CHECK_VKCMD(vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data()));

        // Match the Vulkan physical device to the adapter LUID returned by PVR.
        bool found = false;
        for (const VkPhysicalDevice& device : devices) {
            VkPhysicalDeviceIDProperties deviceId{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
            VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &deviceId};
            vkGetPhysicalDeviceProperties2(device, &properties);

            if (!deviceId.deviceLUIDValid) {
                continue;
            }

            if (!memcmp(&m_adapterLuid, deviceId.deviceLUID, sizeof(LUID))) {
                TraceLoggingWrite(
                    g_traceProvider, "xrGetVulkanDeviceExtensionsKHR", TLPArg(device, "VkPhysicalDevice"));
                *vkPhysicalDevice = device;
                found = true;
                break;
            }
        }

        return found ? XR_SUCCESS : XR_ERROR_RUNTIME_FAILURE;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateVulkanInstanceKHR
    // This wrapper is adapted from Khronos SDK's Vulkan plugin.
    XrResult OpenXrRuntime::xrCreateVulkanInstanceKHR(XrInstance instance,
                                                      const XrVulkanInstanceCreateInfoKHR* createInfo,
                                                      VkInstance* vulkanInstance,
                                                      VkResult* vulkanResult) {
        if (createInfo->type != XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateVulkanInstanceKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)createInfo->systemId, "SystemId"),
                          TLArg((int)createInfo->createFlags, "CreateFlags"),
                          TLPArg(createInfo->pfnGetInstanceProcAddr, "GetInstanceProcAddr"));

        if (!has_XR_KHR_vulkan_enable2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || createInfo->systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        uint32_t extensionNamesSize = 0;
        CHECK_XRCMD(xrGetVulkanInstanceExtensionsKHR(instance, createInfo->systemId, 0, &extensionNamesSize, nullptr));
        std::vector<char> extensionNames(extensionNamesSize);
        CHECK_XRCMD(xrGetVulkanInstanceExtensionsKHR(
            instance, createInfo->systemId, extensionNamesSize, &extensionNamesSize, &extensionNames[0]));

        // Note: This cannot outlive the extensionNames above, since it's just a collection of views into that
        // string!
        std::vector<const char*> extensions = ParseExtensionString(&extensionNames[0]);

        // Merge the runtime's request with the applications requests
        for (uint32_t i = 0; i < createInfo->vulkanCreateInfo->enabledExtensionCount; ++i) {
            extensions.push_back(createInfo->vulkanCreateInfo->ppEnabledExtensionNames[i]);
        }

        for (uint32_t i = 0; i < extensions.size(); i++) {
            TraceLoggingWrite(g_traceProvider, "xrCreateVulkanInstanceKHR", TLArg(extensions[i], "Extension"));
        }

        VkInstanceCreateInfo instInfo = *createInfo->vulkanCreateInfo;
        instInfo.enabledExtensionCount = (uint32_t)extensions.size();
        instInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

        auto pfnCreateInstance = (PFN_vkCreateInstance)createInfo->pfnGetInstanceProcAddr(nullptr, "vkCreateInstance");
        *vulkanResult = pfnCreateInstance(&instInfo, createInfo->vulkanAllocator, vulkanInstance);

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateVulkanInstanceKHR",
                          TLPArg(*vulkanInstance, "VkInstance"),
                          TLArg((int)*vulkanResult, "VkResult"));

        m_vkBootstrapInstance = *vulkanInstance;

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateVulkanDeviceKHR
    // This wrapper is adapted from Khronos SDK's Vulkan plugin.
    XrResult OpenXrRuntime::xrCreateVulkanDeviceKHR(XrInstance instance,
                                                    const XrVulkanDeviceCreateInfoKHR* createInfo,
                                                    VkDevice* vulkanDevice,
                                                    VkResult* vulkanResult) {
        if (createInfo->type != XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "XrVulkanDeviceCreateInfoKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)createInfo->systemId, "SystemId"),
                          TLArg((int)createInfo->createFlags, "CreateFlags"),
                          TLPArg(createInfo->pfnGetInstanceProcAddr, "GetInstanceProcAddr"),
                          TLPArg(createInfo->vulkanPhysicalDevice, "VkPhysicalDevice"));

        if (!has_XR_KHR_vulkan_enable2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || createInfo->systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        uint32_t deviceExtensionNamesSize = 0;
        CHECK_XRCMD(
            xrGetVulkanDeviceExtensionsKHR(instance, createInfo->systemId, 0, &deviceExtensionNamesSize, nullptr));
        std::vector<char> deviceExtensionNames(deviceExtensionNamesSize);
        CHECK_XRCMD(xrGetVulkanDeviceExtensionsKHR(instance,
                                                   createInfo->systemId,
                                                   deviceExtensionNamesSize,
                                                   &deviceExtensionNamesSize,
                                                   &deviceExtensionNames[0]));

        // Note: This cannot outlive the extensionNames above, since it's just a collection of views into that
        // string!
        std::vector<const char*> extensions = ParseExtensionString(&deviceExtensionNames[0]);

        // Merge the runtime's request with the applications requests
        for (uint32_t i = 0; i < createInfo->vulkanCreateInfo->enabledExtensionCount; ++i) {
            extensions.push_back(createInfo->vulkanCreateInfo->ppEnabledExtensionNames[i]);
        }

        for (uint32_t i = 0; i < extensions.size(); i++) {
            TraceLoggingWrite(g_traceProvider, "xrCreateVulkanDeviceKHR", TLArg(extensions[i], "Extension"));
        }

        // Enable timeline semaphores.
        VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
        timelineSemaphoreFeatures.timelineSemaphore = true;

        VkDeviceCreateInfo deviceInfo = *createInfo->vulkanCreateInfo;
        timelineSemaphoreFeatures.pNext = (void*)deviceInfo.pNext;
        deviceInfo.pNext = &timelineSemaphoreFeatures;
        deviceInfo.enabledExtensionCount = (uint32_t)extensions.size();
        deviceInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

        auto pfnCreateDevice =
            (PFN_vkCreateDevice)createInfo->pfnGetInstanceProcAddr(m_vkBootstrapInstance, "vkCreateDevice");
        *vulkanResult =
            pfnCreateDevice(m_vkBootstrapPhysicalDevice, &deviceInfo, createInfo->vulkanAllocator, vulkanDevice);

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateVulkanDeviceKHR",
                          TLPArg(*vulkanDevice, "VkDevice"),
                          TLArg((int)*vulkanResult, "VkResult"));

        m_vkDispatch.vkGetInstanceProcAddr = createInfo->pfnGetInstanceProcAddr;
        m_vkAllocator = createInfo->vulkanAllocator;

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetVulkanGraphicsDevice2KHR
    // This wrapper is adapted from Khronos SDK's Vulkan plugin.
    XrResult OpenXrRuntime::xrGetVulkanGraphicsDevice2KHR(XrInstance instance,
                                                          const XrVulkanGraphicsDeviceGetInfoKHR* getInfo,
                                                          VkPhysicalDevice* vulkanPhysicalDevice) {
        if (getInfo->type != XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetVulkanGraphicsDevice2KHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)getInfo->systemId, "SystemId"),
                          TLPArg(getInfo->vulkanInstance, "VkInstance"));

        if (!has_XR_KHR_vulkan_enable2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || getInfo->systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        CHECK_XRCMD(
            xrGetVulkanGraphicsDeviceKHR(instance, getInfo->systemId, getInfo->vulkanInstance, vulkanPhysicalDevice));

        TraceLoggingWrite(
            g_traceProvider, "xrGetVulkanGraphicsDevice2KHR", TLPArg(*vulkanPhysicalDevice, "VkPhysicalDevice"));

        m_vkBootstrapPhysicalDevice = *vulkanPhysicalDevice;

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetVulkanGraphicsRequirementsKHR
    XrResult OpenXrRuntime::xrGetVulkanGraphicsRequirementsKHR(XrInstance instance,
                                                               XrSystemId systemId,
                                                               XrGraphicsRequirementsVulkanKHR* graphicsRequirements) {
        if (graphicsRequirements->type != XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetVulkanGraphicsRequirementsKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"));

        if (!has_XR_KHR_vulkan_enable && !has_XR_KHR_vulkan_enable2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        // Get the display device LUID.
        fillDisplayDeviceInfo();

        graphicsRequirements->minApiVersionSupported = XR_MAKE_VERSION(1, 1, 0);
        graphicsRequirements->maxApiVersionSupported = XR_MAKE_VERSION(2, 0, 0);

        TraceLoggingWrite(
            g_traceProvider,
            "xrGetVulkanGraphicsRequirementsKHR",
            TLArg(xr::ToString(graphicsRequirements->minApiVersionSupported).c_str(), "MinApiVersionSupported"),
            TLArg(xr::ToString(graphicsRequirements->maxApiVersionSupported).c_str(), "MaxApiVersionSupported"));

        m_graphicsRequirementQueried = true;

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetVulkanGraphicsRequirements2KHR
    XrResult OpenXrRuntime::xrGetVulkanGraphicsRequirements2KHR(XrInstance instance,
                                                                XrSystemId systemId,
                                                                XrGraphicsRequirementsVulkanKHR* graphicsRequirements) {
        return xrGetVulkanGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
    }

    // Initialize all the resources needed for Vulkan interoperation with the D3D11 backend.
    XrResult OpenXrRuntime::initializeVulkan(const XrGraphicsBindingVulkanKHR& vkBindings) {
        if (vkBindings.instance == VK_NULL_HANDLE || vkBindings.device == VK_NULL_HANDLE ||
            vkBindings.physicalDevice == VK_NULL_HANDLE) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        // Gather function pointers for the Vulkan device extensions we are going to use.
        initializeVulkanDispatch(vkBindings.instance);

        // Check that this is the correct adapter for the HMD.
        VkPhysicalDeviceIDProperties deviceId{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &deviceId};
        m_vkDispatch.vkGetPhysicalDeviceProperties2(vkBindings.physicalDevice, &properties);
        if (!deviceId.deviceLUIDValid) {
            return XR_ERROR_RUNTIME_FAILURE;
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
            if (!memcmp(&desc.AdapterLuid, &deviceId.deviceLUID, sizeof(LUID))) {
                const std::string deviceName = xr::wide_to_utf8(desc.Description);

                TraceLoggingWrite(g_traceProvider,
                                  "xrCreateSession",
                                  TLArg("Vulkan", "Api"),
                                  TLArg(deviceName.c_str(), "AdapterName"));
                Log("Using Vulkan on adapter: %s\n", deviceName.c_str());
                break;
            }
        }

        if (memcmp(&deviceId.deviceLUID, &m_adapterLuid, sizeof(LUID))) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        m_vkInstance = vkBindings.instance;
        m_vkDevice = vkBindings.device;
        m_vkPhysicalDevice = vkBindings.physicalDevice;

        // Create the interop device and resources that PVR will be using.
        initializeSubmissionDevice("Vulkan");

        // Initialize common Vulkan resources.
        m_vkDispatch.vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_vkMemoryProperties);
        m_vkDispatch.vkGetDeviceQueue(m_vkDevice, vkBindings.queueFamilyIndex, vkBindings.queueIndex, &m_vkQueue);

        // We will use a shared fence to synchronize between the Vulkan queue and the D3D11
        // context.
        wil::unique_handle fenceHandle;
        CHECK_HRCMD(m_pvrSubmissionFence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, fenceHandle.put()));

        // On the Vulkan side, it is called a timeline semaphore.
        VkSemaphoreTypeCreateInfo timelineCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        VkSemaphoreCreateInfo createInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &timelineCreateInfo};
        CHECK_VKCMD(m_vkDispatch.vkCreateSemaphore(m_vkDevice, &createInfo, m_vkAllocator, &m_vkTimelineSemaphore));
        VkImportSemaphoreWin32HandleInfoKHR importInfo{VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR};
        importInfo.semaphore = m_vkTimelineSemaphore;
        importInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D11_FENCE_BIT;
        importInfo.handle = fenceHandle.get();
        CHECK_VKCMD(m_vkDispatch.vkImportSemaphoreWin32HandleKHR(m_vkDevice, &importInfo));

        // Create an additional semaphore for host-side wait.
        CHECK_VKCMD(
            m_vkDispatch.vkCreateSemaphore(m_vkDevice, &createInfo, m_vkAllocator, &m_vkTimelineSemaphoreForFlush));

        // We will need command buffers to perform layout transitions.
        VkCommandPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolCreateInfo.queueFamilyIndex = vkBindings.queueFamilyIndex;
        CHECK_VKCMD(m_vkDispatch.vkCreateCommandPool(m_vkDevice, &poolCreateInfo, m_vkAllocator, &m_vkCmdPool));
        VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocateInfo.commandPool = m_vkCmdPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;
        CHECK_VKCMD(m_vkDispatch.vkAllocateCommandBuffers(m_vkDevice, &allocateInfo, &m_vkCmdBuffer));

        // Frame timers.
        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            // TODO: m_gpuTimerApp[i] = std::make_unique<GpuTimer>(...);
        }

        return XR_SUCCESS;
    }

    // Initialize the function pointers for the Vulkan instance.
    void OpenXrRuntime::initializeVulkanDispatch(VkInstance instance) {
        PFN_vkGetInstanceProcAddr getProcAddr =
            m_vkDispatch.vkGetInstanceProcAddr ? m_vkDispatch.vkGetInstanceProcAddr : vkGetInstanceProcAddr;

#define VK_GET_PTR(fun) m_vkDispatch.fun = reinterpret_cast<PFN_##fun>(getProcAddr(instance, #fun));

        VK_GET_PTR(vkGetPhysicalDeviceProperties2);
        VK_GET_PTR(vkGetPhysicalDeviceMemoryProperties);
        VK_GET_PTR(vkGetImageMemoryRequirements2KHR);
        VK_GET_PTR(vkGetDeviceQueue);
        VK_GET_PTR(vkQueueSubmit);
        VK_GET_PTR(vkCreateImage);
        VK_GET_PTR(vkDestroyImage);
        VK_GET_PTR(vkAllocateMemory);
        VK_GET_PTR(vkFreeMemory);
        VK_GET_PTR(vkCreateCommandPool);
        VK_GET_PTR(vkDestroyCommandPool);
        VK_GET_PTR(vkAllocateCommandBuffers);
        VK_GET_PTR(vkFreeCommandBuffers);
        VK_GET_PTR(vkResetCommandBuffer);
        VK_GET_PTR(vkBeginCommandBuffer);
        VK_GET_PTR(vkCmdPipelineBarrier);
        VK_GET_PTR(vkEndCommandBuffer);
        VK_GET_PTR(vkGetMemoryWin32HandlePropertiesKHR);
        VK_GET_PTR(vkBindImageMemory2KHR);
        VK_GET_PTR(vkCreateSemaphore);
        VK_GET_PTR(vkDestroySemaphore);
        VK_GET_PTR(vkImportSemaphoreWin32HandleKHR);
        VK_GET_PTR(vkWaitSemaphoresKHR);
        VK_GET_PTR(vkDeviceWaitIdle);

#undef VK_GET_PTR
    }

    void OpenXrRuntime::cleanupVulkan() {
        if (m_vkDispatch.vkDeviceWaitIdle) {
            m_vkDispatch.vkDeviceWaitIdle(m_vkDevice);
        }

        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerApp[i].reset();
        }
        if (m_vkDispatch.vkDestroySemaphore) {
            m_vkDispatch.vkDestroySemaphore(m_vkDevice, m_vkTimelineSemaphore, m_vkAllocator);
            m_vkTimelineSemaphore = VK_NULL_HANDLE;
            m_vkDispatch.vkDestroySemaphore(m_vkDevice, m_vkTimelineSemaphoreForFlush, m_vkAllocator);
            m_vkTimelineSemaphoreForFlush = VK_NULL_HANDLE;
        }
        if (m_vkDispatch.vkResetCommandBuffer) {
            m_vkDispatch.vkResetCommandBuffer(m_vkCmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        }
        if (m_vkDispatch.vkFreeCommandBuffers) {
            m_vkDispatch.vkFreeCommandBuffers(m_vkDevice, m_vkCmdPool, 1, &m_vkCmdBuffer);
            m_vkCmdBuffer = VK_NULL_HANDLE;
        }
        if (m_vkDispatch.vkDestroyCommandPool) {
            m_vkDispatch.vkDestroyCommandPool(m_vkDevice, m_vkCmdPool, m_vkAllocator);
            m_vkCmdPool = VK_NULL_HANDLE;
        }

        // The runtime does not own any of these. Just clear the handles.
        m_vkBootstrapInstance = VK_NULL_HANDLE;
        m_vkBootstrapPhysicalDevice = VK_NULL_HANDLE;
        m_vkInstance = VK_NULL_HANDLE;
        m_vkDevice = VK_NULL_HANDLE;
        ZeroMemory(&m_vkDispatch, sizeof(m_vkDispatch));
        m_vkAllocator = nullptr;
        m_vkPhysicalDevice = VK_NULL_HANDLE;
        m_vkQueue = VK_NULL_HANDLE;
    }

    bool OpenXrRuntime::isVulkanSession() const {
        return m_vkDevice != VK_NULL_HANDLE;
    }

    // Retrieve the swapchain images (VkImage) for the application to use.
    XrResult OpenXrRuntime::getSwapchainImagesVulkan(Swapchain& xrSwapchain,
                                                     XrSwapchainImageVulkanKHR* vkImages,
                                                     uint32_t count) {
        // Detect whether this is the first call for this swapchain.
        const bool initialized = !xrSwapchain.slices[0].empty();

        const bool needTransition = xrSwapchain.xrDesc.usageFlags & (XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                                                                     XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

        std::vector<HANDLE> textureHandles;
        if (!initialized) {
            // Query the swapchain textures.
            textureHandles = getSwapchainImages(xrSwapchain);

            if (needTransition) {
                // We keep our code simple by only using a single command buffer, which means we must wait before
                // reusing it.
                flushVulkanCommandQueue();

                // Prepare to execute layout transitions.
                VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                CHECK_VKCMD(m_vkDispatch.vkBeginCommandBuffer(m_vkCmdBuffer, &beginInfo));
            }
        }

        // Helper to select the memory type.
        auto findMemoryType = [&](uint32_t memoryTypeBitsRequirement, VkFlags requirementsMask) {
            for (uint32_t memoryIndex = 0; memoryIndex < VK_MAX_MEMORY_TYPES; ++memoryIndex) {
                const uint32_t memoryTypeBits = (1 << memoryIndex);
                const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;
                const bool satisfiesFlags = (m_vkMemoryProperties.memoryTypes[memoryIndex].propertyFlags &
                                             requirementsMask) == requirementsMask;

                if (isRequiredMemoryType && satisfiesFlags) {
                    return memoryIndex;
                }
            }

            CHECK_VKCMD(VK_ERROR_UNKNOWN);
            return 0u;
        };

        // Export each D3D11 texture to Vulkan.
        for (uint32_t i = 0; i < count; i++) {
            if (vkImages[i].type != XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (!initialized) {
                // Create an imported texture on the Vulkan device.
                VkImage image;
                {
                    VkExternalMemoryImageCreateInfo externalCreateInfo{
                        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
                    externalCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT;

                    VkImageCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, &externalCreateInfo};
                    createInfo.imageType = VK_IMAGE_TYPE_2D;
                    createInfo.format = (VkFormat)xrSwapchain.xrDesc.format;
                    createInfo.extent.width = xrSwapchain.xrDesc.width;
                    createInfo.extent.height = xrSwapchain.xrDesc.height;
                    createInfo.extent.depth = 1;
                    createInfo.mipLevels = xrSwapchain.xrDesc.mipCount;
                    createInfo.arrayLayers = xrSwapchain.xrDesc.arraySize;
                    createInfo.samples = (VkSampleCountFlagBits)xrSwapchain.xrDesc.sampleCount;
                    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) {
                        createInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                    }
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                        createInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                    }
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_SAMPLED_BIT) {
                        createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
                    }
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) {
                        createInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                    }
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT) {
                        createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                    }
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT) {
                        createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    }
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT) {
                        createInfo.usage |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
                    }
                    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    CHECK_VKCMD(m_vkDispatch.vkCreateImage(m_vkDevice, &createInfo, m_vkAllocator, &image));
                }
                xrSwapchain.vkImages.push_back(image);

                // Import the device memory from D3D.
                VkDeviceMemory memory;
                {
                    VkImageMemoryRequirementsInfo2 requirementInfo{VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
                    requirementInfo.image = image;
                    VkMemoryRequirements2 requirements{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
                    m_vkDispatch.vkGetImageMemoryRequirements2KHR(m_vkDevice, &requirementInfo, &requirements);

                    VkMemoryWin32HandlePropertiesKHR handleProperties{
                        VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR};
                    CHECK_VKCMD(m_vkDispatch.vkGetMemoryWin32HandlePropertiesKHR(
                        m_vkDevice,
                        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT,
                        textureHandles[i],
                        &handleProperties));

                    VkImportMemoryWin32HandleInfoKHR importInfo{VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR};
                    importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT;
                    importInfo.handle = textureHandles[i];

                    VkMemoryDedicatedAllocateInfo memoryAllocateInfo{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
                                                                     &importInfo};
                    memoryAllocateInfo.image = image;

                    VkMemoryAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &memoryAllocateInfo};
                    allocateInfo.allocationSize = requirements.memoryRequirements.size;
                    allocateInfo.memoryTypeIndex =
                        findMemoryType(handleProperties.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),

                    CHECK_VKCMD(m_vkDispatch.vkAllocateMemory(m_vkDevice, &allocateInfo, m_vkAllocator, &memory));
                }
                xrSwapchain.vkDeviceMemory.push_back(memory);

                VkBindImageMemoryInfo bindImageInfo{VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
                bindImageInfo.image = image;
                bindImageInfo.memory = memory;
                CHECK_VKCMD(m_vkDispatch.vkBindImageMemory2KHR(m_vkDevice, 1, &bindImageInfo));

                if (needTransition) {
                    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) {
                        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    }
                    if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    }
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = image;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

                    m_vkDispatch.vkCmdPipelineBarrier(m_vkCmdBuffer,
                                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                                      0,
                                                      0,
                                                      (VkMemoryBarrier*)nullptr,
                                                      0,
                                                      (VkBufferMemoryBarrier*)nullptr,
                                                      1,
                                                      &barrier);
                }
            }

            vkImages[i].image = xrSwapchain.vkImages[i];

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateSwapchainImages",
                              TLArg("Vulkan", "Api"),
                              TLXArg(vkImages[i].image, "Texture"));
        }

        if (!initialized && needTransition) {
            // Transition all images to the desired state.
            CHECK_VKCMD(m_vkDispatch.vkEndCommandBuffer(m_vkCmdBuffer));
            VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_vkCmdBuffer;
            CHECK_VKCMD(m_vkDispatch.vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE));
        }

        return XR_SUCCESS;
    }

    // Wait for all pending commands to finish.
    void OpenXrRuntime::flushVulkanCommandQueue() {
        if (m_vkDispatch.vkQueueSubmit && m_vkDispatch.vkWaitSemaphoresKHR) {
            m_fenceValue++;
            TraceLoggingWrite(
                g_traceProvider, "FlushContext_Wait", TLArg("Vulkan", "Api"), TLArg(m_fenceValue, "FenceValue"));
            VkTimelineSemaphoreSubmitInfo timelineInfo{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
            timelineInfo.signalSemaphoreValueCount = 1;
            timelineInfo.pSignalSemaphoreValues = &m_fenceValue;
            VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO, &timelineInfo};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &m_vkTimelineSemaphoreForFlush;
            CHECK_VKCMD(m_vkDispatch.vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE));
            VkSemaphoreWaitInfo waitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
            waitInfo.semaphoreCount = 1;
            waitInfo.pSemaphores = &m_vkTimelineSemaphoreForFlush;
            waitInfo.pValues = &m_fenceValue;
            CHECK_VKCMD(m_vkDispatch.vkWaitSemaphoresKHR(m_vkDevice, &waitInfo, UINT64_MAX));
        }
    }

    // Serialize commands from the Vulkan queue to the D3D11 context used by PVR.
    void OpenXrRuntime::serializeVulkanFrame() {
        m_fenceValue++;
        TraceLoggingWrite(
            g_traceProvider, "xrEndFrame_Sync", TLArg("Vulkan", "Api"), TLArg(m_fenceValue, "FenceValue"));
        VkTimelineSemaphoreSubmitInfo timelineInfo{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues = &m_fenceValue;
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO, &timelineInfo};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_vkTimelineSemaphore;
        CHECK_VKCMD(m_vkDispatch.vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE));

        waitOnSubmissionDevice();
    }

} // namespace pimax_openxr
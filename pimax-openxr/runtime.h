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

#pragma once

#include "framework/dispatch.gen.h"

#include "appinsights.h"
#include "utils.h"

namespace pimax_openxr {

    using namespace pimax_openxr::appinsights;
    using namespace pimax_openxr::utils;

    const unsigned int RuntimeVersionMajor = 0;
    const unsigned int RuntimeVersionMinor = 2;
    const unsigned int RuntimeVersionPatch = 2;

    const std::string RuntimeName = "pimax-openxr";
    const std::string RuntimePrettyName =
        fmt::format("PimaxXR - v{}.{}.{}", RuntimeVersionMajor, RuntimeVersionMinor, RuntimeVersionPatch);
    const std::string RegPrefix = "SOFTWARE\\PimaxXR";

    // This class implements all APIs that the runtime supports.
    class OpenXrRuntime : public OpenXrApi {
      public:
        OpenXrRuntime();
        ~OpenXrRuntime();

        XrResult xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function);

        XrResult xrEnumerateInstanceExtensionProperties(const char* layerName,
                                                        uint32_t propertyCapacityInput,
                                                        uint32_t* propertyCountOutput,
                                                        XrExtensionProperties* properties) override;
        XrResult xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance) override;
        XrResult xrDestroyInstance(XrInstance instance) override;
        XrResult xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties) override;
        XrResult xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) override;
        XrResult xrResultToString(XrInstance instance, XrResult value, char buffer[XR_MAX_RESULT_STRING_SIZE]) override;
        XrResult xrStructureTypeToString(XrInstance instance,
                                         XrStructureType value,
                                         char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) override;
        XrResult xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) override;
        XrResult xrGetSystemProperties(XrInstance instance,
                                       XrSystemId systemId,
                                       XrSystemProperties* properties) override;
        XrResult xrEnumerateEnvironmentBlendModes(XrInstance instance,
                                                  XrSystemId systemId,
                                                  XrViewConfigurationType viewConfigurationType,
                                                  uint32_t environmentBlendModeCapacityInput,
                                                  uint32_t* environmentBlendModeCountOutput,
                                                  XrEnvironmentBlendMode* environmentBlendModes) override;
        XrResult xrCreateSession(XrInstance instance,
                                 const XrSessionCreateInfo* createInfo,
                                 XrSession* session) override;
        XrResult xrDestroySession(XrSession session) override;
        XrResult xrEnumerateReferenceSpaces(XrSession session,
                                            uint32_t spaceCapacityInput,
                                            uint32_t* spaceCountOutput,
                                            XrReferenceSpaceType* spaces) override;
        XrResult xrCreateReferenceSpace(XrSession session,
                                        const XrReferenceSpaceCreateInfo* createInfo,
                                        XrSpace* space) override;
        XrResult xrGetReferenceSpaceBoundsRect(XrSession session,
                                               XrReferenceSpaceType referenceSpaceType,
                                               XrExtent2Df* bounds) override;
        XrResult xrCreateActionSpace(XrSession session,
                                     const XrActionSpaceCreateInfo* createInfo,
                                     XrSpace* space) override;
        XrResult xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location) override;
        XrResult xrDestroySpace(XrSpace space) override;
        XrResult xrEnumerateViewConfigurations(XrInstance instance,
                                               XrSystemId systemId,
                                               uint32_t viewConfigurationTypeCapacityInput,
                                               uint32_t* viewConfigurationTypeCountOutput,
                                               XrViewConfigurationType* viewConfigurationTypes) override;
        XrResult xrGetViewConfigurationProperties(XrInstance instance,
                                                  XrSystemId systemId,
                                                  XrViewConfigurationType viewConfigurationType,
                                                  XrViewConfigurationProperties* configurationProperties) override;
        XrResult xrEnumerateViewConfigurationViews(XrInstance instance,
                                                   XrSystemId systemId,
                                                   XrViewConfigurationType viewConfigurationType,
                                                   uint32_t viewCapacityInput,
                                                   uint32_t* viewCountOutput,
                                                   XrViewConfigurationView* views) override;
        XrResult xrEnumerateSwapchainFormats(XrSession session,
                                             uint32_t formatCapacityInput,
                                             uint32_t* formatCountOutput,
                                             int64_t* formats) override;
        XrResult xrCreateSwapchain(XrSession session,
                                   const XrSwapchainCreateInfo* createInfo,
                                   XrSwapchain* swapchain) override;
        XrResult xrDestroySwapchain(XrSwapchain swapchain) override;
        XrResult xrEnumerateSwapchainImages(XrSwapchain swapchain,
                                            uint32_t imageCapacityInput,
                                            uint32_t* imageCountOutput,
                                            XrSwapchainImageBaseHeader* images) override;
        XrResult xrAcquireSwapchainImage(XrSwapchain swapchain,
                                         const XrSwapchainImageAcquireInfo* acquireInfo,
                                         uint32_t* index) override;
        XrResult xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo) override;
        XrResult xrReleaseSwapchainImage(XrSwapchain swapchain,
                                         const XrSwapchainImageReleaseInfo* releaseInfo) override;
        XrResult xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) override;
        XrResult xrEndSession(XrSession session) override;
        XrResult xrRequestExitSession(XrSession session) override;
        XrResult xrWaitFrame(XrSession session,
                             const XrFrameWaitInfo* frameWaitInfo,
                             XrFrameState* frameState) override;
        XrResult xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) override;
        XrResult xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) override;
        XrResult xrLocateViews(XrSession session,
                               const XrViewLocateInfo* viewLocateInfo,
                               XrViewState* viewState,
                               uint32_t viewCapacityInput,
                               uint32_t* viewCountOutput,
                               XrView* views) override;
        XrResult xrStringToPath(XrInstance instance, const char* pathString, XrPath* path) override;
        XrResult xrPathToString(XrInstance instance,
                                XrPath path,
                                uint32_t bufferCapacityInput,
                                uint32_t* bufferCountOutput,
                                char* buffer) override;
        XrResult xrCreateActionSet(XrInstance instance,
                                   const XrActionSetCreateInfo* createInfo,
                                   XrActionSet* actionSet) override;
        XrResult xrDestroyActionSet(XrActionSet actionSet) override;
        XrResult xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action) override;
        XrResult xrDestroyAction(XrAction action) override;
        XrResult xrSuggestInteractionProfileBindings(
            XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) override;
        XrResult xrAttachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo* attachInfo) override;
        XrResult xrGetCurrentInteractionProfile(XrSession session,
                                                XrPath topLevelUserPath,
                                                XrInteractionProfileState* interactionProfile) override;
        XrResult xrGetActionStateBoolean(XrSession session,
                                         const XrActionStateGetInfo* getInfo,
                                         XrActionStateBoolean* state) override;
        XrResult xrGetActionStateFloat(XrSession session,
                                       const XrActionStateGetInfo* getInfo,
                                       XrActionStateFloat* state) override;
        XrResult xrGetActionStateVector2f(XrSession session,
                                          const XrActionStateGetInfo* getInfo,
                                          XrActionStateVector2f* state) override;
        XrResult xrGetActionStatePose(XrSession session,
                                      const XrActionStateGetInfo* getInfo,
                                      XrActionStatePose* state) override;
        XrResult xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) override;
        XrResult xrEnumerateBoundSourcesForAction(XrSession session,
                                                  const XrBoundSourcesForActionEnumerateInfo* enumerateInfo,
                                                  uint32_t sourceCapacityInput,
                                                  uint32_t* sourceCountOutput,
                                                  XrPath* sources) override;
        XrResult xrGetInputSourceLocalizedName(XrSession session,
                                               const XrInputSourceLocalizedNameGetInfo* getInfo,
                                               uint32_t bufferCapacityInput,
                                               uint32_t* bufferCountOutput,
                                               char* buffer) override;
        XrResult xrApplyHapticFeedback(XrSession session,
                                       const XrHapticActionInfo* hapticActionInfo,
                                       const XrHapticBaseHeader* hapticFeedback) override;
        XrResult xrStopHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo) override;
        XrResult xrGetVulkanInstanceExtensionsKHR(XrInstance instance,
                                                  XrSystemId systemId,
                                                  uint32_t bufferCapacityInput,
                                                  uint32_t* bufferCountOutput,
                                                  char* buffer) override;
        XrResult xrGetVulkanDeviceExtensionsKHR(XrInstance instance,
                                                XrSystemId systemId,
                                                uint32_t bufferCapacityInput,
                                                uint32_t* bufferCountOutput,
                                                char* buffer) override;
        XrResult xrGetVulkanGraphicsDeviceKHR(XrInstance instance,
                                              XrSystemId systemId,
                                              VkInstance vkInstance,
                                              VkPhysicalDevice* vkPhysicalDevice) override;
        XrResult xrGetVulkanGraphicsRequirementsKHR(XrInstance instance,
                                                    XrSystemId systemId,
                                                    XrGraphicsRequirementsVulkanKHR* graphicsRequirements) override;
        XrResult xrGetD3D11GraphicsRequirementsKHR(XrInstance instance,
                                                   XrSystemId systemId,
                                                   XrGraphicsRequirementsD3D11KHR* graphicsRequirements) override;
        XrResult xrGetD3D12GraphicsRequirementsKHR(XrInstance instance,
                                                   XrSystemId systemId,
                                                   XrGraphicsRequirementsD3D12KHR* graphicsRequirements) override;
        XrResult xrGetVisibilityMaskKHR(XrSession session,
                                        XrViewConfigurationType viewConfigurationType,
                                        uint32_t viewIndex,
                                        XrVisibilityMaskTypeKHR visibilityMaskType,
                                        XrVisibilityMaskKHR* visibilityMask) override;
        XrResult xrConvertWin32PerformanceCounterToTimeKHR(XrInstance instance,
                                                           const LARGE_INTEGER* performanceCounter,
                                                           XrTime* time) override;
        XrResult xrConvertTimeToWin32PerformanceCounterKHR(XrInstance instance,
                                                           XrTime time,
                                                           LARGE_INTEGER* performanceCounter) override;
        XrResult xrCreateVulkanInstanceKHR(XrInstance instance,
                                           const XrVulkanInstanceCreateInfoKHR* createInfo,
                                           VkInstance* vulkanInstance,
                                           VkResult* vulkanResult) override;
        XrResult xrCreateVulkanDeviceKHR(XrInstance instance,
                                         const XrVulkanDeviceCreateInfoKHR* createInfo,
                                         VkDevice* vulkanDevice,
                                         VkResult* vulkanResult) override;
        XrResult xrGetVulkanGraphicsDevice2KHR(XrInstance instance,
                                               const XrVulkanGraphicsDeviceGetInfoKHR* getInfo,
                                               VkPhysicalDevice* vulkanPhysicalDevice) override;
        XrResult xrGetVulkanGraphicsRequirements2KHR(XrInstance instance,
                                                     XrSystemId systemId,
                                                     XrGraphicsRequirementsVulkanKHR* graphicsRequirements) override;

      private:
        struct Swapchain {
            // The PVR swapchain objects. For texture arrays, we must have one swapchain per slice due to PVR
            // limitation.
            std::vector<pvrTextureSwapChain> pvrSwapchain;

            // The cached textures used for copy between swapchains.
            std::vector<std::vector<ID3D11Texture2D*>> slices;

            // The last acquired/released swapchain image index.
            int currentAcquiredIndex{0};
            int pvrLastReleasedIndex{0};

            // Certain depth formats require use to go through an intermediate texture and resolve (copy, convert) the
            // texture later. We manage our own set of textures and image index.
            bool needDepthResolve{false};
            std::vector<ComPtr<ID3D11Texture2D>> images;
            uint32_t nextIndex{0};

            // Resources needed to run the resolve shader.
            std::vector<std::vector<ComPtr<ID3D11ShaderResourceView>>> imagesResourceView;
            ComPtr<ID3D11Texture2D> resolved;
            ComPtr<ID3D11UnorderedAccessView> resolvedAccessView;

            // Resources needed for interop.
            std::vector<ComPtr<ID3D12Resource>> d3d12Images;
            ComPtr<ID3D12GraphicsCommandList> d3d12CommandList;
            std::vector<VkDeviceMemory> vkDeviceMemory;
            std::vector<VkImage> vkImages;
            VkCommandBuffer vkCmdBuffer{VK_NULL_HANDLE};

            // Information recorded at creation.
            XrSwapchainCreateInfo xrDesc;
            pvrTextureSwapChainDesc pvrDesc;
        };

        struct Space {
            // Information recorded at creation.
            XrReferenceSpaceType referenceType;
            XrAction action{XR_NULL_HANDLE};
            XrPath subActionPath{XR_NULL_PATH};
            XrPosef poseInSpace;
        };

        struct Action {
            XrActionType type;

            std::string path;
            XrActionSet actionSet{XR_NULL_HANDLE};

            const float* floatValue{nullptr};
            float lastFloatValue{0.f};
            XrTime lastFloatValueChangedTime{0};

            const pvrVector2f* vector2fValue{nullptr};
            int vector2fIndex{-1};
            XrVector2f lastVector2fValue{0.f, 0.f};
            XrTime lastVector2fValueChangedTime{0};

            const uint32_t* buttonMap{nullptr};
            pvrButton buttonType;
            bool lastBoolValue{false};
            XrTime lastBoolValueChangedTime{0};
        };

        // instance.cpp
        std::optional<int> getSetting(const std::string& value) const;

        // system.cpp
        void fillDisplayDeviceInfo();

        // action.cpp
        void rebindControllerActions(int side);
        std::string getXrPath(XrPath path) const;
        std::string getActionPath(const Action& action, XrPath subActionPath) const;
        int getActionSide(const std::string& fullPath) const;

        // mappings.cpp
        void initializeRemappingTables();
        void mapPathToViveControllerInputState(Action& xrAction, const std::string& path) const;
        void mapPathToIndexControllerInputState(Action& xrAction, const std::string& path) const;
        void mapPathToSimpleControllerInputState(Action& xrAction, const std::string& path) const;
        std::string getViveControllerLocalizedSourceName(const std::string& path) const;
        std::string getIndexControllerLocalizedSourceName(const std::string& path) const;
        std::string getSimpleControllerLocalizedSourceName(const std::string& path) const;
        std::optional<std::string> remapSimpleControllerToViveController(const std::string& path) const;
        std::optional<std::string> remapOculusTouchControllerToViveController(const std::string& path) const;
        std::optional<std::string> remapMicrosoftMotionControllerToViveController(const std::string& path) const;
        std::optional<std::string> remapSimpleControllerToIndexController(const std::string& path) const;
        std::optional<std::string> remapOculusTouchControllerToIndexController(const std::string& path) const;
        std::optional<std::string> remapMicrosoftMotionControllerToIndexController(const std::string& path) const;
        std::optional<std::string> remapOculusTouchControllerToSimpleController(const std::string& path) const;
        std::optional<std::string> remapMicrosoftMotionControllerToSimpleController(const std::string& path) const;

        // space.cpp
        XrSpaceLocationFlags getHmdPose(XrTime time, bool addFloorHeight, XrPosef& pose) const;
        XrSpaceLocationFlags getControllerPose(int side, XrTime time, XrPosef& pose) const;

        // d3d11_native.cpp
        XrResult initializeD3D11(const XrGraphicsBindingD3D11KHR& d3dBindings, bool interop = false);
        void cleanupD3D11();
        XrResult getSwapchainImagesD3D11(Swapchain& xrSwapchain,
                                         XrSwapchainImageD3D11KHR* d3d11Images,
                                         uint32_t count,
                                         bool interop = false);
        void prepareAndCommitSwapchainImage(Swapchain& xrSwapchain,
                                            uint32_t slice,
                                            std::set<std::pair<pvrTextureSwapChain, uint32_t>>& committed) const;

        // d3d12_interop.cpp
        XrResult initializeD3D12(const XrGraphicsBindingD3D12KHR& d3dBindings);
        void cleanupD3D12();
        bool isD3D12Session() const;
        XrResult getSwapchainImagesD3D12(Swapchain& xrSwapchain, XrSwapchainImageD3D12KHR* d3d12Images, uint32_t count);
        void transitionImageD3D12(Swapchain& xrSwapchain, uint32_t index, bool acquire);
        void serializeD3D12Frame();

        // vulkan_interop.cpp
        XrResult initializeVulkan(const XrGraphicsBindingVulkanKHR& vkBindings);
        void initializeVulkanDispatch(VkInstance instance);
        void cleanupVulkan();
        bool isVulkanSession() const;
        XrResult getSwapchainImagesVulkan(Swapchain& xrSwapchain, XrSwapchainImageVulkanKHR* vkImages, uint32_t count);
        void transitionImageVulkan(Swapchain& xrSwapchain, uint32_t index, bool acquire);
        void serializeVulkanFrame();

        // visibility_mask.cpp
        void convertSteamVRToOpenXRHiddenMesh(const pvrFovPort& fov,
                                              XrVector2f* vertices,
                                              uint32_t* indices,
                                              uint32_t count) const;

        // Instance & PVR state.
        pvrEnvHandle m_pvr;
        pvrSessionHandle m_pvrSession{nullptr};
        bool m_instanceCreated{false};
        bool m_systemCreated{false};
        bool m_isVisibilityMaskSupported{false};
        bool m_isD3D11Supported{false};
        bool m_isD3D12Supported{false};
        bool m_isVulkanSupported{false};
        bool m_isVulkan2Supported{false};
        bool m_isDepthSupported{false};
        bool m_graphicsRequirementQueried{false};
        LUID m_adapterLuid{};
        double m_frameDuration{0};
        pvrEyeRenderInfo m_cachedEyeInfo[xr::StereoView::Count];
        float m_floorHeight{0.f};
        LARGE_INTEGER m_qpcFrequency;
        double m_pvrTimeFromQpcTimeOffset{0};
        XrPath m_stringIndex{0};
        std::map<XrPath, std::string> m_strings;
        uint64_t m_actionSetIndex{0};
        std::set<XrActionSet> m_actionSets;
        std::set<XrAction> m_actions;
        std::map<std::pair<std::string, std::string>, std::function<void(Action&, XrPath)>> m_controllerMappingTable;

        // Session state.
        ComPtr<ID3D11Device5> m_d3d11Device;
        ComPtr<ID3D11DeviceContext4> m_d3d11DeviceContext;
        ComPtr<ID3D11ComputeShader> m_resolveShader[2];
        ComPtr<IDXGISwapChain1> m_dxgiSwapchain;
        bool m_sessionCreated{false};
        XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};
        bool m_sessionStateDirty{false};
        double m_sessionStateEventTime{0.0};
        std::set<XrSwapchain> m_swapchains;
        std::set<XrSpace> m_spaces;
        XrSpace m_originSpace{XR_NULL_HANDLE};
        XrSpace m_viewSpace{XR_NULL_HANDLE};
        bool m_isVisibilityMaskEnabled{false};
        bool m_useParallelProjection{false};
        bool m_canBeginFrame{false};
        std::set<XrActionSet> m_activeActionSets;
        std::map<std::string, std::vector<XrActionSuggestedBinding>> m_suggestedBindings;
        std::string m_cachedControllerType[2];
        XrPosef m_controllerAimPose[2];
        std::string m_localizedControllerType[2];
        XrPath m_currentInteractionProfile[2]{XR_NULL_PATH, XR_NULL_PATH};
        bool m_currentInteractionProfileDirty{false};

        // Graphics API interop.
        ComPtr<ID3D12Device> m_d3d12Device;
        ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
        ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;
        VkInstance m_vkBootstrapInstance{VK_NULL_HANDLE};
        VkPhysicalDevice m_vkBootstrapPhysicalDevice{VK_NULL_HANDLE};
        VkInstance m_vkInstance{VK_NULL_HANDLE};
        VkDevice m_vkDevice{VK_NULL_HANDLE};
        VkCommandPool m_vkCmdPool{VK_NULL_HANDLE};
        struct {
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{nullptr};

            // Pointers below must be initialized in initializeVulkanDispatch(),
            PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2{nullptr};
            PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties{nullptr};
            PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR{nullptr};
            PFN_vkGetDeviceQueue vkGetDeviceQueue{nullptr};
            PFN_vkQueueSubmit vkQueueSubmit{nullptr};
            PFN_vkCreateImage vkCreateImage{nullptr};
            PFN_vkDestroyImage vkDestroyImage{nullptr};
            PFN_vkAllocateMemory vkAllocateMemory{nullptr};
            PFN_vkFreeMemory vkFreeMemory{nullptr};
            PFN_vkCreateCommandPool vkCreateCommandPool{nullptr};
            PFN_vkDestroyCommandPool vkDestroyCommandPool{nullptr};
            PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers{nullptr};
            PFN_vkFreeCommandBuffers vkFreeCommandBuffers{nullptr};
            PFN_vkBeginCommandBuffer vkBeginCommandBuffer{nullptr};
            PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier{nullptr};
            PFN_vkEndCommandBuffer vkEndCommandBuffer{nullptr};
            PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR{nullptr};
            PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR{nullptr};
            PFN_vkCreateSemaphore vkCreateSemaphore{nullptr};
            PFN_vkDestroySemaphore vkDestroySemaphore{nullptr};
            PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR{nullptr};
            PFN_vkDeviceWaitIdle vkDeviceWaitIdle{nullptr};
        } m_vkDispatch;
        const VkAllocationCallbacks* m_vkAllocator{nullptr};
        VkPhysicalDevice m_vkPhysicalDevice{VK_NULL_HANDLE};
        VkPhysicalDeviceMemoryProperties m_vkMemoryProperties;
        VkQueue m_vkQueue{VK_NULL_HANDLE};

        ComPtr<ID3D11Fence> m_d3d11Fence;
        ComPtr<ID3D12Fence> m_d3d12Fence;
        VkSemaphore m_vkTimelineSemaphore{VK_NULL_HANDLE};
        UINT64 m_fenceValue{0};

        // Frame state.
        std::mutex m_frameLock;
        std::condition_variable m_frameCondVar;
        bool m_frameWaited{false};
        bool m_frameBegun{false};
        std::optional<double> m_lastFrameWaitedTime;
        pvrInputState m_cachedInputState;
        bool m_isControllerActive[2]{false, false};
        std::set<XrActionSet> m_frameLatchedActionSets;

        // Statistics.
        AppInsights m_telemetry;
        double m_sessionStartTime{0.0};
        uint64_t m_sessionTotalFrameCount{0};
        std::deque<double> m_frameTimes;
        CpuTimer m_cpuTimerApp;
        std::unique_ptr<GpuTimer> m_gpuTimerApp[2];
        std::unique_ptr<GpuTimer> m_gpuTimerSynchronizationDuration[2];
        std::unique_ptr<GpuTimer> m_gpuTimerPrecomposition[2];
        std::unique_ptr<GpuTimer> m_gpuTimerPvrComposition[2];
        uint32_t m_currentTimerIndex{0};

        friend AppInsights* GetTelemetry();
    };

    // Singleton accessor.
    OpenXrApi* GetInstance();

    // A function to reset (delete) the singleton.
    void ResetInstance();

    // Get telemetry object if available.
    AppInsights* GetTelemetry();

    extern std::filesystem::path localAppData;

} // namespace pimax_openxr

// MIT License
//
// Copyright(c) 2024 Matthieu Bucchianeri
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

#include "pch.h"

#include "log.h"
#include "utils.h"
#include "vrs.h"

#include "D3D12Utils.h"
#include "DetoursUtils.h"
#include "VRSUtils.h"

#include "GenerateShadingRateMapNoArrayCS.h"

#pragma comment(lib, "d3d12.lib")

using namespace virtualdesktop_openxr::log;

namespace {

    using namespace D3D12Utils;
    using namespace vrs;

#define Align(value, pad_to) (((value) + (pad_to)-1) & ~((pad_to)-1))

    struct FeatureNotSupported : public std::exception {
        const char* what() const throw() {
            return "Feature is not supported";
        }
    };

    // If the application uses the Streamline SDK, some D3D12 objects are shimmed, and this will confuse our Detours
    // logic. Luckily, the Streamline SDK has a secret UUID that can be used to query the underlying interface. From
    // https://github.com/NVIDIAGameWorks/Streamline/blob/main/source/core/sl.api/internal.h.
    struct DECLSPEC_UUID("ADEC44E2-61F0-45C3-AD9F-1B37379284FF") StreamlineRetrieveBaseInterface : IUnknown {};

    template <class T>
    inline void GetRealD3D12Object(T* shimmedObject, T** realObject) {
        if (FAILED(shimmedObject->QueryInterface(__uuidof(StreamlineRetrieveBaseInterface), (void**)realObject))) {
            // TODO: Is the AddRef() correct here?
            shimmedObject->AddRef();
            *realObject = shimmedObject;
        }
    }

    class VRSManagerD3D12 {
      private:
        struct ShadingRateMap {
            uint64_t Generation{0};
            uint64_t SettingsGeneration{0};
            unsigned int Age{0};
            ComPtr<ID3D12Resource> ShadingRateTexture;
            D3D12_CPU_DESCRIPTOR_HANDLE UAV;
            D3D12_GPU_DESCRIPTOR_HANDLE UAVDescriptor;
            uint64_t CompletedFenceValue{0};
        };

        struct CommandListDependency {
            uint64_t FenceValue;
            unsigned int Age{0};
        };

      public:
        VRSManagerD3D12(ID3D12Device* Device, const Resolution& PresentResolution)
            : m_Device(Device), m_PresentResolution(PresentResolution) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D12_Create", TLPArg(Device, "Device"));

            // Check for support on this device.
            D3D12_FEATURE_DATA_D3D12_OPTIONS6 options{};
            if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options, sizeof(options))) ||
                options.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_2 ||
                options.ShadingRateImageTileSize < 2u) {
                TraceLoggingWriteTagged(local,
                                        "VRSManagerD3D12_Create_NotSupported",
                                        TLArg((UINT)options.VariableShadingRateTier, "VariableShadingRateTier"),
                                        TLArg(options.ShadingRateImageTileSize, "ShadingRateImageTileSize"));
                throw FeatureNotSupported();
            }
            m_VRSTileSize = options.ShadingRateImageTileSize;

            // Create a command context where we will perform the generation of the shading rate textures.
            m_Context = std::make_unique<CommandContext>(m_Device.Get(), L"Shading Rate Map Creation");

            // Create resources for the GenerateShadingRateMap compute shader.
            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
            D3D12_DESCRIPTOR_RANGE uavRange;
            CD3DX12_DESCRIPTOR_RANGE::Init(uavRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
            D3D12_ROOT_PARAMETER rootParameters[2];
            CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParameters[0], 1, &uavRange);
            CD3DX12_ROOT_PARAMETER::InitAsConstants(rootParameters[1], sizeof(GenerateShadingRateMapConstants) / 4, 0);
            rootSignatureDesc.pParameters = rootParameters;
            rootSignatureDesc.NumParameters = ARRAYSIZE(rootParameters);

            ComPtr<ID3DBlob> rootSignatureBlob;
            ComPtr<ID3DBlob> error;
            CHECK_MSG(SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc,
                                                            D3D_ROOT_SIGNATURE_VERSION_1,
                                                            rootSignatureBlob.GetAddressOf(),
                                                            error.GetAddressOf())),
                      (char*)error->GetBufferPointer());

            CHECK_HRCMD(m_Device->CreateRootSignature(0,
                                                      rootSignatureBlob->GetBufferPointer(),
                                                      rootSignatureBlob->GetBufferSize(),
                                                      IID_PPV_ARGS(m_GenerateRootSignature.ReleaseAndGetAddressOf())));
            m_GenerateRootSignature->SetName(L"GenerateShadingRateMapCS Root Signature");

            D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc{};
            computeDesc.CS.pShaderBytecode = g_GenerateShadingRateMapNoArrayCS;
            computeDesc.CS.BytecodeLength = sizeof(g_GenerateShadingRateMapNoArrayCS);
            computeDesc.pRootSignature = m_GenerateRootSignature.Get();
            CHECK_HRCMD(m_Device->CreateComputePipelineState(&computeDesc,
                                                             IID_PPV_ARGS(m_GeneratePSO.ReleaseAndGetAddressOf())));
            m_GeneratePSO->SetName(L"GenerateShadingRateMapCS PSO");

            // Create a descriptor heap for the UAVs for our shading rate textures.
            m_HeapForUAVs = std::make_unique<DescriptorHeap>(
                m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128u, L"Shading Rate Map UAV");

            m_Enabled.store(true);

            TraceLoggingWriteStop(local, "VRSManagerD3D12_Create");
        }

        ~VRSManagerD3D12() {
            Flush();
        }

        void OnSetViewports(ID3D12CommandList* CommandList,
                            const D3D12_VIEWPORT& Viewport0,
                            const D3D12_VIEWPORT& Viewport1) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D12_OnSetViewports", TLPArg(CommandList, "CommandList"));

            ComPtr<ID3D12Device> device;
            CHECK_HRCMD(CommandList->GetDevice(IID_PPV_ARGS(device.ReleaseAndGetAddressOf())));

            if (device == m_Device && m_Enabled.load()) {
                if (IsViewportEligible(m_PresentResolution, {(UINT)Viewport0.Width, (UINT)Viewport0.Height})) {
                    Enable(CommandList,
                           Viewport0,
                           // Detect double-wide.
                           IsViewportEligible(m_PresentResolution, {(UINT)Viewport1.Width, (UINT)Viewport1.Height})
                               ? Viewport1
                               : D3D12_VIEWPORT{});
                } else {
                    Disable(CommandList);
                }
            }

            TraceLoggingWriteStop(local, "VRSManagerD3D12_OnSetViewports");
        }

        void OnExecuteCommandLists(ID3D12CommandQueue* CommandQueue,
                                   const std::vector<ID3D12CommandList*>& CommandLists) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(
                local, "VRSManagerD3D12_OnExecuteCommandLists", TLPArg(CommandQueue, "CommandQueue"));

            ComPtr<ID3D12Device> device;
            CHECK_HRCMD(CommandQueue->GetDevice(IID_PPV_ARGS(device.ReleaseAndGetAddressOf())));

            if (device == m_Device) {
                SyncQueue(CommandQueue, CommandLists);
            }

            TraceLoggingWriteStop(local, "VRSManagerD3D12_OnExecuteCommandLists");
        }

        void OnUpdate() {
            Tick();
        }

        void Inhibit() {
            m_Enabled.store(false);
        }

        void Deinhibit() {
            m_Enabled.store(true);
        }

        void SetParameters(const Parameters& parameters) {
            std::unique_lock lock(m_ParametersMutex);

            m_InnerRing = parameters.InnerRing;
            m_OuterRing = parameters.OuterRing;

            const auto toRate = [](Rate rate) {
                return (D3D12_SHADING_RATE)D3D12_MAKE_COARSE_SHADING_RATE(rate.X, rate.Y);
            };
            m_InnerRate = toRate(parameters.InnerRate);
            m_MiddleRate = toRate(parameters.MiddleRate);
            m_OuterRate = toRate(parameters.OuterRate);

            m_CurrentSettingsGeneration++;
        }

        void Flush() {
            m_Context->Flush();
        }

      private:
        void Enable(ID3D12CommandList* CommandList, const D3D12_VIEWPORT& Viewport0, const D3D12_VIEWPORT& Viewport1) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D12_Enable", TLPArg(CommandList, "CommandList"));

            // The shading rate map is per render target, if a render is used in double-wide, we need to create a
            // shading rate map covering both viewports.
            const float TotalWidth =
                std::max(Viewport0.TopLeftX + Viewport0.Width, Viewport1.TopLeftX + Viewport1.Width);
            const float TotalHeight =
                std::max(Viewport0.TopLeftY + Viewport0.Height, Viewport1.TopLeftY + Viewport1.Height);

            const TiledResolution shadingRateMapResolution{
                Align(static_cast<UINT>(TotalWidth + FLT_EPSILON), m_VRSTileSize) / m_VRSTileSize,
                Align(static_cast<UINT>(TotalHeight + FLT_EPSILON), m_VRSTileSize) / m_VRSTileSize};
            TraceLoggingWriteTagged(local,
                                    "VRSManagerD3D12_Enable",
                                    TLArg(shadingRateMapResolution.Width, "TiledWidth"),
                                    TLArg(shadingRateMapResolution.Height, "TiledHeight"));

            float gazeX = 0.5f, gazeY = 0.5f, scaleFactor = 1.f;
            const bool wasUsingEyeGaze = m_UsingEyeGaze;
            m_UsingEyeGaze = GetGaze(gazeX, gazeY, scaleFactor);
            // When eye gaze becomes unavailable, we revert to fixed foveation, and we need to perform one last
            // update of the shading rate map with the default values above.
            const bool isEyeGazeAvailable = m_UsingEyeGaze || wasUsingEyeGaze;

            bool skipDependency = false;
            ShadingRateMap shadingRateMap{};
            {
                std::unique_lock lock(m_ShadingRateMapsMutex);

                auto it = m_ShadingRateMaps.find(shadingRateMapResolution);
                if (it != m_ShadingRateMaps.end()) {
                    ShadingRateMap& updatableShadingRateMap = it->second;
                    if (isEyeGazeAvailable ||
                        updatableShadingRateMap.SettingsGeneration != m_CurrentSettingsGeneration ||
                        updatableShadingRateMap.Generation != m_CurrentGeneration) {
                        UpdateShadingRateMap(shadingRateMapResolution,
                                             Viewport0,
                                             Viewport1,
                                             updatableShadingRateMap,
                                             gazeX,
                                             gazeY,
                                             scaleFactor);
                    }

                    it->second.Age = 0;
                    shadingRateMap = it->second;

                    if (m_Context->IsCommandListCompleted(shadingRateMap.CompletedFenceValue)) {
                        // No need to create a dependency on the GPU.
                        skipDependency = true;
                    }

                    TraceLoggingWriteTagged(
                        local, "VRSManagerD3D12_Enable_Reuse", TLArg(!skipDependency, "NeedDependency"));

                } else {
                    // Request the shading rate map to be generated.
                    shadingRateMap = RequestShadingRateMap(
                        shadingRateMapResolution, Viewport0, Viewport1, gazeX, gazeY, scaleFactor);
                }

                ComPtr<ID3D12GraphicsCommandList5> vrsCommandList;
                CHECK_HRCMD(CommandList->QueryInterface(vrsCommandList.GetAddressOf()));

                // RSSetShadingRate() function sets both the combiners and the per-drawcall shading rate.
                // We set to 1X1 for all sources and all combiners to MAX, so that the coarsest wins
                // (per-drawcall, per-primitive, VRS surface).
                static const D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] = {
                    D3D12_SHADING_RATE_COMBINER_MAX, D3D12_SHADING_RATE_COMBINER_MAX};
                vrsCommandList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
                vrsCommandList->RSSetShadingRateImage(shadingRateMap.ShadingRateTexture.Get());
            }

            if (!skipDependency) {
                std::unique_lock lock(m_CommandListDependenciesMutex);

                // Add a dependency for command list submission.
                CommandListDependency dependency{};
                dependency.FenceValue = shadingRateMap.CompletedFenceValue;
                m_CommandListDependencies.insert_or_assign(CommandList, std::move(dependency));
            }

            TraceLoggingWriteStop(local, "VRSManagerD3D12_Enable");
        }

        void Disable(ID3D12CommandList* CommandList) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D12_Disable", TLPArg(CommandList, "CommandList"));

            ComPtr<ID3D12GraphicsCommandList5> vrsCommandList;
            CHECK_HRCMD(CommandList->QueryInterface(vrsCommandList.GetAddressOf()));

            vrsCommandList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, nullptr);
            vrsCommandList->RSSetShadingRateImage(nullptr);

            TraceLoggingWriteStop(local, "VRSManagerD3D12_Disable");
        }

        void SyncQueue(ID3D12CommandQueue* CommandQueue, const std::vector<ID3D12CommandList*>& CommandLists) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D12_SyncQueue", TLPArg(CommandQueue, "CommandQueue"));

            std::unique_lock lock(m_CommandListDependenciesMutex);

            for (const auto& commandList : CommandLists) {
                auto it = m_CommandListDependencies.find(commandList);
                if (it != m_CommandListDependencies.end()) {
                    const CommandListDependency& dependency = it->second;

                    // Insert a wait to ensure the shading rate map is ready for use.
                    TraceLoggingWriteTagged(local,
                                            "VRSManagerD3D12_SyncQueue_Wait",
                                            TLPArg(commandList, "CommandList"),
                                            TLArg(dependency.FenceValue, "FenceValue"));
                    CommandQueue->Wait(m_Context->GetCompletionFence(), dependency.FenceValue);

                    // Retire the dependency.
                    m_CommandListDependencies.erase(it);
                }
            }

            TraceLoggingWriteStop(local, "VRSManagerD3D12_SyncQueue", TLPArg(CommandQueue, "CommandQueue"));
        }

        bool GetGaze(float& X, float& Y, float& Scale) {
            // TODO: Implement me.
            return false;
        }

        void Tick() {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D12_Tick");

            {
                std::unique_lock lock(m_ShadingRateMapsMutex);

                TraceLoggingWriteTagged(local,
                                        "VRSManagerD3D12_Tick_Cleanup_ShadingRateMaps",
                                        TLArg(m_ShadingRateMaps.size(), "NumShadingRateMaps"));
                for (auto it = m_ShadingRateMaps.begin(); it != m_ShadingRateMaps.end();) {
                    // Age the unused masks and garbage-collect them.
                    if (++it->second.Age > 100) {
                        TraceLoggingWriteTagged(local,
                                                "VRSManagerD3D12_Tick_Cleanup_ShadingRateMaps",
                                                TLArg(it->first.Width, "TiledWidth"),
                                                TLArg(it->first.Height, "TiledHeight"));
                        m_HeapForUAVs->ReturnDescriptor(it->second.UAV);
                        it = m_ShadingRateMaps.erase(it);
                    } else {
                        it++;
                    }
                }
            }
            {
                std::unique_lock lock(m_CommandListDependenciesMutex);

                TraceLoggingWriteTagged(local,
                                        "VRSManagerD3D12_Tick_Cleanup_CommandListDependencies",
                                        TLArg(m_CommandListDependencies.size(), "NumCommandListDependencies"));
                for (auto it = m_CommandListDependencies.begin(); it != m_CommandListDependencies.end();) {
                    // Age the unused command list dependencies and garbage-collect them.
                    // An application may have started then abandoned a command list.
                    if (++it->second.Age > 100) {
                        TraceLoggingWriteTagged(local,
                                                "VRSManagerD3D12_Tick_Cleanup_CommandListDependencies",
                                                TLPArg(it->first, "CommandList"),
                                                TLArg(it->second.FenceValue, "FenceValue"));
                        it = m_CommandListDependencies.erase(it);
                    } else {
                        it++;
                    }
                }
            }

            m_CurrentGeneration++;

            TraceLoggingWriteStop(local, "VRSManagerD3D12_Tick", TLArg(m_CurrentGeneration, "CurrentGeneration"));
        }

        ShadingRateMap RequestShadingRateMap(const TiledResolution& Resolution,
                                             const D3D12_VIEWPORT& Viewport0,
                                             const D3D12_VIEWPORT& Viewport1,
                                             float CenterX,
                                             float CenterY,
                                             float ScaleFactor) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local,
                                   "VRSManagerD3D12_RequestShadingRateMap",
                                   TLArg(Resolution.Width, "TiledWidth"),
                                   TLArg(Resolution.Height, "TiledHeight"));

            ShadingRateMap newShadingRateMap;

            // Create the resources for the texture.
            const D3D12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            const D3D12_RESOURCE_DESC textureDesc =
                CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UINT,
                                             Resolution.Width,
                                             Resolution.Height,
                                             1 /* arraySize */,
                                             1 /* mipLevels */,
                                             1 /* sampleCount */,
                                             0 /* sampleQuality */,
                                             D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            CHECK_HRCMD(m_Device->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_PPV_ARGS(newShadingRateMap.ShadingRateTexture.ReleaseAndGetAddressOf())));
            newShadingRateMap.ShadingRateTexture->SetName(L"Shading Rate Texture");

            newShadingRateMap.UAV = m_HeapForUAVs->AllocateDescriptor();
            newShadingRateMap.UAVDescriptor = m_HeapForUAVs->GetGPUDescriptor(newShadingRateMap.UAV);

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Format = DXGI_FORMAT_R8_UINT;
            m_Device->CreateUnorderedAccessView(
                newShadingRateMap.ShadingRateTexture.Get(), nullptr, &uavDesc, newShadingRateMap.UAV);

            UpdateShadingRateMap(Resolution,
                                 Viewport0,
                                 Viewport1,
                                 newShadingRateMap,
                                 CenterX,
                                 CenterY,
                                 ScaleFactor,
                                 true /* IsFreshTexture */);

            m_ShadingRateMaps.insert_or_assign(Resolution, newShadingRateMap);

            TraceLoggingWriteStop(local,
                                  "VRSManagerD3D12_RequestShadingRateMap",
                                  TLArg(newShadingRateMap.CompletedFenceValue, "CompletedFenceValue"));

            return newShadingRateMap;
        }

        void UpdateShadingRateMap(const TiledResolution& Resolution,
                                  const D3D12_VIEWPORT& Viewport0,
                                  const D3D12_VIEWPORT& Viewport1,
                                  ShadingRateMap& ShadingRateMap,
                                  float CenterX,
                                  float CenterY,
                                  float ScaleFactor,
                                  bool IsFreshTexture = false) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local,
                                   "VRSManagerD3D12_UpdateShadingRateMap",
                                   TLArg(Resolution.Width, "TiledWidth"),
                                   TLArg(Resolution.Height, "TiledHeight"));

            // Prepare a command list.
            CommandList commandList = m_Context->GetCommandList();
            ID3D12DescriptorHeap* heaps[] = {m_HeapForUAVs->GetDescriptorHeap()};
            commandList.Commands->SetDescriptorHeaps(1, heaps);

            if (!IsFreshTexture) {
                // Transition to UAV state for the compute shader.
                const D3D12_RESOURCE_BARRIER barrier =
                    CD3DX12_RESOURCE_BARRIER::Transition(ShadingRateMap.ShadingRateTexture.Get(),
                                                         D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
                                                         D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                commandList.Commands->ResourceBarrier(1, &barrier);
            }

            // Common state for running the shader to generate the shading rate map.
            GenerateShadingRateMapConstants constants{};
            {
                std::shared_lock lock(m_ParametersMutex);
                constants.InnerRing = ScaleFactor * (m_InnerRing / 2.f) * Resolution.Height;
                constants.OuterRing = ScaleFactor * (m_OuterRing / 2.f) * Resolution.Height;
                constants.Rate1x1 = m_InnerRate;
                constants.RateMedium = m_MiddleRate;
                constants.RateLow = m_OuterRate;
            }

            commandList.Commands->SetComputeRootSignature(m_GenerateRootSignature.Get());
            commandList.Commands->SetPipelineState(m_GeneratePSO.Get());

            // Dispatch the compute shader for each view to generate the map.
            // If double-wide is used, we will draw two (separate) areas. Otherwise, we will additively create a
            // combined area with both views.
            const bool isDoubleWide = Viewport1.Width;
            const UINT viewWidth = !isDoubleWide ? Resolution.Width : (Resolution.Width / 2);
            for (uint32_t i = 0; i < 2; i++) {
                const auto viewport = (i == 0 || !isDoubleWide) ? Viewport0 : Viewport1;

                constants.Left =
                    Align(static_cast<UINT>(viewport.TopLeftX + FLT_EPSILON), m_VRSTileSize) / m_VRSTileSize;
                constants.Top =
                    Align(static_cast<UINT>(viewport.TopLeftY + FLT_EPSILON), m_VRSTileSize) / m_VRSTileSize;
                constants.CenterX = CenterX * viewWidth;
                constants.CenterY = CenterY * Resolution.Height;
                constants.Additive = (i == 1 && !isDoubleWide);

                commandList.Commands->SetComputeRootDescriptorTable(0, ShadingRateMap.UAVDescriptor);
                commandList.Commands->SetComputeRoot32BitConstants(
                    1, sizeof(GenerateShadingRateMapConstants) / 4, &constants, 0);
                commandList.Commands->Dispatch(
                    Align((UINT)viewWidth, 16) / 16, Align((UINT)Resolution.Height, 16) / 16, 1);
            }

            // Transition to the correct state for use with VRS.
            const D3D12_RESOURCE_BARRIER barrier =
                CD3DX12_RESOURCE_BARRIER::Transition(ShadingRateMap.ShadingRateTexture.Get(),
                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                     D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
            commandList.Commands->ResourceBarrier(1, &barrier);

            ShadingRateMap.CompletedFenceValue = m_Context->SubmitCommandList(commandList);
            ShadingRateMap.Generation = m_CurrentGeneration;
            ShadingRateMap.SettingsGeneration = m_CurrentSettingsGeneration;

            TraceLoggingWriteStop(local,
                                  "VRSManagerD3D12_UpdateShadingRateMap",
                                  TLArg(ShadingRateMap.CompletedFenceValue, "CompletedFenceValue"));
        }

        ComPtr<ID3D12Device> m_Device;
        UINT m_VRSTileSize{0};

        std::atomic<bool> m_Enabled;
        Resolution m_PresentResolution;

        std::unique_ptr<CommandContext> m_Context;
        std::unique_ptr<DescriptorHeap> m_HeapForUAVs;

        ComPtr<ID3D12RootSignature> m_GenerateRootSignature;
        ComPtr<ID3D12PipelineState> m_GeneratePSO;

        std::shared_mutex m_ParametersMutex;
        float m_InnerRing{0.35f};
        float m_OuterRing{0.6f};
        D3D12_SHADING_RATE m_InnerRate{D3D12_SHADING_RATE_1X1};
        D3D12_SHADING_RATE m_MiddleRate{D3D12_SHADING_RATE_2X2};
        D3D12_SHADING_RATE m_OuterRate{D3D12_SHADING_RATE_4X4};
        uint64_t m_CurrentSettingsGeneration{0};

        std::mutex m_ShadingRateMapsMutex;
        std::unordered_map<TiledResolution, ShadingRateMap, TiledResolution> m_ShadingRateMaps;
        uint64_t m_CurrentGeneration{0};

        bool m_UsingEyeGaze{false};

        std::mutex m_CommandListDependenciesMutex;
        std::unordered_map<ID3D12CommandList*, CommandListDependency> m_CommandListDependencies;
    };

    std::unique_ptr<VRSManagerD3D12> g_InjectionManager;

    DECLARE_DETOUR_FUNCTION(void,
                            STDMETHODCALLTYPE,
                            ID3D12GraphicsCommandList_RSSetViewports,
                            ID3D12GraphicsCommandList* pCommandList,
                            UINT NumViewports,
                            const D3D12_VIEWPORT* pViewports) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local,
                               "ID3D12GraphicsCommandList_RSSetViewports",
                               TLPArg(pCommandList, "CommandList"),
                               TLArg(NumViewports, "NumViewports"));

        if (IsTraceEnabled() && pViewports) {
            for (UINT i = 0; i < NumViewports; i++) {
                TraceLoggingWriteTagged(local,
                                        "ID3D12GraphicsCommandList_RSSetViewports",
                                        TLArg(i, "ViewportIndex"),
                                        TLArg(pViewports[i].TopLeftX, "TopLeftX"),
                                        TLArg(pViewports[i].TopLeftY, "TopLeftY"),
                                        TLArg(pViewports[i].Width, "Width"),
                                        TLArg(pViewports[i].Height, "Height"));
            }
        }

        assert(original_ID3D12GraphicsCommandList_RSSetViewports);
        original_ID3D12GraphicsCommandList_RSSetViewports(pCommandList, NumViewports, pViewports);

        // Invoke the hook after the state has been set on the command list.
        assert(g_InjectionManager);
        g_InjectionManager->OnSetViewports(pCommandList,
                                           NumViewports > 0 ? pViewports[0] : D3D12_VIEWPORT{},
                                           NumViewports > 1 ? pViewports[1] : D3D12_VIEWPORT{});

        TraceLoggingWriteStop(local, "ID3D12GraphicsCommandList_RSSetViewports");
    }

    DECLARE_DETOUR_FUNCTION(void,
                            STDMETHODCALLTYPE,
                            ID3D12CommandQueue_ExecuteCommandLists,
                            ID3D12CommandQueue* pCommandQueue,
                            UINT NumCommandLists,
                            ID3D12CommandList* const* ppCommandLists) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local,
                               "ID3D12CommandQueue_ExecuteCommandLists",
                               TLPArg(pCommandQueue, "CommandQueue"),
                               TLArg(NumCommandLists, "NumCommandLists"));

        if (IsTraceEnabled() && ppCommandLists) {
            for (UINT i = 0; i < NumCommandLists; i++) {
                TraceLoggingWriteTagged(
                    local, "ID3D12CommandQueue_ExecuteCommandLists", TLPArg(ppCommandLists[i], "pCommandList"));
            }
        }

        // Invoke the hook before the real execution, in order to inject Wait() commands if needed.
        assert(g_InjectionManager);
        std::vector<ID3D12CommandList*> commandLists(ppCommandLists, ppCommandLists + NumCommandLists);
        g_InjectionManager->OnExecuteCommandLists(pCommandQueue, commandLists);

        assert(original_ID3D12CommandQueue_ExecuteCommandLists);
        original_ID3D12CommandQueue_ExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);

        TraceLoggingWriteStop(local, "ID3D12CommandQueue_ExecuteCommandLists");
    }

    ComPtr<ID3D12GraphicsCommandList> g_HookedCommandList;
    ComPtr<ID3D12CommandQueue> g_HookedCommandQueue;

} // namespace

namespace vrs {

    using namespace DetoursUtils;

    void InstallD3D12Hooks(ID3D12Device* device, const Resolution& presentResolution) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "InstallD3D12Hooks");

        const bool needHooks = !g_InjectionManager;
        try {
            g_InjectionManager = std::make_unique<VRSManagerD3D12>(device, presentResolution);
        } catch (FeatureNotSupported&) {
            return;
        }

        if (needHooks) {
            // Hook to the command list's RSSetViewports(), where we will decide whether or not to inject VRS commands.
            ComPtr<ID3D12CommandAllocator> commandAllocator;
            CHECK_HRCMD(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                       IID_PPV_ARGS(commandAllocator.ReleaseAndGetAddressOf())));

            ComPtr<ID3D12GraphicsCommandList> commandList;
            CHECK_HRCMD(device->CreateCommandList(0,
                                                  D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                  commandAllocator.Get(),
                                                  nullptr,
                                                  IID_PPV_ARGS(commandList.ReleaseAndGetAddressOf())));

            GetRealD3D12Object(commandList.Get(), g_HookedCommandList.ReleaseAndGetAddressOf());

            TraceLoggingWriteTagged(
                local, "InstallD3D12Hooks_Detour_RSViewports", TLPArg(g_HookedCommandList.Get(), "CommandList"));
            DetourMethodAttach(g_HookedCommandList.Get(),
                               21, // RSSetViewports()
                               hooked_ID3D12GraphicsCommandList_RSSetViewports,
                               original_ID3D12GraphicsCommandList_RSSetViewports);

            // Hook to the command queue's ExecuteCommandLists() in order to add synchronization between our command
            // lists.
            D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
            commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            ComPtr<ID3D12CommandQueue> commandQueue;
            CHECK_HRCMD(
                device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue.ReleaseAndGetAddressOf())));

            GetRealD3D12Object(commandQueue.Get(), g_HookedCommandQueue.ReleaseAndGetAddressOf());

            TraceLoggingWriteTagged(local,
                                    "InstallD3D12Hooks_Detour_ExecuteCommandLists",
                                    TLPArg(g_HookedCommandQueue.Get(), "CommandQueue"));
            DetourMethodAttach(g_HookedCommandQueue.Get(),
                               10, // ExecuteCommandLists()
                               hooked_ID3D12CommandQueue_ExecuteCommandLists,
                               original_ID3D12CommandQueue_ExecuteCommandLists);
        }

        TraceLoggingWriteStop(local, "InstallD3D12Hooks");
    }

    void UninstallD3D12Hooks() {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "UninstallD3D12Hooks");

        if (g_InjectionManager) {
            g_InjectionManager->Flush();

            TraceLoggingWriteTagged(
                local, "InstallD3D12Hooks_Detour_RSViewports", TLPArg(g_HookedCommandList.Get(), "CommandList"));
            DetourMethodDetach(g_HookedCommandList.Get(),
                               21, // RSSetViewports()
                               hooked_ID3D12GraphicsCommandList_RSSetViewports,
                               original_ID3D12GraphicsCommandList_RSSetViewports);
            g_HookedCommandList.Reset();

            TraceLoggingWriteTagged(local,
                                    "InstallD3D12Hooks_Detour_ExecuteCommandLists",
                                    TLPArg(g_HookedCommandQueue.Get(), "CommandQueue"));
            DetourMethodDetach(g_HookedCommandQueue.Get(),
                               10, // ExecuteCommandLists()
                               hooked_ID3D12CommandQueue_ExecuteCommandLists,
                               original_ID3D12CommandQueue_ExecuteCommandLists);
            g_HookedCommandQueue.Reset();

            g_InjectionManager.reset();
        }

        TraceLoggingWriteStop(local, "UninstallD3D12Hooks");
    }

    void SetStateD3D12(bool state, std::optional<Parameters> parameters) {
        if (g_InjectionManager) {
            if (state) {
                g_InjectionManager->Deinhibit();
            } else {
                g_InjectionManager->Inhibit();
            }
            if (parameters) {
                g_InjectionManager->SetParameters(*parameters);
            }
        }
    }

    void NewFrameD3D12() {
        if (g_InjectionManager) {
            g_InjectionManager->OnUpdate();
        }
    }

} // namespace vrs

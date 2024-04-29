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

#include "DetoursUtils.h"
#include "VRSUtils.h"

#include "GenerateShadingRateMapCS.h"

#ifdef _WIN64
#pragma comment(lib, "nvapi64.lib")
#else
#pragma comment(lib, "nvapi.lib")
#endif

using namespace virtualdesktop_openxr::log;

namespace {

    using namespace vrs;

#define Align(value, pad_to) (((value) + (pad_to)-1) & ~((pad_to)-1))

    struct FeatureNotSupported : public std::exception {
        const char* what() const throw() {
            return "Feature is not supported";
        }
    };

    static inline void SetDebugName(ID3D11DeviceChild* resource, const wchar_t* name) {
        if (resource && name[0]) {
            resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(wcslen(name)), name);
        }
    }

    class VRSManagerD3D11 {
      private:
        struct ShadingRateMap {
            uint64_t Generation{0};
            uint64_t SettingsGeneration{0};
            unsigned int Age{0};
            ComPtr<ID3D11Texture2D> ShadingRateTexture;
            ComPtr<ID3D11Texture2D> ShadingRateTextureArray;
            ComPtr<ID3D11UnorderedAccessView> UAV;
            ComPtr<ID3D11UnorderedAccessView> UAVArray;
            ComPtr<ID3D11NvShadingRateResourceView> SRRV;
            ComPtr<ID3D11NvShadingRateResourceView> SRRVArray;
        };

      public:
        VRSManagerD3D11(ID3D11Device* Device, const Resolution& PresentResolution)
            : m_Device(Device), m_PresentResolution(PresentResolution) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D11_Create", TLPArg(Device, "Device"));

            // Check for support on this device.
            auto status = NvAPI_Initialize();
            if (status != NVAPI_OK) {
                TraceLoggingWriteTagged(
                    local, "VRSManagerD3D11_Create_NotSupported", TLArg((int)status, "InitializeError"));
                NvAPI_ShortString errorMessage{};
                if (NvAPI_GetErrorMessage(status, errorMessage) == NVAPI_OK) {
                    Log("Failed to initialize NVAPI: %s\n", errorMessage);
                }
                throw FeatureNotSupported();
            }

            NV_D3D1x_GRAPHICS_CAPS graphicCaps{};
            status = NvAPI_D3D1x_GetGraphicsCapabilities(Device, NV_D3D1x_GRAPHICS_CAPS_VER, &graphicCaps);
            if (status != NVAPI_OK || !graphicCaps.bVariablePixelRateShadingSupported) {
                TraceLoggingWriteTagged(
                    local,
                    "VRSManagerD3D11_Create_NotSupported",
                    TLArg((int)status, "GetGraphicsCapabilitiesError"),
                    TLArg(!!graphicCaps.bVariablePixelRateShadingSupported, "VariablePixelRateShadingSupported"));
                throw FeatureNotSupported();
            }

            CHECK_NVCMD(NvAPI_D3D_RegisterDevice(m_Device.Get()));

            m_VRSTileSize = NV_VARIABLE_PIXEL_SHADING_TILE_WIDTH;

            // Create a context where we will perform the generation of the shading rate textures.
            ComPtr<ID3D11Device1> device1;
            CHECK_HRCMD(m_Device->QueryInterface(device1.ReleaseAndGetAddressOf()));
            UINT creationFlags = 0;
            if (m_Device->GetCreationFlags() & D3D11_CREATE_DEVICE_SINGLETHREADED) {
                creationFlags |= D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED;
            }
            const D3D_FEATURE_LEVEL featureLevel = m_Device->GetFeatureLevel();
            CHECK_HRCMD(device1->CreateDeviceContextState(creationFlags,
                                                          &featureLevel,
                                                          1,
                                                          D3D11_SDK_VERSION,
                                                          __uuidof(ID3D11Device),
                                                          nullptr,
                                                          m_GenerateContext.ReleaseAndGetAddressOf()));

            // Create resources for the GenerateShadingRateMap compute shader.
            {
                D3D11_BUFFER_DESC desc{};
                desc.ByteWidth = ((sizeof(GenerateShadingRateMapConstants) + 15) / 16) * 16;
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                CHECK_HRCMD(m_Device->CreateBuffer(&desc, nullptr, m_GenerateConstants.ReleaseAndGetAddressOf()));
                SetDebugName(m_GenerateConstants.Get(), L"GenerateShadingRateMap Constants");
            }
            CHECK_HRCMD(m_Device->CreateComputeShader(g_GenerateShadingRateMapCS,
                                                      sizeof(g_GenerateShadingRateMapCS),
                                                      nullptr,
                                                      m_GenerateCS.ReleaseAndGetAddressOf()));
            SetDebugName(m_GenerateCS.Get(), L"GenerateShadingRateMap CS");

            m_Enabled.store(true);

            TraceLoggingWriteStop(local, "VRSManagerD3D11_Create");
        }

        void OnSetViewports(ID3D11DeviceContext* Context,
                            const D3D11_VIEWPORT& Viewport0,
                            const D3D11_VIEWPORT& Viewport1) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D11_OnSetViewports", TLPArg(Context, "Context"));

            ComPtr<ID3D11Device> device;
            Context->GetDevice(device.ReleaseAndGetAddressOf());

            if (device == m_Device) {
                if (m_Enabled.load() &&
                    IsViewportEligible(m_PresentResolution, {(UINT)Viewport0.Width, (UINT)Viewport0.Height})) {
                    Enable(Context,
                           Viewport0,
                           // Detect double-wide.
                           IsViewportEligible(m_PresentResolution, {(UINT)Viewport1.Width, (UINT)Viewport1.Height})
                               ? Viewport1
                               : D3D11_VIEWPORT{});
                } else {
                    Disable(Context);
                }
            }

            TraceLoggingWriteStop(local, "VRSManagerD3D11_OnSetViewports");
        }

        void OnUpdate() {
            Tick();
        }

        void Inhibit() {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D11_Inhibit");

            m_Enabled.store(false);

            ComPtr<ID3D11DeviceContext> context;
            m_Device->GetImmediateContext(context.ReleaseAndGetAddressOf());
            Disable(context.Get());

            TraceLoggingWriteStop(local, "VRSManagerD3D11_Inhibit");
        }

        void Deinhibit() {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D11_Deinhibit");

            m_Enabled.store(true);

            TraceLoggingWriteStop(local, "VRSManagerD3D11_Deinhibit");
        }

        void SetParameters(const Parameters& parameters) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D11_SetParameters");

            std::unique_lock lock(m_ParametersMutex);

            m_InnerRing = parameters.InnerRing;
            m_OuterRing = parameters.OuterRing;

            const auto toRate = [](Rate rate) {
                if (rate.X == RateComponent::_1 && rate.Y == RateComponent::_2) {
                    return NV_PIXEL_X1_PER_1X2_RASTER_PIXELS;
                } else if (rate.X == RateComponent::_2 && rate.Y == RateComponent::_1) {
                    return NV_PIXEL_X1_PER_2X1_RASTER_PIXELS;
                } else if (rate.X == RateComponent::_2 && rate.Y == RateComponent::_2) {
                    return NV_PIXEL_X1_PER_2X2_RASTER_PIXELS;
                } else if (rate.X == RateComponent::_2 && rate.Y == RateComponent::_4) {
                    return NV_PIXEL_X1_PER_2X4_RASTER_PIXELS;
                } else if (rate.X == RateComponent::_4 && rate.Y == RateComponent::_2) {
                    return NV_PIXEL_X1_PER_4X2_RASTER_PIXELS;
                } else if (rate.X == RateComponent::_4 && rate.Y == RateComponent::_4) {
                    return NV_PIXEL_X1_PER_4X4_RASTER_PIXELS;
                }
                return NV_PIXEL_X1_PER_RASTER_PIXEL;
            };
            m_InnerRate = toRate(parameters.InnerRate);
            m_MiddleRate = toRate(parameters.MiddleRate);
            m_OuterRate = toRate(parameters.OuterRate);

            m_CurrentSettingsGeneration++;

            TraceLoggingWriteStop(local,
                                  "VRSManagerD3D11_SetParameters",
                                  TLArg(m_CurrentSettingsGeneration, "CurrentSettingsGeneration"));
        }

        ID3D11Device* GetDevice() const {
            return m_Device.Get();
        }

      private:
        void Enable(ID3D11DeviceContext* Context, const D3D11_VIEWPORT& Viewport0, const D3D11_VIEWPORT& Viewport1) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D11_Enable", TLPArg(Context, "Context"));

            const bool isDoubleWide = Viewport1.Width;
            bool isStereoTextureArray = false;
            {
                ID3D11RenderTargetView* renderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
                Context->OMGetRenderTargets(ARRAYSIZE(renderTargets), renderTargets, nullptr);

                if (renderTargets[0]) {
                    D3D11_RENDER_TARGET_VIEW_DESC desc{};
                    renderTargets[0]->GetDesc(&desc);
                    isStereoTextureArray = (desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY &&
                                            desc.Texture2DArray.ArraySize == 2) ||
                                           (desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY &&
                                            desc.Texture2DMSArray.ArraySize == 2);
                }

                bool empty = true;
                for (uint32_t i = 0; i < ARRAYSIZE(renderTargets); i++) {
                    if (renderTargets[i]) {
                        empty = false;
                        renderTargets[i]->Release();
                    }
                }

                // This means a future call to OMSetRenderTargets() or RSSetViewports() will do the right thing.
                if (empty) {
                    TraceLoggingWriteStop(local, "VRSManagerD3D11_Enable", TLArg("NoRTVs", "SkipReason"));
                    Disable(Context);
                    return;
                }
            }

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
                                    "VRSManagerD3D11_Enable",
                                    TLArg(shadingRateMapResolution.Width, "TiledWidth"),
                                    TLArg(shadingRateMapResolution.Height, "TiledHeight"),
                                    TLArg(isDoubleWide, "IsDoubleWide"),
                                    TLArg(isStereoTextureArray, "IsStereoTextureArray"));

            float gazeX = 0.5f, gazeY = 0.5f, scaleFactor = 1.f;
            const bool wasUsingEyeGaze = m_UsingEyeGaze;
            m_UsingEyeGaze = GetGaze(gazeX, gazeY, scaleFactor);
            // When eye gaze becomes unavailable, we revert to fixed foveation, and we need to perform one last
            // update of the shading rate map with the default values above.
            const bool isEyeGazeAvailable = m_UsingEyeGaze || wasUsingEyeGaze;

            ShadingRateMap shadingRateMap{};
            {
                ComPtr<ID3D11DeviceContext1> context1;
                CHECK_HRCMD(Context->QueryInterface(context1.ReleaseAndGetAddressOf()));

                std::unique_lock lock(m_ShadingRateMapsMutex);

                auto it = m_ShadingRateMaps.find(shadingRateMapResolution);
                if (it != m_ShadingRateMaps.end()) {
                    ShadingRateMap& updatableShadingRateMap = it->second;
                    TraceLoggingWriteTagged(local,
                                            "VRSManagerD3D11_Enable",
                                            TLArg(updatableShadingRateMap.SettingsGeneration, "MapSettingsGeneration"),
                                            TLArg(m_CurrentSettingsGeneration, "CurrentSettingsGeneration"),
                                            TLArg(updatableShadingRateMap.Generation, "MapGeneration"),
                                            TLArg(m_CurrentGeneration, "CurrentGeneration"));
                    if (isEyeGazeAvailable ||
                        updatableShadingRateMap.SettingsGeneration != m_CurrentSettingsGeneration ||
                        updatableShadingRateMap.Generation != m_CurrentGeneration) {
                        UpdateShadingRateMap(context1.Get(),
                                             shadingRateMapResolution,
                                             Viewport0,
                                             Viewport1,
                                             updatableShadingRateMap,
                                             gazeX,
                                             gazeY,
                                             scaleFactor);
                    }

                    it->second.Age = 0;
                    shadingRateMap = it->second;

                } else {
                    // Request the shading rate map to be generated.
                    shadingRateMap = RequestShadingRateMap(
                        context1.Get(), shadingRateMapResolution, Viewport0, Viewport1, gazeX, gazeY, scaleFactor);
                }

                // This is it, now we send the commands to enable VRS on the future draws.
                NV_D3D11_VIEWPORTS_SHADING_RATE_DESC desc{};
                desc.version = NV_D3D11_VIEWPORTS_SHADING_RATE_DESC_VER;
                NV_D3D11_VIEWPORT_SHADING_RATE_DESC rateTable[2]{};
                {
                    std::shared_lock lock(m_ParametersMutex);
                    for (size_t i = 0; i < (!isDoubleWide ? 1 : 2); i++) {
                        rateTable[i].enableVariablePixelShadingRate = true;
                        rateTable[i].shadingRateTable[0] = m_OuterRate;
                        rateTable[i].shadingRateTable[1] = NV_PIXEL_X0_CULL_RASTER_PIXELS; // m_MiddleRate;
                        rateTable[i].shadingRateTable[2] = m_InnerRate;
                    }
                }
                desc.numViewports = !isDoubleWide ? 1 : 2;
                desc.pViewports = rateTable;
                CHECK_NVCMD(NvAPI_D3D11_RSSetViewportsPixelShadingRates(Context, &desc));
                CHECK_NVCMD(NvAPI_D3D11_RSSetShadingRateResourceView(
                    Context, !isStereoTextureArray ? shadingRateMap.SRRV.Get() : shadingRateMap.SRRVArray.Get()));
            }

            m_Active = true;

            TraceLoggingWriteStop(local, "VRSManagerD3D11_Enable");
        }

        void Disable(ID3D11DeviceContext* Context) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D11_Disable", TLPArg(Context, "Context"));

            const bool wasActive = m_Active;
            if (m_Active) {
                NV_D3D11_VIEWPORTS_SHADING_RATE_DESC desc{};
                desc.version = NV_D3D11_VIEWPORTS_SHADING_RATE_DESC_VER;
                CHECK_NVCMD(NvAPI_D3D11_RSSetViewportsPixelShadingRates(Context, &desc));
                CHECK_NVCMD(NvAPI_D3D11_RSSetShadingRateResourceView(Context, nullptr));
            }
            m_Active = false;

            TraceLoggingWriteStop(local, "VRSManagerD3D11_Disable", TLArg(wasActive, "WasActive"));
        }

        bool GetGaze(float& X, float& Y, float& Scale) {
            // TODO: Implement me.
            return false;
        }

        void Tick() {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "VRSManagerD3D11_Tick");

            {
                std::unique_lock lock(m_ShadingRateMapsMutex);

                TraceLoggingWriteTagged(local,
                                        "VRSManagerD3D11_Tick_Cleanup_ShadingRateMaps",
                                        TLArg(m_ShadingRateMaps.size(), "NumShadingRateMaps"));
                for (auto it = m_ShadingRateMaps.begin(); it != m_ShadingRateMaps.end();) {
                    // Age the unused masks and garbage-collect them.
                    if (++it->second.Age > 100) {
                        TraceLoggingWriteTagged(local,
                                                "VRSManagerD3D11_Tick_Cleanup_ShadingRateMaps",
                                                TLArg(it->first.Width, "TiledWidth"),
                                                TLArg(it->first.Height, "TiledHeight"));
                        it = m_ShadingRateMaps.erase(it);
                    } else {
                        it++;
                    }
                }
            }

            m_CurrentGeneration++;

            TraceLoggingWriteStop(local, "VRSManagerD3D11_Tick", TLArg(m_CurrentGeneration, "CurrentGeneration"));
        }

        ShadingRateMap RequestShadingRateMap(ID3D11DeviceContext1* Context,
                                             const TiledResolution& Resolution,
                                             const D3D11_VIEWPORT& Viewport0,
                                             const D3D11_VIEWPORT& Viewport1,
                                             float CenterX,
                                             float CenterY,
                                             float ScaleFactor) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local,
                                   "VRSManagerD3D11_RequestShadingRateMap",
                                   TLArg(Resolution.Width, "TiledWidth"),
                                   TLArg(Resolution.Height, "TiledHeight"));

            ShadingRateMap newShadingRateMap;

            // Create the resources for the texture.
            {
                D3D11_TEXTURE2D_DESC textureDesc{};
                textureDesc.Format = DXGI_FORMAT_R8_UINT;
                textureDesc.Width = Resolution.Width;
                textureDesc.Height = Resolution.Height;
                textureDesc.ArraySize = textureDesc.MipLevels = textureDesc.SampleDesc.Count = 1;
                textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
                CHECK_HRCMD(m_Device->CreateTexture2D(
                    &textureDesc, nullptr, newShadingRateMap.ShadingRateTexture.ReleaseAndGetAddressOf()));
                SetDebugName(newShadingRateMap.ShadingRateTexture.Get(), L"Shading Rate Texture");

                textureDesc.ArraySize = 2;
                CHECK_HRCMD(m_Device->CreateTexture2D(
                    &textureDesc, nullptr, newShadingRateMap.ShadingRateTextureArray.ReleaseAndGetAddressOf()));
                SetDebugName(newShadingRateMap.ShadingRateTextureArray.Get(), L"Shading Rate Texture Array");
            }

            {
                D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                uavDesc.Format = DXGI_FORMAT_R8_UINT;
                CHECK_HRCMD(m_Device->CreateUnorderedAccessView(newShadingRateMap.ShadingRateTexture.Get(),
                                                                &uavDesc,
                                                                newShadingRateMap.UAV.ReleaseAndGetAddressOf()));
                SetDebugName(newShadingRateMap.UAV.Get(), L"Shading Rate Texture UAV");

                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.ArraySize = 2;
                CHECK_HRCMD(m_Device->CreateUnorderedAccessView(newShadingRateMap.ShadingRateTextureArray.Get(),
                                                                &uavDesc,
                                                                newShadingRateMap.UAVArray.ReleaseAndGetAddressOf()));
                SetDebugName(newShadingRateMap.UAV.Get(), L"Shading Rate Texture Array UAV");
            }

            {
                NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC srrvDesc{};
                srrvDesc.version = NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC_VER;
                srrvDesc.Format = DXGI_FORMAT_R8_UINT;
                srrvDesc.ViewDimension = NV_SRRV_DIMENSION_TEXTURE2D;
                CHECK_NVCMD(NvAPI_D3D11_CreateShadingRateResourceView(m_Device.Get(),
                                                                      newShadingRateMap.ShadingRateTexture.Get(),
                                                                      &srrvDesc,
                                                                      newShadingRateMap.SRRV.ReleaseAndGetAddressOf()));

                srrvDesc.ViewDimension = NV_SRRV_DIMENSION_TEXTURE2DARRAY;
                CHECK_NVCMD(
                    NvAPI_D3D11_CreateShadingRateResourceView(m_Device.Get(),
                                                              newShadingRateMap.ShadingRateTextureArray.Get(),
                                                              &srrvDesc,
                                                              newShadingRateMap.SRRVArray.ReleaseAndGetAddressOf()));
            }

            UpdateShadingRateMap(
                Context, Resolution, Viewport0, Viewport1, newShadingRateMap, CenterX, CenterY, ScaleFactor);

            m_ShadingRateMaps.insert_or_assign(Resolution, newShadingRateMap);

            TraceLoggingWriteStop(local, "VRSManagerD3D11_RequestShadingRateMap");

            return newShadingRateMap;
        }

        void UpdateShadingRateMap(ID3D11DeviceContext1* Context,
                                  const TiledResolution& Resolution,
                                  const D3D11_VIEWPORT& Viewport0,
                                  const D3D11_VIEWPORT& Viewport1,
                                  ShadingRateMap& ShadingRateMap,
                                  float CenterX,
                                  float CenterY,
                                  float ScaleFactor) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local,
                                   "VRSManagerD3D11_UpdateShadingRateMap",
                                   TLArg(Resolution.Width, "TiledWidth"),
                                   TLArg(Resolution.Height, "TiledHeight"));

            // TODO: We may consider only pushing/popping the Compute state.
            ComPtr<ID3DDeviceContextState> savedContext;
            Context->SwapDeviceContextState(m_GenerateContext.Get(), savedContext.ReleaseAndGetAddressOf());

            // Ensure that we always restore the application device context.
            auto scopeGuard = MakeScopeGuard([&] { Context->SwapDeviceContextState(savedContext.Get(), nullptr); });

            // Common state for running the shader to generate the shading rate map.
            GenerateShadingRateMapConstants constants{};
            {
                std::shared_lock lock(m_ParametersMutex);
                constants.InnerRing = ScaleFactor * (m_InnerRing / 2.f) * Resolution.Height;
                constants.OuterRing = ScaleFactor * (m_OuterRing / 2.f) * Resolution.Height;
            }
            constants.Rate1x1 = 2;
            constants.RateMedium = 1;
            constants.RateLow = 0;

            Context->CSSetShader(m_GenerateCS.Get(), nullptr, 0);
            ID3D11UnorderedAccessView* UAVs[] = {ShadingRateMap.UAV.Get(), ShadingRateMap.UAVArray.Get()};
            Context->CSSetUnorderedAccessViews(0, ARRAYSIZE(UAVs), UAVs, nullptr);

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
                constants.Slice = i;
                constants.Additive = (i == 1 && !isDoubleWide);
                {
                    D3D11_MAPPED_SUBRESOURCE mappedResources;
                    CHECK_HRCMD(
                        Context->Map(m_GenerateConstants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResources));
                    memcpy(mappedResources.pData, &constants, sizeof(constants));
                    Context->Unmap(m_GenerateConstants.Get(), 0);
                }
                Context->CSSetConstantBuffers(0, 1, m_GenerateConstants.GetAddressOf());
                Context->Dispatch(Align((UINT)viewWidth, 16) / 16, Align((UINT)Resolution.Height, 16) / 16, 1);
            }

            ShadingRateMap.Generation = m_CurrentGeneration;

            TraceLoggingWriteStop(local, "VRSManagerD3D11_UpdateShadingRateMap");
        }

        ComPtr<ID3D11Device> m_Device;
        ComPtr<ID3DDeviceContextState> m_GenerateContext;
        UINT m_VRSTileSize{0};

        std::atomic<bool> m_Enabled;
        bool m_Active{false};
        Resolution m_PresentResolution;

        ComPtr<ID3D11Buffer> m_GenerateConstants;
        ComPtr<ID3D11ComputeShader> m_GenerateCS;

        std::shared_mutex m_ParametersMutex;
        float m_InnerRing{0.35f};
        float m_OuterRing{0.6f};
        NV_PIXEL_SHADING_RATE m_InnerRate{NV_PIXEL_X1_PER_RASTER_PIXEL};
        NV_PIXEL_SHADING_RATE m_MiddleRate{NV_PIXEL_X1_PER_2X2_RASTER_PIXELS};
        NV_PIXEL_SHADING_RATE m_OuterRate{NV_PIXEL_X1_PER_4X4_RASTER_PIXELS};
        uint64_t m_CurrentSettingsGeneration{0};

        std::mutex m_ShadingRateMapsMutex;
        std::unordered_map<TiledResolution, ShadingRateMap, TiledResolution> m_ShadingRateMaps;
        uint64_t m_CurrentGeneration{0};

        bool m_UsingEyeGaze{false};
    };

    std::unique_ptr<VRSManagerD3D11> g_InjectionManager;

    DECLARE_DETOUR_FUNCTION(void,
                            STDMETHODCALLTYPE,
                            ID3D11DeviceContext_RSSetViewports,
                            ID3D11DeviceContext* pContext,
                            UINT NumViewports,
                            const D3D11_VIEWPORT* pViewports) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local,
                               "ID3D11DeviceContext_RSSetViewports",
                               TLPArg(pContext, "Context"),
                               TLArg(NumViewports, "NumViewports"));

        if (IsTraceEnabled() && pViewports) {
            for (UINT i = 0; i < NumViewports; i++) {
                TraceLoggingWriteTagged(local,
                                        "ID3D11DeviceContext_RSSetViewports",
                                        TLArg(i, "ViewportIndex"),
                                        TLArg(pViewports[i].TopLeftX, "TopLeftX"),
                                        TLArg(pViewports[i].TopLeftY, "TopLeftY"),
                                        TLArg(pViewports[i].Width, "Width"),
                                        TLArg(pViewports[i].Height, "Height"));
            }
        }

        assert(original_ID3D11DeviceContext_RSSetViewports);
        original_ID3D11DeviceContext_RSSetViewports(pContext, NumViewports, pViewports);

        // Invoke the hook after the state has been set on the command list.
        assert(g_InjectionManager);
        g_InjectionManager->OnSetViewports(pContext,
                                           NumViewports > 0 ? pViewports[0] : D3D11_VIEWPORT{},
                                           NumViewports > 1 ? pViewports[1] : D3D11_VIEWPORT{});

        TraceLoggingWriteStop(local, "ID3D11DeviceContext_RSSetViewports");
    }

    DECLARE_DETOUR_FUNCTION(static void,
                            STDMETHODCALLTYPE,
                            ID3D11DeviceContext_OMSetRenderTargets,
                            ID3D11DeviceContext* Context,
                            UINT NumViews,
                            ID3D11RenderTargetView* const* ppRenderTargetViews,
                            ID3D11DepthStencilView* pDepthStencilView) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local,
                               "ID3D11DeviceContext_OMSetRenderTargets",
                               TLPArg(Context, "Context"),
                               TLArg(NumViews, "NumViews"),
                               TLPArg(pDepthStencilView, "DSV"));
        if (IsTraceEnabled()) {
            for (UINT i = 0; i < NumViews; i++) {
                TraceLoggingWriteTagged(
                    local, "ID3D11DeviceContext_OMSetRenderTargets", TLPArg(ppRenderTargetViews[i], "RTV"));
            }
        }

        assert(original_ID3D11DeviceContext_OMSetRenderTargets);
        original_ID3D11DeviceContext_OMSetRenderTargets(Context, NumViews, ppRenderTargetViews, pDepthStencilView);

        // Invoke the hook after the state has been set on the command list.
        assert(g_InjectionManager);
        {
            // We re-assert the viewport, in case the app set a viewport first, followed by the render target.
            D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
            UINT numViewports = ARRAYSIZE(viewports);
            Context->RSGetViewports(&numViewports, viewports);
            if (numViewports) {
                g_InjectionManager->OnSetViewports(Context,
                                                   numViewports > 0 ? viewports[0] : D3D11_VIEWPORT{},
                                                   numViewports > 1 ? viewports[1] : D3D11_VIEWPORT{});
            }
        }

        TraceLoggingWriteStop(local, "ID3D11DeviceContext_OMSetRenderTargets");
    }

    DECLARE_DETOUR_FUNCTION(static void,
                            STDMETHODCALLTYPE,
                            ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews,
                            ID3D11DeviceContext* Context,
                            UINT NumRTVs,
                            ID3D11RenderTargetView* const* ppRenderTargetViews,
                            ID3D11DepthStencilView* pDepthStencilView,
                            UINT UAVStartSlot,
                            UINT NumUAVs,
                            ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
                            const UINT* pUAVInitialCounts) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local,
                               "ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews",
                               TLPArg(Context, "Context"),
                               TLArg(NumRTVs, "NumRTVs"),
                               TLPArg(pDepthStencilView, "DSV"));
        if (IsTraceEnabled() && NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL) {
            for (UINT i = 0; i < NumRTVs; i++) {
                TraceLoggingWriteTagged(local,
                                        "ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews",
                                        TLPArg(ppRenderTargetViews[i], "RTV"));
            }
        }

        assert(original_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews);
        original_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews(Context,
                                                                               NumRTVs,
                                                                               ppRenderTargetViews,
                                                                               pDepthStencilView,
                                                                               UAVStartSlot,
                                                                               NumUAVs,
                                                                               ppUnorderedAccessViews,
                                                                               pUAVInitialCounts);

        // Invoke the hook after the state has been set on the command list.
        assert(g_InjectionManager);
        if (NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL) {
            // We re-assert the viewport, in case the app set a viewport first, followed by the render target.
            D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
            UINT numViewports = 0;
            Context->RSGetViewports(&numViewports, viewports);
            if (numViewports) {
                g_InjectionManager->OnSetViewports(Context,
                                                   numViewports > 0 ? viewports[0] : D3D11_VIEWPORT{},
                                                   numViewports > 1 ? viewports[1] : D3D11_VIEWPORT{});
            }
        }

        TraceLoggingWriteStop(local, "ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews");
    }

} // namespace

namespace vrs {

    using namespace DetoursUtils;

    void InstallD3D11Hooks(ID3D11Device* device, const Resolution& presentResolution) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "InstallD3D11Hooks");

        const bool needHooks = !g_InjectionManager;
        try {
            g_InjectionManager = std::make_unique<VRSManagerD3D11>(device, presentResolution);
        } catch (FeatureNotSupported&) {
            return;
        }

        if (needHooks) {
            // Hook to the context's RSSetViewports(), where we will decide whether or not to inject VRS commands.
            ComPtr<ID3D11DeviceContext> context;
            device->GetImmediateContext(context.ReleaseAndGetAddressOf());

            TraceLoggingWriteTagged(local, "InstallD3D11Hooks_Detour_RSViewports", TLPArg(context.Get(), "Context"));
            DetourMethodAttach(context.Get(),
                               44, // RSSetViewports()
                               hooked_ID3D11DeviceContext_RSSetViewports,
                               original_ID3D11DeviceContext_RSSetViewports);
            TraceLoggingWriteTagged(
                local, "InstallD3D11Hooks_Detour_OMSetRenderTargets", TLPArg(context.Get(), "Context"));
            DetourMethodAttach(context.Get(),
                               33, // OMSetRenderTargets()
                               hooked_ID3D11DeviceContext_OMSetRenderTargets,
                               original_ID3D11DeviceContext_OMSetRenderTargets);
            DetourMethodAttach(context.Get(),
                               34, // OMSetRenderTargetsAndUnorderedAccessViews()
                               hooked_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews,
                               original_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews);
        }

        TraceLoggingWriteStop(local, "InstallD3D11Hooks");
    }

    void UninstallD3D11Hooks() {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "UninstallD3D11Hooks");

        if (g_InjectionManager) {
            ComPtr<ID3D11DeviceContext> context;
            g_InjectionManager->GetDevice()->GetImmediateContext(context.ReleaseAndGetAddressOf());

            TraceLoggingWriteTagged(local, "UninstallD3D11Hooks_Detour_RSViewports", TLPArg(context.Get(), "Context"));
            DetourMethodDetach(context.Get(),
                               44, // RSSetViewports()
                               hooked_ID3D11DeviceContext_RSSetViewports,
                               original_ID3D11DeviceContext_RSSetViewports);
            TraceLoggingWriteTagged(
                local, "UnistallD3D11Hooks_Detour_OMSetRenderTargets", TLPArg(context.Get(), "Context"));
            DetourMethodDetach(context.Get(),
                               33, // OMSetRenderTargets()
                               hooked_ID3D11DeviceContext_OMSetRenderTargets,
                               original_ID3D11DeviceContext_OMSetRenderTargets);
            DetourMethodDetach(context.Get(),
                               34, // OMSetRenderTargetsAndUnorderedAccessViews()
                               hooked_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews,
                               original_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews);

            g_InjectionManager.reset();
        }

        TraceLoggingWriteStop(local, "UninstallD3D11Hooks");
    }

    void SetStateD3D11(bool state, std::optional<Parameters> parameters) {
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

    void NewFrameD3D11() {
        if (g_InjectionManager) {
            g_InjectionManager->OnUpdate();
        }
    }

} // namespace vrs

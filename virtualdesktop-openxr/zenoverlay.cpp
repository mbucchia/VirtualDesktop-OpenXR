//
// Runtime-native ZenOverlay integration.
//
// Consumes the external desktop capture transport while
// keeping all overlay submission inside VDXR. It is not enabled
// by configuration; the producer's ready event is the opt-in signal.
//

#include "pch.h"

#include "log.h"
#include "runtime.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <d3d11_1.h>
#include <utility>
#include <vector>
#include <wrl/client.h>

namespace zenoverlay_shader_bytecode {
#include "FullScreenQuadVS.h"
#include "ZenOverlayKeyPS.h"
} // namespace zenoverlay_shader_bytecode

namespace virtualdesktop_openxr {
    struct ZenOverlaySettingsSnapshot {
        uint32_t magic, version, bytes, flags;
        uint64_t revision;
        int32_t mode;
        float x, y, z, pitch, yaw, roll, width, height;
        uint32_t preserveAspect;
    };
    static_assert(sizeof(ZenOverlaySettingsSnapshot) == 64);
    static_assert(offsetof(ZenOverlaySettingsSnapshot, mode) == 24);
    static_assert(offsetof(ZenOverlaySettingsSnapshot, preserveAspect) == 60);

    class ZenOverlaySettingsReader {
      public:
        ZenOverlaySettingsSnapshot current{};
        bool valid{false};
        bool visible{true};
        bool shortcutEnabled{false};
        bool shortcutArmed{false};
        ZenOverlaySettingsReader() = default;
        ZenOverlaySettingsReader(const ZenOverlaySettingsReader&) = delete;
        ZenOverlaySettingsReader& operator=(const ZenOverlaySettingsReader&) = delete;
        ~ZenOverlaySettingsReader() {
            if (view)
                UnmapViewOfFile(view);
            if (mapping)
                CloseHandle(mapping);
            if (mutex)
                CloseHandle(mutex);
            if (visibility)
                CloseHandle(visibility);
            if (shortcut)
                CloseHandle(shortcut);
        }
        bool poll(uint64_t now) {
            if (now >= nextProbe) {
                nextProbe = now + 500;
                if (!visibility)
                    visibility = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, L"Local\\ZenOverlayVisible_v1");
                if (!shortcut)
                    shortcut = OpenEventW(SYNCHRONIZE, FALSE, L"Local\\ZenOverlayFaceShortcutEnabled_v1");
                if (!mutex)
                    mutex = OpenMutexW(
                        SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, L"Local\\ZenOverlayRuntimeSettingsMutex_v3");
                if (!mapping)
                    mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, L"Local\\ZenOverlayRuntimeSettings_v3");
                if (mapping && !view)
                    view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 64);
            }
            if (visibility)
                visible = WaitForSingleObject(visibility, 0) == WAIT_OBJECT_0;
            shortcutEnabled = shortcut && WaitForSingleObject(shortcut, 0) == WAIT_OBJECT_0;
            if (!view || !mutex)
                return false;
            const DWORD result = WaitForSingleObject(mutex, 0);
            if (result == WAIT_ABANDONED) {
                ReleaseMutex(mutex);
                return false;
            }
            if (result != WAIT_OBJECT_0)
                return false;
            ZenOverlaySettingsSnapshot candidate{};
            std::memcpy(&candidate, view, sizeof(candidate));
            ReleaseMutex(mutex);
            if (candidate.magic != 0x5A525331u || candidate.version != 3 || candidate.bytes != 64 ||
                (candidate.flags & ~3u) || !candidate.revision || (valid && candidate.revision <= current.revision) ||
                candidate.mode < 0 || candidate.mode > 4 || candidate.preserveAspect > 1)
                return false;
            const float values[] = {candidate.x,
                                    candidate.y,
                                    candidate.z,
                                    candidate.pitch,
                                    candidate.yaw,
                                    candidate.roll,
                                    candidate.width,
                                    candidate.height};
            for (float value : values)
                if (!std::isfinite(value))
                    return false;
            if (std::fabs(candidate.x) > 20 || std::fabs(candidate.y) > 20 || std::fabs(candidate.z) > 20 ||
                std::fabs(candidate.pitch) > 360 || std::fabs(candidate.yaw) > 360 || std::fabs(candidate.roll) > 360 ||
                candidate.width < .02f || candidate.width > 10 || candidate.height < .02f || candidate.height > 10)
                return false;
            current = candidate;
            valid = true;
            return true;
        }
        bool updateShortcut(ovrSession session, bool focused) {
            if (!shortcutEnabled || !visibility || !focused) {
                shortcutArmed = false;
                return false;
            }
            ovrInputState input{};
            if (OVR_FAILURE(ovr_GetInputState(session, ovrControllerType_Touch, &input)) ||
                (input.ControllerType & ovrControllerType_Touch) != ovrControllerType_Touch) {
                shortcutArmed = false;
                return false;
            }
            constexpr unsigned mask = ovrButton_A | ovrButton_B | ovrButton_X | ovrButton_Y;
            const unsigned pressed = input.Buttons & mask;
            if (!pressed)
                shortcutArmed = true;
            if (pressed != mask || !shortcutArmed)
                return false;
            shortcutArmed = false; // All four must be released before rearming.
            const DWORD state = WaitForSingleObject(visibility, 0);
            if (state != WAIT_OBJECT_0 && state != WAIT_TIMEOUT)
                return false;
            const bool wasVisible = state == WAIT_OBJECT_0;
            if (!(wasVisible ? ResetEvent(visibility) : SetEvent(visibility)))
                return false;
            visible = !wasVisible;
            return true;
        }

      private:
        HANDLE mapping{nullptr}, mutex{nullptr}, visibility{nullptr}, shortcut{nullptr};
        void* view{nullptr};
        uint64_t nextProbe{0};
    };

    // Private completed-frame snapshot: toggles can reprocess a static window
    // without touching the producer's shared texture or blocking on its mutex.
    class ZenOverlayTransparency {
        template <typename T>
        using Ptr = Microsoft::WRL::ComPtr<T>;
        Ptr<ID3D11ShaderResourceView> source;
        Ptr<ID3D11VertexShader> vertex;
        Ptr<ID3D11PixelShader> pixel;
        std::vector<std::pair<Ptr<ID3D11Texture2D>, Ptr<ID3D11RenderTargetView>>> targets;
        UINT width{0}, height{0};

      public:
        Ptr<ID3D11Texture2D> snapshot;
        bool lastEnabled{false};

        bool initialize(ID3D11Device1* device, const D3D11_TEXTURE2D_DESC& input) {
            width = input.Width;
            height = input.Height;
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            return SUCCEEDED(device->CreateTexture2D(&desc, nullptr, snapshot.ReleaseAndGetAddressOf())) &&
                   SUCCEEDED(
                       device->CreateShaderResourceView(snapshot.Get(), nullptr, source.ReleaseAndGetAddressOf())) &&
                   SUCCEEDED(device->CreateVertexShader(zenoverlay_shader_bytecode::g_FullScreenQuadVS,
                                                        sizeof(zenoverlay_shader_bytecode::g_FullScreenQuadVS),
                                                        nullptr,
                                                        vertex.ReleaseAndGetAddressOf())) &&
                   SUCCEEDED(device->CreatePixelShader(zenoverlay_shader_bytecode::g_ZenOverlayKeyPS,
                                                       sizeof(zenoverlay_shader_bytecode::g_ZenOverlayKeyPS),
                                                       nullptr,
                                                       pixel.ReleaseAndGetAddressOf()));
        }

        bool render(ID3D11Device1* device, ID3D11DeviceContext1* context, ID3D11Texture2D* destination, bool enabled) {
            if (!enabled) {
                context->CopyResource(destination, snapshot.Get());
                return true;
            }
            ID3D11RenderTargetView* target = nullptr;
            for (const auto& entry : targets) {
                if (entry.first.Get() == destination) {
                    target = entry.second.Get();
                    break;
                }
            }
            if (!target) {
                D3D11_RENDER_TARGET_VIEW_DESC desc{};
                desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                Ptr<ID3D11RenderTargetView> view;
                if (FAILED(device->CreateRenderTargetView(destination, &desc, view.GetAddressOf())))
                    return false;
                target = view.Get();
                Ptr<ID3D11Texture2D> texture = destination;
                targets.emplace_back(std::move(texture), std::move(view));
            }
            // The caller saves application state through xrEndFrame's existing guard.
            // Establish a clean draw state on the runtime submission context.
            context->ClearState();
            const D3D11_VIEWPORT viewport{0, 0, float(width), float(height), 0, 1};
            context->RSSetViewports(1, &viewport);
            context->OMSetRenderTargets(1, &target, nullptr);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->VSSetShader(vertex.Get(), nullptr, 0);
            context->PSSetShader(pixel.Get(), nullptr, 0);
            ID3D11ShaderResourceView* srv = source.Get();
            context->PSSetShaderResources(0, 1, &srv);
            context->Draw(3, 0);
            context->ClearState();
            return true;
        }
    };

    // Head and result are in LOCAL space. Use a body-yaw approximation.
    static bool zenOverlayPlayerPose(const XrPosef& head, const XrPosef& offset, XrPosef& result) {
        const auto& q = head.orientation;
        const float forwardX = -2.f * (q.x * q.z + q.w * q.y);
        const float forwardZ = -(1.f - 2.f * (q.x * q.x + q.y * q.y));
        if (std::hypot(forwardX, forwardZ) < .0001f)
            return false;
        const float yaw = std::atan2(-forwardX, -forwardZ);
        const float sy = std::sin(yaw), cy = std::cos(yaw);
        result.position = {head.position.x + cy * offset.position.x + sy * offset.position.z,
                           head.position.y + offset.position.y,
                           head.position.z - sy * offset.position.x + cy * offset.position.z};
        const float s = std::sin(yaw * .5f), c = std::cos(yaw * .5f);
        const auto& r = offset.orientation;
        result.orientation = {c * r.x + s * r.z, c * r.y + s * r.w, c * r.z - s * r.x, c * r.w - s * r.y};
        return true;
    }

    // Per-frame wrist position and orientation smoothing.
    static bool zenOverlaySmoothWrist(const XrPosef& target, XrPosef& history, bool& valid) {
        const auto& q = target.orientation;
        const float values[] = {target.position.x, target.position.y, target.position.z, q.x, q.y, q.z, q.w};
        for (float value : values) {
            if (!std::isfinite(value)) {
                valid = false;
                return false;
            }
        }
        const float length = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (length < .000001f || !std::isfinite(length)) {
            valid = false;
            return false;
        }
        XrQuaternionf current{q.x / length, q.y / length, q.z / length, q.w / length};
        if (!valid) {
            history = target;
            history.orientation = current;
            valid = true;
            return true;
        }
        history.position.x += (target.position.x - history.position.x) * .16f;
        history.position.y += (target.position.y - history.position.y) * .16f;
        history.position.z += (target.position.z - history.position.z) * .16f;
        auto& previous = history.orientation;
        if (previous.x * current.x + previous.y * current.y + previous.z * current.z + previous.w * current.w < 0) {
            current = {-current.x, -current.y, -current.z, -current.w};
        }
        previous = {previous.x + (current.x - previous.x) * .14f,
                    previous.y + (current.y - previous.y) * .14f,
                    previous.z + (current.z - previous.z) * .14f,
                    previous.w + (current.w - previous.w) * .14f};
        const float n = std::sqrt(previous.x * previous.x + previous.y * previous.y + previous.z * previous.z +
                                  previous.w * previous.w);
        previous = {previous.x / n, previous.y / n, previous.z / n, previous.w / n};
        return true;
    }

    // Pose is relative to VIEW. Use gaze thresholds including a
    // two-sided wrist facing test. Hysteresis avoids boundary flicker.
    static bool zenOverlayGazeVisible(const XrPosef& pose, bool wrist, bool wasVisible) {
        const auto& p = pose.position;
        const float distanceSquared = p.x * p.x + p.y * p.y + p.z * p.z;
        if (!std::isfinite(distanceSquared) || distanceSquared < 1e-6f) {
            return false;
        }
        const float inverseDistance = 1.f / std::sqrt(distanceSquared);
        const float gaze = -p.z * inverseDistance;
        if (!wrist) {
            return gaze >= (wasVisible ? .88f : .94f);
        }
        const auto& q = pose.orientation;
        const float nx = 2.f * (q.x * q.z + q.w * q.y);
        const float ny = 2.f * (q.y * q.z - q.w * q.x);
        const float nz = 1.f - 2.f * (q.x * q.x + q.y * q.y);
        const float facing = std::abs((nx * p.x + ny * p.y + nz * p.z) * inverseDistance);
        return std::isfinite(facing) && gaze >= (wasVisible ? .76f : .82f) && facing >= (wasVisible ? .40f : .55f);
    }

    class ZenOverlayHelperImpl : public ZenOverlayHelper {
      public:
        explicit ZenOverlayHelperImpl(OpenXrRuntime& runtime) : m_runtime(runtime) {
        }
        bool AppendLayer(std::vector<ovrLayer_Union>& layers) override;
        void Cleanup() override;

      private:
        OpenXrRuntime& m_runtime;
        ZenOverlaySettingsReader m_zenOverlaySettings;
        XrPosef m_zenOverlayWristPose{};
        bool m_zenOverlayWristPoseValid{false};
        bool m_zenOverlayGazeVisible{false};
        ZenOverlayTransparency m_zenOverlayTransparency;
        wil::unique_handle m_zenOverlayReadyEvent;
        wil::unique_handle m_zenOverlayProducerProcess;
        ComPtr<ID3D11Texture2D> m_zenOverlaySharedTexture;
        ComPtr<IDXGIKeyedMutex> m_zenOverlaySharedMutex;
        ovrTextureSwapChain m_zenOverlaySwapchain{nullptr};
        uint32_t m_zenOverlayWidth{0};
        uint32_t m_zenOverlayHeight{0};
        bool m_zenOverlayHasCommittedFrame{false};
        uint64_t m_zenOverlayNextProbeTick{0};
    };
    std::unique_ptr<ZenOverlayHelper> CreateZenOverlayHelper(OpenXrRuntime& runtime) {
        return std::make_unique<ZenOverlayHelperImpl>(runtime);
    }

    void OpenXrRuntime::cleanupZenOverlay() {
        if (m_zenOverlay)
            m_zenOverlay->Cleanup();
    }

    bool OpenXrRuntime::appendZenOverlayLayer(std::vector<ovrLayer_Union>& layers) {
        if (m_isHeadless || !m_ovrSession || !m_ovrSubmissionDevice || !m_ovrSubmissionContext ||
            layers.size() >= ovrMaxLayerCount)
            return false;
        if (!m_zenOverlay)
            m_zenOverlay = CreateZenOverlayHelper(*this);
        return m_zenOverlay->AppendLayer(layers);
    }

    using namespace virtualdesktop_openxr::log;

    namespace {
        constexpr wchar_t CaptureReadyEventName[] = L"Local\\ZenOverlayCaptureReady_v1";
        constexpr wchar_t SharedCaptureTextureName[] = L"Local\\ZenOverlayCaptureTexture_v1";

        // Read only the fixed v2 status header at connection time. Retain a
        // kernel process handle, not a PID, while consuming this producer.
        static HANDLE zenOverlayOpenCaptureProcess(const wchar_t* statusName) {
            HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, statusName);
            if (!mapping)
                return nullptr;
            const void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 16);
            uint32_t header[4]{};
            if (view) {
                std::memcpy(header, view, sizeof(header));
                UnmapViewOfFile(view);
            }
            CloseHandle(mapping);
            if (header[0] != 0x5A454E31u || header[1] != 2 || !header[3])
                return nullptr;
            HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, header[3]);
            if (process && WaitForSingleObject(process, 0) != WAIT_TIMEOUT) {
                CloseHandle(process);
                process = nullptr;
            }
            return process;
        }
    } // namespace

    void ZenOverlayHelperImpl::Cleanup() {
        m_zenOverlaySettings.shortcutArmed = false;
        m_zenOverlayTransparency = {};
        if (m_zenOverlaySwapchain && m_runtime.m_ovrSession) {
            ovr_DestroyTextureSwapChain(m_runtime.m_ovrSession, m_zenOverlaySwapchain);
        }
        m_zenOverlaySwapchain = nullptr;
        m_zenOverlaySharedMutex.Reset();
        m_zenOverlaySharedTexture.Reset();
        m_zenOverlayProducerProcess.reset();
        m_zenOverlayWidth = 0;
        m_zenOverlayHeight = 0;
        m_zenOverlayHasCommittedFrame = false;
        m_zenOverlayWristPoseValid = false;
        m_zenOverlayGazeVisible = false;
    }

    bool ZenOverlayHelperImpl::AppendLayer(std::vector<ovrLayer_Union>& layers) {
        if (m_runtime.m_isHeadless || !m_runtime.m_ovrSession || !m_runtime.m_ovrSubmissionDevice ||
            !m_runtime.m_ovrSubmissionContext || layers.size() >= ovrMaxLayerCount) {
            return false;
        }
        const uint64_t now = GetTickCount64();
        const auto previousSettings = m_zenOverlaySettings.current;
        if (m_zenOverlaySettings.poll(now)) {
            m_zenOverlayWristPoseValid = false;
            const auto& s = m_zenOverlaySettings.current;
            if (s.mode != previousSettings.mode || s.flags != previousSettings.flags) {
                m_zenOverlayGazeVisible = false;
            }
            Log("ZenOverlay settings revision=%llu mode=%d position=(%.3f,%.3f,%.3f) width=%.3f\n",
                static_cast<unsigned long long>(s.revision),
                s.mode,
                s.x,
                s.y,
                s.z,
                s.width);
        }

        if (!m_zenOverlayReadyEvent) {
            if (now < m_zenOverlayNextProbeTick) {
                return false;
            }
            m_zenOverlayReadyEvent.reset(CreateEventW(nullptr, TRUE, FALSE, CaptureReadyEventName));
            if (!m_zenOverlayReadyEvent) {
                // Retry only creation failures. The event otherwise survives
                // producer exits, so idle frames need only a zero-time wait.
                m_zenOverlayNextProbeTick = now + 500;
                return false;
            }
        }

        const bool producerReady =
            m_zenOverlayReadyEvent && WaitForSingleObject(m_zenOverlayReadyEvent.get(), 0) == WAIT_OBJECT_0;

        // A retained event stays signaled after a crash. Conversely, reopening
        // our own retained texture cannot establish that its producer is alive.
        if (!producerReady || (m_zenOverlayProducerProcess &&
                               WaitForSingleObject(m_zenOverlayProducerProcess.get(), 0) != WAIT_TIMEOUT)) {
            Cleanup();
            m_zenOverlayNextProbeTick = 0;
            return false;
        }

        if (!m_zenOverlaySharedTexture || !m_zenOverlaySwapchain) {
            if (!producerReady) {
                return false;
            }
            if (now < m_zenOverlayNextProbeTick) {
                return false;
            }
            Cleanup();

            m_zenOverlayProducerProcess.reset(zenOverlayOpenCaptureProcess(L"Local\\ZenOverlayCaptureStatus_v1"));
            if (!m_zenOverlayProducerProcess) {
                m_zenOverlayNextProbeTick = now + 500;
                return false;
            }

            ComPtr<ID3D11Device1> device1;
            if (FAILED(m_runtime.m_ovrSubmissionDevice.As(&device1)) || !device1) {
                return false;
            }

            ComPtr<ID3D11Texture2D> sharedTexture;
            const HRESULT openResult =
                device1->OpenSharedResourceByName(SharedCaptureTextureName,
                                                  DXGI_SHARED_RESOURCE_READ,
                                                  IID_PPV_ARGS(sharedTexture.ReleaseAndGetAddressOf()));
            if (FAILED(openResult) || !sharedTexture) {
                m_zenOverlayNextProbeTick = now + 100;
                return false;
            }

            D3D11_TEXTURE2D_DESC sourceDesc{};
            sharedTexture->GetDesc(&sourceDesc);
            if (!sourceDesc.Width || !sourceDesc.Height || sourceDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM ||
                sourceDesc.SampleDesc.Count != 1) {
                m_zenOverlayNextProbeTick = now + 500;
                return false;
            }

            ComPtr<IDXGIKeyedMutex> sharedMutex;
            if (FAILED(sharedTexture.As(&sharedMutex)) || !sharedMutex) {
                m_zenOverlayNextProbeTick = now + 500;
                return false;
            }

            if (!m_zenOverlayTransparency.initialize(device1.Get(), sourceDesc)) {
                Log("ZenOverlay transparency resource initialization failed\n");
                Cleanup();
                m_zenOverlayNextProbeTick = now + 500;
                return false;
            }

            ovrTextureSwapChainDesc swapchainDesc{};
            swapchainDesc.Type = ovrTexture_2D;
            // Windows desktop capture is authored in sRGB. Keep the shared
            // transport as BGRA8 UNORM, but expose the compositor-facing OVR
            // swapchain as sRGB so its transfer function matches the desktop.
            swapchainDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
            swapchainDesc.ArraySize = 1;
            swapchainDesc.Width = static_cast<int>(sourceDesc.Width);
            swapchainDesc.Height = static_cast<int>(sourceDesc.Height);
            swapchainDesc.MipLevels = 1;
            swapchainDesc.SampleCount = 1;
            swapchainDesc.StaticImage = ovrFalse;
            // A typeless backing texture allows CopyResource from the UNORM
            // shared transport into the sRGB compositor view.
            swapchainDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
            swapchainDesc.BindFlags = ovrTextureBind_DX_RenderTarget;

            ovrTextureSwapChain swapchain = nullptr;
            const ovrResult createResult = ovr_CreateTextureSwapChainDX(
                m_runtime.m_ovrSession, m_runtime.m_ovrSubmissionDevice.Get(), &swapchainDesc, &swapchain);
            if (OVR_FAILURE(createResult) || !swapchain) {
                m_zenOverlayNextProbeTick = now + 500;
                return false;
            }

            m_zenOverlaySharedTexture = std::move(sharedTexture);
            m_zenOverlaySharedMutex = std::move(sharedMutex);
            m_zenOverlaySwapchain = swapchain;
            m_zenOverlayWidth = sourceDesc.Width;
            m_zenOverlayHeight = sourceDesc.Height;
            m_zenOverlayNextProbeTick = 0;

            Log("ZenOverlay runtime texture opened: %ux%u\n", m_zenOverlayWidth, m_zenOverlayHeight);
        }

        if (m_zenOverlaySettings.updateShortcut(m_runtime.m_ovrSession,
                                                m_runtime.m_sessionState == XR_SESSION_STATE_FOCUSED)) {
            Log("A+B+X+Y runtime shortcut: overlay %s\n", m_zenOverlaySettings.visible ? "shown" : "hidden");
        }

        int destinationIndex = -1;
        if (OVR_FAILURE(ovr_GetTextureSwapChainCurrentIndex(
                m_runtime.m_ovrSession, m_zenOverlaySwapchain, &destinationIndex))) {
            Cleanup();
            return false;
        }

        ComPtr<ID3D11Texture2D> destination;
        if (OVR_FAILURE(ovr_GetTextureSwapChainBufferDX(m_runtime.m_ovrSession,
                                                        m_zenOverlaySwapchain,
                                                        destinationIndex,
                                                        IID_PPV_ARGS(destination.ReleaseAndGetAddressOf()))) ||
            !destination) {
            Cleanup();
            return false;
        }

        // A timeout is nonnegative but does not grant ownership. Keep the last
        // committed frame until the producer has released a completed image.
        const HRESULT acquireResult = m_zenOverlaySharedMutex->AcquireSync(1, 0);
        bool newFrame = false;
        if (acquireResult == S_OK) {
            m_runtime.m_ovrSubmissionContext->CopyResource(m_zenOverlayTransparency.snapshot.Get(),
                                                           m_zenOverlaySharedTexture.Get());
            if (m_zenOverlaySharedMutex->ReleaseSync(0) != S_OK) {
                Log("ZenOverlay runtime shared mutex release failed\n");
                Cleanup();
                return false;
            }
            newFrame = true;
        } else if (acquireResult != WAIT_TIMEOUT) {
            Log("ZenOverlay runtime shared mutex acquisition failed: 0x%08x\n", unsigned(acquireResult));
            Cleanup();
            m_zenOverlayNextProbeTick = now + 500;
            return false;
        } else if (!m_zenOverlayHasCommittedFrame) {
            return false;
        }

        const bool keyEnabled = m_zenOverlaySettings.valid && (m_zenOverlaySettings.current.flags & 2u);
        if (newFrame || keyEnabled != m_zenOverlayTransparency.lastEnabled) {
            // Match resolveSwapchainImage: save the application context only once.
            // xrEndFrame's scope guard restores it, including on early exits.
            if (keyEnabled && m_runtime.m_d3d11Device == m_runtime.m_ovrSubmissionDevice &&
                !m_runtime.m_d3d11ContextState) {
                m_runtime.m_ovrSubmissionContext->SwapDeviceContextState(
                    m_runtime.m_ovrSubmissionContextState.Get(),
                    m_runtime.m_d3d11ContextState.ReleaseAndGetAddressOf());
            }
            if (!m_zenOverlayTransparency.render(m_runtime.m_ovrSubmissionDevice.Get(),
                                                 m_runtime.m_ovrSubmissionContext.Get(),
                                                 destination.Get(),
                                                 keyEnabled) ||
                OVR_FAILURE(ovr_CommitTextureSwapChain(m_runtime.m_ovrSession, m_zenOverlaySwapchain))) {
                Log("ZenOverlay transparency presentation failed\n");
                Cleanup();
                m_zenOverlayNextProbeTick = now + 500;
                return false;
            }
            m_zenOverlayHasCommittedFrame = true;
            m_zenOverlayTransparency.lastEnabled = keyEnabled;
        }

        // Continue consuming completed capture frames while hidden, so the
        // producer can keep updating and Show never exposes an obsolete frame.
        if (!m_zenOverlaySettings.visible) {
            m_zenOverlayWristPoseValid = false;
            m_zenOverlayGazeVisible = false;
            return false;
        }

        layers.push_back({});
        auto& layer = layers.back().Quad;
        layer.Header.Type = ovrLayerType_Quad;
        layer.Header.Flags = ovrLayerFlag_HeadLocked | ovrLayerFlag_HighQuality;
        layer.ColorTexture = m_zenOverlaySwapchain;
        layer.Viewport.Pos = {0, 0};
        layer.Viewport.Size = {static_cast<int>(m_zenOverlayWidth), static_cast<int>(m_zenOverlayHeight)};
        layer.QuadPoseCenter.Orientation = {0.f, 0.f, 0.f, 1.f};
        layer.QuadPoseCenter.Position = {0.f, 0.f, -1.25f};
        layer.QuadSize.x = 0.8f;
        layer.QuadSize.y =
            layer.QuadSize.x * static_cast<float>(m_zenOverlayHeight) / static_cast<float>(m_zenOverlayWidth);
        if (m_zenOverlaySettings.valid) {
            const auto& s = m_zenOverlaySettings.current;
            constexpr float halfRadians = 3.14159265358979323846f / 360.f;
            const float sx = std::sin(s.pitch * halfRadians), cx = std::cos(s.pitch * halfRadians);
            const float sy = std::sin(s.yaw * halfRadians), cy = std::cos(s.yaw * halfRadians);
            const float sz = std::sin(s.roll * halfRadians), cz = std::cos(s.roll * halfRadians);
            // Apply qRoll * (qYaw * qPitch).
            layer.QuadPoseCenter.Orientation = {cz * cy * sx - sz * sy * cx,
                                                cz * sy * cx + sz * cy * sx,
                                                sz * cy * cx - cz * sy * sx,
                                                cz * cy * cx + sz * sy * sx};
            layer.QuadPoseCenter.Position = {s.x, s.y, s.z};
            layer.QuadSize.x = s.width;
            layer.QuadSize.y = s.preserveAspect ? s.width * static_cast<float>(m_zenOverlayHeight) /
                                                      static_cast<float>(m_zenOverlayWidth)
                                                : s.height;
            if (s.mode != 2) {
                OpenXrRuntime::Space localSpace{};
                localSpace.referenceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
                localSpace.poseInSpace = xr::math::Pose::Identity();
                XrPosef localToOrigin{};
                constexpr XrSpaceLocationFlags required =
                    XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
                const XrTime time = m_runtime.m_precompositor.displayTime;
                if (!m_runtime.m_originSpace || !time ||
                    (m_runtime.locateSpace(localSpace, *m_runtime.m_originSpace, time, localToOrigin) & required) !=
                        required) {
                    layers.pop_back();
                    m_zenOverlayWristPoseValid = false;
                    m_zenOverlayGazeVisible = false;
                    return false;
                }
                XrPosef pose = ovrPoseToXrPose(layer.QuadPoseCenter);
                XrPosef gazePose = pose;
                if (s.mode == 3) {
                    XrPosef headInLocal{}, playerPose{};
                    if (!m_runtime.m_viewSpace ||
                        (m_runtime.locateSpace(*m_runtime.m_viewSpace, localSpace, time, headInLocal) & required) !=
                            required ||
                        !zenOverlayPlayerPose(headInLocal, pose, playerPose)) {
                        m_zenOverlayGazeVisible = false;
                        layers.pop_back();
                        return false;
                    }
                    pose = playerPose;
                    gazePose = pose;
                } else if (s.mode == 0 || s.mode == 4) {
                    const int side = s.mode == 4 ? 1 : 0;
                    XrPosef controllerPose{};
                    constexpr XrSpaceLocationFlags tracked =
                        required | XR_SPACE_LOCATION_POSITION_TRACKED_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
                    if ((m_runtime.getControllerPose(side, time, controllerPose, nullptr) & tracked) != tracked) {
                        m_zenOverlayWristPoseValid = false;
                        m_zenOverlayGazeVisible = false;
                        layers.pop_back();
                        return false;
                    }
                    const XrPosef gripInOrigin =
                        xr::math::Pose::Multiply(m_runtime.m_controllerGripPose[side], controllerPose);
                    const XrPosef gripInLocal =
                        xr::math::Pose::Multiply(gripInOrigin, xr::math::Pose::Invert(localToOrigin));
                    const XrPosef wristInLocal = xr::math::Pose::Multiply(pose, gripInLocal);
                    gazePose = wristInLocal;
                    if (!zenOverlaySmoothWrist(wristInLocal, m_zenOverlayWristPose, m_zenOverlayWristPoseValid)) {
                        layers.pop_back();
                        return false;
                    }
                    pose = m_zenOverlayWristPose;
                }
                // Visibility never pauses shared-texture consumption. Use the
                // unsmoothed wrist pose for the gaze decision.
                if (s.flags & 1u) {
                    XrPosef headInLocal{};
                    if (!m_runtime.m_viewSpace ||
                        (m_runtime.locateSpace(*m_runtime.m_viewSpace, localSpace, time, headInLocal) & required) !=
                            required) {
                        m_zenOverlayGazeVisible = false;
                    } else {
                        const XrPosef inView = xr::math::Pose::Multiply(gazePose, xr::math::Pose::Invert(headInLocal));
                        m_zenOverlayGazeVisible =
                            zenOverlayGazeVisible(inView, s.mode == 0 || s.mode == 4, m_zenOverlayGazeVisible);
                    }
                    if (!m_zenOverlayGazeVisible) {
                        layers.pop_back();
                        return false;
                    }
                } else {
                    m_zenOverlayGazeVisible = false;
                }
                // Use the same LOCAL-to-OVR conversion as application quad layers.
                layer.QuadPoseCenter = xrPoseToOvrPose(xr::math::Pose::Multiply(pose, localToOrigin));
                layer.Header.Flags &= ~ovrLayerFlag_HeadLocked;
            }
        }
        return true;
    }

} // namespace virtualdesktop_openxr

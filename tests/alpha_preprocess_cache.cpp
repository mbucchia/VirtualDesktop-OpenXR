// Runtime-linked preprocessing cache regressions. No settings writes or headset required.
#include "pch.h"
#include "runtime.h"
#include <array>

namespace {
    void require(bool condition, const char* message) {
        if (!condition)
            throw std::runtime_error(message);
    }
    void checkHr(HRESULT result) {
        require(SUCCEEDED(result), "D3D operation failed");
    }

    struct CreationCounts {
        unsigned shaders{};
        unsigned buffers{};
        unsigned failShaders{};
        unsigned failBuffers{};
    };
    using CreateShader =
        HRESULT(STDMETHODCALLTYPE*)(ID3D11Device*, const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11ComputeShader**);
    using CreateBuffer = HRESULT(STDMETHODCALLTYPE*)(ID3D11Device*,
                                                     const D3D11_BUFFER_DESC*,
                                                     const D3D11_SUBRESOURCE_DATA*,
                                                     ID3D11Buffer**);
    CreateShader originalShader{};
    CreateBuffer originalBuffer{};
    ID3D11Device* watchedDevice{};
    CreationCounts* watchedCounts{};

    HRESULT STDMETHODCALLTYPE createShader(ID3D11Device* device,
                                           const void* code,
                                           SIZE_T size,
                                           ID3D11ClassLinkage* linkage,
                                           ID3D11ComputeShader** output) {
        if (device == watchedDevice) {
            ++watchedCounts->shaders;
            if (watchedCounts->failShaders) {
                --watchedCounts->failShaders;
                if (output)
                    *output = nullptr;
                return E_OUTOFMEMORY;
            }
        }
        return originalShader(device, code, size, linkage, output);
    }
    HRESULT STDMETHODCALLTYPE createBuffer(ID3D11Device* device,
                                           const D3D11_BUFFER_DESC* desc,
                                           const D3D11_SUBRESOURCE_DATA* initial,
                                           ID3D11Buffer** output) {
        if (device == watchedDevice) {
            ++watchedCounts->buffers;
            if (watchedCounts->failBuffers) {
                --watchedCounts->failBuffers;
                if (output)
                    *output = nullptr;
                return E_OUTOFMEMORY;
            }
        }
        return originalBuffer(device, desc, initial, output);
    }

    class CreationHooks {
      public:
        CreationCounts counts;
        explicit CreationHooks(ID3D11Device* device) : m_device(device) {
            require(!watchedCounts, "Creation hooks already installed");
            // ID3D11Device inherits IUnknown: CreateBuffer is slot 3, CreateComputeShader is slot 18.
            auto** table = *reinterpret_cast<void***>(device);
            originalBuffer = reinterpret_cast<CreateBuffer>(table[3]);
            originalShader = reinterpret_cast<CreateShader>(table[18]);
            LONG result = DetourTransactionBegin();
            const bool transaction = result == NO_ERROR;
            if (result == NO_ERROR)
                result = DetourUpdateThread(GetCurrentThread());
            if (result == NO_ERROR)
                result = DetourAttach(reinterpret_cast<PVOID*>(&originalShader), createShader);
            if (result == NO_ERROR)
                result = DetourAttach(reinterpret_cast<PVOID*>(&originalBuffer), createBuffer);
            watchedDevice = device;
            watchedCounts = &counts;
            if (result == NO_ERROR)
                result = DetourTransactionCommit();
            else if (transaction)
                DetourTransactionAbort();
            if (result != NO_ERROR) {
                watchedDevice = nullptr;
                watchedCounts = nullptr;
                throw std::runtime_error("Creation hook installation failed");
            }
        }
        ~CreationHooks() {
            LONG result = DetourTransactionBegin();
            const bool transaction = result == NO_ERROR;
            if (result == NO_ERROR)
                result = DetourUpdateThread(GetCurrentThread());
            if (result == NO_ERROR)
                result = DetourDetach(reinterpret_cast<PVOID*>(&originalShader), createShader);
            if (result == NO_ERROR)
                result = DetourDetach(reinterpret_cast<PVOID*>(&originalBuffer), createBuffer);
            if (result == NO_ERROR)
                result = DetourTransactionCommit();
            else if (transaction)
                DetourTransactionAbort();
            if (result != NO_ERROR) {
                std::cerr << "FAIL: Creation hook removal failed: " << result << '\n';
                std::terminate(); // Do not release the native device with a live test hook.
            }
            watchedDevice = nullptr;
            watchedCounts = nullptr;
        }
        CreationHooks(const CreationHooks&) = delete;
        CreationHooks& operator=(const CreationHooks&) = delete;

      private:
        ComPtr<ID3D11Device> m_device; // Detach before the native implementation can be unloaded.
    };

    void checkCounts(const CreationCounts& counts, unsigned shaders, unsigned buffers) {
        std::cout << "native creations: shader=" << counts.shaders << " buffer=" << counts.buffers
                  << " expected=" << shaders << ',' << buffers << '\n';
        require(counts.shaders == shaders && counts.buffers == buffers,
                "Unexpected preprocessing resource creation count");
    }

    constexpr unsigned side = 64;
    using Pixel = std::array<uint8_t, 4>;
    constexpr Pixel sourcePixel{153, 102, 51, 128};
    constexpr Pixel clearPixel{153, 102, 51, 255};
    constexpr Pixel linearPixel{77, 51, 26, 128};
    constexpr Pixel gammaPixel{112, 73, 35, 128};
    constexpr XrCompositionLayerFlags premultiply =
        XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
    struct Region {
        XrRect2Di rect;
        Pixel pixel;
        int tolerance{};
    };
    ComPtr<IUnknown> identity(IUnknown* object) {
        ComPtr<IUnknown> result;
        checkHr(object->QueryInterface(IID_PPV_ARGS(&result)));
        return result;
    }
    ComPtr<IUnknown> ownerIdentity(ID3D11DeviceChild* resource) {
        ComPtr<ID3D11Device> device;
        resource->GetDevice(&device);
        return identity(device.Get());
    }
} // namespace

namespace virtualdesktop_openxr {
    struct AlphaPreprocessCacheRegression {
        struct Surface {
            OpenXrRuntime::Swapchain chain{};
            std::vector<Region> regions;
        };

        static void installDevice(OpenXrRuntime& runtime,
                                  ComPtr<ID3D11Device>& device,
                                  ComPtr<ID3D11DeviceContext>& context) {
            checkHr(D3D11CreateDevice(nullptr,
                                      D3D_DRIVER_TYPE_HARDWARE,
                                      nullptr,
                                      0,
                                      nullptr,
                                      0,
                                      D3D11_SDK_VERSION,
                                      &device,
                                      nullptr,
                                      &context));
            checkHr(device.As(&runtime.m_ovrSubmissionDevice));
            checkHr(context.As(&runtime.m_ovrSubmissionContext));
            checkHr(runtime.m_ovrSubmissionDevice->CreateFence(
                0, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&runtime.m_ovrSubmissionFence)));
        }

        static void initializeSurface(Surface& surface, ID3D11Device* device) {
            auto& chain = surface.chain;
            chain.dirty = true;
            chain.xrDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            chain.dxgiFormatForSubmission = DXGI_FORMAT_R8G8B8A8_UNORM;
            chain.resolvedSlices.resize(1);
            chain.resolvedSlices[0].lastCommittedIndex = 0;
            chain.resolvedSlices[0].images.resize(1);
            std::vector<Pixel> pixels(side * side, sourcePixel);
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = desc.Height = side;
            desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
            desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
            D3D11_SUBRESOURCE_DATA initial{pixels.data(), side * 4, 0};
            checkHr(device->CreateTexture2D(&desc, &initial, &chain.resolvedSlices[0].images[0]));
        }

        static void checkPixels(const Surface& surface, ID3D11Device* device, ID3D11DeviceContext* context) {
            D3D11_TEXTURE2D_DESC desc{};
            surface.chain.resolvedSlices[0].images[0]->GetDesc(&desc);
            desc.BindFlags = 0;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Texture2D> staging;
            checkHr(device->CreateTexture2D(&desc, nullptr, &staging));
            context->CopyResource(staging.Get(), surface.chain.resolvedSlices[0].images[0].Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            checkHr(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
            unsigned wrong = 0;
            for (unsigned y = 0; y < side; ++y)
                for (unsigned x = 0; x < side; ++x) {
                    Pixel expected = sourcePixel;
                    int tolerance = 0;
                    for (const auto& region : surface.regions) {
                        const auto& rect = region.rect;
                        if (int(x) >= rect.offset.x && int(x) < rect.offset.x + rect.extent.width &&
                            int(y) >= rect.offset.y && int(y) < rect.offset.y + rect.extent.height) {
                            expected = region.pixel;
                            tolerance = region.tolerance;
                        }
                    }
                    const auto* pixel = static_cast<const uint8_t*>(mapped.pData) + y * mapped.RowPitch + x * 4;
                    for (unsigned c = 0; c < 4; ++c)
                        if (std::abs(int(pixel[c]) - expected[c]) > (c < 3 ? tolerance : 0))
                            ++wrong;
                }
            context->Unmap(staging.Get(), 0);
            checkHr(device->GetDeviceRemovedReason());
            if (wrong)
                std::cerr << "FAIL: rendered pixel channels=" << wrong << '\n';
            require(!wrong, "Preprocessing used stale constants or changed pixels outside its viewport");
        }

        static void process(OpenXrRuntime& runtime,
                            Surface& surface,
                            const Region& region,
                            XrCompositionLayerFlags flags,
                            bool gamma = false) {
            surface.chain.dxgiFormatForSubmission =
                gamma ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
            surface.chain.xrDesc.format = surface.chain.dxgiFormatForSubmission;
            runtime.preprocessSwapchainImage(surface.chain, 1, 0, flags, region.rect);
            surface.regions.push_back(region);
        }

        static void expectCreationFailure(OpenXrRuntime& runtime, Surface& surface) {
            bool threw = false;
            try {
                runtime.preprocessSwapchainImage(surface.chain, 1, 0, 0, {{0, 0}, {32, 32}});
            } catch (const std::exception&) {
                threw = true;
            }
            require(threw, "Injected native creation failure did not propagate");
        }

        static void reuse() {
            OpenXrRuntime runtime;
            runtime.m_registryWatcher.reset();
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;
            installDevice(runtime, device, context);
            std::array<Surface, 4> surfaces;
            for (auto& surface : surfaces)
                initializeSurface(surface, device.Get());
            CreationHooks hooks(runtime.m_ovrSubmissionDevice.Get());
            process(runtime, surfaces[0], {{{0, 0}, {32, 32}}, clearPixel}, 0);
            // Retain COM references so replacement cannot reuse the old object's address.
            ComPtr<ID3D11ComputeShader> shader = runtime.m_alphaCorrectShader;
            ComPtr<ID3D11Buffer> constants = runtime.m_alphaCorrectConstants;
            bool sameResources = true;
            const auto rememberIdentity = [&] {
                sameResources &= shader == runtime.m_alphaCorrectShader && constants == runtime.m_alphaCorrectConstants;
            };
            process(runtime, surfaces[1], {{{32, 0}, {32, 32}}, linearPixel, 1}, premultiply);
            rememberIdentity();
            process(runtime, surfaces[2], {{{0, 32}, {32, 32}}, gammaPixel, 3}, premultiply, true);
            rememberIdentity();
            process(runtime,
                    surfaces[3],
                    {{{32, 32}, {32, 32}}, clearPixel},
                    XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT);
            rememberIdentity();
            process(runtime, surfaces[0], {{{32, 32}, {32, 32}}, gammaPixel, 3}, premultiply, true);
            rememberIdentity();
            // All five dispatches are queued before readback; each must retain its own constants.
            for (const auto& surface : surfaces)
                checkPixels(surface, device.Get(), context.Get());
            checkCounts(hooks.counts, 1, 1);
            require(shader && constants && sameResources,
                    "A successful cache entry was replaced on a later preprocessing call");
            runtime.cleanupSubmissionDevice();
            std::cout << "PASS: cached native resources and fresh viewport/alpha/sRGB constants\n";
        }

        static void retry(bool shaderFailure) {
            OpenXrRuntime runtime;
            runtime.m_registryWatcher.reset();
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;
            installDevice(runtime, device, context);
            Surface surface;
            initializeSurface(surface, device.Get());
            CreationHooks hooks(runtime.m_ovrSubmissionDevice.Get());
            if (shaderFailure)
                hooks.counts.failShaders = 1;
            else
                hooks.counts.failBuffers = 1;
            expectCreationFailure(runtime, surface);
            checkCounts(hooks.counts, 1, shaderFailure ? 0 : 1);
            require(!runtime.m_alphaCorrectConstants && bool(runtime.m_alphaCorrectShader) == !shaderFailure,
                    "Failed creation did not leave the expected empty or partial cache");
            ComPtr<ID3D11ComputeShader> retainedShader = runtime.m_alphaCorrectShader;
            const Region region{{{0, 0}, {32, 32}}, clearPixel};
            process(runtime, surface, region, 0);
            ComPtr<ID3D11ComputeShader> shader = runtime.m_alphaCorrectShader;
            ComPtr<ID3D11Buffer> constants = runtime.m_alphaCorrectConstants;
            require(shader && constants, "Retry did not populate both real cache resources");
            process(runtime, surface, region, 0);
            checkPixels(surface, device.Get(), context.Get());
            if (!shaderFailure)
                require(shader == retainedShader, "Buffer retry replaced the already successful shader");
            checkCounts(hooks.counts, shaderFailure ? 2 : 1, shaderFailure ? 1 : 2);
            require(shader == runtime.m_alphaCorrectShader && constants == runtime.m_alphaCorrectConstants,
                    "Successful retry was not cached");
            runtime.cleanupSubmissionDevice();
            std::cout << "PASS: "
                      << (shaderFailure ? "shader failure skips buffer and retries"
                                        : "buffer failure retains shader and retries")
                      << '\n';
        }

        static void cleanup(bool partial) {
            OpenXrRuntime runtime;
            runtime.m_registryWatcher.reset();
            ComPtr<ID3D11Device> oldDevice;
            ComPtr<ID3D11DeviceContext> oldContext;
            installDevice(runtime, oldDevice, oldContext);
            const auto oldIdentity = identity(oldDevice.Get());
            Surface oldSurface;
            initializeSurface(oldSurface, oldDevice.Get());
            ComPtr<ID3D11ComputeShader> oldShader;
            ComPtr<ID3D11Buffer> oldConstants;
            {
                CreationHooks hooks(runtime.m_ovrSubmissionDevice.Get());
                if (partial) {
                    hooks.counts.failBuffers = 1;
                    expectCreationFailure(runtime, oldSurface);
                } else
                    process(runtime, oldSurface, {{{0, 0}, {32, 32}}, clearPixel}, 0);
                oldShader = runtime.m_alphaCorrectShader;
                oldConstants = runtime.m_alphaCorrectConstants;
                require(oldShader && bool(oldConstants) == !partial, "Cleanup fixture cache state is wrong");
                require(ownerIdentity(oldShader.Get()) == oldIdentity, "Initial shader belongs to the wrong device");
                ComPtr<ID3D11Fence> completion = runtime.m_ovrSubmissionFence;
                runtime.cleanupSubmissionDevice();
                require(completion->GetCompletedValue() >= 1, "Cleanup did not complete queued submission work");
                require(!runtime.m_alphaCorrectShader && !runtime.m_alphaCorrectConstants &&
                            !runtime.m_ovrSubmissionDevice && !runtime.m_ovrSubmissionContext &&
                            !runtime.m_ovrSubmissionFence,
                        "Cleanup retained preprocessing resources or their submission device");
                checkCounts(hooks.counts, 1, 1);
                checkPixels(oldSurface, oldDevice.Get(), oldContext.Get());
            }
            ComPtr<ID3D11Device> newDevice;
            ComPtr<ID3D11DeviceContext> newContext;
            installDevice(runtime, newDevice, newContext);
            const auto newIdentity = identity(newDevice.Get());
            require(newIdentity != oldIdentity, "Replacement fixture did not create a distinct native device");
            Surface newSurface;
            initializeSurface(newSurface, newDevice.Get());
            CreationHooks hooks(runtime.m_ovrSubmissionDevice.Get());
            process(runtime, newSurface, {{{32, 0}, {32, 32}}, linearPixel, 1}, premultiply);
            checkPixels(newSurface, newDevice.Get(), newContext.Get());
            checkCounts(hooks.counts, 1, 1);
            require(runtime.m_alphaCorrectShader != oldShader && runtime.m_alphaCorrectConstants != oldConstants,
                    "New device reused a retired preprocessing resource");
            require(ownerIdentity(runtime.m_alphaCorrectShader.Get()) == newIdentity &&
                        ownerIdentity(runtime.m_alphaCorrectConstants.Get()) == newIdentity,
                    "Recreated preprocessing resources belong to the retired device");
            runtime.cleanupSubmissionDevice();
            std::cout << "PASS: " << (partial ? "partial" : "full") << " cache cleanup and native device recreation\n";
        }

        static void noWork() {
            OpenXrRuntime runtime;
            runtime.m_registryWatcher.reset();
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;
            installDevice(runtime, device, context);
            Surface surface;
            initializeSurface(surface, device.Get());
            CreationHooks hooks(runtime.m_ovrSubmissionDevice.Get());
            const XrRect2Di rect{{0, 0}, {32, 32}};
            surface.chain.dirty = false;
            runtime.preprocessSwapchainImage(surface.chain, 1, 0, 0, rect);
            surface.chain.dirty = true;
            runtime.preprocessSwapchainImage(
                surface.chain, 1, 0, XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT, rect);
            runtime.preprocessSwapchainImage(surface.chain, 0, 0, premultiply, rect);
            surface.chain.resolvedSlices[0].lastCommittedIndex = -1;
            runtime.preprocessSwapchainImage(surface.chain, 1, 0, 0, rect);
            checkPixels(surface, device.Get(), context.Get());
            checkCounts(hooks.counts, 0, 0);
            require(!runtime.m_alphaCorrectShader && !runtime.m_alphaCorrectConstants,
                    "A no-work submission populated the preprocessing cache");
            runtime.cleanupSubmissionDevice();
            std::cout << "PASS: clean/no-alpha/layer-zero/uncommitted submissions create no resources\n";
        }
    };
} // namespace virtualdesktop_openxr

int wmain(int argc, wchar_t** argv) {
    try {
        require(argc == 2,
                "usage: alpha_preprocess_cache reuse | shader-retry | buffer-retry | cleanup | "
                "partial-cleanup | no-work | all");
        const std::wstring mode = argv[1];
        using Regression = virtualdesktop_openxr::AlphaPreprocessCacheRegression;
        if (mode == L"reuse")
            Regression::reuse();
        else if (mode == L"shader-retry")
            Regression::retry(true);
        else if (mode == L"buffer-retry")
            Regression::retry(false);
        else if (mode == L"cleanup")
            Regression::cleanup(false);
        else if (mode == L"partial-cleanup")
            Regression::cleanup(true);
        else if (mode == L"no-work")
            Regression::noWork();
        else if (mode == L"all") {
            Regression::reuse();
            Regression::retry(true);
            Regression::retry(false);
            Regression::cleanup(false);
            Regression::cleanup(true);
            Regression::noWork();
        } else
            throw std::runtime_error("Unknown mode");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

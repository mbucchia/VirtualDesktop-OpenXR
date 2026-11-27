#pragma once

namespace virtualdesktop_openxr {

    class OpenXrRuntime;

    // Public interface only; capture transport and graphics state stay in the implementation.
    struct ZenOverlayHelper {
        virtual ~ZenOverlayHelper() = default;
        virtual bool AppendLayer(std::vector<ovrLayer_Union>& layers) = 0;
        virtual void Cleanup() = 0;
    };

    std::unique_ptr<ZenOverlayHelper> CreateZenOverlayHelper(OpenXrRuntime& runtime);

} // namespace virtualdesktop_openxr

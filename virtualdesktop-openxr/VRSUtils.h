#pragma once

namespace vrs {

    // We will use Root Constants to pass these values to the shader.
    struct GenerateShadingRateMapConstants {
        alignas(4) uint32_t Left;
        alignas(4) uint32_t Top;
        alignas(4) float CenterX;
        alignas(4) float CenterY;
        alignas(4) float InnerRing;
        alignas(4) float OuterRing;
        alignas(4) uint32_t Rate1x1;
        alignas(4) uint32_t RateMedium;
        alignas(4) uint32_t RateLow;
        alignas(4) bool Slice;
        alignas(4) bool Additive;
    };
    static_assert(sizeof(GenerateShadingRateMapConstants) / 4 < 64, "Maximum of 64 constants");

    struct TiledResolution {
        UINT Width;
        UINT Height;

        bool operator==(const TiledResolution& other) const {
            return (Width == other.Width && Height == other.Height);
        }

        size_t operator()(const TiledResolution& key) const {
            auto hash1 = std::hash<UINT>{}(key.Width);
            auto hash2 = std::hash<UINT>{}(key.Height);

            if (hash1 != hash2) {
                return hash1 ^ hash2;
            }

            // If hash1 == hash2, their XOR is zero.
            return hash1;
        }
    };

} // namespace vrs

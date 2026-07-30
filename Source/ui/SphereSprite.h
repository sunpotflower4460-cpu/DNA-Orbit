#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>

namespace dnaorbit::ui
{
    /**
     * Pre-rendered, shaded sphere images used to draw the helix nodes.
     *
     * Two things here are not obvious and must not be "simplified" away:
     *
     * 1. Sprites are generated into a SoftwareImageType image and then converted
     *    with NativeImageType. On Windows JUCE 8 renders through Direct2D, and
     *    drawing a software-backed Image there re-uploads it to the GPU on EVERY
     *    drawImage call (see juce_Direct2DGraphicsContextImpl_windows.cpp, where
     *    PagesAndArea::make falls back to NativeImageType{}.convert). At ~40
     *    sprites x 45 fps that would be thousands of texture creations per second.
     *    Converting once up front makes it a single upload that D2D then caches.
     *
     * 2. There is no tint or blend-mode path in the JUCE 2D API. Graphics'
     *    fillAlphaChannelWithCurrentBrush is not a tint - it pushes a clip layer
     *    and ignores the image's RGB entirely, which would throw away all the
     *    shading below. So a separate sprite is baked per colour.
     *
     * The ladder is indexed by DEPTH rather than by size. In this projection a
     * sprite's screen size is a strictly monotonic function of its depth, so one
     * depth-indexed ladder carries the correct size AND the correct fog colour at
     * no extra cost, with no per-draw tinting needed.
     */
    class SphereSprite
    {
    public:
        /**
         * Renders a single shaded sphere. Shading is computed in linear light and
         * written back through a gamma curve - doing the diffuse falloff directly
         * in sRGB is what makes hand-rolled spheres look chalky.
         */
        static juce::Image render (juce::Colour baseColour, juce::Colour backgroundColour, int diameterPx)
        {
            const int size = juce::jmax (4, diameterPx);

            // Generate into an explicitly software-backed image: the 4-argument
            // juce::Image constructor uses NativeImageType, so writing pixels
            // through BitmapData on it would map a GPU texture on Windows.
            juce::Image image { juce::Image::ARGB, size, size, true, juce::SoftwareImageType() };

            const auto toLinear = [] (float channel) { return channel * channel; };
            const float baseR = toLinear (baseColour.getFloatRed());
            const float baseG = toLinear (baseColour.getFloatGreen());
            const float baseB = toLinear (baseColour.getFloatBlue());

            // Hemispheric ambient: a constant ambient term flattens the terminator,
            // an up/down gradient reads as sky light plus a floor bounce.
            const float ambientUpR = baseR * 0.28f, ambientUpG = baseG * 0.28f, ambientUpB = baseB * 0.28f;
            const float ambientDownR = toLinear (backgroundColour.getFloatRed())   * 0.10f;
            const float ambientDownG = toLinear (backgroundColour.getFloatGreen()) * 0.10f;
            const float ambientDownB = toLinear (backgroundColour.getFloatBlue())  * 0.10f;

            // Light from the upper left front.
            constexpr float lx = -0.45f, ly = 0.72f, lz = 0.52f;
            const float lLen = std::sqrt (lx * lx + ly * ly + lz * lz);
            const float lnx = lx / lLen, lny = ly / lLen, lnz = lz / lLen;

            // Half vector against a viewer at +z.
            const float hx = lnx, hy = lny, hz = lnz + 1.0f;
            const float hLen = std::sqrt (hx * hx + hy * hy + hz * hz);
            const float hnx = hx / hLen, hny = hy / hLen, hnz = hz / hLen;

            juce::Image::BitmapData data { image, juce::Image::BitmapData::writeOnly };
            const float inv = 2.0f / (float) size;
            const float edge = 2.5f / (float) size;

            for (int py = 0; py < size; ++py)
            {
                for (int px = 0; px < size; ++px)
                {
                    const float u = ((float) px + 0.5f) * inv - 1.0f;
                    const float v = ((float) py + 0.5f) * inv - 1.0f;
                    const float r2 = u * u + v * v;

                    float alpha = 1.0f;
                    if (r2 > 1.0f)
                    {
                        const float r = std::sqrt (r2);
                        alpha = juce::jlimit (0.0f, 1.0f, (1.0f + edge - r) / edge);
                        if (alpha <= 0.0f)
                        {
                            data.setPixelColour (px, py, juce::Colours::transparentBlack);
                            continue;
                        }
                    }

                    const float nz = std::sqrt (juce::jmax (0.0f, 1.0f - juce::jmin (r2, 1.0f)));
                    const float nx = u;
                    const float ny = -v; // screen y grows downward, world y grows up

                    const float ndotl = juce::jmax (0.0f, nx * lnx + ny * lny + nz * lnz);
                    const float ndoth = juce::jmax (0.0f, nx * hnx + ny * hny + nz * hnz);

                    // Specular: a tight highlight up-left of centre.
                    float spec = ndoth;
                    spec = spec * spec; spec = spec * spec; spec = spec * spec;
                    spec = spec * spec; spec = spec * spec; spec = spec * spec; // ^64

                    // Fresnel rim, biased toward the lit side so it reads as grazing
                    // light rather than a cheap outline.
                    const float oneMinusNz = 1.0f - nz;
                    const float rim = oneMinusNz * oneMinusNz * oneMinusNz * (0.35f + 0.65f * ndotl);

                    // Cheap silhouette ambient occlusion.
                    const float occ = 0.75f + 0.25f * std::sqrt (nz);

                    const float ambientMix = 0.5f * (ny + 1.0f);
                    const float ambR = ambientDownR + (ambientUpR - ambientDownR) * ambientMix;
                    const float ambG = ambientDownG + (ambientUpG - ambientDownG) * ambientMix;
                    const float ambB = ambientDownB + (ambientUpB - ambientDownB) * ambientMix;

                    const float diffuse = 0.90f * ndotl;
                    float linR = (baseR * (ambR / juce::jmax (baseR, 1.0e-6f) + diffuse)) * occ;
                    float linG = (baseG * (ambG / juce::jmax (baseG, 1.0e-6f) + diffuse)) * occ;
                    float linB = (baseB * (ambB / juce::jmax (baseB, 1.0e-6f) + diffuse)) * occ;

                    // Specular is mostly white, tinted slightly by the base colour.
                    const float specAmount = 1.15f * spec;
                    linR += specAmount * (0.85f + 0.15f * baseR);
                    linG += specAmount * (0.85f + 0.15f * baseG);
                    linB += specAmount * (0.85f + 0.15f * baseB);

                    const float rimAmount = 0.55f * rim;
                    linR += rimAmount * (baseR + (1.0f - baseR) * 0.6f);
                    linG += rimAmount * (baseG + (1.0f - baseG) * 0.6f);
                    linB += rimAmount * (baseB + (1.0f - baseB) * 0.6f);

                    const auto toGamma = [] (float linear)
                    {
                        return juce::jlimit (0.0f, 1.0f, std::sqrt (juce::jmax (0.0f, linear)));
                    };

                    data.setPixelColour (px, py, juce::Colour::fromFloatRGBA (toGamma (linR),
                                                                              toGamma (linG),
                                                                              toGamma (linB),
                                                                              alpha));
                }
            }

            // One GPU upload now instead of one per draw call on Direct2D.
            return juce::NativeImageType().convert (image);
        }

        /** Soft radial glow. Alpha-over on a near-black background reads as additive. */
        static juce::Image renderGlow (juce::Colour colour, int diameterPx)
        {
            const int size = juce::jmax (8, diameterPx);
            juce::Image image { juce::Image::ARGB, size, size, true, juce::SoftwareImageType() };

            juce::Image::BitmapData data { image, juce::Image::BitmapData::writeOnly };
            const float inv = 2.0f / (float) size;

            for (int py = 0; py < size; ++py)
            {
                for (int px = 0; px < size; ++px)
                {
                    const float u = ((float) px + 0.5f) * inv - 1.0f;
                    const float v = ((float) py + 0.5f) * inv - 1.0f;
                    const float r = std::sqrt (u * u + v * v);

                    if (r >= 1.0f)
                    {
                        data.setPixelColour (px, py, juce::Colours::transparentBlack);
                        continue;
                    }

                    const float t = 1.0f - r;
                    const float alpha = t * t * std::sqrt (t); // ~pow(1-r, 2.5)
                    data.setPixelColour (px, py, colour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
                }
            }

            return juce::NativeImageType().convert (image);
        }
    };

    /**
     * A depth-indexed ladder of sphere sprites for one colour. Index 0 is the
     * farthest (smallest, most fogged), back is the nearest.
     */
    class SpriteLadder
    {
    public:
        static constexpr int steps = 14;

        void build (juce::Colour strandColour, juce::Colour backgroundColour,
                    double nearestDiameterPx, double farthestDiameterPx)
        {
            sprites.clear();
            sprites.reserve ((size_t) steps);

            for (int k = 0; k < steps; ++k)
            {
                const double t = steps > 1 ? (double) k / (double) (steps - 1) : 1.0;
                const double diameter = farthestDiameterPx + (nearestDiameterPx - farthestDiameterPx) * t;

                // Fog: lerp from the background toward the strand colour with depth.
                const float fog = 0.28f + 0.72f * std::pow ((float) t, 0.85f);
                const auto fogged = backgroundColour.interpolatedWith (strandColour, fog);

                sprites.push_back (SphereSprite::render (fogged, backgroundColour,
                                                          juce::roundToInt (juce::jmax (3.0, diameter))));
            }

            builtNearest = nearestDiameterPx;
        }

        bool isBuilt() const noexcept { return ! sprites.empty(); }
        double builtNearestDiameter() const noexcept { return builtNearest; }

        /** Picks the sprite for a normalised depth in [0, 1]. */
        const juce::Image& forDepth (double depth01) const noexcept
        {
            const int index = juce::jlimit (0, steps - 1,
                                            (int) std::lround (depth01 * (double) (steps - 1)));
            return sprites[(size_t) index];
        }

    private:
        std::vector<juce::Image> sprites;
        double builtNearest = 0.0;
    };
}

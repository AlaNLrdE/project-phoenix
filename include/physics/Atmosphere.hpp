#pragma once

#include <math/Constants.hpp>
#include <string>
#include <vector>

namespace Phoenix::Physics
{
    using namespace Math;

    /// One layer of an exponential atmosphere.
    struct AtmosphereLayer
    {
        double altBase;     ///< m — base altitude above surface
        double altTop;      ///< m — top altitude above surface
        double rho0;        ///< kg/m³ — density at altBase
        double scaleHeight; ///< m — exponential decay constant (H)
    };

    /**
     * @class Atmosphere
     * @brief Layered exponential atmosphere model.
     *
     * Density at altitude h (within a layer) follows:
     *   rho(h) = rho0 * exp(-(h - altBase) / scaleHeight)
     *
     * Above atmosphereHeight, density is 0.
     */
    class Atmosphere
    {
    public:
        std::string bodyName;
        double atmosphereHeight; ///< m — altitude above which density = 0
        std::vector<AtmosphereLayer> layers;

        Atmosphere() = default;

        /**
         * Build an Earth-like layered atmosphere using the given KSP scale factor.
         * All altitude thresholds and scale heights are multiplied by kspScale.
         * Densities are kept at their real-world ISA values.
         *
         * @param kspScale Scale factor (default = Constants::KSP_SCALE = 0.1)
         */
        static Atmosphere makeEarthLike(double kspScale = Constants::KSP_SCALE);

        /**
         * Air density at altitude h above the surface.
         * @return kg/m³ (0 if h >= atmosphereHeight or h < 0)
         */
        double getDensity(double altitude) const;

        /**
         * True if altitude is within the atmosphere (0 <= h < atmosphereHeight).
         */
        bool isInAtmosphere(double altitude) const;
    };

} // namespace Phoenix::Physics

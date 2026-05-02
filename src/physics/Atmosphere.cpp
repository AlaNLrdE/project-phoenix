#include <physics/Atmosphere.hpp>
#include <cmath>

namespace Phoenix::Physics
{

    Atmosphere Atmosphere::makeEarthLike(double kspScale)
    {
        Atmosphere atm;
        atm.bodyName = "Earth";
        atm.atmosphereHeight = 100000.0 * kspScale;

        // ISA-based layers, altitudes scaled by kspScale.
        // Density at each layer base follows the ISA standard atmosphere.
        // Scale heights are also scaled proportionally to preserve the shape.
        atm.layers = {
            //   altBase               altTop                rho0(kg/m³)   H(m)
            {0.0 * kspScale, 11000.0 * kspScale, 1.225, 8500.0 * kspScale},
            {11000.0 * kspScale, 25000.0 * kspScale, 0.3639, 6600.0 * kspScale},
            {25000.0 * kspScale, 47000.0 * kspScale, 0.0401, 7200.0 * kspScale},
            {47000.0 * kspScale, 86000.0 * kspScale, 1.43e-3, 5800.0 * kspScale},
            {86000.0 * kspScale, 100000.0 * kspScale, 6.5e-6, 8000.0 * kspScale},
        };

        return atm;
    }

    double Atmosphere::getDensity(double altitude) const
    {
        if (altitude < 0.0 || altitude >= atmosphereHeight)
            return 0.0;

        for (const auto &layer : layers)
        {
            if (altitude >= layer.altBase && altitude < layer.altTop)
            {
                double dh = altitude - layer.altBase;
                return layer.rho0 * std::exp(-dh / layer.scaleHeight);
            }
        }

        return 0.0;
    }

    bool Atmosphere::isInAtmosphere(double altitude) const
    {
        return altitude >= 0.0 && altitude < atmosphereHeight;
    }

} // namespace Phoenix::Physics

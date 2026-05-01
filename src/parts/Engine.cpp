#include <parts/Engine.hpp>
#include <math/Constants.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Phoenix::Parts
{

    using namespace Math;

    // ── Constructor ──────────────────────────────────────────────────────────────

    Engine::Engine(const std::string &name_,
                   double dryMass_,
                   double maxThrust_,
                   double Isp_)
        : Part(name_, PartType::Engine, dryMass_, /*maxFuel=*/0.0),
          maxThrust(maxThrust_),
          Isp(Isp_),
          throttle(0.0),
          isRunning(false)
    {
    }

    // ── Empuje y flujo ───────────────────────────────────────────────────────────

    double Engine::getCurrentThrust() const
    {
        if (!isRunning)
            return 0.0;
        return throttle * maxThrust;
    }

    double Engine::getMassFlowRate() const
    {
        if (!isRunning || Isp <= 0.0)
            return 0.0;
        return getCurrentThrust() / (Isp * Constants::G0);
    }

    double Engine::getExhaustVelocity() const
    {
        return Isp * Constants::G0;
    }

    // ── Control ──────────────────────────────────────────────────────────────────

    void Engine::ignite()
    {
        isRunning = true;
    }

    void Engine::shutdown()
    {
        isRunning = false;
        throttle = 0.0;
    }

    void Engine::setThrottle(double t)
    {
        throttle = std::clamp(t, 0.0, 1.0);
        if (throttle == 0.0)
        {
            isRunning = false;
        }
    }

    // ── Ecuación de cohete ───────────────────────────────────────────────────────

    double Engine::computeDeltaV(double totalFuelMass, double dryVehicleMass) const
    {
        if (totalFuelMass <= 0.0 || dryVehicleMass <= 0.0)
            return 0.0;
        double ve = getExhaustVelocity();
        double m0 = dryVehicleMass + totalFuelMass;
        double mf = dryVehicleMass;
        return ve * std::log(m0 / mf);
    }

    double Engine::computeBurnTime(double deltaV,
                                   double totalFuelMass,
                                   double vehicleMass) const
    {
        double ve = getExhaustVelocity();
        double mdot = getMassFlowRate();
        if (ve <= 0.0 || mdot <= 0.0)
            return -1.0;

        // Masa al final del burn requerida: mf = m0 * e^(-ΔV / ve)
        double mf_required = vehicleMass * std::exp(-deltaV / ve);
        double fuelNeeded = vehicleMass - mf_required;

        if (fuelNeeded > totalFuelMass)
            return -1.0; // No factible

        // t_burn = fuelNeeded / mdot
        return fuelNeeded / mdot;
    }

} // namespace Phoenix::Parts

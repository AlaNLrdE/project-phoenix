#pragma once

#include <math/Constants.hpp>
#include <physics/Atmosphere.hpp>
#include <vector>

namespace Phoenix::Physics
{
    using namespace Math;

    /// Instantaneous aerodynamic state at a single point in time.
    struct AeroState
    {
        double density = 0.0;         ///< kg/m³
        double dynamicPressure = 0.0; ///< Pa (½ρv²)
        double dragForce = 0.0;       ///< N (magnitude)
        double heatFlux = 0.0;        ///< W/m² (Chapman approximation)
        dvec3 dragAcceleration{0.0};  ///< m/s² — opposing velocity direction
    };

    /// One logged point along a simulated aerobraking trajectory.
    struct AeroTrajectoryPoint
    {
        double time = 0.0;      ///< s — elapsed simulation time
        double altitude = 0.0;  ///< m above body surface
        double speed = 0.0;     ///< m/s
        double density = 0.0;   ///< kg/m³
        double heatFlux = 0.0;  ///< W/m²
        double dragForce = 0.0; ///< N
        dvec3 position{0.0};    ///< m (inertial, body-centered frame)
        dvec3 velocity{0.0};    ///< m/s
    };

    /// Result of a full reentry simulation.
    struct AeroTrajectoryResult
    {
        std::vector<AeroTrajectoryPoint> points;
        bool landed = false;
        bool burnedUp = false;
        double impactSpeed = 0.0;  ///< m/s at surface contact
        double peakHeatFlux = 0.0; ///< W/m²
        double peakDecel = 0.0;    ///< m/s²
    };

    /**
     * @class AeroForces
     * @brief Static aerodynamic utilities and reentry trajectory simulator.
     *
     * Drag model:     D = ½ρv²CdA
     * Heat flux:      q = 1.83×10⁻⁴ × √(ρ/R_nose) × v³   [Chapman, W/m²]
     * Integrator:     4th-order Runge-Kutta (RK4)
     * Gravity model:  point mass (a_grav = −μ/r³ × r̂)
     */
    class AeroForces
    {
    public:
        /**
         * Compute instantaneous aerodynamic state.
         *
         * @param altitude   m above surface
         * @param velocity   m/s (velocity vector relative to atmosphere)
         * @param mass       kg (for deceleration computation)
         * @param area       m² — cross-sectional reference area
         * @param Cd         dimensionless drag coefficient
         * @param atm        Atmosphere model
         * @param noseRadius m — nose radius for Chapman heat flux
         */
        static AeroState compute(
            double altitude,
            const dvec3 &velocity,
            double mass,
            double area,
            double Cd,
            const Atmosphere &atm,
            double noseRadius = 0.5);

        /**
         * Full reentry trajectory simulation using 4th-order Runge-Kutta.
         * Integrates gravity + aerodynamic drag until landing, burn-up, or maxSteps.
         *
         * @param r0, v0       Initial position (m) and velocity (m/s)
         * @param mass         kg
         * @param area         m² reference area
         * @param Cd           drag coefficient
         * @param noseRadius   m — nose radius for heat flux
         * @param bodyRadius   m — body surface radius (landed when altitude <= 0)
         * @param bodyMu       m³/s² — gravitational parameter
         * @param atm          Atmosphere model
         * @param dt           s — fixed RK4 timestep
         * @param maxSteps     maximum integration steps before stopping
         * @param logInterval  steps between logged AeroTrajectoryPoints
         */
        static AeroTrajectoryResult simulate(
            dvec3 r0,
            dvec3 v0,
            double mass,
            double area,
            double Cd,
            double noseRadius,
            double bodyRadius,
            double bodyMu,
            const Atmosphere &atm,
            double dt = 0.05,
            int maxSteps = 100000,
            int logInterval = 100);

    private:
        struct State
        {
            dvec3 r, v;
        };

        static dvec3 netAcceleration(
            const State &s,
            double mass,
            double area,
            double Cd,
            double noseRadius,
            double bodyRadius,
            double bodyMu,
            const Atmosphere &atm);

        static State rk4Step(
            const State &s,
            double dt,
            double mass,
            double area,
            double Cd,
            double noseRadius,
            double bodyRadius,
            double bodyMu,
            const Atmosphere &atm);
    };

} // namespace Phoenix::Physics

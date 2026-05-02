#include <physics/AeroForces.hpp>
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

namespace Phoenix::Physics
{

    // ── AeroState compute ────────────────────────────────────────────────────

    AeroState AeroForces::compute(
        double altitude,
        const dvec3 &velocity,
        double mass,
        double area,
        double Cd,
        const Atmosphere &atm,
        double noseRadius)
    {
        AeroState s;
        s.density = atm.getDensity(altitude);

        double speed = glm::length(velocity);
        if (speed < 1e-9 || s.density <= 0.0)
            return s;

        s.dynamicPressure = 0.5 * s.density * speed * speed;
        s.dragForce = s.dynamicPressure * Cd * area;

        // Chapman (1959) stagnation-point heat flux:
        //   q = 1.83×10⁻⁴ × sqrt(ρ / R_nose) × v³   [W/m²]
        if (noseRadius > 0.0)
            s.heatFlux = 1.83e-4 * std::sqrt(s.density / noseRadius) * speed * speed * speed;

        // Drag acceleration opposes the velocity vector
        dvec3 velDir = velocity / speed;
        double dragAccelMag = (mass > 0.0) ? s.dragForce / mass : 0.0;
        s.dragAcceleration = -velDir * dragAccelMag;

        return s;
    }

    // ── Private helpers ──────────────────────────────────────────────────────

    dvec3 AeroForces::netAcceleration(
        const State &s,
        double mass,
        double area,
        double Cd,
        double noseRadius,
        double bodyRadius,
        double bodyMu,
        const Atmosphere &atm)
    {
        double r_mag = glm::length(s.r);
        double altitude = r_mag - bodyRadius;

        // Inverse-square gravity toward body centre
        dvec3 grav = -(bodyMu / (r_mag * r_mag * r_mag)) * s.r;

        // Aerodynamic drag (only inside atmosphere)
        dvec3 drag(0.0);
        if (atm.isInAtmosphere(altitude))
        {
            AeroState aero = compute(altitude, s.v, mass, area, Cd, atm, noseRadius);
            drag = aero.dragAcceleration;
        }

        return grav + drag;
    }

    AeroForces::State AeroForces::rk4Step(
        const State &s,
        double dt,
        double mass,
        double area,
        double Cd,
        double noseRadius,
        double bodyRadius,
        double bodyMu,
        const Atmosphere &atm)
    {
        auto acc = [&](const State &st)
        {
            return netAcceleration(st, mass, area, Cd, noseRadius, bodyRadius, bodyMu, atm);
        };

        // k1
        dvec3 k1v = acc(s);
        dvec3 k1r = s.v;

        // k2
        State s2 = {s.r + k1r * (dt * 0.5), s.v + k1v * (dt * 0.5)};
        dvec3 k2v = acc(s2);
        dvec3 k2r = s2.v;

        // k3
        State s3 = {s.r + k2r * (dt * 0.5), s.v + k2v * (dt * 0.5)};
        dvec3 k3v = acc(s3);
        dvec3 k3r = s3.v;

        // k4
        State s4 = {s.r + k3r * dt, s.v + k3v * dt};
        dvec3 k4v = acc(s4);
        dvec3 k4r = s4.v;

        const double sixth = 1.0 / 6.0;
        State result;
        result.r = s.r + (k1r + 2.0 * k2r + 2.0 * k3r + k4r) * (dt * sixth);
        result.v = s.v + (k1v + 2.0 * k2v + 2.0 * k3v + k4v) * (dt * sixth);
        return result;
    }

    // ── Main simulation loop ─────────────────────────────────────────────────

    AeroTrajectoryResult AeroForces::simulate(
        dvec3 r0,
        dvec3 v0,
        double mass,
        double area,
        double Cd,
        double noseRadius,
        double bodyRadius,
        double bodyMu,
        const Atmosphere &atm,
        double dt,
        int maxSteps,
        int logInterval)
    {
        AeroTrajectoryResult result;
        State state = {r0, v0};
        double t = 0.0;

        for (int step = 0; step < maxSteps; ++step)
        {
            double r_mag = glm::length(state.r);
            double altitude = r_mag - bodyRadius;
            double speed = glm::length(state.v);

            // Compute current aero state for metrics / logging
            AeroState aero;
            if (atm.isInAtmosphere(altitude))
                aero = compute(altitude, state.v, mass, area, Cd, atm, noseRadius);

            // Track peak values every step (cheap)
            if (aero.heatFlux > result.peakHeatFlux)
                result.peakHeatFlux = aero.heatFlux;
            double decel = glm::length(aero.dragAcceleration);
            if (decel > result.peakDecel)
                result.peakDecel = decel;

            // Log trajectory point
            if (step % logInterval == 0)
            {
                result.points.push_back({t, altitude, speed,
                                         aero.density, aero.heatFlux, aero.dragForce,
                                         state.r, state.v});
            }

            // Termination: surface contact
            if (altitude <= 0.0)
            {
                result.landed = true;
                result.impactSpeed = speed;
                break;
            }

            // Termination: structural failure / ablation
            // Chapman heat flux > 500 MW/m² → vehicle disintegrates
            constexpr double BURNUP_FLUX = 500.0e6; // W/m²
            if (aero.heatFlux > BURNUP_FLUX)
            {
                result.burnedUp = true;
                break;
            }

            // RK4 integration step
            state = rk4Step(state, dt, mass, area, Cd, noseRadius, bodyRadius, bodyMu, atm);
            t += dt;
        }

        return result;
    }

} // namespace Phoenix::Physics

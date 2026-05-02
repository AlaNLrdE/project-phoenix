#pragma once

#include <launch/AscentIntegrator.hpp>
#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>

namespace Phoenix::Launch {

/**
 * @struct CircularizationResult
 * @brief Results of a circularization burn computed from an ascent trajectory.
 *
 * The burn is modelled as an impulsive prograde maneuver at apoapsis of the
 * insertion orbit (Hohmann-style). It raises the periapsis from sub-surface
 * to match the apoapsis, yielding a circular orbit at the same altitude.
 */
struct CircularizationResult {
    Physics::Orbit insertionOrbit;  ///< elliptical orbit built from state vector at engine cutoff
    Physics::Orbit circularOrbit;   ///< circular orbit in the same plane at apoapsis altitude
    double deltaV        = 0.0;     ///< m/s — prograde ΔV required at apoapsis
    double burnAltitude  = 0.0;     ///< m above surface — altitude of the circularization burn
    bool   valid         = false;   ///< false if ascent.orbitInserted was not set
};

/**
 * @class OrbitalManeuvers
 * @brief Static helpers for computing orbital maneuvers from ascent results.
 */
class OrbitalManeuvers {
public:
    /**
     * Compute the circularization burn at apoapsis.
     *
     * Requires ascent.orbitInserted == true and valid cutoffPosition/Velocity.
     * The burn raises the periapsis from the insertion ellipse to match the
     * apoapsis, producing a circular orbit at apoapsis altitude.
     *
     * ΔV = v_circ − v_apo   where:
     *   v_apo  = √(μ · (2/r_apo − 1/a_ins))   (vis-viva at apoapsis of insertion ellipse)
     *   v_circ = √(μ / r_apo)                  (circular velocity at same altitude)
     *
     * @param ascent  Result of an AscentIntegrator::simulate() call with orbitInserted=true.
     * @param body    Reference body (must be the same one used in AscentIntegrator).
     * @return CircularizationResult with insertion orbit, circular orbit, and ΔV.
     */
    static CircularizationResult circularize(const AscentResult&          ascent,
                                             const Physics::CelestialBody& body);
};

} // namespace Phoenix::Launch

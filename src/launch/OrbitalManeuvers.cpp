#include <launch/OrbitalManeuvers.hpp>
#include <cmath>
#include <glm/glm.hpp>

namespace Phoenix::Launch {

using namespace Phoenix::Physics;
using namespace Phoenix::Math;

CircularizationResult OrbitalManeuvers::circularize(
    const AscentResult&    ascent,
    const CelestialBody&   body)
{
    CircularizationResult cr;

    if (!ascent.orbitInserted || glm::length(ascent.cutoffPosition) < 1.0) {
        return cr;  // valid=false; caller should check
    }

    // Build insertion orbit from ECI state vector at engine cutoff
    cr.insertionOrbit = Orbit(ascent.cutoffPosition, ascent.cutoffVelocity,
                               body.mu, ascent.cutoffTime);

    // r_apo: distance from Earth centre to apoapsis
    double r_apo = body.radius + ascent.apoapsis;
    double a_ins  = cr.insertionOrbit.a;

    // Vis-viva: speed at apoapsis of the insertion ellipse
    // v² = μ(2/r − 1/a)
    double v_apo  = std::sqrt(body.mu * (2.0 / r_apo - 1.0 / a_ins));

    // Circular velocity at the same altitude
    double v_circ = std::sqrt(body.mu / r_apo);

    cr.deltaV       = v_circ - v_apo;  // prograde — always positive
    cr.burnAltitude = ascent.apoapsis;
    cr.valid        = true;

    // Circular orbit: same orbital plane (i, Omega), e=0, a=r_apo
    cr.circularOrbit = Orbit(r_apo, 0.0,
                              cr.insertionOrbit.i,
                              cr.insertionOrbit.Omega,
                              0.0, 0.0,
                              body.mu, ascent.cutoffTime);

    return cr;
}

} // namespace Phoenix::Launch

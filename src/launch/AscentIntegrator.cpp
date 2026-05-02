#include <launch/AscentIntegrator.hpp>
#include <cmath>
#include <algorithm>

namespace Phoenix::Launch {

using namespace Phoenix::Math;
using namespace Phoenix::Physics;

// ── Constructor ───────────────────────────────────────────────────────────────

AscentIntegrator::AscentIntegrator(const LaunchVehicle& v,
                                   const CelestialBody& b,
                                   const Atmosphere*    a)
    : veh(v), body(b), atm(a) {}

// ── Helpers físicos ───────────────────────────────────────────────────────────

dvec3 AscentIntegrator::gravAccel(const dvec3& pos) const {
    double r2 = glm::dot(pos, pos);
    double r  = std::sqrt(r2);
    return -(body.mu / (r2 * r)) * pos;
}

// Phase 8C: altitude-based pitch program.
//
// Pitches from vertical (0°) to 45° off-vertical (NOT to horizontal/east).
// Pure east-blending would make the rocket horizontal at low altitude and crash.
// 45° keeps radial acceleration > gravity at typical TWR ~2.5 (upward net: ~7 m/s²).
// After hTurn the rocket tracks actual prograde, which by then has a meaningful
// eastward component — the gravity turn completes naturally in vacuum.
dvec3 AscentIntegrator::thrustDir(const dvec3& pos, const dvec3& vel) const {
    double alt = glm::length(pos) - body.radius;
    dvec3  up  = glm::normalize(pos);

    // "East" — tangential to the equator at the current position.
    // north pole = +Z axis in ECI; east = cross(north, up).
    dvec3  north(0.0, 0.0, 1.0);
    dvec3  east = glm::cross(north, up);
    double eLen = glm::length(east);
    if (eLen < 1e-6) east = dvec3(0.0, 1.0, 0.0);  // fallback near poles
    else             east = east / eLen;

    // Below kick altitude: straight up
    if (alt < guidance.hKick) return up;

    // Above turn-completion altitude: follow actual prograde (gravity turn continues)
    if (alt >= guidance.hTurn) {
        double spd = glm::length(vel);
        return (spd > 1.0) ? glm::normalize(vel)
                           : glm::normalize(up * 0.707 + east * 0.707);
    }

    // Smoothstep from 0° to 45° pitch angle (from vertical) between hKick and hTurn.
    // Using angle parametrization avoids the "pure east → horizontal crash" problem.
    double t     = (alt - guidance.hKick) / (guidance.hTurn - guidance.hKick);
    double blend = t * t * (3.0 - 2.0 * t);         // smoothstep 0→1
    double pitch = blend * (M_PI / 4.0);             // 0° → 45° from vertical
    return glm::normalize(up * std::cos(pitch) + east * std::sin(pitch));
}

double AscentIntegrator::crossSection(const StageConfig& s) {
    double propT = s.getPropellantMass() / 1000.0;
    double r     = 0.3 + 0.03 * std::sqrt(propT);
    return M_PI * r * r;
}

double AscentIntegrator::instantApoapsis(const dvec3& pos, const dvec3& vel) const {
    double r  = glm::length(pos);
    double v2 = glm::dot(vel, vel);
    double e  = 0.5 * v2 - body.mu / r;
    if (e >= 0.0) return 1e18;  // hyperbolic / escape
    double a    = -body.mu / (2.0 * e);
    dvec3  hvec = glm::cross(pos, vel);
    double h2   = glm::dot(hvec, hvec);
    double ecc  = std::sqrt(std::max(0.0, 1.0 - h2 / (body.mu * a)));
    return a * (1.0 + ecc) - body.radius;
}

// ── Drag force (N) — static helper ───────────────────────────────────────────

static dvec3 dragForce(const Atmosphere* atm, double bodyRadius,
                       const dvec3& pos, const dvec3& vel,
                       double area, double Cd)
{
    if (!atm) return dvec3(0.0);
    double alt = glm::length(pos) - bodyRadius;
    if (!atm->isInAtmosphere(alt)) return dvec3(0.0);
    double rho = atm->getDensity(alt);
    double v2  = glm::dot(vel, vel);
    if (v2 < 1e-6) return dvec3(0.0);
    double v  = std::sqrt(v2);
    double Fd = 0.5 * rho * v2 * Cd * area;
    return -(Fd / v) * vel;
}

// ── Simulación principal ──────────────────────────────────────────────────────

AscentResult AscentIntegrator::simulate(double dt, double maxTime,
                                         double targetAlt) const
{
    AscentResult result;
    result.exitReason = "max time reached";

    if (veh.stageCount() == 0) {
        result.exitReason = "no stages";
        return result;
    }

    // ── Estado inicial: cohete en la superficie del ecuador ─────────────────
    dvec3 pos(body.radius, 0.0, 0.0);
    dvec3 vel(0.0, 0.0, 0.0);

    // ── Masa y combustible por etapa ─────────────────────────────────────────
    const int N = veh.stageCount();
    struct StageFuel { double propLeft, dryMass; };
    std::vector<StageFuel> stages(N);
    double mass = 0.0;
    for (int i = 0; i < N; ++i) {
        stages[i].propLeft = veh.getStage(i).getPropellantMass();
        stages[i].dryMass  = veh.getStage(i).getDryMass();
        mass              += veh.getStage(i).getWetMass();
    }

    int    stageIdx     = 0;
    double time         = 0.0;
    int    step         = 0;
    bool   engineCutoff = false;
    bool   peakPassed   = false;   // for coast phase after engine cutoff

    double karmanLine = (atm ? atm->atmosphereHeight : 100000.0 * Constants::KSP_SCALE);

    const double Cd = 0.3;
    double area = crossSection(veh.getStage(0));

    // ── Bucle de integración ──────────────────────────────────────────────────
    while (time < maxTime) {
        double alt = glm::length(pos) - body.radius;
        double spd = glm::length(vel);

        // ── Presión dinámica ─────────────────────────────────────────────────
        double dynPressure = 0.0;
        if (atm && atm->isInAtmosphere(alt)) {
            double rho = atm->getDensity(alt);
            dynPressure = 0.5 * rho * spd * spd;
        }
        if (dynPressure > result.maxDynPressure) {
            result.maxDynPressure     = dynPressure;
            result.maxDynPressureTime = time;
            result.maxDynPressureAlt  = alt;
        }

        // ── Engine cutoff check ──────────────────────────────────────────────
        // Conditions: (a) instantaneous apoapsis >= target AND
        //             (b) horizontal velocity >= vertical velocity
        //                 (trajectory is past 45° from vertical)
        // Condition (b) prevents premature cutoff on a nearly-radial ascent
        // where the instantaneous apoapsis can hit the target altitude very
        // early while the rocket is still mostly going straight up.
        if (!engineCutoff && guidance.targetApoapsis > 0.0 && stageIdx < N) {
            double apo = instantApoapsis(pos, vel);
            if (apo >= guidance.targetApoapsis) {
                dvec3  up_hat = glm::normalize(pos);
                double radVel = std::abs(glm::dot(vel, up_hat));
                double tanVel = std::sqrt(std::max(0.0, spd*spd - radVel*radVel));
                if (tanVel >= radVel) {   // more horizontal than vertical
                    engineCutoff          = true;
                    result.orbitInserted  = true;
                    result.exitReason     = "orbit insertion";
                    result.cutoffTime     = time;
                    result.cutoffAltitude = alt;
                    result.cutoffSpeed    = spd;
                    result.cutoffPosition = pos;
                    result.cutoffVelocity = vel;
                    double r  = glm::length(pos);
                    double v2 = glm::dot(vel, vel);
                    double e  = 0.5 * v2 - body.mu / r;
                    if (e < 0.0) {
                        double a    = -body.mu / (2.0 * e);
                        dvec3  hvec = glm::cross(pos, vel);
                        double h2   = glm::dot(hvec, hvec);
                        double ecc  = std::sqrt(std::max(0.0, 1.0 - h2 / (body.mu * a)));
                        result.apoapsis  = a * (1.0 + ecc) - body.radius;
                        result.periapsis = a * (1.0 - ecc) - body.radius;
                    }
                    result.totalBurnTime = time;
                }
            }
        }

        // ── Muestrear trayectoria ────────────────────────────────────────────
        if (step % recordInterval == 0) {
            AscentPoint pt;
            pt.time        = time;
            pt.position    = pos;
            pt.velocity    = vel;
            pt.mass        = mass;
            pt.altitude    = alt;
            pt.speed       = spd;
            pt.dynPressure = dynPressure;
            pt.stageIndex  = stageIdx;
            bool burning = !engineCutoff && stageIdx < N && stages[stageIdx].propLeft > 0.0;
            pt.thrust = burning ? veh.getStage(stageIdx).getMaxThrust() * throttle : 0.0;
            pt.drag   = glm::length(dragForce(atm, body.radius, pos, vel, area, Cd));
            result.trajectory.push_back(pt);
        }

        // ── Condiciones de parada ────────────────────────────────────────────
        if (alt < -200.0) {
            if (result.exitReason == "max time reached") result.exitReason = "crashed";
            break;
        }
        bool fuelDepleted = (stageIdx >= N);
        if (fuelDepleted && !engineCutoff) {
            result.exitReason = "fuel depleted";
            break;
        }
        // After engine cutoff: coast briefly to record apoapsis state, then stop.
        if (engineCutoff) {
            if (alt >= result.apoapsis * 0.95) peakPassed = true;
            if (peakPassed && alt < result.apoapsis * 0.85) {
                result.exitReason = "orbit coast complete";
                break;
            }
            // Safety: stop coasting after 600 s from cutoff
            if (time > result.cutoffTime + 600.0) {
                result.exitReason = "orbit coast complete";
                break;
            }
        }
        if (targetAlt > 0.0 && alt >= targetAlt) {
            result.exitReason = "target altitude reached";
            break;
        }

        // ── Empuje de la etapa activa ────────────────────────────────────────
        bool burning   = !engineCutoff && stageIdx < N && stages[stageIdx].propLeft > 0.0;
        double Isp     = burning ? veh.getStage(stageIdx).getWeightedIsp()  : 0.0;
        double maxThr  = burning ? veh.getStage(stageIdx).getMaxThrust()     : 0.0;

        // Max-Q throttle clamp
        double effThrottle = throttle;
        if (burning && dynPressure > guidance.maxQLimit)
            effThrottle = std::min(effThrottle, guidance.maxQThrottle);

        double thrust = burning ? maxThr * effThrottle : 0.0;
        double mflow  = (thrust > 0.0 && Isp > 0.0) ? thrust / (Isp * Constants::G0) : 0.0;
        dvec3  tDir   = thrustDir(pos, vel);

        // ── RK4 ──────────────────────────────────────────────────────────────
        auto deriv = [&](const dvec3& p, const dvec3& v) -> std::pair<dvec3, dvec3> {
            dvec3 Fg  = gravAccel(p) * mass;
            dvec3 Fd  = dragForce(atm, body.radius, p, v, area, Cd);
            dvec3 Ft  = tDir * thrust;
            dvec3 acc = (Fg + Fd + Ft) / mass;
            return { v, acc };
        };

        auto [dp1, dv1] = deriv(pos,               vel              );
        auto [dp2, dv2] = deriv(pos + dp1*(dt/2.), vel + dv1*(dt/2.));
        auto [dp3, dv3] = deriv(pos + dp2*(dt/2.), vel + dv2*(dt/2.));
        auto [dp4, dv4] = deriv(pos + dp3*dt,      vel + dv3*dt     );

        pos += (dp1 + dp2*2.0 + dp3*2.0 + dp4) * (dt / 6.0);
        vel += (dv1 + dv2*2.0 + dv3*2.0 + dv4) * (dt / 6.0);

        // ── Consumo de propelante y separación de etapa ──────────────────────
        if (mflow > 0.0) {
            double needed = mflow * dt;
            if (stages[stageIdx].propLeft >= needed) {
                stages[stageIdx].propLeft -= needed;
                mass                      -= needed;
            } else {
                double burned = stages[stageIdx].propLeft;
                mass -= burned + stages[stageIdx].dryMass;
                mass  = std::max(mass, 1.0);
                stages[stageIdx].propLeft = 0.0;
                result.stagingTimes.push_back(time);
                result.stagesUsed++;
                stageIdx++;
                if (stageIdx < N) area = crossSection(veh.getStage(stageIdx));
            }
        }

        result.maxAltitude  = std::max(result.maxAltitude, alt);
        result.maxSpeed     = std::max(result.maxSpeed,    spd);
        result.reachedSpace = result.reachedSpace || (alt >= karmanLine);

        time += dt;
        ++step;
    }

    if (!engineCutoff) result.totalBurnTime = time;

    // ── Elementos orbitales del estado final (si no fueron calculados ya) ────
    if (!result.orbitInserted) {
        double r  = glm::length(pos);
        double v2 = glm::dot(vel, vel);
        double e  = 0.5 * v2 - body.mu / r;
        if (e < 0.0 && r > body.radius) {
            double a    = -body.mu / (2.0 * e);
            dvec3  hvec = glm::cross(pos, vel);
            double h2   = glm::dot(hvec, hvec);
            double ecc  = std::sqrt(std::max(0.0, 1.0 - h2 / (body.mu * a)));
            result.apoapsis  = a * (1.0 + ecc) - body.radius;
            result.periapsis = a * (1.0 - ecc) - body.radius;
        }
    }

    return result;
}

} // namespace Phoenix::Launch

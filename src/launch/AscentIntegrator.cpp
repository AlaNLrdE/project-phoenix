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
    return -(body.mu / (r2 * r)) * pos;  // m/s²
}

dvec3 AscentIntegrator::dragAccel(const dvec3& pos, const dvec3& vel,
                                   double /*mass*/, double area, double Cd) const {
    if (!atm) return dvec3(0.0);
    double alt = glm::length(pos) - body.radius;
    if (!atm->isInAtmosphere(alt)) return dvec3(0.0);
    double rho = atm->getDensity(alt);
    double v2  = glm::dot(vel, vel);
    if (v2 < 1e-6) return dvec3(0.0);
    double v      = std::sqrt(v2);
    double Fd     = 0.5 * rho * v2 * Cd * area;  // N
    return -(Fd / (v2 / v)) * glm::normalize(vel); // m/s² — requiere división por masa fuera
    // Nota: devuelve fuerza/unidad_masa sólo si llamamos con mass=1.
    // El caller debe dividir por la masa del vehículo.
}

// El dragAccel anterior devuelve aceleración ya dividida "de forma incorrecta" — reescribo:
// Devuelve la FUERZA de drag (N), el caller divide por masa.
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
    return -(Fd / v) * vel;  // N, opuesto a la velocidad
}

dvec3 AscentIntegrator::thrustDir(const dvec3& pos, const dvec3& vel,
                                   double t) const
{
    dvec3 up = glm::normalize(pos);  // dirección radial (vertical)

    double spd = glm::length(vel);
    // En los primeros 5 s o si la velocidad es muy pequeña: empuje vertical
    if (t < 5.0 || spd < 5.0)
        return up;

    dvec3 prograde = glm::normalize(vel);

    // Transición suave vertical → prograde entre t=5 y t=60 s
    if (t < 60.0) {
        double blend = (t - 5.0) / 55.0;
        blend = blend * blend;  // easing cuadrático
        return glm::normalize(up * (1.0 - blend) + prograde * blend);
    }

    return prograde;
}

double AscentIntegrator::crossSection(const StageConfig& s) {
    // Mismo heurístico que stageVis en demo_launch: r = 0.3 + 0.03*sqrt(propTons)
    double propT = s.getPropellantMass() / 1000.0;
    double r     = 0.3 + 0.03 * std::sqrt(propT);
    return M_PI * r * r;  // m²
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
    dvec3 pos(body.radius, 0.0, 0.0);  // Ecuador, ECI
    dvec3 vel(0.0, 0.0, 0.0);

    // ── Masa y combustible por etapa ─────────────────────────────────────────
    const int N = veh.stageCount();
    struct StageFuel {
        double propLeft;
        double dryMass;
    };
    std::vector<StageFuel> stages(N);
    double mass = 0.0;
    for (int i = 0; i < N; ++i) {
        stages[i].propLeft = veh.getStage(i).getPropellantMass();
        stages[i].dryMass  = veh.getStage(i).getDryMass();
        mass              += veh.getStage(i).getWetMass();
    }

    int    stageIdx = 0;
    double time     = 0.0;
    int    step     = 0;

    // Línea de Kármán del cuerpo (altitud "espacio")
    double karmanLine = (atm ? atm->atmosphereHeight : 100000.0 * Constants::KSP_SCALE);

    // Cd y área aerodinámica (del primer stage activo)
    const double Cd = 0.3;
    double area = crossSection(veh.getStage(0));

    // ── Bucle de integración ──────────────────────────────────────────────────
    while (time < maxTime) {
        double alt = glm::length(pos) - body.radius;
        double spd = glm::length(vel);

        // ── Muestrear trayectoria ────────────────────────────────────────────
        if (step % recordInterval == 0) {
            AscentPoint pt;
            pt.time       = time;
            pt.position   = pos;
            pt.velocity   = vel;
            pt.mass       = mass;
            pt.altitude   = alt;
            pt.speed      = spd;
            pt.stageIndex = stageIdx;
            if (stageIdx < N && stages[stageIdx].propLeft > 0.0) {
                pt.thrust = veh.getStage(stageIdx).getMaxThrust() * throttle;
                dvec3 Fd  = dragForce(atm, body.radius, pos, vel, area, Cd);
                pt.drag   = glm::length(Fd);
            } else {
                pt.thrust = 0.0;
                pt.drag   = glm::length(dragForce(atm, body.radius, pos, vel, area, Cd));
            }
            result.trajectory.push_back(pt);
        }

        // ── Condiciones de parada ────────────────────────────────────────────
        if (alt < -200.0) {
            result.exitReason = "crashed";
            break;
        }
        if (stageIdx >= N) {
            result.exitReason = "fuel depleted";
            break;
        }
        if (targetAlt > 0.0 && alt >= targetAlt) {
            result.exitReason = "target altitude reached";
            break;
        }

        // ── Empuje de la etapa activa ────────────────────────────────────────
        double Isp       = veh.getStage(stageIdx).getWeightedIsp();
        double maxThrust = veh.getStage(stageIdx).getMaxThrust();
        bool   burning   = stageIdx < N && stages[stageIdx].propLeft > 0.0 && Isp > 0.0;
        double thrust    = burning ? maxThrust * throttle : 0.0;
        double mflow     = (thrust > 0.0) ? thrust / (Isp * Constants::G0) : 0.0;
        dvec3  tDir      = thrustDir(pos, vel, time);

        // ── RK4 ──────────────────────────────────────────────────────────────
        // Derivada: [dpos/dt = vel, dvel/dt = accel]
        // La masa cambia lentamente → usamos el valor al inicio del paso.
        auto deriv = [&](const dvec3& p, const dvec3& v) -> std::pair<dvec3, dvec3> {
            dvec3 Fg = gravAccel(p) * mass;                           // N
            dvec3 Fd = dragForce(atm, body.radius, p, v, area, Cd);  // N
            dvec3 Ft = tDir * thrust;                                 // N
            dvec3 acc = (Fg + Fd + Ft) / mass;                       // m/s²
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
                // Etapa agotada → separación
                double burned = stages[stageIdx].propLeft;
                mass -= burned;
                stages[stageIdx].propLeft = 0.0;

                // Jettisona la masa seca de la etapa separada
                mass -= stages[stageIdx].dryMass;
                mass  = std::max(mass, 1.0);  // safety floor

                result.stagingTimes.push_back(time);
                result.stagesUsed++;
                stageIdx++;

                // Actualizar área aerodinámica al nuevo stage
                if (stageIdx < N)
                    area = crossSection(veh.getStage(stageIdx));
            }
        }

        // Actualizar estadísticas
        result.maxAltitude = std::max(result.maxAltitude, alt);
        result.maxSpeed    = std::max(result.maxSpeed,    spd);
        result.reachedSpace = result.reachedSpace || (alt >= karmanLine);

        time += dt;
        ++step;
    }

    result.totalBurnTime = time;

    // ── Elementos orbitales del estado final (si hay energía negativa) ───────
    {
        double r  = glm::length(pos);
        double v  = glm::length(vel);
        double e  = 0.5 * v * v - body.mu / r;  // energía específica orbital
        if (e < 0.0 && r > body.radius) {
            double a      = -body.mu / (2.0 * e);
            dvec3  h_vec  = glm::cross(pos, vel);
            double h2     = glm::dot(h_vec, h_vec);
            double ecc    = std::sqrt(std::max(0.0, 1.0 - h2 / (body.mu * a)));
            result.apoapsis  = a * (1.0 + ecc) - body.radius;
            result.periapsis = a * (1.0 - ecc) - body.radius;
        }
    }

    return result;
}

} // namespace Phoenix::Launch

#pragma once

#include <launch/LaunchVehicle.hpp>
#include <physics/CelestialBody.hpp>
#include <physics/Atmosphere.hpp>
#include <math/Constants.hpp>
#include <vector>
#include <string>

namespace Phoenix::Launch {

using namespace Math;

// ── Punto de la trayectoria de ascenso ───────────────────────────────────────

struct AscentPoint {
    double time;       ///< s desde el encendido
    dvec3  position;   ///< m, marco ECI (Earth-centered inertial)
    dvec3  velocity;   ///< m/s, marco ECI
    double mass;       ///< kg, masa actual del vehículo
    double altitude;   ///< m sobre la superficie del cuerpo de referencia
    double speed;      ///< m/s, módulo de velocidad
    double thrust;     ///< N, empuje actual
    double drag;       ///< N, arrastre aerodinámico actual
    int    stageIndex; ///< etapa activa en este instante (0 = booster)
};

// ── Resultado de la simulación de ascenso ────────────────────────────────────

struct AscentResult {
    std::vector<AscentPoint> trajectory;  ///< puntos muestreados de la trayectoria
    double maxAltitude   = 0.0;  ///< m — altitud máxima alcanzada
    double maxSpeed      = 0.0;  ///< m/s — velocidad máxima
    double apoapsis      = 0.0;  ///< m sobre superficie (del estado final)
    double periapsis     = 0.0;  ///< m sobre superficie
    double totalBurnTime = 0.0;  ///< s — tiempo total de quemado
    int    stagesUsed    = 0;    ///< número de etapas separadas
    bool   reachedSpace  = false;///< altitud > Karman line del cuerpo
    std::string exitReason;      ///< "target altitude" | "fuel depleted" | "crashed" | ...

    /// Tiempos de separación de etapa (s desde liftoff), indexados por stageIndex separado.
    std::vector<double> stagingTimes;
};

// ── Integrador de ascenso ────────────────────────────────────────────────────

/**
 * @class AscentIntegrator
 * @brief Integrador RK4 para la trayectoria de un cohete multi-etapa.
 *
 * Modela bajo empuje + gravedad + arrastre atmosférico (opcional).
 *
 * Fuerzas en cada paso:
 *   F_thrust = throttle * maxThrust * thrustDir
 *   F_gravity = -mu/|r|^3 * r * m
 *   F_drag    = -½ρv²CdA * v̂  (solo dentro de la atmósfera)
 *
 * Dirección de empuje (Phase 8B — gravity turn simple):
 *   t < 5 s   : vertical (radial desde el centro)
 *   5–60 s    : interpolación cuadrática vertical→prograde
 *   t ≥ 60 s  : prograde (dirección de velocidad)
 *
 * Separación de etapas:
 *   Cuando la etapa activa agota su propelante, se jettisona (masa seca
 *   eliminada) y la siguiente etapa empieza a quemar inmediatamente.
 */
class AscentIntegrator {
public:
    /**
     * @param vehicle  Vehículo de lanzamiento (LeunchVehicle ya construido)
     * @param body     Cuerpo de referencia (Tierra, etc.)
     * @param atm      Modelo de atmósfera. nullptr = vacío total.
     */
    AscentIntegrator(const LaunchVehicle&                vehicle,
                     const Physics::CelestialBody&       body,
                     const Physics::Atmosphere*          atm = nullptr);

    /** Throttle global [0,1]. Default 1.0 (full throttle). */
    void setThrottle(double t) { throttle = std::max(0.0, std::min(1.0, t)); }

    /**
     * Muestrear la trayectoria cada `n` pasos de integración.
     * Default 20 (guarda ≈ 1 punto por segundo con dt=0.05 s).
     */
    void setRecordInterval(int n) { recordInterval = std::max(1, n); }

    /**
     * Ejecuta la simulación de ascenso.
     *
     * @param dt            Paso de integración RK4 (s). Recomendado ≤ 0.1 s.
     * @param maxTime       Tiempo máximo de simulación (s). Safety limit.
     * @param targetAlt     Si > 0: parar cuando altitud ≥ targetAlt (m).
     *                      Si = 0: simular hasta agotar combustible.
     * @return AscentResult con trayectoria completa y métricas orbitales.
     */
    AscentResult simulate(double dt        = 0.05,
                          double maxTime   = 800.0,
                          double targetAlt = 0.0) const;

private:
    const LaunchVehicle&          veh;
    const Physics::CelestialBody& body;
    const Physics::Atmosphere*    atm;
    double throttle       = 1.0;
    int    recordInterval = 20;

    /// Aceleración gravitatoria en `pos` (m/s²)
    dvec3 gravAccel(const dvec3& pos) const;

    /// Aceleración de drag en `pos` con velocidad `vel` (m/s²)
    dvec3 dragAccel(const dvec3& pos, const dvec3& vel,
                    double mass, double area, double Cd) const;

    /// Dirección unitaria de empuje en función del tiempo y estado
    dvec3 thrustDir(const dvec3& pos, const dvec3& vel, double t) const;

    /// Área de referencia del vehículo (m²) desde la sección del primer stage activo
    static double crossSection(const StageConfig& s);
};

} // namespace Phoenix::Launch

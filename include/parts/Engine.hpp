#pragma once

#include <parts/Part.hpp>
#include <string>

namespace Phoenix::Parts
{

    using namespace Math;

    /**
     * @class Engine
     * @brief Motor cohete — extiende Part con propiedades de propulsión.
     *
     * Modela un motor de propergol líquido bipropelante. El combustible
     * se consume del depósito más cercano en el árbol (o del propio motor
     * si tiene reserva interna). La ecuación de empuje es:
     *
     *   F = throttle * maxThrust          (N)
     *   ṁ = F / (Isp * g0)               (kg/s, flujo másico)
     *
     * La velocidad de exhaust efectiva es:
     *   v_e = Isp * g0
     *
     * El ΔV disponible sigue la ecuación de Tsiolkovsky:
     *   ΔV = Isp * g0 * ln(m0 / mf)
     */
    class Engine : public Part
    {
    public:
        // ── Parámetros del motor ─────────────────────────────────────────────────
        double maxThrust; ///< Empuje máximo al vacío (N)
        double Isp;       ///< Impulso específico al vacío (s)
        double throttle;  ///< Throttle actual [0.0, 1.0]
        bool isRunning;   ///< true mientras el motor está encendido

        // ── Constructor ──────────────────────────────────────────────────────────

        /**
         * @param name_      Identificador del motor
         * @param dryMass_   Masa seca del motor (kg) — sin propelante
         * @param maxThrust_ Empuje máximo al vacío (N)
         * @param Isp_       Impulso específico al vacío (s)
         */
        Engine(const std::string &name_,
               double dryMass_,
               double maxThrust_,
               double Isp_);

        ~Engine() = default;

        // ── Empuje y flujo ───────────────────────────────────────────────────────

        /** Empuje actual = throttle * maxThrust (N). */
        double getCurrentThrust() const;

        /**
         * Flujo másico actual (kg/s).
         * ṁ = F / (Isp * g0)
         */
        double getMassFlowRate() const;

        /**
         * Velocidad de exhaust efectiva (m/s).
         * v_e = Isp * g0
         */
        double getExhaustVelocity() const;

        // ── Control ──────────────────────────────────────────────────────────────

        /** Enciende el motor (throttle permanece sin cambios). */
        void ignite();

        /** Apaga el motor (throttle → 0). */
        void shutdown();

        /**
         * Ajusta el throttle. Clampea a [0, 1].
         * Si throttle == 0 el motor se apaga automáticamente.
         */
        void setThrottle(double t);

        // ── Ecuación de cohete ───────────────────────────────────────────────────

        /**
         * ΔV disponible dado el combustible total accesible (kg).
         * Usa la ecuación de Tsiolkovsky: ΔV = v_e * ln(m0 / mf)
         *
         * @param totalFuelMass   Masa total de propelante disponible (kg)
         * @param dryVehicleMass  Masa seca del vehículo completo (kg)
         * @return ΔV en m/s; 0 si no hay combustible o motor apagado.
         */
        double computeDeltaV(double totalFuelMass, double dryVehicleMass) const;

        /**
         * Tiempo de quemado para un ΔV dado.
         * t_burn = (m0 / ṁ) * (1 - e^(-ΔV / v_e))
         *
         * @param deltaV         ΔV deseado (m/s)
         * @param totalFuelMass  Masa de propelante disponible (kg)
         * @param vehicleMass    Masa total del vehículo al inicio del burn (kg)
         * @return Tiempo de quemado (s); -1 si no factible con el combustible dado.
         */
        double computeBurnTime(double deltaV,
                               double totalFuelMass,
                               double vehicleMass) const;
    };

} // namespace Phoenix::Parts

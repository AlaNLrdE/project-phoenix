#pragma once

#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>
#include <physics/AeroForces.hpp>
#include <parts/Part.hpp>
#include <parts/Engine.hpp>
#include <math/Constants.hpp>
#include <string>
#include <vector>
#include <memory>

namespace Phoenix::Vessels
{

    using namespace Math;
    using namespace Physics;
    using namespace Parts;

    /**
     * @class Vessel
     * @brief Representa una nave espacial.
     *
     * En Phase 1, la nave es un punto de masa. En fases posteriores,
     * se añadirá la jerarquía de partes, propulsión, etc.
     */
    class Vessel
    {
    public:
        // Identificadores y estado
        std::string name;   ///< Nombre de la nave
        std::string status; ///< Estado: "active", "docked", "crashed", etc.

        // Masa y geometría (Phase 2+)
        double dryMass;  ///< Masa en seco (kg)
        double fuelMass; ///< Masa de combustible (kg)

        // Estado orbital
        Orbit orbit;        ///< Órbita actual
        double currentTime; ///< Tiempo actual de simulación (segundos desde epoch)

        // Referencias celestes
        std::string referenceBodyName; ///< Cuerpo alrededor del cual orbita
        CelestialBody *referenceBody;  ///< Puntero al cuerpo de referencia

        // ── Phase 2: Jerarquía de partes ────────────────────────────────────
        /// Raíz del árbol de partes. nullptr = nave como punto de masa (Phase 1).
        std::shared_ptr<Part> rootPart;

        // ── Phase 5: Aerodinámica ────────────────────────────────────────────
        double crossSectionalArea; ///< m² — reference area for drag (default 10.0)
        double dragCoefficient;    ///< Cd — dimensionless drag coefficient (default 0.5)
        double noseRadius;         ///< m  — nose radius for Chapman heat flux (default 0.5)

        /**
         * Constructor.
         *
         * @param name_ Nombre de la nave
         * @param dryMass_ Masa seca (kg)
         * @param orbit_ Órbita inicial
         * @param refBodyName Nombre del cuerpo de referencia
         * @param refBody Puntero al cuerpo de referencia
         */
        Vessel(const std::string &name_, double dryMass_,
               const Orbit &orbit_, const std::string &refBodyName,
               CelestialBody *refBody);

        /**
         * Destructor.
         */
        ~Vessel() = default;

        /**
         * Obtiene la masa total.
         * Si rootPart está definido, se calcula desde el árbol de partes.
         * En caso contrario, devuelve dryMass + fuelMass (modo Phase 1).
         */
        double getTotalMass() const;

        /**
         * Obtiene la posición en el marco inercial del cuerpo de referencia.
         *
         * @param t Tiempo de simulación (segundos)
         * @return Posición (metros)
         */
        dvec3 getPosition(double t) const;

        /**
         * Obtiene la velocidad en el marco inercial.
         *
         * @param t Tiempo de simulación (segundos)
         * @return Velocidad (m/s)
         */
        dvec3 getVelocity(double t) const;

        /**
         * Obtiene ambos vectores de estado.
         * Más eficiente que llamarlos por separado.
         */
        void getState(double t, dvec3 &position, dvec3 &velocity) const;

        /**
         * Actualiza la órbita manualmente.
         * Usado después de maniobras o impulsos.
         */
        void updateOrbit(const Orbit &newOrbit);

        /**
         * Aplica un impulso deltaV instantáneo a la nave.
         *
         * @param deltaV Cambio de velocidad (m/s)
         * @param direction Dirección unitaria del impulso
         */
        void applyDeltaV(double deltaV, const dvec3 &direction);

        /**
         * Consume combustible.
         *
         * @param mass Masa a consumir (kg)
         * @return true si había suficiente combustible
         */
        bool consumeFuel(double mass);

        /**
         * Obtiene la altitud sobre el cuerpo de referencia.
         */
        double getAltitude() const;

        /**
         * Obtiene velocidad relativa respecto al cuerpo de referencia (si rota).
         * En Phase 5, esto considerará la rotación del cuerpo.
         */
        dvec3 getRelativeVelocity() const;

        /**
         * Imprime el estado de la nave (debug).
         */
        void printStatus() const;

        // ── Phase 2: Part hierarchy API ──────────────────────────────────────

        /** Asigna la raíz del árbol de partes. */
        void setRootPart(std::shared_ptr<Part> root);

        /**
         * Devuelve punteros a todas las partes activas del árbol.
         * Vacío si la nave está en modo punto-de-masa (rootPart == nullptr).
         */
        std::vector<Part *> getAllParts();

        /**
         * Centro de masa del vehículo en el marco local de la nave.
         * Retorna (0,0,0) si no hay árbol de partes.
         */
        dvec3 getCenterOfMass() const;

        /**
         * Dispara el primer Decoupler activo del árbol.
         * Separa el árbol en dos: la nave actual conserva la parte superior
         * y retorna una nueva Vessel con la etapa inferior.
         * @return Nueva Vessel separada; nullptr si no hay Decoupler.
         */
        std::shared_ptr<Vessel> stage();

        /**
         * Acopla otra nave: mueve su rootPart como hijo del puerto indicado.
         * Tras el acoplamiento, other.rootPart queda vacío.
         * @param other         Nave a acoplar (se modifica in-place).
         * @param ownPortName   Nombre del DockingPort en esta nave.
         * @param otherPortName Nombre del DockingPort en la otra nave.
         * @return true si ambos puertos se encontraron y el acoplamiento fue exitoso.
         */
        bool dock(Vessel &other,
                  const std::string &ownPortName,
                  const std::string &otherPortName);

        /**
         * Desacopla la nave anclada al puerto indicado.
         * @param portName Nombre del DockingPort por el que se desacopla.
         * @return Nueva Vessel con las partes desacopladas; nullptr si no encontrado.
         */
        std::shared_ptr<Vessel> undock(const std::string &portName);

        // ── Phase 3: Propulsion API ─────────────────────────────────────────

        /**
         * Devuelve punteros a todos los motores activos del árbol de partes.
         */
        std::vector<Engine *> getActiveEngines();

        /**
         * Enciende todos los motores disponibles.
         */
        void igniteEngines();

        /**
         * Apaga todos los motores.
         */
        void shutdownEngines();

        /**
         * Ajusta el throttle de todos los motores activos.
         * @param t Throttle [0.0, 1.0]
         */
        void setThrottle(double t);

        /**
         * Masa total de combustible disponible en todos los depósitos activos.
         */
        double getTotalFuelMass() const;

        /**
         * Empuje total actual de todos los motores encendidos (N).
         */
        double getTotalThrust() const;

        /**
         * ΔV disponible aplicando la ecuación de Tsiolkovsky a la configuración
         * actual (motores + depósitos).
         * @return ΔV en m/s; 0 si no hay motores o combustible.
         */
        double computeAvailableDeltaV() const;

        /**
         * Simula un quemado finito de duración burnTime (segundos).
         *
         * - Consume combustible proporcional al flujo másico.
         * - Aplica el ΔV acumulado como impulso instantáneo al final
         *   (aproximación de impulso impulsivo).
         * - La dirección del empuje es la velocidad orbital normalizada
         *   (pro-grado) por defecto.
         *
         * @param burnTime  Duración del quemado (s).
         * @param direction Dirección unitaria del empuje en el marco inercial.
         *                  Si es cero, se usa la dirección pro-grado.
         * @return ΔV real aplicado (m/s); 0 si no hay motores encendidos.
         */
        double executeBurn(double burnTime, dvec3 direction = dvec3(0.0));

        // ── Phase 5: Aerodinámica y reentrada ───────────────────────────────

        /**
         * Simula una trayectoria de reentrada atmosférica usando integración
         * numérica RK4. Toma posición y velocidad actuales de la órbita.
         *
         * La integración combina gravedad e inyectividad del arrastre aerodinámico
         * hasta que la nave aterriza (altitud <= 0), se destruye por calor excesivo,
         * o se agota el número máximo de pasos.
         *
         * @param atm       Modelo de atmósfera para el cuerpo de referencia
         * @param dt        Paso de integración (s). Usar ≤ 0.1 s para precisión.
         * @param maxSteps  Número máximo de pasos RK4
         * @return          Trayectoria y resultado de la reentrada
         */
        Physics::AeroTrajectoryResult simulateReentry(
            const Physics::Atmosphere &atm,
            double dt = 0.05,
            int maxSteps = 100000) const;
    };

} // namespace Phoenix::Vessels

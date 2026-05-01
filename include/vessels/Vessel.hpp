#pragma once

#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>
#include <math/Constants.hpp>
#include <string>
#include <vector>
#include <memory>

namespace Phoenix::Vessels
{

    using namespace Math;
    using namespace Physics;

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
         * Obtiene la masa total (seca + combustible).
         */
        double getTotalMass() const { return dryMass + fuelMass; }

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
    };

} // namespace Phoenix::Vessels

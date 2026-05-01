#pragma once

#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>
#include <parts/Part.hpp>
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
    };

} // namespace Phoenix::Vessels

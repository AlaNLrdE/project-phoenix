#pragma once

#include <math/Constants.hpp>
#include <string>
#include <vector>
#include <memory>

namespace Phoenix::Parts
{

    using namespace Math;

    /**
     * @enum PartType
     * @brief Tipos funcionales de partes de una nave espacial.
     */
    enum class PartType
    {
        Command,     ///< Cápsula de comando o núcleo de sonda
        FuelTank,    ///< Depósito de combustible líquido
        Engine,      ///< Motor cohete
        Decoupler,   ///< Separador de etapas (consumido al disparar)
        Parachute,   ///< Paracaídas de recuperación
        Structure,   ///< Elemento estructural pasivo
        DockingPort, ///< Puerto de atraque
        Unknown      ///< Tipo desconocido / genérico
    };

    /**
     * @class Part
     * @brief Representa una parte individual de una nave espacial.
     *
     * Las partes forman un árbol jerárquico (raíz → hijos).
     * Cada parte tiene:
     *  - Masa seca y combustible propio (si aplica)
     *  - Posición local relativa al padre (para cálculo de CoM)
     *  - Lista de hijos (owned via shared_ptr)
     *  - Puntero al padre (raw, sin ownership)
     *
     * Phase 2: jerarquía de partes, CoM dinámico, staging y docking básico.
     */
    class Part
    {
    public:
        std::string name;    ///< Nombre único dentro de la nave
        PartType type;       ///< Tipo funcional de la parte
        double dryMass;      ///< Masa seca (kg)
        double fuelMass;     ///< Combustible actual (kg)
        double maxFuelMass;  ///< Capacidad máxima de combustible (kg)
        dvec3 localPosition; ///< Posición relativa al padre (metros)
        bool isActive;       ///< false si fue separada o destruida

        Part *parent;                                ///< Padre (sin ownership, nullptr si es raíz)
        std::vector<std::shared_ptr<Part>> children; ///< Hijos directos (owned)

        // ── Constructor / Destructor ─────────────────────────────────────────────

        /**
         * @param name_    Nombre identificador
         * @param type_    Tipo de parte
         * @param dryMass_ Masa seca (kg)
         * @param maxFuel  Capacidad de combustible (kg). 0 para partes sin combustible.
         */
        Part(const std::string &name_, PartType type_,
             double dryMass_, double maxFuel = 0.0);

        ~Part() = default;

        // ── Masa ─────────────────────────────────────────────────────────────────

        /** Masa de esta parte sola (seca + combustible, sin hijos). */
        double getOwnMass() const { return dryMass + fuelMass; }

        /** Masa total del árbol enraizado en esta parte (recursivo). */
        double getTreeMass() const;

        // ── Centro de masa ───────────────────────────────────────────────────────

        /**
         * Centro de masa del árbol en el sistema de coordenadas de esta parte.
         * Esta parte se sitúa en el origen de su propio marco local.
         * Para la raíz del vehículo, el resultado es el CoM en el marco de la nave.
         */
        dvec3 getTreeCoM() const;

        // ── Jerarquía ────────────────────────────────────────────────────────────

        /** Agrega una parte como hija directa (ajusta child->parent). */
        void addChild(std::shared_ptr<Part> child);

        /**
         * Devuelve punteros a todas las partes activas del árbol (DFS preorder).
         * Los punteros son válidos mientras el árbol exista.
         */
        std::vector<Part *> getAllParts();

        /**
         * Desconecta esta parte de su padre y devuelve su shared_ptr.
         * Los hijos permanecen intactos bajo esta parte.
         * @return shared_ptr a esta parte; nullptr si ya es raíz.
         */
        std::shared_ptr<Part> detachFromParent();

        // ── Combustible ──────────────────────────────────────────────────────────

        /**
         * Consume combustible de esta parte (no propaga a hijos).
         * @return true si había suficiente combustible.
         */
        bool consumeFuel(double amount);

        /**
         * Agrega combustible respetando la capacidad máxima.
         * @return Cantidad realmente añadida (kg).
         */
        double addFuel(double amount);

        /** Fracción de llenado [0,1]. Retorna 0 si no es depósito. */
        double getFuelFraction() const;

        // ── Utilidades ───────────────────────────────────────────────────────────

        /** Nombre legible del PartType dado. */
        static std::string typeToString(PartType t);
    };

} // namespace Phoenix::Parts

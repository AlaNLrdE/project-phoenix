#pragma once

#include <physics/CelestialBody.hpp>
#include <vessels/Vessel.hpp>
#include <math/Constants.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace Phoenix::World
{

    using namespace Math;
    using namespace Physics;
    using namespace Vessels;

    /**
     * @class WorldManager
     * @brief Gestor central del universo.
     *
     * Responsable de:
     * - Almacenar y gestionar cuerpos celestes
     * - Almacenar y gestionar naves
     * - Coordinar la propagación temporal
     * - En Phase 1 (actual): Almacenamiento básico
     * - En Phase 4+: Desplazamiento flotante (Origin Floating)
     */
    class WorldManager
    {
    public:
        /**
         * Constructor.
         */
        WorldManager();

        /**
         * Destructor.
         */
        ~WorldManager();

        /**
         * Registra un cuerpo celeste en el universo.
         *
         * @param body Puntero al cuerpo (el WorldManager toma posesión)
         */
        void registerCelestialBody(CelestialBody *body);

        /**
         * Obtiene un cuerpo celeste por nombre.
         *
         * @param name Nombre del cuerpo
         * @return Puntero al cuerpo, o nullptr si no existe
         */
        CelestialBody *getCelestialBody(const std::string &name) const;

        /**
         * Registra una nave en el universo.
         *
         * @param vessel Puntero a la nave
         */
        void registerVessel(Vessel *vessel);

        /**
         * Obtiene una nave por nombre.
         *
         * @param name Nombre de la nave
         * @return Puntero a la nave, o nullptr si no existe
         */
        Vessel *getVessel(const std::string &name) const;

        /**
         * Establece la nave activa.
         * Relevante para Phase 4+ (desplazamiento flotante).
         *
         * @param vesselName Nombre de la nave a activar
         */
        void setActiveVessel(const std::string &vesselName);

        /**
         * Obtiene la nave activa.
         */
        Vessel *getActiveVessel() const { return activeVessel; }

        /**
         * Avanza la simulación un paso de tiempo.
         *
         * @param deltaTime Incremento de tiempo (segundos)
         */
        void updateSimulation(double deltaTime);

        /**
         * Establece el tiempo global de simulación.
         *
         * @param t Tiempo en segundos desde epoch
         */
        void setSimulationTime(double t) { simulationTime = t; }

        /**
         * Obtiene el tiempo actual de simulación.
         */
        double getSimulationTime() const { return simulationTime; }

        /**
         * Obtiene el factor de time warp (Phase 1: siempre 1.0).
         */
        int getTimeWarp() const { return timeWarp; }

        /**
         * Establece el factor de time warp.
         * En Phase 3 alto, se ignoran colisiones y se usa propagación analítica.
         */
        void setTimeWarp(int warp) { timeWarp = (warp < 1) ? 1 : warp; }

        /**
         * Listar todos los cuerpos celestes (debug).
         */
        void printCelestialBodies() const;

        /**
         * Listar todas las naves (debug).
         */
        void printVessels() const;

        /**
         * Obtiene todas las naves.
         */
        const std::vector<Vessel *> &getAllVessels() const { return vessels; }

        /**
         * Obtiene todos los cuerpos celestes.
         */
        const std::map<std::string, CelestialBody *> &getAllBodies() const
        {
            return celestialBodies;
        }

    private:
        std::map<std::string, CelestialBody *> celestialBodies; ///< Mapa de cuerpos
        std::vector<Vessel *> vessels;                          ///< Naves activas
        Vessel *activeVessel;                                   ///< Nave foco de cámara

        double simulationTime; ///< Tiempo actual (s)
        int timeWarp;          ///< Factor de aceleración

        dvec3 floatingOrigin; ///< Desplazamiento flotante (Phase 4+)
    };

} // namespace Phoenix::World

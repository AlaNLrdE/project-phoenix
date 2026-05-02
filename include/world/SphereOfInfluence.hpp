#pragma once

#include <physics/CelestialBody.hpp>
#include <vessels/Vessel.hpp>
#include <math/Constants.hpp>
#include <string>
#include <map>

namespace Phoenix::World
{

    using namespace Math;
    using namespace Physics;
    using namespace Vessels;

    /**
     * @class SphereOfInfluence
     * @brief Utilidades estáticas para el modelo de esferas de influencia (SoI).
     *
     * Implementa la aproximación de cónicas empalmadas (patched conics) usada en KSP:
     * cada nave está en la SoI de exactamente un cuerpo celeste a la vez.
     *
     * El radio de la SoI de un cuerpo m₁ orbitando m₂ es:
     *   r_SoI = a · (m₁/m₂)^(2/5)
     *
     * Las transiciones de SoI requieren convertir el estado orbital a un nuevo marco
     * de referencia:
     *   pos_new = pos_world - body_new.worldPos
     *   vel_new = vel_world - body_new.worldVel
     *   orbit_new = Orbit(pos_new, vel_new, body_new.mu, t)
     */
    class SphereOfInfluence
    {
    public:
        // ── Cálculos básicos ──────────────────────────────────────────────────────

        /**
         * Calcula el radio de la SoI.
         * @param sma       Semieje mayor de la órbita del cuerpo hijo alrededor del padre (m).
         * @param childMass Masa del cuerpo hijo (kg).
         * @param parentMass Masa del cuerpo padre (kg).
         * @return Radio de la SoI en metros.
         */
        static double computeRadius(double sma, double childMass, double parentMass);

        /**
         * Verifica si una posición relativa al centro del cuerpo está dentro de su SoI.
         * @param relativePos Posición relativa al centro del cuerpo (metros).
         * @param soiRadius   Radio de la SoI (metros).
         */
        static bool isInside(const dvec3 &relativePos, double soiRadius);

        // ── Transiciones de SoI ───────────────────────────────────────────────────

        /**
         * Resultado de una comprobación de transición de SoI.
         */
        struct TransitionResult
        {
            bool hasTransition = false;       ///< true si se detectó una transición
            CelestialBody *newBody = nullptr; ///< Nuevo cuerpo de referencia
            dvec3 newPosition{0.0};           ///< Posición en el nuevo marco
            dvec3 newVelocity{0.0};           ///< Velocidad en el nuevo marco
        };

        /**
         * Detecta si una nave debe cambiar su cuerpo de referencia.
         *
         * Comprueba dos casos:
         *  1. Entrada en SoI de un hijo del cuerpo actual: nave se acerca a un satélite.
         *  2. Salida de la SoI del cuerpo actual: nave se aleja hacia el padre.
         *
         * @param vessel     Nave cuyo SoI se verifica.
         * @param allBodies  Todos los cuerpos registrados en el WorldManager.
         * @param t          Tiempo de simulación actual (s).
         * @return TransitionResult con la transición detectada (si existe).
         */
        static TransitionResult checkTransition(
            const Vessel &vessel,
            const std::map<std::string, CelestialBody *> &allBodies,
            double t);
    };

} // namespace Phoenix::World

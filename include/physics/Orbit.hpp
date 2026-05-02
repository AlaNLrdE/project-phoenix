#pragma once

#include <math/Constants.hpp>
#include <glm/glm.hpp>
#include <cmath>
#include <stdexcept>

namespace Phoenix::Physics
{

    using namespace Math;

    /**
     * @class Orbit
     * @brief Representa una órbita Kepleriana mediante elementos orbitales clásicos.
     *
     * Mantiene los 6 elementos orbitales clásicos y proporciona métodos para:
     * - Propagar la posición y velocidad en el tiempo (ecuación de Kepler)
     * - Convertir entre elementos orbitales y vectores de estado
     * - Calcular anomalías (verdadera, excéntrica, media)
     */
    class Orbit
    {
    public:
        // Elementos orbitales (TLE-style denominación)
        double a;     ///< Semieje mayor (metros)
        double e;     ///< Excentricidad (0 ≤ e < 1 para órbitas elípticas)
        double i;     ///< Inclinación (radianes, 0 ≤ i ≤ π)
        double Omega; ///< Longitud del nodo ascendente (radianes, 0 ≤ Ω < 2π)
        double omega; ///< Argumento del periapsis (radianes, 0 ≤ ω < 2π)
        double nu;    ///< Anomalía verdadera (radianes, 0 ≤ ν < 2π)

        double mu;    ///< Parámetro gravitacional del cuerpo central (m³/s²)
        double epoch; ///< Época de referencia (tiempo en segundos)
        double M0;    ///< Anomalía media en la época (radianes)

        /**
         * Constructor por defecto. Crea una órbita circular ecuatorial.
         */
        Orbit();

        /**
         * Constructor con elementos orbitales clásicos.
         *
         * @param a_ Semieje mayor (metros)
         * @param e_ Excentricidad
         * @param i_ Inclinación (radianes)
         * @param Omega_ Longitud del nodo ascendente (radianes)
         * @param omega_ Argumento del periapsis (radianes)
         * @param nu_ Anomalía verdadera (radianes)
         * @param mu_ Parámetro gravitacional
         * @param epoch_ Época de referencia
         */
        Orbit(double a_, double e_, double i_, double Omega_, double omega_,
              double nu_, double mu_, double epoch_ = 0.0);

        /**
         * Constructor a partir de vectores de estado (posición y velocidad).
         *
         * @param position Posición orbital (m)
         * @param velocity Velocidad orbital (m/s)
         * @param mu_ Parámetro gravitacional
         * @param epoch_ Época de referencia
         */
        Orbit(const dvec3 &position, const dvec3 &velocity, double mu_, double epoch_ = 0.0);

        /**
         * Obtiene la posición en el espacio 3D en un tiempo dado.
         *
         * @param t Tiempo desde la época (segundos)
         * @return Posición en coordenadas cartesianas (metros)
         */
        dvec3 getPositionAtTime(double t) const;

        /**
         * Obtiene la velocidad en el espacio 3D en un tiempo dado.
         *
         * @param t Tiempo desde la época (segundos)
         * @return Velocidad en coordenadas cartesianas (m/s)
         */
        dvec3 getVelocityAtTime(double t) const;

        /**
         * Obtiene ambos vectores de estado (posición y velocidad) en un tiempo dado.
         * Más eficiente que llamar por separado.
         *
         * @param t Tiempo desde la época (segundos)
         * @param[out] position Posición orbital (m)
         * @param[out] velocity Velocidad orbital (m/s)
         */
        void getStateAtTime(double t, dvec3 &position, dvec3 &velocity) const;

        /**
         * Actualiza los elementos orbitales manteniendo la época.
         * Usado en propagación por pasos.
         */
        void updateElements(double a_, double e_, double i_, double Omega_,
                            double omega_, double nu_);

        /**
         * Calcula el período orbital (segundos).
         */
        double getPeriod() const;

        /**
         * Calcula la altitud sobre la superficie de un cuerpo (aproximado a radio esférico).
         *
         * @param bodyRadius Radio del cuerpo central (metros)
         * @return Altitud (metros) - negativi si está bajo tierra
         */
        double getAltitude(double bodyRadius) const;

        /**
         * Calcula la distancia actual al periapsis (metros).
         */
        double getDistanceToPeriapsis() const;

        /**
         * Calcula la distancia actual al apoapsis (metros).
         */
        double getDistanceToApoapsis() const;

        /**
         * Verifica si la órbita es estable (e < 1).
         */
        bool isStable() const { return e < 1.0; }

        /**
         * Verifica si la órbita es parabólica (e ≈ 1).
         */
        bool isParabolic() const { return std::abs(e - 1.0) < 1e-6; }

        /**
         * Verifica si la órbita es hiperbólica (e > 1).
         */
        bool isHyperbolic() const { return e > 1.0; }

    private:
        /**
         * Resuelve la ecuación de Kepler: M = E - e·sin(E)
         * Retorna la anomalía excéntrica E dado M (anomalía media).
         * Usa método de Newton-Raphson.
         *
         * @param M Anomalía media (radianes)
         * @return Anomalía excéntrica E (radianes)
         */
        double solveKeplersEquation(double M) const;

        /**
         * Convierte anomalía excéntrica a anomalía verdadera.
         * ν = 2·arctan( sqrt((1+e)/(1-e)) · tan(E/2) )
         */
        double eccentricToTrueAnomaly(double E) const;

        /**
         * Convierte anomalía verdadera a anomalía excéntrica.
         */
        double trueToEccentricAnomaly(double nu_) const;

        /**
         * Calcula los vectores de estado en el plano orbital (2D).
         * @param nu_ Anomalía verdadera
         * @param[out] r_orb Posición en plano orbital (m)
         * @param[out] v_orb Velocidad en plano orbital (m/s)
         */
        void getOrbitalPlaneState(double nu_, dvec3 &r_orb, dvec3 &v_orb) const;

        /**
         * Transforma un vector del plano orbital al marco inercial 3D.
         * Aplica las rotaciones de (Ω, i, ω).
         */
        dvec3 orbitalToInertial(const dvec3 &r_orb) const;
    };

} // namespace Phoenix::Physics

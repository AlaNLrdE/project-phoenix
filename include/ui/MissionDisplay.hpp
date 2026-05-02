#pragma once

#include <vessels/Vessel.hpp>
#include <physics/CelestialBody.hpp>
#include <ostream>
#include <iostream>

namespace Phoenix::UI
{

    using namespace Vessels;
    using namespace Physics;

    /**
     * @class MissionDisplay
     * @brief HUD de telemetría de vuelo para visualización en terminal.
     *
     * Proporciona un conjunto de funciones estáticas que formatean y muestran
     * el estado completo de una misión: panel de control principal, tabla de
     * elementos orbitales, informe de combustible y nodo de maniobra.
     *
     * Todas las funciones escriben en un `std::ostream` (por defecto std::cout)
     * con caracteres Unicode para bordes de tabla.
     */
    class MissionDisplay
    {
    public:
        /**
         * Panel de control de misión completo.
         * Muestra nombre de nave, tiempo, cuerpo de referencia, estado,
         * altitud, velocidad, periapsis/apoapsis, excentricidad e inclinación.
         *
         * @param vessel Nave a mostrar.
         * @param body   Cuerpo de referencia (para radio y nombre).
         * @param t      Tiempo de simulación actual (segundos).
         * @param os     Stream de salida.
         */
        static void printDashboard(const Vessel &vessel, const CelestialBody &body,
                                   double t, std::ostream &os = std::cout);

        /**
         * Tabla de los 6 elementos orbitales Keplerianos más altitudes
         * de periapsis/apoapsis y período orbital.
         *
         * @param orbit Órbita a describir.
         * @param body  Cuerpo de referencia (radio para calcular altitudes).
         * @param os    Stream de salida.
         */
        static void printOrbitalElements(const Orbit &orbit, const CelestialBody &body,
                                         std::ostream &os = std::cout);

        /**
         * Informe de combustible: masas, barra de progreso y ΔV disponible
         * (si la nave tiene motores en el árbol de partes).
         *
         * @param vessel Nave cuyo combustible se reporta.
         * @param os     Stream de salida.
         */
        static void printFuelReport(const Vessel &vessel, std::ostream &os = std::cout);

        /**
         * Nodo de maniobra: nombre, ΔV requerido y tiempo de quemado estimado.
         *
         * @param name     Identificador de la maniobra.
         * @param deltaV   ΔV total (m/s).
         * @param burnTime Duración estimada del quemado (s).
         * @param os       Stream de salida.
         */
        static void printManeuverNode(const std::string &name, double deltaV,
                                      double burnTime, std::ostream &os = std::cout);

    private:
        /** Genera una barra de progreso estilo [####----] de longitud `width`. */
        static std::string progressBar(double fraction, int width = 28);

        /** Formatea un tiempo (s) como "HHHh MMm SSs". */
        static std::string formatTime(double t);
    };

} // namespace Phoenix::UI

#pragma once

#include <physics/Orbit.hpp>
#include <physics/AeroForces.hpp>
#include <math/Constants.hpp>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>

namespace Phoenix::UI
{

    using namespace Math;
    using namespace Physics;

    /**
     * @class AsciiRenderer
     * @brief Dibuja órbitas y trayectorias como arte ASCII en un canvas 2D.
     *
     * Soporta dos modos:
     *  - Mapa orbital: muestra la elipse orbital y el cuerpo central en el
     *    plano perifocal (X = dirección periapsis, Y = perpendicular orbital).
     *  - Perfil de reentrada: gráfica altitud vs tiempo con velocidad superpuesta.
     *
     * El canvas es una cuadrícula de caracteres `width × height`. Las
     * coordenadas del mundo se proyectan mediante un viewport ajustable.
     *
     * Nota: los caracteres de texto son ~2× más altos que anchos, por lo que
     * el viewport X se ensancha automáticamente para reducir la distorsión.
     */
    class AsciiRenderer
    {
    public:
        /**
         * @param width  Anchura del canvas en caracteres (sin bordes).
         * @param height Altura del canvas en caracteres (sin bordes).
         */
        AsciiRenderer(int width = 78, int height = 22);

        /** Rellena el canvas con espacios. */
        void clear();

        /** Coloca un carácter en coordenadas de canvas (origen arriba-izquierda). */
        void putChar(int x, int y, char c);

        /**
         * Define el viewport en coordenadas del mundo (metros).
         * Las llamadas posteriores a draw* usarán este mapeo.
         */
        void setViewport(double xmin, double xmax, double ymin, double ymax);

        /**
         * Dibuja el cuerpo central (centrado en el origen del mundo)
         * como un bloque sólido de radio `worldRadius`.
         */
        void drawBody(double worldRadius, char c = 'O');

        /**
         * Dibuja la elipse orbital en el plano perifocal.
         * Muestrea `samples` puntos de anomalía verdadera uniformemente.
         */
        void drawOrbit(const Orbit &orbit, char c = '*', int samples = 500);

        /** Dibuja los ejes cartesianos (X='-', Y=':') que pasen por el origen. */
        void drawAxes();

        /**
         * Escribe el canvas al stream de salida con borde y título opcional.
         * @param os    Stream de destino.
         * @param title Título centrado en el borde superior (vacío = sin título).
         */
        void render(std::ostream &os = std::cout, const std::string &title = "") const;

        // ─── Métodos de conveniencia (estáticos) ──────────────────────────

        /**
         * Muestra un mapa ASCII de la órbita dada.
         * El viewport se ajusta automáticamente al tamaño de la órbita.
         * P = Periapsis, A = Apoapsis, * = Órbita, O = Cuerpo central.
         *
         * @param orbit      Órbita a visualizar (en plano perifocal).
         * @param bodyRadius Radio del cuerpo central (metros).
         * @param title      Título del mapa.
         */
        static void displayOrbit(const Orbit &orbit, double bodyRadius,
                                 const std::string &title = "Mapa Orbital");

        /**
         * Muestra un perfil ASCII de altitud-tiempo de una simulación de reentrada.
         * (*) = altitud,  (:) = velocidad (normalizada a escala de altitud).
         *
         * @param result Resultado de AeroForces::simulate().
         * @param title  Título del gráfico.
         */
        static void displayReentryProfile(const AeroTrajectoryResult &result,
                                          const std::string &title = "Perfil de Reentrada");

    private:
        int width_;
        int height_;
        std::vector<std::vector<char>> canvas_;
        double xmin_, xmax_, ymin_, ymax_;

        /** Convierte coordenadas del mundo a posición de canvas (col, row). */
        std::pair<int, int> worldToCanvas(double wx, double wy) const;
    };

} // namespace Phoenix::UI

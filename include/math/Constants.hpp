#pragma once

#include <glm/glm.hpp>
#include <cmath>

namespace Phoenix::Math {

/// Constantes astronómicas y físicas
namespace Constants {
    // Parámetro gravitacional estándar para la Tierra (m³/s²)
    constexpr double MU_EARTH = 3.986004418e14;
    
    // Parámetro gravitacional para el Sol (m³/s²)
    constexpr double MU_SUN = 1.32712440018e20;
    
    // Radio ecuatorial de la Tierra (metros)
    constexpr double EARTH_RADIUS = 6378137.0;
    
    // Escala KSP: 1/10 del tamaño real
    constexpr double KSP_SCALE = 0.1;
    
    // Gravedad superficial estándar (m/s²)
    constexpr double G0 = 9.81;
    
    // Tolerancia numérica para ecuación de Kepler
    constexpr double KEPLER_TOLERANCE = 1e-10;
    
    // Máximo de iteraciones para resolver ecuación de Kepler
    constexpr int KEPLER_MAX_ITERATIONS = 100;
} // namespace Constants

/// Tipos numéricos de precisión
using dvec3 = glm::dvec3;  // Vector 3D de doble precisión (posición, velocidad)
using dquat = glm::dquat;  // Cuaternión de doble precisión (rotaciones)
using dmat3 = glm::dmat3;  // Matriz 3x3 de doble precisión
using dmat4 = glm::dmat4;  // Matriz 4x4 de doble precisión

/// Conversiones de unidades
namespace Units {
    // De grados a radianes
    constexpr double DEG_TO_RAD(double deg) { return deg * M_PI / 180.0; }
    
    // De radianes a grados
    constexpr double RAD_TO_DEG(double rad) { return rad * 180.0 / M_PI; }
}

} // namespace Phoenix::Math

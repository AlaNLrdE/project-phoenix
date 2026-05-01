#include <iostream>
#include <iomanip>
#include <cmath>
#include <world/WorldManager.hpp>
#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>
#include <vessels/Vessel.hpp>
#include <math/Constants.hpp>

using namespace Phoenix::Math;
using namespace Phoenix::Physics;
using namespace Phoenix::Vessels;
using namespace Phoenix::World;

/**
 * Función auxiliar para imprimir vectores 3D.
 */
void printVector(const dvec3 &v, const std::string &label)
{
    std::cout << label << ": [" << std::fixed << std::setprecision(2)
              << v.x << ", " << v.y << ", " << v.z << "] ";
}

/**
 * Ejemplo 1: Crear cuerpos celestes y mostrar sus órbitas.
 */
void example1_CelestialBodies()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 1: Cuerpos Celestes (Tierra)   ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    // Crear Tierra
    CelestialBody *earth = new CelestialBody(
        "Earth",
        5.972e24,                                                          // masa (kg)
        6371000.0 * Constants::KSP_SCALE,                                  // radio (escala KSP)
        86164.0,                                                           // período rotación (s)
        Constants::MU_EARTH * Constants::KSP_SCALE * Constants::KSP_SCALE, // μ
        9.81,                                                              // gravedad superficial
        true,                                                              // tiene atmósfera
        100000.0 * Constants::KSP_SCALE                                    // altura atmosférica
    );

    std::cout << "✓ Tierra creada\n";
    std::cout << "  - Radio: " << std::scientific << std::setprecision(2)
              << earth->radius << " m (escala KSP)\n";
    std::cout << "  - μ = " << earth->mu << " m³/s²\n";
    std::cout << "  - Gravedad superficial: " << std::fixed << std::setprecision(2)
              << earth->surfaceGravity << " m/s²\n";

    delete earth;
}

/**
 * Ejemplo 2: Órbita circular de prueba.
 */
void example2_CircularOrbit()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 2: Órbita Circular (LEO)       ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    double earth_mu = Constants::MU_EARTH;
    double earth_radius = 6371000.0 * Constants::KSP_SCALE;

    // Órbita circular a 400 km de altitud (LEO típico)
    double altitude = 400000.0;
    double a = (earth_radius + altitude) * Constants::KSP_SCALE; // ¡Atención a escala!

    // Para órbita circular, crear desde posición y velocidad
    double r_mag = earth_radius + altitude;
    double v_mag = std::sqrt(earth_mu / r_mag);

    dvec3 position(r_mag, 0.0, 0.0);
    dvec3 velocity(0.0, v_mag, 0.0);

    Orbit leo(position, velocity, earth_mu, 0.0);

    std::cout << "✓ Órbita LEO creada (desde vectores de estado)\n";
    std::cout << "  - Posición inicial: (" << std::scientific << std::setprecision(3)
              << position.x << ", " << position.y << ", " << position.z << ") m\n";
    std::cout << "  - Velocidad inicial: (" << velocity.x << ", " << velocity.y
              << ", " << velocity.z << ") m/s\n";
    std::cout << "  - Magnitud velocidad: " << std::fixed << std::setprecision(2)
              << glm::length(velocity) << " m/s\n\n";

    std::cout << "Elementos orbitales calculados:\n";
    std::cout << "  - a = " << std::scientific << std::setprecision(3) << leo.a
              << " m\n";
    std::cout << "  - e = " << std::fixed << std::setprecision(6) << leo.e << "\n";
    std::cout << "  - i = " << Units::RAD_TO_DEG(leo.i) << "°\n";
    std::cout << "  - Ω = " << Units::RAD_TO_DEG(leo.Omega) << "°\n";
    std::cout << "  - ω = " << Units::RAD_TO_DEG(leo.omega) << "°\n";
    std::cout << "  - ν = " << Units::RAD_TO_DEG(leo.nu) << "°\n";

    double period = leo.getPeriod();
    std::cout << "\n  - Período: " << period / 60.0 << " minutos\n";
    std::cout << "  - Período: " << period / 3600.0 << " horas\n";
}

/**
 * Ejemplo 3: Propagación Kepleriana - Seguimiento de posición en el tiempo.
 */
void example3_KeplanianPropagation()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 3: Propagación Kepleriana      ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    double earth_mu = Constants::MU_EARTH;
    double earth_radius = 6371000.0 * Constants::KSP_SCALE;

    // Órbita LEO
    double altitude = 400000.0;
    double r_mag = earth_radius + altitude;
    double v_mag = std::sqrt(earth_mu / r_mag);

    dvec3 position(r_mag, 0.0, 0.0);
    dvec3 velocity(0.0, v_mag, 0.0);

    Orbit leo(position, velocity, earth_mu, 0.0);
    double period = leo.getPeriod();

    std::cout << "✓ Órbita LEO (período = " << std::fixed << std::setprecision(1)
              << period / 60.0 << " min)\n\n";

    // Propagar en 6 pasos equidistantes durante un período
    std::cout << "Propagación durante " << std::setprecision(2) << period / 3600.0
              << " horas (1 período):\n\n";

    std::cout << std::setw(10) << "Tiempo(s)"
              << std::setw(12) << "t/T"
              << std::setw(16) << "r (km)"
              << std::setw(16) << "v (m/s)"
              << std::setw(16) << "Altitud (km)\n";
    std::cout << std::string(70, '-') << "\n";

    for (int step = 0; step <= 6; ++step)
    {
        double t = (step / 6.0) * period;

        dvec3 pos = leo.getPositionAtTime(t);
        dvec3 vel = leo.getVelocityAtTime(t);

        double r_mag_current = glm::length(pos);
        double v_mag_current = glm::length(vel);
        double alt = (r_mag_current - earth_radius) / 1000.0;

        std::cout << std::fixed << std::setprecision(0) << std::setw(10) << t
                  << std::setw(12) << (t / period)
                  << std::setw(16) << (r_mag_current / 1000.0)
                  << std::setw(16) << std::setprecision(2) << v_mag_current
                  << std::setw(16) << std::setprecision(0) << alt << "\n";
    }

    std::cout << "\n";
}

/**
 * Ejemplo 4: Nave en órbita con WorldManager.
 */
void example4_VesselInOrbit()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 4: Nave en Órbita (Sistema)    ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    // Crear WorldManager
    WorldManager world;

    // Crear Tierra
    double earth_mu = Constants::MU_EARTH;
    double earth_radius = 6371000.0 * Constants::KSP_SCALE;

    CelestialBody *earth = new CelestialBody(
        "Earth",
        5.972e24,
        earth_radius,
        86164.0,
        earth_mu,
        9.81,
        true,
        100000.0 * Constants::KSP_SCALE);

    world.registerCelestialBody(earth);
    std::cout << "✓ Tierra registrada en WorldManager\n";

    // Crear órbita LEO
    double altitude = 400000.0;
    double r_mag = earth_radius + altitude;
    double v_mag = std::sqrt(earth_mu / r_mag);

    dvec3 position(r_mag, 0.0, 0.0);
    dvec3 velocity(0.0, v_mag, 0.0);

    Orbit leo(position, velocity, earth_mu, 0.0);

    // Crear nave
    Vessel *satellite = new Vessel(
        "MyCanSat",
        5000.0,  // masa seca (kg)
        leo,     // órbita
        "Earth", // referencia
        earth    // puntero
    );

    satellite->fuelMass = 1000.0; // 1 tonelada de combustible

    world.registerVessel(satellite);
    world.setActiveVessel("MyCanSat");

    std::cout << "✓ Nave 'MyCanSat' creada y registrada\n\n";

    // Propagar la nave
    double period = leo.getPeriod();
    std::cout << "Simulación: 1 período orbital (" << std::fixed
              << std::setprecision(1) << period / 3600.0 << " horas)\n\n";

    for (int orbit_step = 0; orbit_step <= 4; ++orbit_step)
    {
        double t = (orbit_step / 4.0) * period;
        world.setSimulationTime(t);
        satellite->currentTime = t;

        dvec3 pos = satellite->getPosition(t);
        dvec3 vel = satellite->getVelocity(t);

        double alt = satellite->getAltitude();

        std::cout << "T+" << std::setw(5) << std::setprecision(0) << t / 3600.0
                  << "h | Alt: " << std::setprecision(0) << alt / 1000.0
                  << " km | v: " << std::setprecision(3) << glm::length(vel)
                  << " m/s\n";
    }

    std::cout << "\n";
    world.printCelestialBodies();
    satellite->printStatus();
}

/**
 * Ejemplo 5: Maniobra básica (cambio de velocidad).
 */
void example5_ManeuverBasic()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 5: Maniobra Básica (Hohmann)   ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    double earth_mu = Constants::MU_EARTH;
    double earth_radius = 6371000.0 * Constants::KSP_SCALE;

    // Órbita inicial: LEO a 200 km
    double alt1 = 200000.0;
    double r1 = earth_radius + alt1;
    double v1 = std::sqrt(earth_mu / r1);

    dvec3 pos1(r1, 0.0, 0.0);
    dvec3 vel1(0.0, v1, 0.0);

    Orbit leo(pos1, vel1, earth_mu, 0.0);

    std::cout << "Órbita inicial (LEO 200 km):\n";
    std::cout << "  - Altitud: " << std::fixed << std::setprecision(0)
              << alt1 / 1000.0 << " km\n";
    std::cout << "  - Velocidad: " << std::setprecision(2) << v1 << " m/s\n";
    std::cout << "  - Período: " << leo.getPeriod() / 60.0 << " minutos\n\n";

    // Aplicar impulso para pasar a órbita de transferencia Hohmann (apoapsis 400 km)
    double alt2 = 400000.0;
    double r2 = earth_radius + alt2;

    // Velocidad en periapsis de la órbita de transferencia
    double a_transfer = (r1 + r2) / 2.0;
    double v_periapsis = std::sqrt(earth_mu * (2.0 / r1 - 1.0 / a_transfer));

    double deltaV_hohmann = v_periapsis - v1;

    std::cout << "Maniobra Hohmann a órbita de 400 km:\n";
    std::cout << "  - ΔV requerido: " << std::setprecision(2) << deltaV_hohmann
              << " m/s\n";

    // Aplicar cambio de velocidad (en dirección prograda)
    dvec3 new_vel = vel1 + glm::normalize(vel1) * deltaV_hohmann;

    Orbit hohmann(pos1, new_vel, earth_mu, 0.0);

    std::cout << "\nTrayectoria de transferencia:\n";
    std::cout << "  - Semieje mayor: " << std::scientific << std::setprecision(3)
              << hohmann.a << " m\n";
    std::cout << "  - Excentricidad: " << std::fixed << std::setprecision(6)
              << hohmann.e << "\n";
    std::cout << "  - Periapsis: " << std::setprecision(0)
              << (hohmann.getDistanceToPeriapsis() - earth_radius) / 1000.0
              << " km\n";
    std::cout << "  - Apoapsis: "
              << (hohmann.getDistanceToApoapsis() - earth_radius) / 1000.0
              << " km\n";
    std::cout << "  - Tiempo de transferencia: "
              << hohmann.getPeriod() / 2.0 / 60.0 << " minutos\n";
}

// Función principal
int main()
{
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║           PROJECT PHOENIX - PHASE 1 DEMONSTRATION            ║
║          Kerbal Space Program Clone (1:10 Scale)             ║
║                                                              ║
║              Orbital Mechanics & Propagation                 ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
    )";

    try
    {
        example1_CelestialBodies();
        example2_CircularOrbit();
        example3_KeplanianPropagation();
        example4_VesselInOrbit();
        example5_ManeuverBasic();

        std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                   EJEMPLOS COMPLETADOS                       ║
║                                                              ║
║  Phase 1 establece:                                          ║
║  • Propagación orbital Kepleriana (6 elementos)              ║
║  • Resolución numérica de ecuación de Kepler                 ║
║  • Sistema de cuerpos celestes y naves                       ║
║  • Tiempo de simulación con time warp                        ║
║                                                              ║
║  Próxima fase: Arquitectura de partes y CoM                  ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
        )";
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n✗ Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

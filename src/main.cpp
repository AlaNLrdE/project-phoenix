#include <iostream>
#include <iomanip>
#include <cmath>
#include <world/WorldManager.hpp>
#include <world/SphereOfInfluence.hpp>
#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>
#include <physics/Atmosphere.hpp>
#include <physics/AeroForces.hpp>
#include <vessels/Vessel.hpp>
#include <parts/Part.hpp>
#include <parts/Engine.hpp>
#include <ui/AsciiRenderer.hpp>
#include <ui/MissionDisplay.hpp>
#include <math/Constants.hpp>
#include <launch/LaunchVehicle.hpp>
#include <launch/AscentIntegrator.hpp>

using namespace Phoenix::Math;
using namespace Phoenix::Physics;
using namespace Phoenix::Vessels;
using namespace Phoenix::World;
using namespace Phoenix::Parts;
using namespace Phoenix::UI;
using namespace Phoenix::Launch;

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

/**
 * Ejemplo 6: Jerarquía de partes — cohete de 2 etapas (Phase 2).
 */
void example6_PartHierarchy()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 6: Jerarquía de Partes (Ph.2)  ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    // ── Construir cohete de 2 etapas ────────────────────────────────────────
    //
    //   [Command Pod]  ← raíz
    //      └─ [Decoupler]
    //            └─ [FuelTank S2]  (etapa 2)
    //                  └─ [Engine S2]
    //                  └─ [Decoupler S2]
    //                        └─ [FuelTank S1]  (etapa 1)
    //                              └─ [Engine S1]

    auto pod = std::make_shared<Part>("CommandPod", PartType::Command, 850.0);
    auto decoTop = std::make_shared<Part>("Decoupler-2", PartType::Decoupler, 120.0);
    auto tank2 = std::make_shared<Part>("FuelTank-S2", PartType::FuelTank, 500.0, 8000.0);
    auto decoMid = std::make_shared<Part>("Decoupler-1", PartType::Decoupler, 120.0);
    auto tank1 = std::make_shared<Part>("FuelTank-S1", PartType::FuelTank, 800.0, 15000.0);

    // Llenar depósitos
    tank2->fuelMass = tank2->maxFuelMass;
    tank1->fuelMass = tank1->maxFuelMass;

    // Posiciones locales (metros, eje +Z ascendente)
    decoTop->localPosition = dvec3(0, 0, -1.2);
    tank2->localPosition = dvec3(0, 0, -3.0);
    decoMid->localPosition = dvec3(0, 0, -5.5);
    tank1->localPosition = dvec3(0, 0, -8.0);

    // Ensamblar árbol
    pod->addChild(decoTop);
    decoTop->addChild(tank2);
    tank2->addChild(decoMid);
    decoMid->addChild(tank1);

    std::cout << "✓ Cohete ensamblado:\n";
    std::cout << "  Partes activas: " << pod->getAllParts().size() << "\n";
    std::cout << "  Masa total:     " << std::fixed << std::setprecision(1)
              << pod->getTreeMass() << " kg\n";

    dvec3 com = pod->getTreeCoM();
    std::cout << "  CoM local:      ("
              << std::setprecision(3) << com.x << ", " << com.y << ", " << com.z
              << ") m\n\n";

    // ── Simulación de staging ────────────────────────────────────────────────
    double earth_mu = Constants::MU_EARTH;
    double earth_radius = 6371000.0 * Constants::KSP_SCALE;
    double alt = 200000.0;
    double r_mag = earth_radius + alt;
    double v_mag = std::sqrt(earth_mu / r_mag);
    dvec3 pos(r_mag, 0.0, 0.0);
    dvec3 vel(0.0, v_mag, 0.0);
    Orbit leo(pos, vel, earth_mu, 0.0);

    CelestialBody earth_body("Earth", 5.972e24, earth_radius,
                             86164.0, earth_mu, 9.81, true,
                             100000.0 * Constants::KSP_SCALE);

    Vessel rocket("Rocket2Stage", 0.0, leo, "Earth", &earth_body);
    rocket.setRootPart(pod);

    std::cout << "Masa antes del staging: " << std::setprecision(1)
              << rocket.getTotalMass() << " kg\n";

    auto stage1 = rocket.stage(); // separa Decoupler-2 y todo lo de abajo
    if (stage1)
    {
        std::cout << "✓ Staging ejecutado\n";
        std::cout << "  Nave principal:  " << std::setprecision(1)
                  << rocket.getTotalMass() << " kg  ("
                  << rocket.getAllParts().size() << " partes)\n";
        std::cout << "  Etapa separada:  "
                  << stage1->getTotalMass() << " kg\n";
    }
}

/**
 * Ejemplo 7: Sistema de propulsión — Tsiolkovsky y burn (Phase 3).
 */
void example7_Propulsion()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 7: Propulsión (Phase 3)        ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    // ── Configuración del vehículo ───────────────────────────────────────────
    //
    //   [CommandPod]  850 kg seco
    //      └─ [FuelTank]  500 kg seco + 8 000 kg prop.
    //            └─ [Engine]  Merlin-1D  (throttleable)

    auto pod = std::make_shared<Part>("CommandPod", PartType::Command, 850.0);
    auto tank = std::make_shared<Part>("MainTank", PartType::FuelTank, 500.0, 8000.0);
    auto engine = std::make_shared<Engine>("Merlin-1D", 630.0, 845000.0, 311.0);
    //                                                   ↑dry    ↑thrust N  ↑Isp s

    tank->fuelMass = tank->maxFuelMass;
    tank->localPosition = dvec3(0, 0, -2.0);
    engine->localPosition = dvec3(0, 0, -4.5);

    pod->addChild(tank);
    tank->addChild(engine);

    double earth_mu = Constants::MU_EARTH;
    double earth_radius = 6371000.0 * Constants::KSP_SCALE;
    double alt = 200000.0;
    double r_mag = earth_radius + alt;
    double v_mag = std::sqrt(earth_mu / r_mag);
    dvec3 pos0(r_mag, 0.0, 0.0);
    dvec3 vel0(0.0, v_mag, 0.0);
    Orbit leo(pos0, vel0, earth_mu, 0.0);

    CelestialBody earth_body("Earth", 5.972e24, earth_radius,
                             86164.0, earth_mu, 9.81, true,
                             100000.0 * Constants::KSP_SCALE);

    Vessel ship("Falcon9S2", 0.0, leo, "Earth", &earth_body);
    ship.setRootPart(pod);

    std::cout << "✓ Vehículo ensamblado\n";
    std::cout << "  Masa total:         " << std::fixed << std::setprecision(1)
              << ship.getTotalMass() << " kg\n";
    std::cout << "  Combustible:        " << ship.getTotalFuelMass() << " kg\n\n";

    // ── Encender motor al 100 % throttle ────────────────────────────────────
    ship.igniteEngines();
    ship.setThrottle(1.0);

    double thrust = ship.getTotalThrust();
    std::cout << "Motor encendido (throttle 100 %):\n";
    std::cout << "  Empuje:             " << std::setprecision(0)
              << thrust / 1000.0 << " kN\n";
    std::cout << "  Flujo másico:       " << std::setprecision(2)
              << engine->getMassFlowRate() << " kg/s\n";
    std::cout << "  Vel. de exhaust:    " << engine->getExhaustVelocity()
              << " m/s\n\n";

    // ── ΔV disponible (Tsiolkovsky) ──────────────────────────────────────────
    double avail_dv = ship.computeAvailableDeltaV();
    std::cout << "Ecuación de Tsiolkovsky:\n";
    std::cout << "  ΔV disponible:      " << std::setprecision(1)
              << avail_dv << " m/s  (" << avail_dv / 1000.0 << " km/s)\n";

    double burnTime = engine->computeBurnTime(avail_dv * 0.5,
                                              ship.getTotalFuelMass(),
                                              ship.getTotalMass());
    std::cout << "  t_burn (50 % ΔV):   " << std::setprecision(2)
              << burnTime << " s\n\n";

    // ── Ejecutar burn pro-grado de 60 segundos ───────────────────────────────
    dvec3 posAntes, velAntes;
    ship.getState(ship.currentTime, posAntes, velAntes);
    double altAntes = (glm::length(posAntes) - earth_radius) / 1000.0;
    double vAntes = glm::length(velAntes);

    double dv_aplicado = ship.executeBurn(60.0); // 60 s pro-grado

    dvec3 posDes, velDes;
    ship.getState(ship.currentTime, posDes, velDes);
    double altDesp = (glm::length(posDes) - earth_radius) / 1000.0;
    double vDesp = glm::length(velDes);

    std::cout << "Burn pro-grado de 60 s:\n";
    std::cout << "  ΔV aplicado:        " << std::setprecision(2)
              << dv_aplicado << " m/s\n";
    std::cout << "  Velocidad:          " << vAntes << " → " << vDesp << " m/s\n";
    std::cout << "  Altitud periapsis:  " << std::setprecision(0) << altAntes
              << " → " << altDesp << " km\n";
    std::cout << "  Combustible restante: " << std::setprecision(1)
              << ship.getTotalFuelMass() << " kg\n\n";

    // ── Apagar motor ─────────────────────────────────────────────────────────
    ship.shutdownEngines();
    std::cout << "✓ Motor apagado\n";
    std::cout << "  Empuje (post-shutdown): " << std::setprecision(0)
              << ship.getTotalThrust() << " N\n";
}

/**
 * Ejemplo 8: Esferas de Influencia — sistema Tierra-Luna (Phase 4).
 * Todos los objetos son stack-allocated para evitar problemas de ownership.
 */
void example8_SphereOfInfluence()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 8: Esferas de Influencia (Ph.4)║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    // ── Constantes ───────────────────────────────────────────────────────────
    const double earthMu = Constants::MU_EARTH;
    const double earthRadius = 6371000.0 * Constants::KSP_SCALE;
    const double moonSMA = 384400000.0 * Constants::KSP_SCALE;
    const double moonMu = 4.9048e12 * Constants::KSP_SCALE * Constants::KSP_SCALE;
    const double moonRadius = 1737400.0 * Constants::KSP_SCALE;
    const double moonMass = 7.342e22;
    const double earthMass = 5.972e24;

    // ── Radio de SoI ─────────────────────────────────────────────────────────
    double moonSoI = SphereOfInfluence::computeRadius(moonSMA, moonMass, earthMass);

    std::cout << "Sistema Tierra–Luna (escala KSP 1:10):\n";
    std::cout << std::fixed << std::setprecision(0);
    std::cout << "  Distancia Tierra–Luna: " << moonSMA / 1000.0 << " km\n";
    std::cout << "  SoI de la Luna:        " << moonSoI / 1000.0 << " km\n\n";

    // ── Nave en TLI ──────────────────────────────────────────────────────────
    const double alt = 300000.0;
    const double r = earthRadius + alt;
    const double vLEO = std::sqrt(earthMu / r);
    const double vTLI = std::sqrt(earthMu * (2.0 / r - 1.0 / ((r + moonSMA) / 2.0)));

    Orbit tliOrbit(dvec3(r, 0.0, 0.0), dvec3(0.0, vTLI, 0.0), earthMu, 0.0);

    std::cout << "Nave 'Apollo' en TLI:\n";
    std::cout << std::setprecision(2);
    std::cout << "  v_LEO:    " << vLEO << " m/s\n";
    std::cout << "  v_TLI:    " << vTLI << " m/s  (ΔV = " << vTLI - vLEO << " m/s)\n";
    std::cout << std::setprecision(0);
    std::cout << "  Apoapsis: "
              << (tliOrbit.getDistanceToApoapsis() - earthRadius) / 1000.0 << " km\n\n";

    // ── Posición en apoapsis vs SoI lunar ────────────────────────────────────
    const double tFly = tliOrbit.getPeriod() * 0.5;
    const dvec3 shipAtApo = tliOrbit.getPositionAtTime(tFly);
    const dvec3 moonAtT0(moonSMA, 0.0, 0.0);
    const dvec3 relToMoon = shipAtApo - moonAtT0;
    const double distToMoon = glm::length(relToMoon);

    std::cout << "En t = " << std::setprecision(1) << tFly / 3600.0 << " h (apoapsis):\n";
    std::cout << std::setprecision(0);
    std::cout << "  Distancia a la Luna: " << distToMoon / 1000.0 << " km\n";
    std::cout << "  SoI lunar:           " << moonSoI / 1000.0 << " km\n";
    std::cout << "  Dentro SoI lunar:    "
              << (SphereOfInfluence::isInside(relToMoon, moonSoI)
                      ? "SÍ ✓"
                      : "NO (necesita corrección de trayectoria)")
              << "\n\n";

    // ── Demo de transición: nave dentro de la SoI lunar ──────────────────────
    // Objetos stack-allocated; CelestialBody no posee la Orbit (raw ptr no-owning).
    CelestialBody earthBody("Earth", earthMass, earthRadius,
                            86164.0, earthMu, 9.81, true,
                            100000.0 * Constants::KSP_SCALE);

    Orbit moonOrbit(dvec3(moonSMA, 0.0, 0.0),
                    dvec3(0.0, std::sqrt(earthMu / moonSMA), 0.0),
                    earthMu, 0.0);
    CelestialBody moonBody("Moon", moonMass, moonRadius,
                           2360448.0, moonMu, 1.62, false, 0.0);
    moonBody.setOrbit(&moonOrbit, "Earth", &earthBody);

    // Colocar nave a 0.5 * r_SoI del centro de la Luna
    // Obtener posición real de la Luna según su órbita en t=0
    dvec3 moonAt0 = moonBody.getWorldPosition(0.0);
    const dvec3 nearMoonRelPos(moonSoI * 0.5, 0.0, 0.0);
    const dvec3 nearMoonRelVel(0.0, -500.0, 0.0);
    const dvec3 vesselPosEarth = moonAt0 + nearMoonRelPos;
    const dvec3 vesselVelEarth = moonOrbit.getVelocityAtTime(0.0) + nearMoonRelVel;

    Orbit vesselOrbit(vesselPosEarth, vesselVelEarth, earthMu, 0.0);
    Vessel demoShip("DemoApollo", 50000.0, vesselOrbit, "Earth", &earthBody);

    std::map<std::string, CelestialBody *> bodies{{"Earth", &earthBody}, {"Moon", &moonBody}};
    auto transition = SphereOfInfluence::checkTransition(demoShip, bodies, 0.0);

    std::cout << "Demo: nave colocada a 0.5 * r_SoI del centro lunar:\n";
    if (transition.hasTransition)
    {
        std::cout << "  ✓ Transición detectada: Tierra → " << transition.newBody->name << "\n";
        std::cout << "  Pos. en marco lunar:  ("
                  << std::setprecision(0)
                  << transition.newPosition.x / 1000.0 << ", "
                  << transition.newPosition.y / 1000.0 << ", "
                  << transition.newPosition.z / 1000.0 << ") km\n";
        std::cout << std::setprecision(2);
        std::cout << "  |v| relativa:         " << glm::length(transition.newVelocity) << " m/s\n";

        Orbit lunarOrbit(transition.newPosition, transition.newVelocity, moonMu, 0.0);
        std::cout << "  Excentricidad:        " << std::setprecision(4) << lunarOrbit.e << "\n";
        if (lunarOrbit.e < 1.0)
        {
            std::cout << "  Periapsis lunar:      " << std::setprecision(0)
                      << (lunarOrbit.getDistanceToPeriapsis() - moonRadius) / 1000.0 << " km\n";
        }
        else
        {
            std::cout << "  Trayectoria hiperbólica (flyby)\n";
        }
    }
    else
    {
        std::cout << "  [Sin transición — revisar lógica de checkTransition]\n";
    }
}

/**
 * Ejemplo 9: Re-entrada atmosférica con arrastre aerodinámico (Phase 5).
 *
 * Simula el descenso de una cápsula desde una órbita elíptica (apoapsis 400 km /
 * periapsis 2 km, escala KSP 1:10) integrando numéricamente la gravedad y el
 * arrastre D = ½ρv²CdA con RK4.  El calor de reentrada usa la fórmula de
 * Chapman: q = 1.83×10⁻⁴ × √(ρ/R_nose) × v³.
 */
void example9_AtmosphericReentry()
{
    using namespace Phoenix::Physics;
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 9: Re-entrada Atmosférica(Ph5)║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    // ── Parámetros de la Tierra (escala KSP 1:10) ─────────────────────────
    const double earthRadius = 6371000.0 * Constants::KSP_SCALE; // 637 100 m
    const double earthMu = Constants::MU_EARTH;                  // 3.986×10¹⁴ m³/s²
    Atmosphere atm = Atmosphere::makeEarthLike(Constants::KSP_SCALE);

    std::cout << "Atmósfera (Earth, KSP 1:10):\n";
    std::cout << std::fixed << std::setprecision(0);
    std::cout << "  Altura total:   " << atm.atmosphereHeight / 1000.0 << " km\n";
    std::cout << std::scientific << std::setprecision(3);
    std::cout << "  ρ @ superficie: " << atm.getDensity(0.0) << " kg/m³ (1.225)\n";
    std::cout << "  ρ @ 10 km:      " << atm.getDensity(10000.0) << " kg/m³\n\n";

    // ── Parámetros de la cápsula ──────────────────────────────────────────
    const double mass = 3000.0; // kg
    const double area = 12.0;   // m²  (escudo ~4 m diámetro)
    const double Cd = 1.5;      // blunt body
    const double noseR = 2.0;   // m   (radio nariz del escudo)

    std::cout << std::fixed;
    std::cout << "Cápsula de reentrada:\n";
    std::cout << "  Masa:        " << std::setprecision(0) << mass << " kg\n";
    std::cout << "  Área ref.:   " << area << " m²\n";
    std::cout << "  Cd:          " << std::setprecision(1) << Cd << "\n";
    std::cout << "  Radio nariz: " << noseR << " m\n\n";

    // ── Condiciones de entrada atmosférica ────────────────────────────────
    // Órbita elíptica de deórbita: apoapsis 400 km / periapsis 2 km
    // Condiciones calculadas en el punto de entrada (= top de atmósfera = 10 km)
    // mediante mecánica orbital (conservación de h y energía vis-viva).
    const double r_apo = earthRadius + 400000.0; // 1 037 100 m
    const double r_peri = earthRadius + 2000.0;  //   639 100 m
    const double a = 0.5 * (r_apo + r_peri);
    const double ecc = (r_apo - r_peri) / (r_apo + r_peri);

    const double r_entry = earthRadius + atm.atmosphereHeight; // 647 100 m
    const double v_total = std::sqrt(earthMu * (2.0 / r_entry - 1.0 / a));

    // Momento angular específico de la órbita
    const double h_ang = std::sqrt(earthMu * a * (1.0 - ecc * ecc));
    const double v_tang = h_ang / r_entry; // tangencial (+y)
    const double v_rad = -std::sqrt(std::max(0.0, v_total * v_total - v_tang * v_tang));

    dvec3 r0(r_entry, 0.0, 0.0);
    dvec3 v0(v_rad, v_tang, 0.0);

    const double entryAngle = Units::RAD_TO_DEG(std::atan2(-v_rad, v_tang));

    std::cout << "Condiciones de entrada (altitud "
              << std::setprecision(0) << atm.atmosphereHeight / 1000.0 << " km):\n";
    std::cout << "  Velocidad total:   " << std::setprecision(1) << v_total << " m/s\n";
    std::cout << "  v tangencial:      " << v_tang << " m/s\n";
    std::cout << "  v radial:          " << v_rad << " m/s\n";
    std::cout << "  Ángulo de entrada: " << std::setprecision(2) << entryAngle << "°\n\n";

    // Velocidad terminal teórica en superficie (D = mg)
    const double v_term = std::sqrt(2.0 * mass * 9.81 / (1.225 * Cd * area));
    std::cout << "Vel. terminal teórica (cota): " << std::setprecision(1) << v_term
              << " m/s (" << v_term * 3.6 << " km/h)\n\n";

    // ── Ejecutar simulación RK4 ───────────────────────────────────────────
    std::cout << "Simulando (RK4, dt=0.05 s, máx 100 000 pasos = 5 000 s)...\n\n";

    auto result = AeroForces::simulate(
        r0, v0,
        mass, area, Cd, noseR,
        earthRadius, earthMu, atm,
        0.05,   // dt
        100000, // maxSteps
        5       // logInterval (log every 0.25 s)
    );

    // ── Mostrar trayectoria ───────────────────────────────────────────────
    std::cout << std::setw(9) << "T(s)"
              << std::setw(10) << "Alt(km)"
              << std::setw(10) << "v(km/s)"
              << std::setw(13) << "ρ(kg/m³)"
              << std::setw(13) << "q(MW/m²)"
              << std::setw(11) << "Drag(kN)"
              << std::setw(8) << "a(g)\n";
    std::cout << std::string(74, '-') << "\n";

    // With logInterval=5 (every 0.25 s), print all points in atmosphere.
    // In vacuum, print every 4th (every 1 s).
    int vacuumCount = 0;
    for (const auto &p : result.points)
    {
        bool inAtm = p.altitude < atm.atmosphereHeight;
        if (!inAtm)
        {
            ++vacuumCount;
            if (vacuumCount % 4 != 0)
                continue;
        }

        double g_load = (mass > 0.0 && p.dragForce > 0.0)
                            ? p.dragForce / (mass * 9.81)
                            : 0.0;

        std::cout << std::fixed
                  << std::setw(9) << std::setprecision(2) << p.time
                  << std::setw(10) << std::setprecision(2) << p.altitude / 1000.0
                  << std::setw(10) << std::setprecision(3) << p.speed / 1000.0
                  << std::setw(13) << std::scientific << std::setprecision(2) << p.density
                  << std::setw(13) << std::fixed << std::setprecision(2) << p.heatFlux / 1e6
                  << std::setw(11) << std::setprecision(1) << p.dragForce / 1000.0
                  << std::setw(8) << std::setprecision(1) << g_load << "\n";
    }
    std::cout << std::string(74, '-') << "\n\n";

    // ── Resultado final ───────────────────────────────────────────────────
    if (result.landed)
    {
        std::cout << "✓ ATERRIZAJE\n";
        std::cout << "  Velocidad de impacto: " << std::setprecision(1)
                  << result.impactSpeed << " m/s  ("
                  << result.impactSpeed * 3.6 << " km/h)\n";
    }
    else if (result.burnedUp)
    {
        std::cout << "✗ DESTRUCCIÓN — flujo de calor superó 500 MW/m²\n";
    }
    else
    {
        std::cout << "⏱  Simulación terminada (5 000 s alcanzados)\n";
    }

    std::cout << "  Pico flujo de calor:  " << std::setprecision(1)
              << result.peakHeatFlux / 1e6 << " MW/m²\n";
    std::cout << "  Pico deceleración:    " << std::setprecision(0)
              << result.peakDecel / 9.81 << " g\n";
    std::cout << "  Puntos de trayectoria registrados: " << result.points.size() << "\n";
}

/**
 * Ejemplo 10: Visualización ASCII — mapas orbitales y HUD de misión (Phase 6).
 */
void example10_Visualization()
{
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║  EJEMPLO 10: Visualización (Phase 6)   ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";

    const double earthMu     = Constants::MU_EARTH;
    const double earthRadius = 6371000.0 * Constants::KSP_SCALE;

    CelestialBody earth("Earth", 5.972e24, earthRadius, 86164.0,
                        earthMu, 9.81, true, 100000.0 * Constants::KSP_SCALE);

    // ── A) Mapa orbital: LEO circular ─────────────────────────────────────
    std::cout << "\u2500\u2500 A) \xc3\x93rbita circular LEO (200 km) "
                 "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n\n";

    const double alt_leo = 200000.0;
    const double r1      = earthRadius + alt_leo;
    const double v1      = std::sqrt(earthMu / r1);
    dvec3        pos1(r1, 0.0, 0.0);
    dvec3        vel1(0.0, v1, 0.0);
    Orbit        leo(pos1, vel1, earthMu, 0.0);

    AsciiRenderer::displayOrbit(leo, earthRadius, "LEO 200 km");

    // ── B) Mapa orbital: transferencia Hohmann ────────────────────────────
    std::cout << "\n\u2500\u2500 B) Transferencia Hohmann LEO (200 km) \u2192 MEO (400 km) "
                 "\u2500\u2500\u2500\u2500\n\n";

    const double alt2  = 400000.0;
    const double r2    = earthRadius + alt2;
    const double a_tx  = (r1 + r2) / 2.0;
    const double v_tx  = std::sqrt(earthMu * (2.0 / r1 - 1.0 / a_tx));
    dvec3        vel_tx(0.0, v_tx, 0.0);
    Orbit        hohmann(pos1, vel_tx, earthMu, 0.0);

    AsciiRenderer::displayOrbit(hohmann, earthRadius, "Hohmann LEO\u2192MEO");

    // ── C) HUD de control de misi\xc3\xb3n ─────────────────────────────────
    std::cout << "\n\u2500\u2500 C) HUD de control de misi\xc3\xb3n "
                 "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

    Vessel phoenix("Phoenix-1", 3000.0, leo, "Earth", &earth);
    phoenix.fuelMass    = 800.0;
    phoenix.status      = "active";
    const double t_hud  = leo.getPeriod() * 0.25;
    phoenix.currentTime = t_hud;

    MissionDisplay::printDashboard(phoenix, earth, t_hud);
    MissionDisplay::printOrbitalElements(leo, earth);
    MissionDisplay::printFuelReport(phoenix);
    MissionDisplay::printManeuverNode("Hohmann \u0394V1 (pro-grado)",
                                      v_tx - v1,
                                      (v_tx - v1) / 2.5);

    // ── D) Perfil ASCII de reentrada ──────────────────────────────────────
    std::cout << "── D) Perfil gráfico de reentrada atmosférica "
                 "─────────────────────────\n\n";

    Atmosphere atm = Atmosphere::makeEarthLike(Constants::KSP_SCALE);

    // Reentry conditions: de-orbit ellipse apo=400km / peri=2km, entry at atm top
    const double r_apo  = earthRadius + 400000.0;
    const double r_peri = earthRadius + 2000.0;
    const double a_deo  = 0.5 * (r_apo + r_peri);
    const double ecc    = (r_apo - r_peri) / (r_apo + r_peri);
    const double r_ent  = earthRadius + atm.atmosphereHeight;
    const double v_ent  = std::sqrt(earthMu * (2.0 / r_ent - 1.0 / a_deo));
    const double h_ang  = std::sqrt(earthMu * a_deo * (1.0 - ecc * ecc));
    const double v_tang = h_ang / r_ent;
    const double v_rad  = -std::sqrt(std::max(0.0, v_ent * v_ent - v_tang * v_tang));

    dvec3 r0(r_ent, 0.0, 0.0);
    dvec3 v0(v_rad, v_tang, 0.0);

    auto result = AeroForces::simulate(
        r0, v0, 3000.0, 12.0, 1.5, 2.0,
        earthRadius, earthMu, atm,
        0.05, 100000, 5);

    AsciiRenderer::displayReentryProfile(result, "Reentrada KSP-Scale");

    if (result.landed)
        std::cout << "  Impacto: " << std::fixed << std::setprecision(1)
                  << result.impactSpeed << " m/s  |  "
                  << "Pico calor: " << result.peakHeatFlux / 1e6 << " MW/m\xc2\xb2"
                  << "  |  " << result.points.size() << " puntos\n";
}

/**
 * Phase 8B — Launch Sequence: ascenso RK4 con separación de etapas.
 *
 * Simula el lanzamiento completo del Soyuz-FG KSP-0.1 usando AscentIntegrator:
 *   - Integración RK4 bajo empuje + gravedad + arrastre atmosférico
 *   - Gravity turn simple (vertical → prograde entre t=5 y t=60 s)
 *   - Separación automática de etapas al agotar propelante
 *   - Métricas orbitales del estado final (apoapsis, periapsis)
 */
void demo_phase8B()
{
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   PHASE 8B — LAUNCH SEQUENCE (AscentIntegrator + staging)    ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    // ── Tierra y atmósfera (escala KSP 0.1) ──────────────────────────────────
    CelestialBody earth(
        "Earth", 5.972e24,
        6371000.0 * Constants::KSP_SCALE, 86164.0,
        Constants::MU_EARTH * Constants::KSP_SCALE * Constants::KSP_SCALE,
        9.81, true, 70000.0 * Constants::KSP_SCALE);

    Atmosphere atm = Atmosphere::makeEarthLike(Constants::KSP_SCALE);

    // ── Soyuz-FG KSP 0.1 (misma config que Phase 8A) ────────────────────────
    LaunchVehicle soyuz("Soyuz-FG");
    {
        StageConfig b; b.name="Boosters"; b.strutMass=800; b.decouplerMass=120;
        b.hasDecoupler=true;
        b.engines.push_back({"RD-107A",1100,838800,310.7,4});
        b.tanks.push_back({"BoosterTank",3200,39600});
        soyuz.addStage(b);
    }
    {
        StageConfig c; c.name="CoreStage"; c.strutMass=600; c.decouplerMass=80;
        c.hasDecoupler=true;
        c.engines.push_back({"RD-108A",1250,792400,314.2,1});
        c.tanks.push_back({"CoreTank",6900,91400});
        soyuz.addStage(c);
    }
    {
        StageConfig u; u.name="UpperStage"; u.strutMass=200; u.hasDecoupler=false;
        u.engines.push_back({"RD-0110",410,297900,326.0,1});
        u.tanks.push_back({"UpperTank",2355,22000});
        u.tanks.push_back({"SoyuzCapsule",2800,0});
        soyuz.addStage(u);
    }

    // ── Simular ascenso ──────────────────────────────────────────────────────
    AscentIntegrator integrator(soyuz, earth, &atm);
    integrator.setThrottle(1.0);
    integrator.setRecordInterval(40);  // ~2s entre puntos

    std::cout << "  Simulando ascenso (RK4 dt=0.05s)...\n";
    AscentResult res = integrator.simulate(0.05, 800.0, 0.0);

    // ── Imprimir resultados ──────────────────────────────────────────────────
    const std::string SEP(60, '-');
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "\n+" << std::string(60,'=') << "+\n";
    std::cout << "|  RESULTADO DEL ASCENSO                                     |\n";
    std::cout << "+" << SEP << "+\n";
    std::cout << "|  Razon de parada  : " << std::left << std::setw(38) << res.exitReason << "|\n";
    std::cout << "|  Tiempo total     : " << std::right << std::setw(8) << res.totalBurnTime << " s                          |\n";
    std::cout << "|  Altitud maxima   : " << std::setw(8) << res.maxAltitude/1000.0 << " km                         |\n";
    std::cout << "|  Velocidad maxima : " << std::setw(8) << res.maxSpeed << " m/s                       |\n";
    std::cout << "|  Etapas separadas : " << std::setw(8) << res.stagesUsed << "                            |\n";
    std::cout << "|  Llego al espacio : " << std::setw(38) << (res.reachedSpace ? "SI" : "NO") << "|\n";
    bool hasOrbit = (res.apoapsis > 0.0 && res.periapsis != 0.0);
    std::cout << "|  Apoapsis         : "
              << (hasOrbit ? std::to_string((int)(res.apoapsis/1000.0))+" km" : "escape traj.")
              << std::string(hasOrbit ? 25 : 20, ' ') << "|\n";
    std::cout << "|  Periapsis        : "
              << (hasOrbit ? std::to_string((int)(res.periapsis/1000.0))+" km" : "escape traj.")
              << std::string(hasOrbit ? 25 : 20, ' ') << "|\n";
    std::cout << "+" << std::string(60,'=') << "+\n";

    std::cout << "\n  Tiempos de separacion de etapa:\n";
    for (int i = 0; i < (int)res.stagingTimes.size(); ++i)
        std::cout << "    Etapa " << i << " (" << soyuz.getStage(i).name << "): "
                  << std::setprecision(1) << res.stagingTimes[i] << " s\n";

    // ── Muestra de trayectoria (cada 10 puntos) ──────────────────────────────
    std::cout << "\n  Muestra de trayectoria (t, alt, vel, stage):\n";
    std::cout << "  " << std::string(50, '-') << "\n";
    int skip = std::max(1, (int)res.trajectory.size() / 12);
    for (int i = 0; i < (int)res.trajectory.size(); i += skip) {
        const auto& pt = res.trajectory[i];
        std::cout << "  t=" << std::setw(6) << pt.time
                  << " s  alt=" << std::setw(7) << pt.altitude/1000.0
                  << " km  v=" << std::setw(7) << pt.speed
                  << " m/s  stage=" << pt.stageIndex << "\n";
    }

    // ── Prueba de Vessel::launchStage() ─────────────────────────────────────
    std::cout << "\n  Test Vessel::launchStage() (separacion bottom-up):\n";
    Orbit dummyOrbit(earth.radius + 1000.0, 0.0, 0.0, 0.0, 0.0, 0.0, earth.mu, 0.0);
    auto vessel = soyuz.assemble(dummyOrbit, &earth);
    std::cout << "    Masa inicial: " << vessel->getTotalMass()/1000.0 << " t ("
              << vessel->getAllParts().size() << " partes)\n";

    auto s1 = vessel->launchStage();  // separa booster
    if (s1) std::cout << "    Etapa separada: " << s1->name
                      << " (" << s1->getTotalMass()/1000.0 << " t)\n";
    std::cout << "    Masa restante: " << vessel->getTotalMass()/1000.0 << " t\n";

    auto s2 = vessel->launchStage();  // separa core
    if (s2) std::cout << "    Etapa separada: " << s2->name
                      << " (" << s2->getTotalMass()/1000.0 << " t)\n";
    std::cout << "    Masa restante: " << vessel->getTotalMass()/1000.0 << " t (payload)\n";

    std::cout << "\n  ✓ Phase 8B complete — AscentIntegrator + launchStage() operativos\n";
}

/**
 * Phase 8C — Ascent Guidance: pitch program, Max-Q, orbit insertion.
 *
 * Mejoras sobre Phase 8B:
 *   - Pitch program basado en altitud (no en tiempo): más robusto
 *   - Detección de Max-Q y reducción de empuje durante la zona de máxima presión dinámica
 *   - Motor corta cuando apoapsis alcanza la altitud objetivo → órbita elíptica real
 *   - La trayectoria ya NO alcanza velocidad de escape
 */
void demo_phase8C()
{
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        PHASE 8C — ASCENT GUIDANCE (Pitch + Max-Q + Orbit)   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    CelestialBody earth(
        "Earth", 5.972e24,
        6371000.0 * Constants::KSP_SCALE, 86164.0,
        Constants::MU_EARTH * Constants::KSP_SCALE * Constants::KSP_SCALE,
        9.81, true, 70000.0 * Constants::KSP_SCALE);

    Atmosphere atm = Atmosphere::makeEarthLike(Constants::KSP_SCALE);

    // Atlas-K: 2-stage with TWR ~1.37 at liftoff.
    // A high-TWR rocket (like the Falcon-K at 2.4) shoots through the atmosphere
    // too fast for the gravity turn to work — tanVel never exceeds radVel.
    // Lower TWR gives the pitch program time to build meaningful horizontal velocity.
    LaunchVehicle falcon("Atlas-K (KSP 0.1)");
    {
        // Stage 0 — First Stage (Atlas-class, TWR ~1.37 at liftoff, 147s burn)
        StageConfig s0; s0.name="Booster"; s0.strutMass=100; s0.decouplerMass=50;
        s0.hasDecoupler=true;
        s0.engines.push_back({"RD-180K", 200, 180000, 300, 1});
        s0.tanks.push_back({"S1-Tank", 600, 9000});
        falcon.addStage(s0);
    }
    {
        // Stage 1 — Upper Stage + payload (TWR ~2.67, vacuum burn)
        StageConfig s1; s1.name="UpperStage"; s1.strutMass=30; s1.hasDecoupler=false;
        s1.engines.push_back({"RL10K", 100, 90000, 350, 1});
        s1.tanks.push_back({"S2-Tank", 300, 2500});
        s1.tanks.push_back({"Capsule", 500, 0});
        falcon.addStage(s1);
    }

    // ── Ley de guiado para órbita ISS (400 km real = 40 km escala KSP) ───────
    GuidanceLaw guidance;
    guidance.hKick           = 200.0;    // m — inicio de pitch-over
    guidance.hTurn           = 6000.0;   // m — prograde tracking comienza (pitch a 45°)
    guidance.maxQLimit       = 35000.0;  // Pa — límite de presión dinámica
    guidance.maxQThrottle    = 0.75;     // fracción de throttle en zona Max-Q
    guidance.targetApoapsis  = 40000.0;  // m — apoapsis objetivo (400 km escala 0.1)

    AscentIntegrator integrator(falcon, earth, &atm);
    integrator.setThrottle(1.0);
    integrator.setRecordInterval(40);
    integrator.setGuidance(guidance);

    std::cout << "  Simulando ascenso guiado (target apo = "
              << guidance.targetApoapsis/1000.0 << " km)...\n";
    AscentResult res = integrator.simulate(0.05, 900.0, 0.0);

    // ── Resultados ────────────────────────────────────────────────────────────
    const std::string SEP(60, '-');
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "\n+" << std::string(60,'=') << "+\n";
    std::cout << "|  RESULTADO DEL ASCENSO GUIADO                              |\n";
    std::cout << "+" << SEP << "+\n";
    std::cout << "|  Razon de parada   : " << std::left << std::setw(37) << res.exitReason << "|\n";
    std::cout << "|  Insercion orbital : " << std::setw(37) << (res.orbitInserted ? "SI" : "NO") << "|\n";
    std::cout << std::right;
    std::cout << "|  Tiempo de corte   : " << std::setw(8) << res.cutoffTime     << " s                          |\n";
    std::cout << "|  Alt. de corte     : " << std::setw(8) << res.cutoffAltitude/1000.0 << " km                         |\n";
    std::cout << "|  Vel. de corte     : " << std::setw(8) << res.cutoffSpeed    << " m/s                       |\n";
    std::cout << "|  Altitud maxima    : " << std::setw(8) << res.maxAltitude/1000.0 << " km                         |\n";
    std::cout << "|  Velocidad maxima  : " << std::setw(8) << res.maxSpeed       << " m/s                       |\n";
    std::cout << "|  Etapas separadas  : " << std::setw(8) << res.stagesUsed     << "                            |\n";
    std::cout << "|  Llego al espacio  : " << std::setw(37) << (res.reachedSpace ? "SI" : "NO") << "|\n";

    auto fmtKm = [](double m) -> std::string {
        if (m > 1e15) return "escape traj.";
        return std::to_string((int)(m/1000.0)) + " km";
    };
    std::cout << "|  Apoapsis          : " << std::left << std::setw(37) << fmtKm(res.apoapsis)  << "|\n";
    std::cout << "|  Periapsis         : " << std::setw(37) << fmtKm(res.periapsis) << "|\n";
    std::cout << std::right;
    std::cout << "+" << SEP << "+\n";
    std::cout << "|  Max-Q             : " << std::setw(8) << res.maxDynPressure/1000.0
              << " kPa   t=" << std::setw(5) << res.maxDynPressureTime
              << " s  alt=" << std::setw(5) << res.maxDynPressureAlt/1000.0 << " km      |\n";
    std::cout << "+" << std::string(60,'=') << "+\n";

    // Velocidad orbital circular a la apoapsis para referencia
    double r_apo = earth.radius + res.apoapsis;
    double v_circ_apo = std::sqrt(earth.mu / r_apo);
    std::cout << "\n  Vel. circular en apoapsis (" << fmtKm(res.apoapsis) << "): "
              << std::setprecision(1) << v_circ_apo << " m/s\n";
    std::cout << "  (circularizacion en Phase 8E; delta-V necesario: ~"
              << std::setprecision(0) << std::abs(res.cutoffSpeed - v_circ_apo) << " m/s)\n";

    std::cout << "\n  Tiempos de separacion de etapa:\n";
    for (int i = 0; i < (int)res.stagingTimes.size(); ++i)
        std::cout << "    Etapa " << i << " (" << falcon.getStage(i).name << "): "
                  << std::setprecision(1) << res.stagingTimes[i] << " s\n";

    std::cout << "\n  Muestra de trayectoria (t, alt, vel, q, stage):\n";
    std::cout << "  " << std::string(65, '-') << "\n";
    int skip = std::max(1, (int)res.trajectory.size() / 12);
    for (int i = 0; i < (int)res.trajectory.size(); i += skip) {
        const auto& pt = res.trajectory[i];
        std::cout << "  t=" << std::setw(6) << pt.time
                  << " s  alt=" << std::setw(6) << pt.altitude/1000.0
                  << " km  v=" << std::setw(6) << pt.speed
                  << " m/s  q=" << std::setw(5) << pt.dynPressure/1000.0
                  << " kPa  s=" << pt.stageIndex << "\n";
    }

    std::cout << "\n  ✓ Phase 8C complete — guiado de ascenso + insercion orbital operativos\n";
}

// ── (fim demo_phase8C) ────────────────────────────────────────────────────────

/**
 * Phase 8A — Vehicle Modeling: multi-stage launch vehicle.
 *
 * Modela un cohete de 3 etapas al estilo Soyuz (escala KSP 0.1):
 *   Etapa 0 — Booster (4 cohetes laterales RD-107A)
 *   Etapa 1 — Etapa central (RD-108A)
 *   Etapa 2 — Tercera etapa (RD-0110) + cápsula Soyuz
 *
 * Demuestra: ΔV por etapa, TWR, fracción de payload, árbol de partes,
 * masa total del Vessel ensamblado, y desplazamiento de CoM.
 */
void demo_phase8A()
{
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        PHASE 8A — VEHICLE MODELING (LaunchVehicle)           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    // ── Tierra (escala KSP 0.1) ──────────────────────────────────────────────
    CelestialBody earth(
        "Earth",
        5.972e24,
        6371000.0 * Constants::KSP_SCALE,
        86164.0,
        Constants::MU_EARTH * Constants::KSP_SCALE * Constants::KSP_SCALE,
        9.81,
        true,
        70000.0 * Constants::KSP_SCALE);

    LaunchVehicle soyuz("Soyuz-FG (KSP 0.1)");

    // ── Etapa 0: 4 boosters laterales (RD-107A × 4) ─────────────────────────
    {
        StageConfig booster;
        booster.name         = "Boosters";
        booster.strutMass    = 800.0;
        booster.decouplerMass= 120.0;
        booster.hasDecoupler = true;
        booster.engines.push_back({"RD-107A", 1100.0, 838800.0, 310.7, 4});
        booster.tanks.push_back(  {"BoosterTank", 3200.0, 39600.0});
        soyuz.addStage(booster);
    }

    // ── Etapa 1: Etapa central (RD-108A × 1) ────────────────────────────────
    {
        StageConfig core;
        core.name         = "CoreStage";
        core.strutMass    = 600.0;
        core.decouplerMass= 80.0;
        core.hasDecoupler = true;
        core.engines.push_back({"RD-108A", 1250.0, 792400.0, 314.2, 1});
        core.tanks.push_back(  {"CoreTank", 6900.0, 91400.0});
        soyuz.addStage(core);
    }

    // ── Etapa 2: Tercera etapa (RD-0110) + cápsula Soyuz ────────────────────
    {
        StageConfig upper;
        upper.name         = "UpperStage";
        upper.strutMass    = 200.0;
        upper.hasDecoupler = false; // es la cima — no tiene decoupler propio
        upper.engines.push_back({"RD-0110", 410.0, 297900.0, 326.0, 1});
        upper.tanks.push_back(  {"UpperTank", 2355.0, 22000.0});
        upper.tanks.push_back(  {"SoyuzCapsule", 2800.0, 0.0}); // carga útil (seca)
        soyuz.addStage(upper);
    }

    // ── Análisis de rendimiento ──────────────────────────────────────────────
    soyuz.printSummary();

    // ── Verificación del Vessel ensamblado ───────────────────────────────────
    std::cout << "  Ensamblando Vessel...\n";
    Orbit launchOrbit(
        (earth.radius + 200000.0 * Constants::KSP_SCALE) * 1.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
        earth.mu, 0.0);

    auto vessel = soyuz.assemble(launchOrbit, &earth);

    std::cout << "  ✓ Vessel: " << vessel->name << "\n";
    std::cout << "    Masa total (árbol de partes): "
              << std::fixed << std::setprecision(1)
              << vessel->getTotalMass() / 1000.0 << " t\n";
    std::cout << "    CoM (modelo analítico): "
              << soyuz.getCoMHeight() << " m sobre la base\n";
    std::cout << "    Partes en el árbol: "
              << vessel->getAllParts().size() << "\n";

    // ── Desglose de CoM por burn ─────────────────────────────────────────────
    std::cout << "\n  CoM shift por etapa:\n";
    for (int i = 0; i < soyuz.stageCount(); ++i) {
        double shift = soyuz.getCoMShiftDuringBurn(i);
        std::cout << "    Etapa " << i << " (" << soyuz.getStage(i).name << "): "
                  << std::showpos << std::setprecision(2) << shift << " m\n"
                  << std::noshowpos;
    }

    std::cout << "\n  ✓ Phase 8A complete — LaunchVehicle operativo\n";
}

int main()
{
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║           PROJECT PHOENIX - PHASE 8 DEMONSTRATION            ║
║          Kerbal Space Program Clone (1:10 Scale)             ║
║                                                              ║
║  Orbital Mechanics · Parts · Propulsion · SoI · Aero · UI   ║
║  Phase 8A: LaunchVehicle — Stage modeling, DV, TWR, CoM     ║
║  Phase 8B: AscentIntegrator — RK4 launch, staging           ║
║  Phase 8C: GuidanceLaw — Pitch program, Max-Q, orbit ins.   ║
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
        example6_PartHierarchy();
        example7_Propulsion();
        example8_SphereOfInfluence();
        example9_AtmosphericReentry();
        example10_Visualization();
        demo_phase8A();
        demo_phase8B();
        demo_phase8C();

        std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                   EJEMPLOS COMPLETADOS                       ║
║                                                              ║
║  Phase 1 — Mecanica orbital Kepleriana (6 elementos)         ║
║  Phase 2 — Jerarquia de partes, CoM dinamico, staging        ║
║  Phase 3 — Motor cohete, Tsiolkovsky, burn pro-grado         ║
║  Phase 4 — Esferas de influencia, transicion SoI             ║
║  Phase 5 — Atmosfera ISA, arrastre D=1/2pv2CdA, RK4         ║
║  Phase 6 — Visualizacion ASCII: mapas orbitales, HUD         ║
║  Phase 8A — LaunchVehicle: etapas, DV, TWR, CoM             ║
║  Phase 8B — AscentIntegrator: RK4, staging, gravity turn    ║
║  Phase 8C — GuidanceLaw: pitch program, Max-Q, insercion    ║
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

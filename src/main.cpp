#include <iostream>
#include <iomanip>
#include <cmath>
#include <world/WorldManager.hpp>
#include <world/SphereOfInfluence.hpp>
#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>
#include <vessels/Vessel.hpp>
#include <parts/Part.hpp>
#include <parts/Engine.hpp>
#include <math/Constants.hpp>

using namespace Phoenix::Math;
using namespace Phoenix::Physics;
using namespace Phoenix::Vessels;
using namespace Phoenix::World;
using namespace Phoenix::Parts;

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

int main()
{
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║           PROJECT PHOENIX - PHASE 4 DEMONSTRATION            ║
║          Kerbal Space Program Clone (1:10 Scale)             ║
║                                                              ║
║   Orbital Mechanics · Parts · Propulsion · Esferas de SoI   ║
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

        std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                   EJEMPLOS COMPLETADOS                       ║
║                                                              ║
║  Phase 1 — Mecánica orbital Kepleriana (6 elementos)         ║
║  Phase 2 — Jerarquía de partes, CoM dinámico, staging        ║
║  Phase 3 — Motor cohete, Tsiolkovsky, burn pro-grado         ║
║  Phase 4 — Esferas de influencia, transición SoI            ║
║                                                              ║
║  Próxima fase: Aerodinámica y re-entrada                     ║
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

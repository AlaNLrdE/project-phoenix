#include <world/WorldManager.hpp>
#include <physics/Orbit.hpp>
#include <iostream>
#include <iomanip>

namespace Phoenix::World
{

    WorldManager::WorldManager()
        : activeVessel(nullptr), simulationTime(0.0), timeWarp(1),
          floatingOrigin(0.0, 0.0, 0.0) {}

    WorldManager::~WorldManager()
    {
        // Limpiar cuerpos celestes
        for (auto &pair : celestialBodies)
        {
            if (pair.second)
            {
                delete pair.second;
            }
        }
        celestialBodies.clear();

        // Limpiar naves
        for (auto vessel : vessels)
        {
            if (vessel)
            {
                delete vessel;
            }
        }
        vessels.clear();
    }

    void WorldManager::registerCelestialBody(CelestialBody *body)
    {
        if (body)
        {
            celestialBodies[body->name] = body;
        }
    }

    CelestialBody *WorldManager::getCelestialBody(const std::string &name) const
    {
        auto it = celestialBodies.find(name);
        if (it != celestialBodies.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void WorldManager::registerVessel(Vessel *vessel)
    {
        if (vessel)
        {
            vessels.push_back(vessel);
        }
    }

    Vessel *WorldManager::getVessel(const std::string &name) const
    {
        for (auto vessel : vessels)
        {
            if (vessel && vessel->name == name)
            {
                return vessel;
            }
        }
        return nullptr;
    }

    void WorldManager::setActiveVessel(const std::string &vesselName)
    {
        activeVessel = getVessel(vesselName);
    }

    void WorldManager::updateSimulation(double deltaTime)
    {
        // Aplicar time warp
        double actualDelta = deltaTime * timeWarp;
        simulationTime += actualDelta;

        // En Phase 1, solo avanzamos el tiempo
        // Actualizar tiempo de todas las naves
        for (auto vessel : vessels)
        {
            if (vessel)
            {
                vessel->currentTime = simulationTime;
            }
        }

        // Phase 4: verificar transiciones de SoI
        updateSoITransitions(simulationTime);

        // Phase 4: mantener precisión numérica con floating origin
        updateFloatingOrigin();
    }

    void WorldManager::printCelestialBodies() const
    {
        std::cout << "\n========== CUERPOS CELESTES ==========\n";
        std::cout << "Total: " << celestialBodies.size() << "\n\n";

        for (const auto &pair : celestialBodies)
        {
            const CelestialBody *body = pair.second;
            std::cout << "▪ " << body->name << "\n";
            std::cout << "  - Radio: " << std::scientific << std::setprecision(3)
                      << body->radius << " m\n";
            std::cout << "  - Masa: " << body->mass << " kg\n";
            std::cout << "  - μ = " << body->mu << " m³/s²\n";
            std::cout << "  - g_sup: " << std::fixed << std::setprecision(2)
                      << body->surfaceGravity << " m/s²\n";

            if (body->orbit)
            {
                std::cout << "  - Órbita alrededor de: " << body->parentBodyName
                          << "\n";
                std::cout << "    • Período: " << std::setprecision(2)
                          << body->orbit->getPeriod() / 86400.0 << " días\n";
            }
            else
            {
                std::cout << "  - Cuerpo primario\n";
            }

            if (body->hasAtmosphere)
            {
                std::cout << "  - Atmósfera: SÍ (" << std::scientific
                          << std::setprecision(0) << body->atmosphericHeight
                          << " m)\n";
            }
            std::cout << "\n";
        }

        std::cout << "======================================\n";
    }

    void WorldManager::printVessels() const
    {
        std::cout << "\n============ NAVES ============\n";
        std::cout << "Total: " << vessels.size() << "\n";
        if (activeVessel)
        {
            std::cout << "Nave activa: " << activeVessel->name << "\n";
        }
        std::cout << "\n";

        for (const auto vessel : vessels)
        {
            if (vessel)
            {
                vessel->printStatus();
            }
        }

        std::cout << "===============================\n";
    }

    // ── Phase 4: SoI y origin floating ──────────────────────────────────────────

    void WorldManager::buildBodyHierarchy()
    {
        for (auto &[name, body] : celestialBodies)
        {
            if (!body || body->parentBodyName.empty())
                continue;
            auto it = celestialBodies.find(body->parentBodyName);
            if (it != celestialBodies.end() && it->second)
            {
                body->setParentBody(it->second);
                it->second->addSatellite(body);
            }
        }
    }

    int WorldManager::updateSoITransitions(double t)
    {
        int transitions = 0;
        for (auto vessel : vessels)
        {
            if (!vessel || !vessel->referenceBody)
                continue;

            auto result = SphereOfInfluence::checkTransition(*vessel, celestialBodies, t);
            if (result.hasTransition)
            {
                applySoITransition(vessel, result.newBody,
                                   result.newPosition, result.newVelocity, t);
                ++transitions;
            }
        }
        return transitions;
    }

    void WorldManager::applySoITransition(Vessel *vessel,
                                          CelestialBody *newBody,
                                          const dvec3 &newPos,
                                          const dvec3 &newVel,
                                          double t)
    {
        if (!vessel || !newBody)
            return;

        std::cout << "[SoI] " << vessel->name
                  << ": " << vessel->referenceBodyName
                  << " → " << newBody->name << "\n";

        vessel->referenceBodyName = newBody->name;
        vessel->referenceBody = newBody;
        Orbit newOrbit(newPos, newVel, newBody->mu, t);
        vessel->updateOrbit(newOrbit);
    }

    void WorldManager::updateFloatingOrigin(double threshold)
    {
        if (!activeVessel)
            return;

        dvec3 vesselPos = activeVessel->getPosition(simulationTime);
        if (glm::length(vesselPos) < threshold)
            return;

        // Desplazar todos los cuerpos y naves
        floatingOrigin += vesselPos;

        for (auto &[name, body] : celestialBodies)
        {
            if (!body || !body->orbit)
                continue;
            // Los cuerpos se reposicionan mediante su órbita; no modificamos
            // sus posiciones directamente. El floating origin se aplica al
            // renderizador (fuera del alcance de este módulo de física).
        }

        // Para la nave activa, recentrar la órbita a posición relativa
        // Nota: solo recentramos el estado interno si la nave usa coordenadas
        // absolutas. En la arquitectura actual, la órbita ya es relativa al
        // referenceBody, por lo que el FO es solo para el renderizador.
        (void)threshold; // usado en la comprobación de longitud
    }

} // namespace Phoenix::World

#include <world/WorldManager.hpp>
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
        // En Phase 3+, aquí irá la propagación con empuje
        // En Phase 4+, aquí irá el desplazamiento flotante

        // Actualizar tiempo de todas las naves
        for (auto vessel : vessels)
        {
            if (vessel)
            {
                vessel->currentTime = simulationTime;
            }
        }
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

} // namespace Phoenix::World

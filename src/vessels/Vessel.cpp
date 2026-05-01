#include <vessels/Vessel.hpp>
#include <physics/CelestialBody.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <algorithm>

namespace Phoenix::Vessels
{

    using namespace Math;

    Vessel::Vessel(const std::string &name_, double dryMass_,
                   const Orbit &orbit_, const std::string &refBodyName,
                   CelestialBody *refBody)
        : name(name_), status("active"), dryMass(dryMass_), fuelMass(0.0),
          orbit(orbit_), currentTime(0.0),
          referenceBodyName(refBodyName), referenceBody(refBody),
          rootPart(nullptr) {}

    dvec3 Vessel::getPosition(double t) const
    {
        return orbit.getPositionAtTime(t);
    }

    dvec3 Vessel::getVelocity(double t) const
    {
        return orbit.getVelocityAtTime(t);
    }

    void Vessel::getState(double t, dvec3 &position, dvec3 &velocity) const
    {
        orbit.getStateAtTime(t, position, velocity);
    }

    void Vessel::updateOrbit(const Orbit &newOrbit)
    {
        orbit = newOrbit;
    }

    void Vessel::applyDeltaV(double deltaV, const dvec3 &direction)
    {
        // Normalizar dirección
        dvec3 dir = glm::normalize(direction);

        // Obtener estado actual
        dvec3 pos, vel;
        getState(currentTime, pos, vel);

        // Aplicar cambio de velocidad
        dvec3 newVel = vel + dir * deltaV;

        // Construir nueva órbita a partir del nuevo estado
        Orbit newOrbit(pos, newVel, orbit.mu, currentTime);
        updateOrbit(newOrbit);
    }

    bool Vessel::consumeFuel(double mass)
    {
        if (fuelMass >= mass)
        {
            fuelMass -= mass;
            return true;
        }
        return false;
    }

    double Vessel::getAltitude() const
    {
        if (!referenceBody)
            return 0.0;

        dvec3 pos = getPosition(currentTime);
        double distance = glm::length(pos);
        return distance - referenceBody->radius;
    }

    dvec3 Vessel::getRelativeVelocity() const
    {
        if (!referenceBody)
            return getVelocity(currentTime);

        // En Phase 1, no hay rotación del cuerpo considerada
        // En Phase 5, aquí irá la velocidad relativa a la atmósfera/superficie
        return getVelocity(currentTime);
    }

    void Vessel::printStatus() const
    {
        std::cout << "\n=== Estado de Nave: " << name << " ===\n";
        std::cout << "Referencia: " << referenceBodyName << "\n";
        std::cout << "Estado: " << status << "\n";
        std::cout << "Masa total: " << std::fixed << std::setprecision(1)
                  << getTotalMass() << " kg\n";
        std::cout << "  - Seca: " << dryMass << " kg\n";
        std::cout << "  - Combustible: " << fuelMass << " kg\n";
        std::cout << "Altitud: " << std::setprecision(0) << getAltitude()
                  << " m\n";

        dvec3 pos = getPosition(currentTime);
        dvec3 vel = getVelocity(currentTime);

        std::cout << "Posición: (" << std::setprecision(2)
                  << pos.x << ", " << pos.y << ", " << pos.z << ") m\n";
        std::cout << "Velocidad: (" << vel.x << ", " << vel.y << ", " << vel.z
                  << ") m/s\n";
        std::cout << "Velocidad orbital: " << std::setprecision(3)
                  << glm::length(vel) << " m/s\n";

        // Órbita
        std::cout << "\nElementos orbitales:\n";
        std::cout << "  Semieje mayor: " << std::setprecision(0) << orbit.a << " m\n";
        std::cout << "  Excentricidad: " << std::setprecision(6) << orbit.e << "\n";
        std::cout << "  Inclinación: " << std::setprecision(2)
                  << Units::RAD_TO_DEG(orbit.i) << "°\n";
        std::cout << "  Período: " << orbit.getPeriod() / 60.0 << " min\n";
        std::cout << "  Altitud periapsis: " << std::setprecision(0)
                  << (orbit.getDistanceToPeriapsis() - referenceBody->radius)
                  << " m\n";

        if (orbit.isStable())
        {
            std::cout << "  Altitud apoapsis: "
                      << (orbit.getDistanceToApoapsis() - referenceBody->radius)
                      << " m\n";
        }
        std::cout << "================================\n";
    }

    // ── Phase 2: implementaciones de Part hierarchy ────────────────────────────

    double Vessel::getTotalMass() const
    {
        if (rootPart)
            return rootPart->getTreeMass();
        return dryMass + fuelMass;
    }

    void Vessel::setRootPart(std::shared_ptr<Part> root)
    {
        rootPart = std::move(root);
    }

    std::vector<Part *> Vessel::getAllParts()
    {
        if (!rootPart)
            return {};
        return rootPart->getAllParts();
    }

    dvec3 Vessel::getCenterOfMass() const
    {
        if (!rootPart)
            return dvec3(0.0);
        return rootPart->getTreeCoM();
    }

    std::shared_ptr<Vessel> Vessel::stage()
    {
        if (!rootPart)
            return nullptr;

        // Buscar el primer Decoupler activo (DFS preorder)
        Part *decouplerRaw = nullptr;
        {
            auto all = getAllParts();
            for (auto *p : all)
            {
                if (p->type == PartType::Decoupler && p->isActive)
                {
                    decouplerRaw = p;
                    break;
                }
            }
        }

        if (!decouplerRaw)
            return nullptr;

        // Sin hijos: el separador estaba vacío, simplemente lo desactivamos
        if (decouplerRaw->children.empty())
        {
            decouplerRaw->isActive = false;
            return nullptr;
        }

        // Capturar la raíz de la etapa inferior ANTES de modificar el árbol
        std::shared_ptr<Part> newRoot = decouplerRaw->children.front();
        decouplerRaw->children.clear(); // newRoot sigue vivo por la copia anterior
        newRoot->parent = nullptr;

        // Desconectar el separador del árbol principal (lo mantiene vivo hasta fin de scope)
        auto decouplerRef = decouplerRaw->detachFromParent();
        (void)decouplerRef; // el separador se destruye al salir de scope

        // Crear la nueva nave con la etapa separada
        auto newVessel = std::make_shared<Vessel>(
            name + "_stage",
            0.0, // masa viene del árbol de partes
            orbit, referenceBodyName, referenceBody);
        newVessel->setRootPart(std::move(newRoot));
        return newVessel;
    }

    bool Vessel::dock(Vessel &other,
                      const std::string &ownPortName,
                      const std::string &otherPortName)
    {
        if (!rootPart || !other.rootPart)
            return false;

        // Encontrar nuestro puerto
        Part *ownPort = nullptr;
        for (auto *p : getAllParts())
        {
            if (p->name == ownPortName && p->type == PartType::DockingPort)
            {
                ownPort = p;
                break;
            }
        }
        if (!ownPort)
            return false;

        // Validar que el puerto de la otra nave existe
        bool otherFound = false;
        for (auto *p : other.getAllParts())
        {
            if (p->name == otherPortName && p->type == PartType::DockingPort)
            {
                otherFound = true;
                break;
            }
        }
        if (!otherFound)
            return false;

        // Mover la raíz de la otra nave como hijo de nuestro puerto
        ownPort->addChild(std::move(other.rootPart)); // other.rootPart = nullptr
        return true;
    }

    std::shared_ptr<Vessel> Vessel::undock(const std::string &portName)
    {
        if (!rootPart)
            return nullptr;

        Part *port = nullptr;
        for (auto *p : getAllParts())
        {
            if (p->name == portName && p->type == PartType::DockingPort)
            {
                port = p;
                break;
            }
        }
        if (!port || port->children.empty())
            return nullptr;

        // Separar el primer hijo del puerto (la nave acoplada)
        auto dockedRoot = port->children.front();
        port->children.erase(port->children.begin());
        dockedRoot->parent = nullptr;

        auto newVessel = std::make_shared<Vessel>(
            name + "_undocked",
            0.0,
            orbit, referenceBodyName, referenceBody);
        newVessel->setRootPart(std::move(dockedRoot));
        return newVessel;
    }

} // namespace Phoenix::Vessels

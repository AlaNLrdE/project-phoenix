#include <vessels/Vessel.hpp>
#include <parts/Engine.hpp>
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
          rootPart(nullptr),
          crossSectionalArea(10.0), dragCoefficient(0.5), noseRadius(0.5) {}

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

    std::shared_ptr<Vessel> Vessel::launchStage()
    {
        if (!rootPart)
            return nullptr;

        // Buscar el ÚLTIMO Decoupler activo en DFS preorder
        // (es el más profundo en el árbol → está en la etapa inferior)
        Part *decouplerRaw = nullptr;
        {
            auto all = getAllParts();
            for (auto *p : all) {
                if (p->type == PartType::Decoupler && p->isActive)
                    decouplerRaw = p;  // se actualiza en cada hallazgo → queda el último
            }
        }

        if (!decouplerRaw)
            return nullptr;

        if (decouplerRaw->children.empty()) {
            decouplerRaw->isActive = false;
            return nullptr;
        }

        std::shared_ptr<Part> newRoot = decouplerRaw->children.front();
        decouplerRaw->children.clear();
        newRoot->parent = nullptr;

        auto decouplerRef = decouplerRaw->detachFromParent();
        (void)decouplerRef;

        auto jettisoned = std::make_shared<Vessel>(
            name + "_jettisoned",
            0.0,
            orbit, referenceBodyName, referenceBody);
        jettisoned->setRootPart(std::move(newRoot));
        return jettisoned;
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

    // ── Phase 3: Propulsion ───────────────────────────────────────────────────

    std::vector<Engine *> Vessel::getActiveEngines()
    {
        std::vector<Engine *> engines;
        if (!rootPart)
            return engines;
        for (auto *p : getAllParts())
        {
            if (p->type == PartType::Engine && p->isActive)
            {
                engines.push_back(static_cast<Engine *>(p));
            }
        }
        return engines;
    }

    void Vessel::igniteEngines()
    {
        for (auto *e : getActiveEngines())
            e->ignite();
    }

    void Vessel::shutdownEngines()
    {
        for (auto *e : getActiveEngines())
            e->shutdown();
    }

    void Vessel::setThrottle(double t)
    {
        for (auto *e : getActiveEngines())
            e->setThrottle(t);
    }

    double Vessel::getTotalFuelMass() const
    {
        if (!rootPart)
            return fuelMass;
        double total = 0.0;
        // const_cast: getAllParts is logically const but not marked so
        auto &self = const_cast<Vessel &>(*this);
        for (auto *p : self.getAllParts())
        {
            if (p->isActive && p->type == PartType::FuelTank)
            {
                total += p->fuelMass;
            }
        }
        return total;
    }

    double Vessel::getTotalThrust() const
    {
        double total = 0.0;
        auto &self = const_cast<Vessel &>(*this);
        for (auto *p : self.getAllParts())
        {
            if (p->isActive && p->type == PartType::Engine)
            {
                total += static_cast<Engine *>(p)->getCurrentThrust();
            }
        }
        return total;
    }

    double Vessel::computeAvailableDeltaV() const
    {
        auto &self = const_cast<Vessel &>(*this);
        auto engines = self.getActiveEngines();
        if (engines.empty())
            return 0.0;

        double totalFuel = getTotalFuelMass();
        double dryVehicle = getTotalMass() - totalFuel;
        if (dryVehicle <= 0.0 || totalFuel <= 0.0)
            return 0.0;

        // Usa el primer motor como representativo del Isp (caso multi-motor homogéneo).
        // Para motores heterogéneos se usaría el Isp promedio ponderado por flujo.
        double totalMdot = 0.0;
        double thrustSum = 0.0;
        for (auto *e : engines)
        {
            thrustSum += e->getCurrentThrust() > 0.0 ? e->getCurrentThrust()
                                                     : e->maxThrust;
            totalMdot += e->getMassFlowRate() > 0.0 ? e->getMassFlowRate()
                                                    : (e->maxThrust / (e->Isp * Constants::G0));
        }
        double ispEff = (totalMdot > 0.0) ? (thrustSum / (totalMdot * Constants::G0)) : 0.0;
        if (ispEff <= 0.0)
            return 0.0;

        double ve = ispEff * Constants::G0;
        double m0 = getTotalMass();
        double mf = dryVehicle;
        return ve * std::log(m0 / mf);
    }

    double Vessel::executeBurn(double burnTime, dvec3 direction)
    {
        auto engines = getActiveEngines();
        if (engines.empty() || burnTime <= 0.0)
            return 0.0;

        // Dirección pro-grado por defecto
        dvec3 vel = getVelocity(currentTime);
        double velLen = glm::length(vel);
        if (glm::length(direction) < 1e-12)
        {
            direction = (velLen > 1e-6) ? (vel / velLen) : dvec3(0.0, 1.0, 0.0);
        }
        else
        {
            direction = glm::normalize(direction);
        }

        // Calcular ΔV real con la ecuación de Tsiolkovsky finita
        // Paso discreto: integrar flujo másico en dt pequeños
        constexpr double dt = 0.5; // s — paso de integración
        double timeLeft = burnTime;
        double m = getTotalMass();
        double deltaVAccum = 0.0;

        while (timeLeft > 1e-9)
        {
            double step = std::min(dt, timeLeft);

            // Flujo másico total
            double mdot = 0.0;
            double thrust = 0.0;
            for (auto *e : engines)
            {
                mdot += e->getMassFlowRate();
                thrust += e->getCurrentThrust();
            }
            if (mdot <= 0.0 || thrust <= 0.0)
                break;

            double fuelToBurn = mdot * step;
            double fuelAvail = getTotalFuelMass();

            if (fuelAvail <= 0.0)
                break;

            // Si no alcanza el combustible, reducir el paso
            if (fuelToBurn > fuelAvail)
            {
                step = fuelAvail / mdot;
                fuelToBurn = fuelAvail;
            }

            // Consumir combustible de los depósitos activos (en orden DFS)
            double remaining = fuelToBurn;
            for (auto *p : getAllParts())
            {
                if (!p->isActive || p->type != PartType::FuelTank)
                    continue;
                double taken = std::min(remaining, p->fuelMass);
                p->fuelMass -= taken;
                remaining -= taken;
                if (remaining <= 1e-12)
                    break;
            }

            // ΔV elemental: F * dt / m (aproximación de Euler)
            double dv = (thrust * step) / m;
            deltaVAccum += dv;
            m -= fuelToBurn;

            timeLeft -= step;
        }

        // Aplicar el ΔV acumulado como impulso instantáneo
        if (deltaVAccum > 1e-6)
        {
            dvec3 pos, v;
            getState(currentTime, pos, v);
            dvec3 newVel = v + direction * deltaVAccum;
            Orbit newOrbit(pos, newVel, orbit.mu, currentTime);
            updateOrbit(newOrbit);
        }

        return deltaVAccum;
    }

    // ── Phase 5: Aerodinámica ─────────────────────────────────────────────────

    Physics::AeroTrajectoryResult Vessel::simulateReentry(
        const Physics::Atmosphere &atm,
        double dt,
        int maxSteps) const
    {
        dvec3 r0 = getPosition(currentTime);
        dvec3 v0 = getVelocity(currentTime);
        double mass = getTotalMass();
        double bRadius = referenceBody ? referenceBody->radius
                                       : 6371000.0 * Constants::KSP_SCALE;
        double bMu = referenceBody ? referenceBody->mu : Constants::MU_EARTH;

        return Physics::AeroForces::simulate(
            r0, v0, mass,
            crossSectionalArea, dragCoefficient, noseRadius,
            bRadius, bMu, atm,
            dt, maxSteps);
    }

} // namespace Phoenix::Vessels

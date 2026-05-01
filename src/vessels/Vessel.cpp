#include <vessels/Vessel.hpp>
#include <physics/CelestialBody.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <cmath>
#include <iomanip>

namespace Phoenix::Vessels
{

    using namespace Math;

    Vessel::Vessel(const std::string &name_, double dryMass_,
                   const Orbit &orbit_, const std::string &refBodyName,
                   CelestialBody *refBody)
        : name(name_), status("active"), dryMass(dryMass_), fuelMass(0.0),
          orbit(orbit_), currentTime(0.0),
          referenceBodyName(refBodyName), referenceBody(refBody) {}

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

} // namespace Phoenix::Vessels

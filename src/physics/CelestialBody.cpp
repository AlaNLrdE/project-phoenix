#include <physics/CelestialBody.hpp>
#include <physics/Orbit.hpp>
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

namespace Phoenix::Physics
{

    using namespace Math;

    CelestialBody::CelestialBody(const std::string &name_, double mass_,
                                 double radius_, double rotPeriod, double mu_,
                                 double surfGrav, bool hasAtm, double atmHeight)
        : name(name_), mass(mass_), radius(radius_), rotationPeriod(rotPeriod),
          mu(mu_), surfaceGravity(surfGrav), orbit(nullptr), parentBody(nullptr),
          rotationAxis(0.0, 0.0, 1.0), atmosphericHeight(atmHeight),
          hasAtmosphere(hasAtm) {}

    CelestialBody::~CelestialBody()
    {
        // No destruimos la órbita aquí, ya que podría ser compartida
        // La responsabilidad de memoria recae en WorldManager
        satellites.clear();
    }

    void CelestialBody::setOrbit(Orbit *orbit_, const std::string &parentName,
                                 CelestialBody *parentPtr)
    {
        orbit = orbit_;
        parentBodyName = parentName;
        if (parentPtr)
            parentBody = parentPtr;
    }

    dvec3 CelestialBody::getPositionAtTime(double t) const
    {
        if (!orbit)
        {
            // Es un cuerpo primario
            return dvec3(0.0);
        }
        return orbit->getPositionAtTime(t);
    }

    dvec3 CelestialBody::getVelocityAtTime(double t) const
    {
        if (!orbit)
        {
            // Es un cuerpo primario
            return dvec3(0.0);
        }
        return orbit->getVelocityAtTime(t);
    }

    double CelestialBody::getOrbitalPeriodDays() const
    {
        if (!orbit)
            return 0.0;
        double period_s = orbit->getPeriod();
        return period_s / 86400.0; // Convertir a días
    }

    void CelestialBody::addSatellite(CelestialBody *satellite)
    {
        satellites.push_back(satellite);
    }

    bool CelestialBody::isInAtmosphere(const dvec3 &position) const
    {
        if (!hasAtmosphere)
            return false;

        double distance = glm::length(position);
        return distance >= radius && distance <= (radius + atmosphericHeight);
    }

    // ── Phase 4: SoI y marco mundial ──────────────────────────────────────────────────

    dvec3 CelestialBody::getWorldPosition(double t) const
    {
        if (!orbit || !parentBody)
            return dvec3(0.0); // cuerpo raíz
        return parentBody->getWorldPosition(t) + orbit->getPositionAtTime(t);
    }

    dvec3 CelestialBody::getWorldVelocity(double t) const
    {
        if (!orbit || !parentBody)
            return dvec3(0.0); // cuerpo raíz está quieto
        return parentBody->getWorldVelocity(t) + orbit->getVelocityAtTime(t);
    }

    double CelestialBody::getSoIRadius() const
    {
        if (!orbit || !parentBody || parentBody->mass <= 0.0)
            return -1.0;
        // r_SoI = a * (m_child / m_parent)^(2/5)
        return orbit->a * std::pow(mass / parentBody->mass, 0.4);
    }

} // namespace Phoenix::Physics

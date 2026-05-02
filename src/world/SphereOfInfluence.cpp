#include <world/SphereOfInfluence.hpp>
#include <physics/Orbit.hpp>
#include <glm/glm.hpp>
#include <cmath>

namespace Phoenix::World
{

    using namespace Math;
    using namespace Physics;

    // ── Cálculos básicos ──────────────────────────────────────────────────────────

    double SphereOfInfluence::computeRadius(double sma,
                                            double childMass,
                                            double parentMass)
    {
        if (parentMass <= 0.0 || sma <= 0.0)
            return 0.0;
        return sma * std::pow(childMass / parentMass, 0.4);
    }

    bool SphereOfInfluence::isInside(const dvec3 &relativePos, double soiRadius)
    {
        return glm::length(relativePos) < soiRadius;
    }

    // ── Transiciones de SoI ───────────────────────────────────────────────────────

    SphereOfInfluence::TransitionResult SphereOfInfluence::checkTransition(
        const Vessel &vessel,
        const std::map<std::string, CelestialBody *> &allBodies,
        double t)
    {
        TransitionResult result;
        CelestialBody *refBody = vessel.referenceBody;

        // Posición y velocidad de la nave en el marco inercial del cuerpo raíz
        dvec3 vesselPos = const_cast<Vessel &>(vessel).getPosition(t);
        dvec3 vesselVel = const_cast<Vessel &>(vessel).getVelocity(t);

        dvec3 refWorldPos(0.0), refWorldVel(0.0);
        if (refBody)
        {
            refWorldPos = refBody->getWorldPosition(t);
            refWorldVel = refBody->getWorldVelocity(t);
        }

        dvec3 vesselWorldPos = refWorldPos + vesselPos;
        dvec3 vesselWorldVel = refWorldVel + vesselVel;

        // ── Caso 1: Entrada en SoI de un hijo del cuerpo actual ─────────────────
        for (auto &[name, body] : allBodies)
        {
            if (body == refBody)
                continue; // es el cuerpo actual
            if (!body->orbit || !body->parentBody)
                continue; // es cuerpo raíz
            if (body->parentBody != refBody)
                continue; // no es hijo del actual

            double soiR = body->getSoIRadius();
            if (soiR <= 0.0)
                continue;

            dvec3 bodyWorldPos = body->getWorldPosition(t);
            dvec3 relPos = vesselWorldPos - bodyWorldPos;


            if (isInside(relPos, soiR))
            {
                // Nave entró en la SoI de este hijo
                dvec3 bodyWorldVel = body->getWorldVelocity(t);
                result.hasTransition = true;
                result.newBody = body;
                result.newPosition = relPos;
                result.newVelocity = vesselWorldVel - bodyWorldVel;
                return result;
            }
        }

        // ── Caso 2: Salida de la SoI del cuerpo actual (hacia el padre) ─────────
        if (refBody && refBody->orbit && refBody->parentBody)
        {
            double soiR = refBody->getSoIRadius();
            if (soiR > 0.0 && !isInside(vesselPos, soiR))
            {
                CelestialBody *parent = refBody->parentBody;
                dvec3 parentWorldPos = parent->getWorldPosition(t);
                dvec3 parentWorldVel = parent->getWorldVelocity(t);

                result.hasTransition = true;
                result.newBody = parent;
                result.newPosition = vesselWorldPos - parentWorldPos;
                result.newVelocity = vesselWorldVel - parentWorldVel;
                return result;
            }
        }

        return result; // sin transición
    }

} // namespace Phoenix::World

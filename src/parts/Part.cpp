#include <parts/Part.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include <stdexcept>

namespace Phoenix::Parts
{

    // ── Constructor ──────────────────────────────────────────────────────────────

    Part::Part(const std::string &name_, PartType type_,
               double dryMass_, double maxFuel)
        : name(name_), type(type_), dryMass(dryMass_),
          fuelMass(0.0), maxFuelMass(maxFuel),
          localPosition(0.0, 0.0, 0.0), isActive(true),
          parent(nullptr)
    {
    }

    // ── Masa ─────────────────────────────────────────────────────────────────────

    double Part::getTreeMass() const
    {
        double total = getOwnMass();
        for (const auto &child : children)
        {
            if (child && child->isActive)
            {
                total += child->getTreeMass();
            }
        }
        return total;
    }

    // ── Centro de masa ───────────────────────────────────────────────────────────

    dvec3 Part::getTreeCoM() const
    {
        double treeMass = getTreeMass();
        if (treeMass <= 0.0)
            return dvec3(0.0);

        // Esta parte se halla en el origen de su propio marco → contribuye (0,0,0).
        // Cada hijo contribuye desde (localPosition + CoM_del_hijo_en_su_marco).
        dvec3 weightedSum(0.0);
        for (const auto &child : children)
        {
            if (!child || !child->isActive)
                continue;
            double childMass = child->getTreeMass();
            dvec3 childCoM = child->localPosition + child->getTreeCoM();
            weightedSum += childCoM * childMass;
        }

        return weightedSum / treeMass;
    }

    // ── Jerarquía ────────────────────────────────────────────────────────────────

    void Part::addChild(std::shared_ptr<Part> child)
    {
        if (!child)
            return;
        child->parent = this;
        children.push_back(std::move(child));
    }

    std::vector<Part *> Part::getAllParts()
    {
        std::vector<Part *> result;
        result.push_back(this);
        for (const auto &child : children)
        {
            if (child && child->isActive)
            {
                auto sub = child->getAllParts();
                result.insert(result.end(), sub.begin(), sub.end());
            }
        }
        return result;
    }

    std::shared_ptr<Part> Part::detachFromParent()
    {
        if (!parent)
            return nullptr;
        auto &siblings = parent->children;
        for (auto it = siblings.begin(); it != siblings.end(); ++it)
        {
            if (it->get() == this)
            {
                std::shared_ptr<Part> self = *it;
                siblings.erase(it);
                parent = nullptr;
                return self;
            }
        }
        return nullptr;
    }

    // ── Combustible ──────────────────────────────────────────────────────────────

    bool Part::consumeFuel(double amount)
    {
        if (fuelMass >= amount)
        {
            fuelMass -= amount;
            return true;
        }
        return false;
    }

    double Part::addFuel(double amount)
    {
        double space = maxFuelMass - fuelMass;
        double added = std::min(amount, space);
        fuelMass += added;
        return added;
    }

    double Part::getFuelFraction() const
    {
        if (maxFuelMass <= 0.0)
            return 0.0;
        return fuelMass / maxFuelMass;
    }

    // ── Utilidades ───────────────────────────────────────────────────────────────

    std::string Part::typeToString(PartType t)
    {
        switch (t)
        {
        case PartType::Command:
            return "Command";
        case PartType::FuelTank:
            return "FuelTank";
        case PartType::Engine:
            return "Engine";
        case PartType::Decoupler:
            return "Decoupler";
        case PartType::Parachute:
            return "Parachute";
        case PartType::Structure:
            return "Structure";
        case PartType::DockingPort:
            return "DockingPort";
        default:
            return "Unknown";
        }
    }

} // namespace Phoenix::Parts

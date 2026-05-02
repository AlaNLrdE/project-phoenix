#include <launch/LaunchVehicle.hpp>
#include <parts/Part.hpp>
#include <parts/Engine.hpp>
#include <math/Constants.hpp>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace Phoenix::Launch {

using namespace Phoenix::Parts;
using namespace Phoenix::Physics;
using namespace Phoenix::Vessels;
using namespace Phoenix::Math;

// ── StageConfig ───────────────────────────────────────────────────────────────

double StageConfig::getDryMass() const {
    double m = strutMass;
    if (hasDecoupler) m += decouplerMass;
    for (const auto& e : engines) m += e.dryMass * e.count;
    for (const auto& t : tanks)   m += t.dryMass;
    return m;
}

double StageConfig::getPropellantMass() const {
    double m = 0.0;
    for (const auto& t : tanks) m += t.propellantMass;
    return m;
}

double StageConfig::getWetMass() const {
    return getDryMass() + getPropellantMass();
}

double StageConfig::getMaxThrust() const {
    double t = 0.0;
    for (const auto& e : engines) t += e.maxThrust * e.count;
    return t;
}

double StageConfig::getWeightedIsp() const {
    double totalThrust = getMaxThrust();
    if (totalThrust <= 0.0) return 0.0;
    double sum = 0.0;
    for (const auto& e : engines)
        sum += e.maxThrust * e.count * e.Isp;
    return sum / totalThrust;
}

// ── LaunchVehicle — constructor ───────────────────────────────────────────────

LaunchVehicle::LaunchVehicle(const std::string& n) : name(n) {}

void LaunchVehicle::addStage(StageConfig config) {
    stages.push_back(std::move(config));
}

// ── Masa ──────────────────────────────────────────────────────────────────────

double LaunchVehicle::getTotalMass() const {
    double m = 0.0;
    for (const auto& s : stages) m += s.getWetMass();
    return m;
}

double LaunchVehicle::getPayloadMass() const {
    return stages.empty() ? 0.0 : stages.back().getWetMass();
}

double LaunchVehicle::wetMassFromStage(int i) const {
    // Masa húmeda de stages[i..N-1]: la etapa i y todo lo que lleva encima.
    double m = 0.0;
    for (int j = i; j < (int)stages.size(); ++j)
        m += stages[j].getWetMass();
    return m;
}

// ── Rendimiento ───────────────────────────────────────────────────────────────

double LaunchVehicle::getStageDeltaV(int i) const {
    if (i < 0 || i >= (int)stages.size()) return 0.0;
    const auto& s = stages[i];
    double prop = s.getPropellantMass();
    if (prop <= 0.0) return 0.0;
    double isp = s.getWeightedIsp();
    if (isp <= 0.0) return 0.0;

    // Tsiolkovsky: ΔV = Isp * g0 * ln(m0 / mf)
    // m0 = masa de la etapa i + todo lo de arriba (al inicio del burn)
    // mf = m0 - propelante de la etapa i
    double m0 = wetMassFromStage(i);
    double mf = m0 - prop;
    if (mf <= 0.0) return 0.0;
    return isp * Constants::G0 * std::log(m0 / mf);
}

double LaunchVehicle::getTotalDeltaV() const {
    double dv = 0.0;
    for (int i = 0; i < (int)stages.size(); ++i)
        dv += getStageDeltaV(i);
    return dv;
}

double LaunchVehicle::getStageTWR(int i, double g) const {
    if (i < 0 || i >= (int)stages.size() || g <= 0.0) return 0.0;
    double thrust = stages[i].getMaxThrust();
    double mass   = wetMassFromStage(i);
    if (mass <= 0.0) return 0.0;
    return thrust / (mass * g);
}

// ── Centro de masa ────────────────────────────────────────────────────────────

double LaunchVehicle::stageHeight(const StageConfig& s) {
    // Heurística: cada tonelada de propelante aporta ~0.15 m de altura de columna,
    // más 0.5 m de estructura base.
    double propTons = s.getPropellantMass() / 1000.0;
    double engineLen = s.engines.empty() ? 0.0 : 1.2;
    return 0.5 + propTons * 0.15 + engineLen;
}

double LaunchVehicle::getCoMHeight() const {
    // CoM ponderado por masa con el booster (stages[0]) en la base.
    double num = 0.0, totalMass = 0.0, base = 0.0;
    for (const auto& s : stages) {
        double h   = stageHeight(s);
        double mid = base + h / 2.0;
        double m   = s.getWetMass();
        num       += m * mid;
        totalMass += m;
        base      += h;
    }
    return (totalMass > 0.0) ? num / totalMass : 0.0;
}

double LaunchVehicle::getCoMHeightAfterBurn(int i) const {
    if (i < 0 || i >= (int)stages.size()) return getCoMHeight();
    double num = 0.0, totalMass = 0.0, base = 0.0;
    for (int j = 0; j < (int)stages.size(); ++j) {
        const auto& s = stages[j];
        double h   = stageHeight(s);
        double mid = base + h / 2.0;
        // Después del burn de la etapa i: sólo queda la masa seca de esa etapa
        double m = (j == i) ? s.getDryMass() : s.getWetMass();
        num       += m * mid;
        totalMass += m;
        base      += h;
    }
    return (totalMass > 0.0) ? num / totalMass : 0.0;
}

double LaunchVehicle::getCoMShiftDuringBurn(int i) const {
    return getCoMHeightAfterBurn(i) - getCoMHeight();
}

double LaunchVehicle::getPayloadFraction() const {
    double total = getTotalMass();
    return (total > 0.0) ? getPayloadMass() / total : 0.0;
}

// ── Ensamblaje del árbol de partes ────────────────────────────────────────────

std::shared_ptr<Vessel> LaunchVehicle::assemble(
    const Orbit& initialOrbit, CelestialBody* refBody) const
{
    if (stages.empty())
        throw std::runtime_error("LaunchVehicle::assemble — no hay etapas definidas");

    // Construimos el árbol de arriba (payload) hacia abajo (booster).
    // stages.back() = upper/payload → root del árbol
    // stages[0]     = booster       → hoja más profunda

    std::shared_ptr<Part> treeRoot  = nullptr;
    Part*                 attachTo  = nullptr; // nodo donde conectar la siguiente etapa

    for (int i = (int)stages.size() - 1; i >= 0; --i) {
        const auto& s     = stages[i];
        bool        isTop = (i == (int)stages.size() - 1);

        // Nodo raíz de esta etapa (Command para la superior, Structure para el resto)
        auto stageRoot = std::make_shared<Part>(
            s.name + "_struct",
            isTop ? PartType::Command : PartType::Structure,
            s.strutMass > 0.0 ? s.strutMass : 10.0);

        // Depósitos de combustible
        for (const auto& t : s.tanks) {
            auto tank = std::make_shared<Part>(
                t.name, PartType::FuelTank, t.dryMass, t.propellantMass);
            tank->fuelMass = t.propellantMass;
            stageRoot->addChild(tank);
        }

        // Motores
        for (const auto& e : s.engines) {
            for (int k = 0; k < e.count; ++k) {
                std::string eName = (e.count > 1)
                    ? e.name + "_" + std::to_string(k + 1)
                    : e.name;
                auto eng = std::make_shared<Engine>(eName, e.dryMass,
                                                    e.maxThrust, e.Isp);
                stageRoot->addChild(eng);
            }
        }

        if (treeRoot == nullptr) {
            // Primera iteración: etapa superior = raíz del árbol
            treeRoot = stageRoot;
            attachTo = stageRoot.get();
        } else {
            // Conectar la etapa siguiente mediante un separador (Decoupler).
            // El Decoupler se cuelga del nodo de anclaje de la etapa superior.
            auto decoupler = std::make_shared<Part>(
                s.name + "_decoupler",
                PartType::Decoupler,
                stages[i + 1].decouplerMass); // masa del decoupler de la etapa sobre este
            attachTo->addChild(decoupler);
            decoupler->addChild(stageRoot);
            attachTo = stageRoot.get();
        }
    }

    auto vessel = std::make_shared<Vessel>(
        name, 0.0, initialOrbit,
        refBody ? refBody->name : "Unknown",
        refBody);
    vessel->setRootPart(treeRoot);
    return vessel;
}

// ── Resumen de rendimiento ────────────────────────────────────────────────────

void LaunchVehicle::printSummary() const {
    const std::string SEP(64, '=');
    const std::string DIV(64, '-');
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\n+" << SEP << "+\n";
    std::cout << "|  COHETE: " << std::left << std::setw(55) << name << "|\n";
    std::cout << "+" << SEP << "+\n";

    double totalMass   = getTotalMass();
    double totalDV     = getTotalDeltaV();
    double payloadFrac = getPayloadFraction() * 100.0;
    double com         = getCoMHeight();

    std::cout << "|  Masa total : " << std::right << std::setw(9) << totalMass / 1000.0
              << " t    |  DV total  : " << std::setw(9) << totalDV << " m/s  |\n";
    std::cout << "|  Payload    : " << std::setw(8) << payloadFrac
              << " %    |  CoM base  : " << std::setw(8) << com    << " m        |\n";
    std::cout << "+" << SEP << "+\n";
    std::cout << "|  #  Etapa          | Wet(t) | DV(m/s)  |  TWR  | Isp | F(kN) |\n";
    std::cout << "+" << DIV << "+\n";

    for (int i = (int)stages.size() - 1; i >= 0; --i) {
        const auto& s   = stages[i];
        double twr = getStageTWR(i);
        double dv  = getStageDeltaV(i);
        double wet = s.getWetMass() / 1000.0;
        double isp = s.getWeightedIsp();
        double kn  = s.getMaxThrust() / 1000.0;
        std::cout << "|  " << std::setw(2) << i
                  << "  " << std::left << std::setw(14) << s.name.substr(0, 14)
                  << "| " << std::right << std::setw(6) << wet
                  << " | " << std::setw(8) << dv
                  << " | " << std::setw(5) << twr
                  << " | " << std::setw(3) << (int)isp
                  << " | " << std::setw(5) << kn << " |\n";
    }

    std::cout << "+" << DIV << "+\n";
    std::cout << "|  Desplazamiento de CoM durante cada burn:                               |\n";
    for (int i = (int)stages.size() - 1; i >= 0; --i) {
        double shift = getCoMShiftDuringBurn(i);
        std::cout << "|    Etapa " << std::left << std::setw(14) << stages[i].name.substr(0, 14)
                  << ": " << std::right << std::showpos << std::setprecision(3)
                  << std::setw(7) << shift << std::noshowpos
                  << " m                                         |\n";
    }
    std::cout << "+" << SEP << "+\n\n";
}

} // namespace Phoenix::Launch

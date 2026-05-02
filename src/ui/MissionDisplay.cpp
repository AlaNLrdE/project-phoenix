#include <ui/MissionDisplay.hpp>
#include <parts/Engine.hpp>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Phoenix::UI
{

// Inner content width (chars between the two ║ borders)
static constexpr int W = 56;

// ─── Private helpers ──────────────────────────────────────────────────────────

std::string MissionDisplay::progressBar(double fraction, int width)
{
    fraction = std::clamp(fraction, 0.0, 1.0);
    int filled = static_cast<int>(fraction * width + 0.5);
    return "[" + std::string(filled, '#') + std::string(width - filled, '-') + "]";
}

std::string MissionDisplay::formatTime(double t)
{
    int total = static_cast<int>(std::abs(t));
    int hrs   = total / 3600;
    int min   = (total % 3600) / 60;
    int sec   = total % 60;
    std::ostringstream s;
    s << std::setfill('0')
      << std::setw(3) << hrs << "h "
      << std::setw(2) << min << "m "
      << std::setw(2) << sec << "s"
      << std::setfill(' ');
    return s.str();
}

// Emit a row padded to W chars between ║ borders
static void drow(std::ostream &os, const std::string &s)
{
    int padLen = W - static_cast<int>(s.size());
    if (padLen < 0) padLen = 0;
    os << "  \u2551" << s << std::string(padLen, ' ') << "\u2551\n";
}

// ─── Public interface ─────────────────────────────────────────────────────────

void MissionDisplay::printDashboard(const Vessel &vessel, const CelestialBody &body,
                                    double t, std::ostream &os)
{
    // Helper lambdas
    auto row = [&](const std::string &s) { drow(os, s); };
    auto div = [&]() {
        std::string line;
        for (int i = 0; i < W; ++i) line += "\u2550";
        os << "  \u2560" << line << "\u2563\n";
    };

    // Top border
    {
        std::string line;
        for (int i = 0; i < W; ++i) line += "\u2550";
        os << "\n  \u2554" << line << "\u2557\n";
    }

    // Title
    {
        std::ostringstream s;
        s << "  MISSION CONTROL  \u2014  " << vessel.name;
        row(s.str());
    }
    div();

    // Time and reference
    {
        std::ostringstream s;
        s << "  T+" << formatTime(t) << "   Ref: " << body.name;
        row(s.str());
    }
    {
        std::ostringstream s;
        s << "  Estado: " << vessel.status;
        row(s.str());
    }
    div();

    // Orbital telemetry
    dvec3 pos = vessel.getPosition(t);
    dvec3 vel = vessel.getVelocity(t);
    double r   = glm::length(pos);
    double v   = glm::length(vel);
    double alt = (r - body.radius) / 1000.0;

    const Orbit &orb = vessel.orbit;
    double rp  = orb.a * (1.0 - orb.e);
    double ra  = orb.a * (1.0 + orb.e);
    double altp = (rp - body.radius) / 1000.0;
    double alta = (ra - body.radius) / 1000.0;

    {
        std::ostringstream s;
        s << "  Altitud:    " << std::fixed << std::setprecision(1) << alt
          << " km     Velocidad: " << std::setprecision(2) << v << " m/s";
        row(s.str());
    }
    {
        std::ostringstream s;
        s << "  Periapsis:  " << std::fixed << std::setprecision(1) << altp
          << " km     Apoapsis:  " << std::setprecision(1) << alta << " km";
        row(s.str());
    }
    {
        std::ostringstream s;
        double Tmin = orb.getPeriod() / 60.0;
        s << "  Período:    " << std::fixed << std::setprecision(1) << Tmin
          << " min    e = " << std::setprecision(4) << orb.e;
        row(s.str());
    }
    {
        std::ostringstream s;
        s << "  i = " << std::fixed << std::setprecision(2) << Units::RAD_TO_DEG(orb.i)
          << "\xc2\xb0" // °
          << "   \u03a9 = " << Units::RAD_TO_DEG(orb.Omega) // Ω
          << "\xc2\xb0"
          << "   \u03c9 = " << Units::RAD_TO_DEG(orb.omega) // ω
          << "\xc2\xb0";
        row(s.str());
    }
    div();

    // Mass
    {
        std::ostringstream s;
        s << "  Masa total: " << std::fixed << std::setprecision(1)
          << vessel.getTotalMass() << " kg";
        row(s.str());
    }

    // Bottom border
    {
        std::string line;
        for (int i = 0; i < W; ++i) line += "\u2550";
        os << "  \u255a" << line << "\u255d\n\n";
    }
}

void MissionDisplay::printOrbitalElements(const Orbit &orbit, const CelestialBody &body,
                                          std::ostream &os)
{
    double bodyRadius = body.radius;
    double a   = orbit.a;
    double e   = orbit.e;
    double rp  = a * (1.0 - e);
    double ra  = a * (1.0 + e);
    double T   = orbit.getPeriod();

    auto row = [&](const std::string &label, const std::string &val) {
        os << "    " << std::left << std::setw(24) << label
           << std::right << std::setw(18) << val << "\n";
    };

    os << "  Elementos orbitales:\n";
    os << "  " << std::string(46, '-') << "\n";

    { std::ostringstream s; s << std::scientific << std::setprecision(4) << a << " m";      row("Semieje mayor (a):", s.str()); }
    { std::ostringstream s; s << std::fixed << std::setprecision(6) << e;                   row("Excentricidad (e):", s.str()); }
    { std::ostringstream s; s << std::fixed << std::setprecision(3) << Units::RAD_TO_DEG(orbit.i) << "°";     row("Inclinación (i):", s.str()); }
    { std::ostringstream s; s << std::fixed << std::setprecision(3) << Units::RAD_TO_DEG(orbit.Omega) << "°"; row("RAAN (Ω):", s.str()); }
    { std::ostringstream s; s << std::fixed << std::setprecision(3) << Units::RAD_TO_DEG(orbit.omega) << "°"; row("Arg. periapsis (ω):", s.str()); }
    { std::ostringstream s; s << std::fixed << std::setprecision(3) << Units::RAD_TO_DEG(orbit.nu) << "°";   row("Anom. verdadera (ν):", s.str()); }

    os << "  " << std::string(46, '-') << "\n";

    { std::ostringstream s; s << std::fixed << std::setprecision(1) << (rp - bodyRadius) / 1000.0 << " km"; row("Periapsis (alt.):", s.str()); }
    { std::ostringstream s; s << std::fixed << std::setprecision(1) << (ra - bodyRadius) / 1000.0 << " km"; row("Apoapsis (alt.):", s.str()); }
    {
        std::ostringstream s;
        if (T < 3600.0)        s << std::fixed << std::setprecision(1) << T / 60.0   << " min";
        else if (T < 86400.0)  s << std::fixed << std::setprecision(2) << T / 3600.0  << " h";
        else                   s << std::fixed << std::setprecision(2) << T / 86400.0 << " días";
        row("Período:", s.str());
    }

    os << "  " << std::string(46, '-') << "\n\n";
}

void MissionDisplay::printFuelReport(const Vessel &vessel, std::ostream &os)
{
    double totalMass = vessel.getTotalMass();
    double fuelMass  = vessel.fuelMass;
    double fuelFrac  = (totalMass > 0.0) ? fuelMass / totalMass : 0.0;

    os << "  Informe de combustible:\n";
    os << "  " << std::string(46, '-') << "\n";
    os << "    Masa total:     " << std::fixed << std::setprecision(1)
       << std::setw(10) << totalMass << " kg\n";
    os << "    Masa seca:      " << std::setw(10) << vessel.dryMass << " kg\n";
    os << "    Combustible:    " << std::setw(10) << fuelMass       << " kg\n";
    os << "    " << progressBar(fuelFrac, 28)
       << "  " << std::setprecision(1) << fuelFrac * 100.0 << "%\n";

    // ΔV from Tsiolkovsky if engines present in part tree
    if (fuelMass > 0.0 && vessel.dryMass > 0.0 && vessel.rootPart)
    {
        double totalIspF = 0.0, totalF = 0.0;
        for (Parts::Part *part : vessel.rootPart->getAllParts())
        {
            if (part->type == PartType::Engine)
            {
                auto *eng = static_cast<Parts::Engine *>(part);
                totalF    += eng->maxThrust;
                totalIspF += eng->maxThrust * eng->Isp;
            }
        }
        if (totalF > 0.0)
        {
            double Isp = totalIspF / totalF;
            double dv  = Isp * 9.80665 * std::log(totalMass / vessel.dryMass);
            os << "    Isp efectivo:   " << std::setw(10) << std::setprecision(1) << Isp << " s\n";
            os << "    \u0394V disponible:  " << std::setw(10) << std::setprecision(1) << dv  << " m/s\n";
        }
    }

    os << "  " << std::string(46, '-') << "\n\n";
}

void MissionDisplay::printManeuverNode(const std::string &name, double deltaV,
                                       double burnTime, std::ostream &os)
{
    os << "  Nodo de maniobra: " << name << "\n";
    os << "  " << std::string(46, '-') << "\n";
    os << "    \u0394V requerido:    " << std::fixed << std::setprecision(2)
       << std::setw(10) << deltaV   << " m/s\n";
    os << "    Tiempo quemado:  " << std::setprecision(1)
       << std::setw(10) << burnTime << " s\n";
    os << "  " << std::string(46, '-') << "\n\n";
}

} // namespace Phoenix::UI

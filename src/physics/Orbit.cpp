#include <physics/Orbit.hpp>
#include <cmath>
#include <stdexcept>
#include <glm/glm.hpp>

namespace Phoenix::Physics
{

    using namespace Math;

    // Constructor por defecto
    // Convierte anomalía verdadera a media para una órbita elíptica
    static double trueToMeanAnomaly(double nu, double e)
    {
        // Anomalía excéntrica
        double E = 2.0 * std::atan2(std::sqrt(1.0 - e) * std::sin(nu / 2.0),
                                    std::sqrt(1.0 + e) * std::cos(nu / 2.0));
        if (E < 0.0)
            E += 2.0 * M_PI;
        double M = E - e * std::sin(E);
        if (M < 0.0)
            M += 2.0 * M_PI;
        return M;
    }

    Orbit::Orbit()
        : a(7000000.0), e(0.0), i(0.0), Omega(0.0), omega(0.0), nu(0.0),
          mu(Constants::MU_EARTH), epoch(0.0), M0(0.0) {}

    // Constructor con elementos orbitales
    Orbit::Orbit(double a_, double e_, double i_, double Omega_, double omega_,
                 double nu_, double mu_, double epoch_)
        : a(a_), e(e_), i(i_), Omega(Omega_), omega(omega_), nu(nu_),
          mu(mu_), epoch(epoch_),
          M0(e_ < 1.0 ? trueToMeanAnomaly(nu_, e_) : 0.0) {}

    // Constructor desde vectores de estado
    Orbit::Orbit(const dvec3 &r, const dvec3 &v, double mu_, double epoch_)
        : mu(mu_), epoch(epoch_)
    {
        // Cálculo de elementos orbitales a partir de r y v
        // (Six-Vector to Orbital Elements conversion)

        double rmag = glm::length(r);
        double vmag = glm::length(v);

        if (rmag < 1e-6)
            throw std::invalid_argument("Posición muy cercana al origen");

        // Parámetro específico de energía orbital
        double xi = (vmag * vmag) / 2.0 - mu_ / rmag;

        // Semieje mayor
        a = -mu_ / (2.0 * xi);

        // Vector momento angular
        dvec3 h = glm::cross(r, v);
        double hmag = glm::length(h);

        // Excentricidad (vector de excentricidad)
        dvec3 e_vec = glm::cross(v, h) / mu_ - r / rmag;
        e = glm::length(e_vec);

        // Inclinación
        i = std::acos(glm::clamp(h.z / hmag, -1.0, 1.0));

        // Nodo ascendente (intersección con plano ecuatorial)
        dvec3 n_vec = glm::cross(dvec3(0, 0, 1), h);
        double nmag = glm::length(n_vec);

        if (nmag > 1e-6)
        {
            Omega = std::atan2(n_vec.y, n_vec.x);
        }
        else
        {
            Omega = 0.0; // Órbita ecuatorial, no hay nodo definido
        }

        if (Omega < 0)
            Omega += 2.0 * M_PI;

        // Argumento del periapsis
        if (nmag > 1e-6)
        {
            omega = std::atan2(e_vec.z, glm::dot(n_vec, e_vec));
        }
        else
        {
            omega = std::atan2(e_vec.y, e_vec.x);
        }

        if (omega < 0)
            omega += 2.0 * M_PI;

        // Anomalía verdadera
        double cos_nu = (e > 1e-10) ? glm::dot(e_vec, r) / (e * rmag) : 0.0;
        cos_nu = glm::clamp(cos_nu, -1.0, 1.0);

        nu = std::acos(cos_nu);
        if (glm::dot(r, v) < 0)
            nu = 2.0 * M_PI - nu;

        // Anomalía media inicial
        M0 = (e < 1.0) ? trueToMeanAnomaly(nu, e) : 0.0;
    }

    // Resuelve la ecuación de Kepler: M = E - e·sin(E)
    double Orbit::solveKeplersEquation(double M) const
    {
        // Normalizar M a [0, 2π]
        M = std::fmod(M, 2.0 * M_PI);
        if (M < 0)
            M += 2.0 * M_PI;

        // Adivinanza inicial (método de Ramanujan mejorado)
        double E = (e < 0.8) ? M : M_PI;

        // Newton-Raphson
        for (int iter = 0; iter < Constants::KEPLER_MAX_ITERATIONS; ++iter)
        {
            double sin_E = std::sin(E);
            double cos_E = std::cos(E);

            double f = E - e * sin_E - M;
            double f_prime = 1.0 - e * cos_E;

            double dE = f / f_prime;
            E -= dE;

            if (std::abs(dE) < Constants::KEPLER_TOLERANCE)
            {
                return E;
            }
        }

        return E; // Convergencia obtenida o máx iteraciones
    }

    // Convierte anomalía excéntrica a verdadera
    double Orbit::eccentricToTrueAnomaly(double E) const
    {
        double tan_half_E = std::tan(E / 2.0);
        double tan_half_nu = std::sqrt((1.0 + e) / (1.0 - e)) * tan_half_E;
        return 2.0 * std::atan(tan_half_nu);
    }

    // Convierte anomalía verdadera a excéntrica
    double Orbit::trueToEccentricAnomaly(double nu_) const
    {
        double cos_nu = std::cos(nu_);
        double sin_nu = std::sin(nu_);

        double E = 2.0 * std::atan2(std::sqrt(1.0 - e) * sin_nu,
                                    1.0 + e + (1.0 + e) * cos_nu);
        return E;
    }

    // Calcula los vectores de estado en el plano orbital
    void Orbit::getOrbitalPlaneState(double nu_, dvec3 &r_orb, dvec3 &v_orb) const
    {
        // Semi-latus rectum
        double p = a * (1.0 - e * e);

        // Distancia radial
        double r_mag = p / (1.0 + e * std::cos(nu_));

        // Posición en plano orbital (periapsis como referencia)
        r_orb = dvec3(r_mag * std::cos(nu_), r_mag * std::sin(nu_), 0.0);

        // Velocidad en plano orbital
        double v_coeff = std::sqrt(mu / p);
        v_orb = dvec3(-v_coeff * std::sin(nu_),
                      v_coeff * (e + std::cos(nu_)),
                      0.0);
    }

    // Transforma del plano orbital al inercial 3D
    dvec3 Orbit::orbitalToInertial(const dvec3 &r_orb) const
    {
        // Matriz de rotación: primero ω, luego i, luego Ω
        // R = R_z(Ω) · R_x(i) · R_z(ω)

        double cos_w = std::cos(omega), sin_w = std::sin(omega);
        double cos_i = std::cos(i), sin_i = std::sin(i);
        double cos_O = std::cos(Omega), sin_O = std::sin(Omega);

        // Matriz de rotación combinada
        dmat3 R(
            cos_O * cos_w - sin_O * sin_w * cos_i,
            -cos_O * sin_w - sin_O * cos_w * cos_i,
            sin_O * sin_i,

            sin_O * cos_w + cos_O * sin_w * cos_i,
            -sin_O * sin_w + cos_O * cos_w * cos_i,
            -cos_O * sin_i,

            sin_w * sin_i,
            cos_w * sin_i,
            cos_i);

        return R * r_orb;
    }

    // Obtiene la posición en tiempo t
    dvec3 Orbit::getPositionAtTime(double t) const
    {
        double dt = t - epoch;

        // Movimiento medio
        double n = std::sqrt(mu / (a * a * a));

        // Anomalía media
        double M = std::fmod(M0 + n * dt, 2.0 * M_PI); if (M < 0.0) M += 2.0 * M_PI;

        // Resolver ecuación de Kepler
        double E = solveKeplersEquation(M);

        // Anomalía verdadera
        double nu_t = eccentricToTrueAnomaly(E);

        // Vectores en plano orbital
        dvec3 r_orb, v_orb;
        getOrbitalPlaneState(nu_t, r_orb, v_orb);

        // Transformar a marco inercial
        return orbitalToInertial(r_orb);
    }

    // Obtiene la velocidad en tiempo t
    dvec3 Orbit::getVelocityAtTime(double t) const
    {
        double dt = t - epoch;

        // Movimiento medio
        double n = std::sqrt(mu / (a * a * a));

        // Anomalía media
        double M = std::fmod(M0 + n * dt, 2.0 * M_PI); if (M < 0.0) M += 2.0 * M_PI;

        // Resolver ecuación de Kepler
        double E = solveKeplersEquation(M);

        // Anomalía verdadera
        double nu_t = eccentricToTrueAnomaly(E);

        // Vectores en plano orbital
        dvec3 r_orb, v_orb;
        getOrbitalPlaneState(nu_t, r_orb, v_orb);

        // Transformar a marco inercial
        double cos_w = std::cos(omega), sin_w = std::sin(omega);
        double cos_i = std::cos(i), sin_i = std::sin(i);
        double cos_O = std::cos(Omega), sin_O = std::sin(Omega);

        dmat3 R(
            cos_O * cos_w - sin_O * sin_w * cos_i,
            -cos_O * sin_w - sin_O * cos_w * cos_i,
            sin_O * sin_i,

            sin_O * cos_w + cos_O * sin_w * cos_i,
            -sin_O * sin_w + cos_O * cos_w * cos_i,
            -cos_O * sin_i,

            sin_w * sin_i,
            cos_w * sin_i,
            cos_i);

        return R * v_orb;
    }

    // Obtiene ambos vectores de estado
    void Orbit::getStateAtTime(double t, dvec3 &position, dvec3 &velocity) const
    {
        double dt = t - epoch;

        // Movimiento medio
        double n = std::sqrt(mu / (a * a * a));

        // Anomalía media
        double M = std::fmod(M0 + n * dt, 2.0 * M_PI); if (M < 0.0) M += 2.0 * M_PI;

        // Resolver ecuación de Kepler una sola vez
        double E = solveKeplersEquation(M);

        // Anomalía verdadera
        double nu_t = eccentricToTrueAnomaly(E);

        // Vectores en plano orbital
        dvec3 r_orb, v_orb;
        getOrbitalPlaneState(nu_t, r_orb, v_orb);

        // Matriz de rotación (calcular una vez)
        double cos_w = std::cos(omega), sin_w = std::sin(omega);
        double cos_i = std::cos(i), sin_i = std::sin(i);
        double cos_O = std::cos(Omega), sin_O = std::sin(Omega);

        dmat3 R(
            cos_O * cos_w - sin_O * sin_w * cos_i,
            -cos_O * sin_w - sin_O * cos_w * cos_i,
            sin_O * sin_i,

            sin_O * cos_w + cos_O * sin_w * cos_i,
            -sin_O * sin_w + cos_O * cos_w * cos_i,
            -cos_O * sin_i,

            sin_w * sin_i,
            cos_w * sin_i,
            cos_i);

        position = R * r_orb;
        velocity = R * v_orb;
    }

    // Actualiza elementos orbitales
    void Orbit::updateElements(double a_, double e_, double i_, double Omega_,
                               double omega_, double nu_)
    {
        a = a_;
        e = e_;
        i = i_;
        Omega = Omega_;
        omega = omega_;
        nu = nu_;
    }

    // Calcula el período orbital
    double Orbit::getPeriod() const
    {
        if (!isStable())
        {
            return 0.0; // Sin período para trayectorias hiperbólicas/parabólicas
        }
        return 2.0 * M_PI * std::sqrt(a * a * a / mu);
    }

    // Calcula altitud
    double Orbit::getAltitude(double bodyRadius) const
    {
        double p = a * (1.0 - e * e);
        double r_mag = p / (1.0 + e * std::cos(nu));
        return r_mag - bodyRadius;
    }

    // Distancia al periapsis
    double Orbit::getDistanceToPeriapsis() const
    {
        return a * (1.0 - e);
    }

    // Distancia al apoapsis
    double Orbit::getDistanceToApoapsis() const
    {
        if (!isStable())
            return 0.0;
        return a * (1.0 + e);
    }

} // namespace Phoenix::Physics

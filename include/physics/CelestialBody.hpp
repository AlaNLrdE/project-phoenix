#pragma once

#include <math/Constants.hpp>
#include <string>
#include <vector>

namespace Phoenix::Physics {

using namespace Math;

/**
 * @class CelestialBody
 * @brief Representa un cuerpo celeste (planeta, luna, estrella).
 * 
 * Almacena propiedades físicas y orbitales del cuerpo.
 * En Phase 1, los cuerpos están en posiciones analíticas fijas.
 * En Phase 4 (SoI), se implementará la propagación dinámica.
 */
class CelestialBody {
public:
    // Propiedades físicas
    std::string name;           ///< Nombre del cuerpo
    double mass;                ///< Masa (kg)
    double radius;              ///< Radio ecuatorial (metros)
    double rotationPeriod;      ///< Período de rotación (segundos)
    
    // Propiedades gravitacionales
    double mu;                  ///< Parámetro gravitacional μ = GM (m³/s²)
    double surfaceGravity;      ///< Gravedad superficial (m/s²)
    
    // Propiedades de órbita
    std::string parentBodyName; ///< Nombre del cuerpo padre (vacío si es el primario)
    Orbit* orbit;               ///< Órbita relativa al padre (nullptr si es primario)
    
    // Propiedades visuales/físicas adicionales
    dvec3 rotationAxis;         ///< Eje de rotación normalizado
    double atmosphericHeight;   ///< Altura de la atmósfera (metros)
    bool hasAtmosphere;         ///< ¿Tiene atmósfera?
    
    // Para uso posterior
    std::vector<CelestialBody*> satellites;  ///< Lunas/satélites orbitantes

    /**
     * Constructor.
     * 
     * @param name_ Nombre del cuerpo
     * @param mass_ Masa (kg)
     * @param radius_ Radio ecuatorial (m)
     * @param rotPeriod Período de rotación (s)
     * @param mu_ Parámetro gravitacional
     * @param surfGrav Gravedad superficial (m/s²)
     * @param hasAtm ¿Tiene atmósfera?
     * @param atmHeight Altura de la atmósfera (m)
     */
    CelestialBody(const std::string& name_, double mass_, double radius_,
                  double rotPeriod, double mu_, double surfGrav,
                  bool hasAtm = false, double atmHeight = 0.0);

    /**
     * Destructor.
     */
    ~CelestialBody();

    /**
     * Asigna una órbita a este cuerpo.
     * 
     * @param orbit_ Puntero a la órbita
     * @param parentName Nombre del cuerpo padre
     */
    void setOrbit(Orbit* orbit_, const std::string& parentName);

    /**
     * Obtiene la posición actual en el tiempo relativa al cuerpo padre.
     * 
     * @param t Tiempo (segundos desde época)
     * @return Posición cartesiana (metros)
     */
    dvec3 getPositionAtTime(double t) const;

    /**
     * Obtiene la velocidad en el tiempo relativa al cuerpo padre.
     * 
     * @param t Tiempo (segundos)
     * @return Velocidad cartesiana (m/s)
     */
    dvec3 getVelocityAtTime(double t) const;

    /**
     * Calcula el período orbital en días.
     */
    double getOrbitalPeriodDays() const;

    /**
     * Añade un satélite.
     */
    void addSatellite(CelestialBody* satellite);

    /**
     * Obtiene la altura media de la atmósfera.
     */
    double getAtmosphericHeight() const { return atmosphericHeight; }

    /**
     * Verifica si un punto está dentro de la atmósfera.
     */
    bool isInAtmosphere(const dvec3& position) const;
};

} // namespace Phoenix::Physics

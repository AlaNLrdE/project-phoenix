#pragma once

#include <string>
#include <vector>

namespace Phoenix::Launch {

struct EngineSpec {
    std::string name;
    double dryMass;     ///< kg — motor sin propelante
    double maxThrust;   ///< N  — empuje máximo al vacío
    double Isp;         ///< s  — impulso específico al vacío
    int    count = 1;   ///< número de motores en el cluster
};

struct TankSpec {
    std::string name;
    double dryMass;         ///< kg — masa seca del depósito vacío
    double propellantMass;  ///< kg — masa de propelante (combustible + oxidizante)
};

/**
 * @struct StageConfig
 * @brief Descriptor de una etapa de cohete.
 *
 * Las etapas se añaden al LaunchVehicle en orden de ignición:
 *   addStage(booster) → addStage(second_stage) → addStage(payload)
 *
 * hasDecoupler debe ser false solo para la etapa más alta (cápsula/payload).
 */
struct StageConfig {
    std::string             name;
    std::vector<EngineSpec> engines;
    std::vector<TankSpec>   tanks;
    double strutMass     = 0.0;   ///< kg — masa del adaptador/interetapa
    double decouplerMass = 50.0;  ///< kg — masa del separador (si hasDecoupler)
    bool   hasDecoupler  = true;  ///< false solo para la etapa superior

    double getDryMass()        const;
    double getPropellantMass() const;
    double getWetMass()        const;
    double getMaxThrust()      const;
    double getWeightedIsp()    const; ///< Isp ponderado por empuje
};

} // namespace Phoenix::Launch

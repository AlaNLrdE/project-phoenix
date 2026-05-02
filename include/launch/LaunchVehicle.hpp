#pragma once

#include <launch/StageConfig.hpp>
#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>
#include <vessels/Vessel.hpp>
#include <string>
#include <vector>
#include <memory>

namespace Phoenix::Launch {

/**
 * @class LaunchVehicle
 * @brief Constructor y analizador de cohetes multi-etapa.
 *
 * Workflow:
 *   1. Crear instancia con nombre del vehículo.
 *   2. Llamar addStage() en orden de ignición:
 *        addStage(booster)       ← primera etapa en encenderse (abajo)
 *        addStage(second_stage)
 *        addStage(payload)       ← última etapa (arriba, sin decoupler)
 *   3. Consultar rendimiento: getTotalDeltaV(), getStageTWR(), etc.
 *   4. Ensamblar Vessel con assemble() para usarlo en WorldManager.
 *
 * Convención de índices de etapa:
 *   stage 0 = booster (primera en encenderse, físicamente abajo del cohete)
 *   stage N-1 = upper stage / payload (físicamente arriba)
 *
 * Nota de staging: el árbol de partes tiene el payload como raíz y el booster
 * como hoja más profunda. La secuencia de separación correcta para lanzamiento
 * (bottom-up) se implementa en Phase 8B con Vessel::launchStage().
 */
class LaunchVehicle {
public:
    explicit LaunchVehicle(const std::string& name);

    /**
     * Agrega una etapa en orden de ignición.
     * Primera llamada = booster (fondo del cohete).
     * Última llamada  = upper stage / payload (sin decoupler).
     */
    void addStage(StageConfig config);

    // ── Masa ─────────────────────────────────────────────────────────────────

    double getTotalMass()   const;  ///< kg — masa húmeda total del vehículo
    double getPayloadMass() const;  ///< kg — masa de la etapa superior (carga útil)

    // ── Rendimiento por etapa ─────────────────────────────────────────────────

    /**
     * ΔV de la etapa i (Tsiolkovsky), asumiendo que todas las etapas 0..i-1
     * ya han sido separadas antes del encendido de la etapa i.
     * @param i Índice de etapa (0 = booster)
     */
    double getStageDeltaV(int i)                const;

    /** ΔV total del vehículo (suma de todas las etapas). */
    double getTotalDeltaV()                     const;

    /**
     * Relación empuje/peso de la etapa i en el momento de ignición.
     * @param i Índice de etapa (0 = booster)
     * @param g Aceleración gravitacional (m/s²), default 9.81
     */
    double getStageTWR(int i, double g = 9.81)  const;

    // ── Centro de masa ────────────────────────────────────────────────────────

    /**
     * Altura del CoM sobre la base del cohete (modelo de columna simple, metros).
     * Útil para verificar estabilidad y punto de aplicación de empuje.
     */
    double getCoMHeight() const;

    /**
     * Altura del CoM después de que la etapa i ha quemado todo su propelante
     * (antes de la separación). Refleja el desplazamiento de CoM durante el burn.
     * @param i Índice de etapa
     */
    double getCoMHeightAfterBurn(int i) const;

    /**
     * Desplazamiento neto del CoM durante el burn completo de la etapa i (metros).
     * Positivo = CoM sube (normal: el propelante inferior se consume).
     */
    double getCoMShiftDuringBurn(int i) const;

    // ── Métricas globales ─────────────────────────────────────────────────────

    /** Fracción de payload = masa_payload / masa_total. */
    double getPayloadFraction() const;

    // ── Ensamblaje ───────────────────────────────────────────────────────────

    /**
     * Construye el árbol de partes y devuelve un Vessel listo para WorldManager.
     *
     * El árbol tiene la siguiente estructura (ejemplo 3 etapas):
     *   [Payload struct]   ← rootPart (Part::Command)
     *     [Payload tank(s)]
     *     [Payload engine(s)]
     *     [Decoupler]
     *       [S2 struct]    ← Part::Structure
     *         [S2 tank(s)]
     *         [S2 engine(s)]
     *         [Decoupler]
     *           [Booster struct]
     *             [Booster tank(s)]
     *             [Booster engine(s)]
     *
     * @param initialOrbit Órbita inicial (puede ser órbita de lanzamiento en tierra)
     * @param refBody      Cuerpo de referencia (Earth, etc.)
     */
    std::shared_ptr<Vessels::Vessel> assemble(
        const Physics::Orbit&   initialOrbit,
        Physics::CelestialBody* refBody) const;

    // ── Salida ────────────────────────────────────────────────────────────────

    /** Imprime tabla de rendimiento del vehículo. */
    void printSummary() const;

    int                stageCount()    const { return (int)stages.size(); }
    const StageConfig& getStage(int i) const { return stages[i]; }
    const std::string& getName()       const { return name; }

private:
    std::string              name;
    std::vector<StageConfig> stages; ///< [0]=booster, [back()]=upper/payload

    /** Suma de masa húmeda de las etapas i..N-1 (etapa i + todo lo de arriba). */
    double wetMassFromStage(int i) const;

    /** Altura física estimada de una etapa (modelo heurístico, metros). */
    static double stageHeight(const StageConfig& s);
};

} // namespace Phoenix::Launch

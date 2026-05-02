# PROJECT PHOENIX — ARQUITECTURA TÉCNICA (Phase 4)

## Diagrama de capas

```
┌─────────────────────────────────────────────────────────────────────┐
│                        PROJECT PHOENIX                              │
│                   Kerbal Space Program Clone                        │
│                    C++20 | GLM | Escala 1:10                        │
└─────────────────────────────────────────────────────────────────────┘

CAPA WORLD (WorldManager + SphereOfInfluence)
┌─────────────────────────────────────────────────────────────────────┐
│  WorldManager                                                       │
│  • Registro de cuerpos celestes y naves                             │
│  • Avance temporal + time warp                                      │
│  • buildBodyHierarchy() — enlaza parentBody en árbol de cuerpos     │
│  • updateSoITransitions() — detecta y aplica transiciones SoI       │
│  • updateFloatingOrigin() — evita pérdida de precisión              │
│                                                                     │
│  SphereOfInfluence                                                  │
│  • computeRadius(sma, m_child, m_parent)                            │
│  • isInside(relPos, soiRadius)                                      │
│  • checkTransition(vessel, allBodies, t) → TransitionResult         │
└─────────────────────────────────────────────────────────────────────┘
                               ▼
CAPA ENTIDAD (CelestialBody + Vessel)
┌─────────────────────────────────────────────────────────────────────┐
│  CelestialBody                    Vessel                            │
│  • nombre, masa, radio, μ         • nombre, masa (seca + fuel)      │
│  • parentBody* (no-owning)        • orbit (Orbit)                   │
│  • orbit* (no-owning)             • referenceBody* (no-owning)      │
│  • satellites[] (no-owning)       • rootPart (shared_ptr<Part>)     │
│  • getWorldPosition(t)            • getPosition(t)                  │
│  • getWorldVelocity(t)            • executeBurn(dv, dt)             │
│  • getSoIRadius()                 • stage()                         │
└─────────────────────────────────────────────────────────────────────┘
                               ▼
CAPA PARTES (Part + Engine)
┌─────────────────────────────────────────────────────────────────────┐
│  Part                             Engine : Part                     │
│  • tipo (PartType enum)           • maxThrust, Isp, throttle        │
│  • dryMass, fuelMass              • ignite(), shutdown()            │
│  • localPosition (dvec3)          • getCurrentThrust()              │
│  • parent* (raw, no-owning)       • getMassFlowRate()               │
│  • children[] (shared_ptr)        • getExhaustVelocity()            │
│  • getCenterOfMass()              • computeDeltaV(m_fuel, m_total)  │
│  • getTotalMass()                                                   │
└─────────────────────────────────────────────────────────────────────┘
                               ▼
CAPA FÍSICA (Orbit)
┌─────────────────────────────────────────────────────────────────────┐
│  Orbit                                                              │
│  • a, e, i, Ω, ω, ν, μ, epoch, M0                                  │
│  • Ecuación de Kepler (Newton-Raphson, tol=1e-10)                   │
│  • getPositionAtTime(t), getVelocityAtTime(t)                       │
│  • getStateAtTime(t, r, v)                                          │
│  • Conversión: vectores estado ↔ elementos orbitales                │
│  • Transformación: plano orbital → marco inercial                   │
│    orbital [x, y, 0] → R_z(Ω)·R_x(i)·R_z(ω) → ECI [X, Y, Z]     │
└─────────────────────────────────────────────────────────────────────┘
                               ▼
CAPA MATEMÁTICA (Constants + GLM)
┌─────────────────────────────────────────────────────────────────────┐
│  Constants: MU_EARTH, KSP_SCALE, G0, KEPLER_TOLERANCE              │
│  GLM: dvec3 (posición, velocidad), dquat, dmat3, dmat4              │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Jerarquía de clases

```
Phoenix::Math
└── Constants
    ├── MU_EARTH = 3.986×10¹⁴ m³/s²
    ├── KSP_SCALE = 0.1
    ├── G0 = 9.81 m/s²
    ├── KEPLER_TOLERANCE = 1×10⁻¹⁰
    └── KEPLER_MAX_ITERATIONS = 100

Phoenix::Physics
├── Orbit
│   ├── elementos: a, e, i, Ω, ω, ν, μ, epoch, M0
│   ├── constructores: Orbit(clásicos), Orbit(r, v, μ, epoch)
│   ├── propagación: getPositionAtTime(t), getVelocityAtTime(t)
│   ├── privados: solveKeplersEquation(M), eccentricToTrueAnomaly(E)
│   │            getOrbitalPlaneState(ν, r, v), orbitalToInertial(r_orb)
│   └── consultas: getPeriod(), getDistanceToPeriapsis(), isStable()
│
└── CelestialBody
    ├── propiedades: name, mass, radius, mu, surfaceGravity
    ├── atmósfera: hasAtmosphere, atmosphereHeight
    ├── jerarquía: parentBody* (raw, no-owning), satellites[] (raw)
    ├── órbita: orbit* (raw, no-owning), parentBodyName
    └── métodos: setOrbit(), setParentBody(), getWorldPosition(t),
                 getWorldVelocity(t), getSoIRadius(), addSatellite()

Phoenix::Parts
├── PartType (enum class)
│   └── Command, FuelTank, Engine, Decoupler, Parachute, Structure,
│       DockingPort, Unknown
│
├── Part
│   ├── propiedades: name, type, dryMass, fuelMass, localPosition
│   ├── jerarquía: parent* (raw), children[] (shared_ptr)
│   ├── estado: isActive, isDecoupled
│   └── métodos: attachChild(), getCenterOfMass(), getTotalMass(),
│                getActiveParts(), decouple()
│
└── Engine : Part
    ├── parámetros: maxThrust, Isp, throttle, isRunning
    └── métodos: ignite(), shutdown(), setThrottle(),
                 getCurrentThrust(), getMassFlowRate(),
                 getExhaustVelocity(), computeDeltaV(), computeBurnTime()

Phoenix::Vessels
└── Vessel
    ├── propiedades: name, dryMass, fuelMass, status
    ├── orbital: orbit (Orbit), referenceBodyName, referenceBody*
    ├── partes: rootPart (shared_ptr<Part>)
    ├── tiempo: currentTime
    └── métodos: getPosition(t), getVelocity(t), getTotalMass(),
                 setRootPart(), stage(), dock(), undock(),
                 getActiveEngines(), igniteEngines(), setThrottle(),
                 getTotalThrust(), computeAvailableDeltaV(), executeBurn()

Phoenix::World
├── WorldManager
│   ├── datos: celestialBodies (map), vessels (map)
│   ├── tiempo: simulationTime, timeWarp
│   ├── activo: activeVessel*
│   ├── precisión: floatingOrigin (dvec3)
│   └── métodos: registerCelestialBody(), registerVessel(),
│                buildBodyHierarchy(), updateSimulation(),
│                updateSoITransitions(), applySoITransition(),
│                updateFloatingOrigin(), getAllBodies()
│
└── SphereOfInfluence
    ├── TransitionResult { hasTransition, newBody*, newPosition, newVelocity }
    └── métodos estáticos: computeRadius(), isInside(), checkTransition()
```

---

## Modelo de memoria y ownership

```
WorldManager
├── celestialBodies: map<string, CelestialBody*>  → OWNING (delete en destructor)
└── vessels: map<string, Vessel*>                 → NO owning (caller gestiona)

CelestialBody
├── orbit*          → NO owning (apunta a Orbit en stack/heap externo)
├── parentBody*     → NO owning (apunta a CelestialBody en WorldManager)
└── satellites[]    → NO owning (raw ptrs a otros CelestialBody)

Part
├── parent*         → NO owning (raw ptr al padre)
└── children[]      → OWNING (shared_ptr<Part>)

Vessel
├── referenceBody*  → NO owning
└── rootPart        → OWNING (shared_ptr<Part>)
```

---

## Algoritmo de propagación orbital

```
getPositionAtTime(t):
  1. dt = t - epoch
  2. n = sqrt(μ / a³)                    ← movimiento medio (rad/s)
  3. M = (M0 + n·dt) mod 2π             ← anomalía media con offset inicial
  4. E = solveKeplersEquation(M)         ← Newton-Raphson: E - e·sin(E) = M
  5. ν = eccentricToTrueAnomaly(E)       ← arctan2 con signos correctos
  6. (r_orb, v_orb) = getOrbitalPlaneState(ν)
  7. return R_z(Ω)·R_x(i)·R_z(ω) · r_orb
```

La anomalía media inicial `M0` se computa en el constructor mediante:

```
M0 = trueToMeanAnomaly(ν₀, e)
   = E₀ - e·sin(E₀)
donde E₀ = 2·arctan2(√(1-e)·sin(ν₀/2), √(1+e)·cos(ν₀/2))
```

Esto garantiza que `getPositionAtTime(epoch) == posición_inicial` incluso para órbitas con $e \approx 0$ donde $\omega$ es indeterminado.

---

## Algoritmo de transición de SoI

```
checkTransition(vessel, allBodies, t):
  vesselWorldPos = refBody.getWorldPosition(t) + vessel.getPosition(t)
  vesselWorldVel = refBody.getWorldVelocity(t) + vessel.getVelocity(t)

  // Caso 1: Entrada en SoI de un hijo del cuerpo actual
  for each body in allBodies:
      if body.parentBody == refBody:
          soiR = body.getSoIRadius()
          bodyWorldPos = body.getWorldPosition(t)
          relPos = vesselWorldPos - bodyWorldPos
          if |relPos| < soiR:
              return TransitionResult {
                  newBody = body,
                  newPosition = relPos,
                  newVelocity = vesselWorldVel - body.getWorldVelocity(t)
              }

  // Caso 2: Salida de la SoI del cuerpo actual hacia el padre
  if refBody.hasOrbit():
      soiR = refBody.getSoIRadius()
      if |vessel.getPosition(t)| > soiR:
          parent = refBody.parentBody
          return TransitionResult {
              newBody = parent,
              newPosition = vesselWorldPos - parent.getWorldPosition(t),
              newVelocity = vesselWorldVel - parent.getWorldVelocity(t)
          }
```

---

## Archivos fuente y responsabilidades

| Archivo                           | Responsabilidad principal                              |
|-----------------------------------|--------------------------------------------------------|
| `include/math/Constants.hpp`      | Constantes físicas, tipos GLM, alias de namespaces     |
| `include/physics/Orbit.hpp`       | Declaración de `Orbit` — 6 elementos + M0              |
| `src/physics/Orbit.cpp`           | Propagación Kepleriana, Newton-Raphson, transformaciones |
| `include/physics/CelestialBody.hpp` | Cuerpo celeste con jerarquía SoI                    |
| `src/physics/CelestialBody.cpp`   | Posición mundial recursiva, SoI radius                 |
| `include/parts/Part.hpp`          | Árbol de partes, CoM dinámico, tipos de parte          |
| `src/parts/Part.cpp`              | CoM recursivo, staging, docking                        |
| `include/parts/Engine.hpp`        | Motor cohete, Isp, Tsiolkovsky                         |
| `src/parts/Engine.cpp`            | Cálculo de empuje, flujo másico, ΔV                    |
| `include/vessels/Vessel.hpp`      | Nave espacial con partes y propulsión                  |
| `src/vessels/Vessel.cpp`          | executeBurn (Euler), CoM total, getActiveEngines       |
| `include/world/WorldManager.hpp`  | Gestor central del universo                            |
| `src/world/WorldManager.cpp`      | Simulación, SoI, floating origin                       |
| `include/world/SphereOfInfluence.hpp` | Utilidades estáticas de SoI                        |
| `src/world/SphereOfInfluence.cpp` | checkTransition, patched-conics                        |
| `src/main.cpp`                    | 8 ejemplos de demostración (Ph1–Ph4)                   |

---

## Namespaces

| Namespace          | Contenido                                   |
|--------------------|---------------------------------------------|
| `Phoenix::Math`    | `Constants`, tipos GLM (`dvec3`, `dquat`)   |
| `Phoenix::Physics` | `Orbit`, `CelestialBody`                    |
| `Phoenix::Parts`   | `Part`, `Engine`, `PartType`                |
| `Phoenix::Vessels` | `Vessel`                                    |
| `Phoenix::World`   | `WorldManager`, `SphereOfInfluence`         |

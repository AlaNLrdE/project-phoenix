# PROJECT PHOENIX

**Versión:** 0.4.0  
**Lenguaje:** C++20  
**Escala:** 1:10 (Kerbal Space Program)  
**Librería de Matemáticas:** GLM (`glm::dvec3`, `glm::dquat`)

Simulador de mecánica orbital inspirado en Kerbal Space Program, desarrollado en C++20 con una escala 1:10 de las distancias y radios reales. Implementa un modelo de cónicos empalmados (patched-conics) con esferas de influencia, jerarquía de partes, propulsión y motores cohete.

---

## Estado por fases

| Fase | Nombre                                 | Estado         | Commit     |
|------|----------------------------------------|----------------|------------|
| 1    | Mecánica orbital Kepleriana            | ✅ Completa     | `7c16032`  |
| 2    | Jerarquía de partes                    | ✅ Completa     | `ab82ad5`  |
| 3    | Propulsión (Engine, Tsiolkovsky)       | ✅ Completa     | `724a9c3`  |
| 4    | Esferas de influencia (SoI)            | ✅ Completa     | `72110d4`  |
| 5    | Aerodinámica y reentrada               | ✅ Completa     | `9f65503`  |
| 6    | Visualización ASCII / HUD              | ✅ Completa     | `cf38f7f`  |
| 7    | Visualización 3D (Raylib 5)            | ✅ Completa     | `78b6ab8`  |
| 8    | Launch to Orbit (staged rockets)       | 🔄 En progreso | —          |

---

## Estructura del proyecto

```
project-phoenix/
├── CMakeLists.txt
├── build.sh
├── include/
│   ├── math/
│   │   └── Constants.hpp          # Constantes y alias de tipos GLM
│   ├── physics/
│   │   ├── Orbit.hpp              # 6 elementos orbitales + propagación Kepleriana
│   │   └── CelestialBody.hpp      # Cuerpos celestes con jerarquía SoI
│   ├── parts/
│   │   ├── Part.hpp               # Árbol de partes, CoM dinámico, staging
│   │   └── Engine.hpp             # Motor cohete (Isp, throttle, Tsiolkovsky)
│   ├── vessels/
│   │   └── Vessel.hpp             # Nave espacial
│   └── world/
│       ├── WorldManager.hpp       # Gestor central del universo
│       └── SphereOfInfluence.hpp  # Detección y transición de SoI
└── src/
    ├── main.cpp                   # 8 ejemplos de demostración
    ├── physics/
    │   ├── Orbit.cpp
    │   └── CelestialBody.cpp
    ├── parts/
    │   ├── Part.cpp
    │   └── Engine.cpp
    ├── vessels/
    │   └── Vessel.cpp
    └── world/
        ├── WorldManager.cpp
        └── SphereOfInfluence.cpp
```

---

## Compilación

### Requisitos

- macOS (Homebrew) o Linux (apt/dnf)
- Compilador C++20: Clang 12+ o GCC 10+
- CMake 3.20+
- GLM (header-only)

### macOS

```bash
brew install cmake glm
```

### Linux (Ubuntu/Debian)

```bash
sudo apt-get install build-essential cmake libglm-dev
```

### Linux (Fedora)

```bash
sudo dnf install gcc-c++ cmake glm-devel
```

### Compilar y ejecutar

```bash
cd project-phoenix

# Configurar y compilar (Release)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(sysctl -n hw.ncpu 2>/dev/null || nproc)

# Ejecutar
./build/phoenix
```

O usando el script incluido:

```bash
./build.sh release   # build optimizado
./build.sh debug     # build con símbolos de depuración
./build.sh run       # build + ejecutar
./build.sh clean     # eliminar artefactos de build
```

---

## Conceptos clave

### Escala KSP (1:10)

Todos los radios y distancias usan `Constants::KSP_SCALE = 0.1`. Las masas no se escalan. El parámetro gravitacional μ = G·M se calcula con la masa real para mantener la dinámica orbital correcta.

```cpp
double earthRadius = 6371000.0 * Constants::KSP_SCALE;  // 637.1 km
double moonSMA     = 384400000.0 * Constants::KSP_SCALE; // 38 440 km
```

### Elementos orbitales clásicos

| Parámetro           | Símbolo  | Rango          | Descripción                        |
|---------------------|----------|----------------|------------------------------------|
| Semieje mayor       | $a$      | $> 0$ m        | Tamaño de la órbita                |
| Excentricidad       | $e$      | $[0, 1)$       | Forma (0 = circular)               |
| Inclinación         | $i$      | $[0, \pi]$ rad | Ángulo respecto al ecuador         |
| Nodo ascendente     | $\Omega$ | $[0, 2\pi)$    | Orientación del plano orbital      |
| Argumento periapsis | $\omega$ | $[0, 2\pi)$    | Rotación del periapsis en el plano |
| Anomalía verdadera  | $\nu$    | $[0, 2\pi)$    | Posición actual en la órbita       |

La propagación usa la **ecuación de Kepler** ($M = E - e\sin E$) resuelta con Newton-Raphson (tolerancia $10^{-10}$ rad, convergencia en 3–5 iteraciones).

`Orbit` almacena `M0` (anomalía media en la época) para que `getPositionAtTime(epoch)` devuelva exactamente el vector de posición inicial, incluso en órbitas casi-circulares donde $\omega$ es indeterminado.

### Radio de esfera de influencia

$$r_{\text{SoI}} = a \left(\frac{m_{\text{hijo}}}{m_{\text{padre}}}\right)^{2/5}$$

---

## API principal

### `Orbit`

```cpp
// Desde elementos clásicos
Orbit(a, e, i, Omega, omega, nu, mu, epoch);

// Desde vectores de estado
Orbit(const dvec3& position, const dvec3& velocity, double mu, double epoch);

// Propagación
dvec3 getPositionAtTime(double t) const;
dvec3 getVelocityAtTime(double t) const;
void  getStateAtTime(double t, dvec3& r, dvec3& v) const;

// Consultas
double getPeriod() const;
double getDistanceToPeriapsis() const;
double getDistanceToApoapsis() const;
bool   isStable() const;      // e < 1
bool   isHyperbolic() const;  // e > 1
```

### `CelestialBody`

```cpp
CelestialBody(name, mass, radius, rotPeriod, mu, surfGrav, hasAtm, atmHeight);

void   setOrbit(Orbit* orbit, parentName, CelestialBody* parentPtr = nullptr);
dvec3  getWorldPosition(double t) const;  // posición recursiva en marco inercial
dvec3  getWorldVelocity(double t) const;
double getSoIRadius() const;              // r_SoI = a*(m/m_parent)^0.4
```

### `Part` y `Engine`

```cpp
// Part — nodo en árbol de partes
Part(name, type, dryMass, fuelMass, maxFuelMass, localPosition);
void   attachChild(std::shared_ptr<Part> child);
dvec3  getCenterOfMass() const;   // CoM recursivo en marco local
double getTotalMass() const;

// Engine : public Part
Engine(name, dryMass, maxThrust, Isp);
void   ignite();
void   shutdown();
void   setThrottle(double t);         // [0.0, 1.0]
double getCurrentThrust() const;      // throttle × maxThrust (N)
double getMassFlowRate() const;       // F / (Isp × g0) (kg/s)
double getExhaustVelocity() const;    // Isp × g0 (m/s)
double computeDeltaV(double fuelMass, double totalMass) const;
```

### `Vessel`

```cpp
Vessel(name, dryMass, orbit, refBodyName, refBody);

dvec3  getPosition(double t) const;
dvec3  getVelocity(double t) const;
void   setRootPart(std::shared_ptr<Part> root);
double getTotalMass() const;
void   stage();
void   executeBurn(double deltaV, double dt = 0.5);

// Propulsión
std::vector<Engine*> getActiveEngines() const;
void   igniteEngines();
void   shutdownEngines();
void   setThrottle(double t);
double getTotalThrust() const;
double computeAvailableDeltaV() const;
```

### `SphereOfInfluence`

```cpp
// Métodos estáticos
static double computeRadius(double sma, double childMass, double parentMass);
static bool   isInside(const dvec3& relativePos, double soiRadius);
static TransitionResult checkTransition(
    const Vessel& vessel,
    const std::map<std::string, CelestialBody*>& allBodies,
    double t);

struct TransitionResult {
    bool           hasTransition = false;
    CelestialBody* newBody       = nullptr;
    dvec3          newPosition{0.0};  // en marco del nuevo cuerpo
    dvec3          newVelocity{0.0};  // relativa al nuevo cuerpo
};
```

### `WorldManager`

```cpp
void registerCelestialBody(CelestialBody* body);
void registerVessel(Vessel* vessel);
void setActiveVessel(const std::string& name);

void buildBodyHierarchy();                       // enlaza punteros parentBody
void updateSimulation(double dt);                // tiempo + SoI + floating origin
void updateSoITransitions(double t);
void updateFloatingOrigin(double threshold = 1e8);

void   setTimeWarp(int warp);
double getSimulationTime() const;
const std::map<std::string, CelestialBody*>& getAllBodies() const;
```

---

## Ejemplos de uso

### Órbita circular y propagación

```cpp
#include <physics/Orbit.hpp>
using namespace Phoenix::Physics;
using namespace Phoenix::Math;

double r  = 6371000.0 * 0.1 + 400000.0;          // LEO a 400 km (escala KSP)
double vc = std::sqrt(Constants::MU_EARTH / r);

Orbit leo(dvec3(r, 0, 0), dvec3(0, vc, 0), Constants::MU_EARTH, 0.0);

dvec3 pos = leo.getPositionAtTime(3600.0);        // 1 hora después
std::cout << "Período: " << leo.getPeriod() / 60.0 << " min\n";
```

### Motor y ecuación de Tsiolkovsky

```cpp
#include <parts/Engine.hpp>
using namespace Phoenix::Parts;

Engine merlin("Merlin", 470.0, 845000.0, 311.0);  // 845 kN, Isp = 311 s
merlin.ignite();
merlin.setThrottle(1.0);

double ve = merlin.getExhaustVelocity();                    // Isp * g0
double dv = merlin.computeDeltaV(8000.0, 10000.0);          // ΔV disponible
```

### Sistema Tierra–Luna con SoI

```cpp
#include <world/WorldManager.hpp>
#include <world/SphereOfInfluence.hpp>
using namespace Phoenix;

double moonSoI = World::SphereOfInfluence::computeRadius(
    384400000.0 * 0.1,  // SMA en escala KSP
    7.342e22,           // masa Luna
    5.972e24            // masa Tierra
);  // ≈ 6 617 km

auto result = World::SphereOfInfluence::checkTransition(vessel, allBodies, t);
if (result.hasTransition) {
    // Recalcular órbita en marco del nuevo cuerpo
    Orbit newOrbit(result.newPosition, result.newVelocity, moonMu, t);
}
```

---

## Constantes de referencia

| Constante                          | Valor               | Descripción                     |
|------------------------------------|---------------------|---------------------------------|
| `Constants::MU_EARTH`              | `3.986×10¹⁴` m³/s² | Parámetro gravitacional Tierra  |
| `Constants::KSP_SCALE`             | `0.1`               | Factor de escala KSP 1:10       |
| `Constants::G0`                    | `9.81` m/s²         | Gravedad estándar (para Isp)    |
| `Constants::KEPLER_TOLERANCE`      | `1×10⁻¹⁰` rad       | Tolerancia Newton-Raphson       |
| `Constants::KEPLER_MAX_ITERATIONS` | `100`               | Máximo iteraciones Kepler       |

---

## Precisión numérica

- Todos los cálculos en `double` (64-bit IEEE 754)
- Newton-Raphson para la ecuación de Kepler converge en 3–5 iteraciones
- `WorldManager::updateFloatingOrigin()` reposiciona el origen cuando la nave activa supera `threshold` metros del origen, evitando pérdida de precisión en flotante

---


## Próxima fase (8): Launch to Orbit (staged rockets)

### 8A — Vehicle Modeling
- `Stage` class: motor, tanque, estructura
- `LaunchVehicle`: vector de stages, CoM/Isp agregados
- Fuel distribution: transferencia entre tanques por gravedad
- Masa de estructura (dry mass) por stage
- Tracking de centro de masa dinámico durante quemado

### 8B — Launch Sequence
- `LaunchController`: secuencia de ignición
- Separation logic: detector de motor apagado, decoupler automático
- Collision physics: separación segura entre stages (distancia mínima)
- Launch clamps: restricción inicial de posición/velocidad

### 8C — Ascent Guidance
- Gravity turn profile: inicio de pitch at 50m/s, progresión temporal
- Max-Q detection: búsqueda dinámica de punto de máxima presión dinámica
- Throttle management: limitación por Max-Q
- Target orbit specification: altitud, inclinación, argumento del nodo
- Pitch program: cálculo de ángulo requerido vs. vuelo actual

### 8D — Autopilot (Thrust Vector Control)
- Gimbal control: vector thrust hacia proa deseada
- PID controller: error de pitch/yaw/roll → gimbal angle
- Fin control: aeroaletas para estabilidad (si aplica)
- Throttle servo: rampa de empuje suave
- g-limit: protección contra sobrecarga estructural

### 8E — Orbital Insertion
- Coast phase detection: apogeo → periapsis fijos
- Circularization burn: cálculo ΔV, tiempo de ignición
- Burn execution: timing, throttle profile
- Achieve orbit: validación de elemento a circular

### 8F — Launch Simulation UI
- Real-time telemetry: altitud, velocidad, ángulo de vuelo, aceleración
- Stage info: masa actual, empuje, ΔV disponible
- Abort modes: emergency staging, chutes, reentrada
- Flight director: guía visual de pitch/yaw/roll
- Replay system: grabación y reproducción de vuelo

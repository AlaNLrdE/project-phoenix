# QUICK START — PROJECT PHOENIX

**Lectura:** 5 minutos | **Compilación:** 1-2 minutos

---

## Inicio rápido (macOS)

```bash
# 1. Instalar dependencias (una sola vez)
brew install cmake glm

# 2. Compilar
cd project-phoenix
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(sysctl -n hw.ncpu)

# 3. Ejecutar
./build/phoenix
```

## Inicio rápido (Linux)

```bash
sudo apt-get install build-essential cmake libglm-dev
cd project-phoenix
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
./build/phoenix
```

---

## Los 8 conceptos del simulador

### 1. Órbita = 6 elementos clásicos

```cpp
Orbit orbit(
    7000000.0,            // a: semieje mayor (m)
    0.0,                  // e: excentricidad
    0.0,                  // i: inclinación (rad)
    0.0,                  // Ω: nodo ascendente (rad)
    0.0,                  // ω: argumento periapsis (rad)
    0.0,                  // ν: anomalía verdadera (rad)
    Constants::MU_EARTH,  // μ (m³/s²)
    0.0                   // época (s)
);
```

O directamente desde posición y velocidad:

```cpp
dvec3 r(r_orbit, 0, 0);
dvec3 v(0, v_circular, 0);
Orbit leo(r, v, Constants::MU_EARTH, 0.0);
```

### 2. Propagar = ecuación de Kepler

```cpp
dvec3  pos  = orbit.getPositionAtTime(3600.0);  // 1 hora después
double T    = orbit.getPeriod();                 // ~330 s para LEO KSP
```

### 3. Cuerpo celeste = masa + radio + μ

```cpp
CelestialBody earth(
    "Earth",
    5.972e24,                        // masa (kg)
    6371000.0 * Constants::KSP_SCALE,// radio (m) — escala 1:10
    86164.0,                         // período de rotación (s)
    Constants::MU_EARTH,             // μ (m³/s²)
    9.81,                            // gravedad superficial (m/s²)
    true,                            // tiene atmósfera
    100000.0 * Constants::KSP_SCALE  // altura de atmósfera (m)
);
```

### 4. Nave = órbita + masa + árbol de partes

```cpp
Vessel ship("MyShip", 5000.0, leo, "Earth", &earth);

// Opcional: árbol de partes para CoM y propulsión
auto capsule = std::make_shared<Part>("Capsule", PartType::Command, 800.0, ...);
ship.setRootPart(capsule);
```

### 5. Motor = Isp + empuje + Tsiolkovsky

```cpp
auto eng = std::make_shared<Engine>("Merlin", 470.0, 845000.0, 311.0);
eng->ignite();
eng->setThrottle(1.0);

double dv = eng->computeDeltaV(fuelMass, totalMass);  // Tsiolkovsky
ship.executeBurn(dv);                                  // integración Euler
```

### 6. Esfera de influencia = radio de dominancia

```cpp
double soiR = SphereOfInfluence::computeRadius(
    moonSMA, moonMass, earthMass);    // r_SoI = a*(m1/m2)^0.4

bool inside = SphereOfInfluence::isInside(relativePos, soiR);
```

### 7. Transición de SoI (cónicos empalmados)

```cpp
auto tr = SphereOfInfluence::checkTransition(vessel, allBodies, t);
if (tr.hasTransition) {
    // tr.newBody     → nuevo cuerpo de referencia
    // tr.newPosition → posición en marco del nuevo cuerpo
    // tr.newVelocity → velocidad relativa al nuevo cuerpo
}
```

### 8. Universo = WorldManager

```cpp
WorldManager world;
world.registerCelestialBody(&earth);
world.registerCelestialBody(&moon);
world.buildBodyHierarchy();        // enlaza parentBody
world.registerVessel(&ship);
world.setActiveVessel("MyShip");
world.setTimeWarp(100);            // 100× velocidad
world.updateSimulation(1.0);       // avanzar 1 segundo simulado
```

---

## Ejemplo mínimo completo

```cpp
#include <world/WorldManager.hpp>
#include <world/SphereOfInfluence.hpp>
using namespace Phoenix;

int main()
{
    CelestialBody earth("Earth", 5.972e24,
        6371000.0 * Math::Constants::KSP_SCALE, 86164.0,
        Math::Constants::MU_EARTH, 9.81, true,
        100000.0 * Math::Constants::KSP_SCALE);

    double r  = 6371000.0 * Math::Constants::KSP_SCALE + 400000.0;
    double vc = std::sqrt(Math::Constants::MU_EARTH / r);
    Physics::Orbit leo(Math::dvec3(r, 0, 0),
                       Math::dvec3(0, vc, 0),
                       Math::Constants::MU_EARTH, 0.0);

    Vessels::Vessel ship("MySat", 5000.0, leo, "Earth", &earth);

    World::WorldManager world;
    world.registerCelestialBody(&earth);
    world.registerVessel(&ship);
    world.setActiveVessel("MySat");
    world.updateSimulation(60.0);   // 1 minuto

    ship.printStatus();
    return 0;
}
```

---

## Escala KSP (recordatorio)

| Valor real              | Valor en simulación              |
|-------------------------|----------------------------------|
| Radio Tierra: 6 371 km  | `6371000.0 * 0.1` = 637.1 km    |
| Dist. Tierra-Luna: 384 400 km | `384400000.0 * 0.1` = 38 440 km |
| LEO (400 km real)       | 400 000 m sobre la superficie     |
| μ Tierra                | `3.986×10¹⁴` m³/s² (sin cambio) |

---

## Comandos útiles de build

```bash
# Build debug con warnings
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Limpiar y recompilar
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(sysctl -n hw.ncpu 2>/dev/null || nproc)

# Ejecutar con salida detallada
./build/phoenix 2>&1 | less
```

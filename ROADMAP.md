# PROJECT PHOENIX - TECHNICAL ROADMAP

## Phase Overview

```
Phase 1 ✅  →  Phase 2  →  Phase 3  →  Phase 4  →  Phase 5  →  Phase 6
Planetario   Partes    Propulsión   SoI      Aero      UI
```

---

## PHASE 2: Arquitectura de Partes (En Planificación)

### Objetivos

- [ ] Estructura jerárquica de partes (Part Nodes)
- [ ] Cálculo dinámico de Centro de Masa (CoM)
- [ ] Sistema de atracoplamiento (docking)
- [ ] Ruptura de jerarquía → 2 Vessels independientes

### Cambios en Arquitectura

#### Nueva clase: `Part`

```cpp
namespace Phoenix::Vessels {

class Part {
public:
    std::string name;
    double mass;
    dvec3 positionRelative;    // Relativa al CoM del padre
    dquat rotation;             // Rotación en marco local

    Part* parentPart;          // nullptr si es raíz (comando)
    std::vector<Part*> children;
    std::vector<Part*> attachNodes;  // Puertos de atracoplamiento

    // Propiedades específicas de parte
    enum PartType { COMMAND, ENGINE, TANK, FUSELAGE, WING };
    PartType type;

    // Parámetros según tipo
    double maxThrust;          // Para engines
    double fuelCapacity;       // Para tanks
    double dragCoefficient;    // Para aerodinámicas

    // Métodos
    void attachChild(Part* child);
    void detach();
    dvec3 getAbsolutePosition() const;
    dquat getAbsoluteRotation() const;
    double getTotalMass() const;
};

}  // namespace Phoenix::Vessels
```

#### Modificación: `Vessel`

```cpp
class Vessel {
    Part* rootPart;  // Comando (root of hierarchy)
    dvec3 centerOfMass;

    double getTotalMass() const;  // Suma recursiva
    dvec3 calculateCoM() const;   // Cálculo iterativo

    // Ruptura de naves
    std::vector<Vessel*> breakApart(Part* rupturePath);
};
```

### Algoritmo de CoM

```
1. Iterar todos los parts recursivamente
2. Sum(mass_i * position_i) / Sum(mass_i)
3. Actualizar en cada propagación o cambio de combustible
4. Recalcular momento de inercia I_xyz
```

### Ruptura de Jerarquía

Cuando un nodo falla:

```
Original:
    Command
    ├─ Tank (ruptura aquí)
    │  ├─ Engine1
    │  └─ Engine2
    └─ Wing

Resultado:
    Vessel A: Command + Wing
    Vessel B: Tank + Engine1 + Engine2
```

### Implementación (Estimado)

- **Headers:** `include/vessels/Part.hpp`
- **Fuente:** `src/vessels/Part.cpp`
- **Ejemplos:** `src/examples_phase2.cpp`
- **Líneas de código:** ~800

---

## PHASE 3: Propulsión (En Planificación)

### Objetivos

- [ ] Sistema de motores vectorizados
- [ ] Consumo de LiquidFuel + Oxidizer
- [ ] Propagación numérica (RK4) bajo empuje
- [ ] Throttle variable
- [ ] Gimbal de motor

### Nueva clase: `Engine`

```cpp
namespace Phoenix::Vessels {

class Engine {
public:
    Part* part;  // Referencia a su Part

    double currentThrottle;  // [0.0, 1.0]
    double isp;              // ISP (s)
    double maxThrust;        // Empuje máximo (N)
    double massFlowRate;     // kg/s a throttle=100%

    // Gimbal vectorial
    dvec3 gimbalRange;       // Max deflection angles (rad)
    dvec3 gimbalAngles;      // Current angles

    // Encendido
    bool ignited;
    double timeBurnStart;

    // Métodos
    void setThrottle(double t);
    double getThrust() const;
    dvec3 getThrustVector() const;
    bool consumeFuel(double dt);
};

class Fuel {
public:
    std::string resourceName;  // "LiquidFuel", "Oxidizer"
    double amount;
    double maxAmount;

    double getRatio() const { return amount / maxAmount; }
};

}  // namespace Phoenix::Vessels
```

### Propagación Numérica

Sustituir propagación Kepleriana por Runge-Kutta 4 cuando hay empuje:

```cpp
class Orbit {
    // ... existing ...

    // Phase 3 addition
    struct PropagationState {
        dvec3 position;
        dvec3 velocity;
        double mass;
        double time;
    };

    PropagationState propagateRK4(PropagationState state,
                                   double dt,
                                   const dvec3& accel_thrust);
};
```

### Ecuaciones de movimiento

$$\mathbf{a} = \frac{\mathbf{F}_{thrust}}{m} - \frac{\mu \mathbf{r}}{r^3}$$

Donde:

- $\mathbf{F}\_{thrust} = $ empuje del motor
- $m = $ masa total (decreciente por consumo)
- Segundo término: aceleración gravitacional

### Recursos

Crear sistema básico:

```cpp
namespace Phoenix::Resources {

class ResourceManager {
private:
    std::map<std::string, double> resources;  // name -> amount
    std::map<std::string, double> capacities;

public:
    bool consume(const std::string& name, double amount);
    void transfer(const std::string& name, double amount);
    double getAmount(const std::string& name) const;
};

}  // namespace Phoenix::Resources
```

### Implementación (Estimado)

- **Headers:** `include/vessels/Engine.hpp`, `include/resources/Resource.hpp`
- **Fuente:** `src/vessels/Engine.cpp`, etc.
- **Líneas de código:** ~1500

---

## PHASE 4: Esferas de Influencia (En Planificación)

### Objetivos

- [ ] Detección de transiciones entre SoI
- [ ] Cambio automático de cuerpo central
- [ ] Origin floating system (desplazamiento flotante)
- [ ] Propagación multi-cuerpo

### Concepto

Una nave está bajo la influencia gravitacional de varios cuerpos, pero el dominante es el más cercano (en términos de esfera de influencia):

$$r_{SoI} = a \left(\frac{m_1}{m_2}\right)^{2/5}$$

Donde:

- $a$ = semieje mayor de la órbita de $m_1$ alrededor de $m_2$
- $m_1, m_2$ = masas de los cuerpos

### Algoritmo de Detección

```
1. Calcular SoI para todos los cuerpos respecto a padre
2. Si nave dentro de SoI:
   - Cambiar referencia gravitacional
   - Re-calcular órbita relativa al nuevo cuerpo
   - Eventos: "EnterSoI(body)", "ExitSoI(body)"
3. Mantener árbol de cuerpos jerárquico
```

### Origin Floating System

En lugar de coordenadas absolutas globales, desplazar el origen para mantener la nave activa cerca de (0,0,0):

```cpp
class WorldManager {
private:
    dvec3 floatingOrigin;  // Desplazamiento actual

public:
    void updateFloatingOrigin(const Vessel* activeVessel) {
        // Si nave a distancia > THRESHOLD, desplazar todo
        if (glm::length(activeVessel->getPosition()) > THRESHOLD) {
            floatingOrigin = activeVessel->getPosition();
            // Reposicionar todos los cuerpos y naves
        }
    }
};
```

**Ventaja:** Evita problemas de precisión de punto flotante en grandes distancias.

### Implementación (Estimado)

- **Headers:** `include/world/SphereOfInfluence.hpp`
- **Fuente:** `src/world/SphereOfInfluence.cpp`
- **Cambios en WorldManager:** +300 líneas
- **Líneas totales:** ~800

---

## PHASE 5: Aerodinámica y Reentrada (En Planificación)

### Objetivos

- [ ] Modelo de drag atmosférico
- [ ] Presión dinámica (q)
- [ ] Reentrada térmica
- [ ] Deformación/ablación de partes

### Modelo de Drag

$$D = \frac{1}{2} \rho v^2 C_d A$$

Donde:

- $\rho$ = densidad atmosférica
- $v$ = velocidad relativa a atmósfera
- $C_d$ = coeficiente de drag
- $A$ = área frontal

### Atmósfera Exponencial

```cpp
double atmosphericDensity(double altitude, double scaleHeight, double rho0) {
    return rho0 * exp(-altitude / scaleHeight);
}
```

### Calor de Reentrada

$$Q = \frac{1}{2} \rho v^3 C_h A$$

Donde $C_h$ es coeficiente térmico (~0.001-0.01).

### Implementación (Estimado)

- **Headers:** `include/physics/Atmosphere.hpp`
- **Fuente:** `src/physics/Atmosphere.cpp`
- **Líneas de código:** ~600

---

## PHASE 6: UI y Visualización (En Planificación)

### Objetivos

- [ ] Rendering 3D (OpenGL/Vulkan)
- [ ] Navball
- [ ] HUD con vectores prograde/retrograde
- [ ] Mapa de órbitas
- [ ] Instrumentos (altitud, velocidad, etc.)

### Stack tecnológico propuesto

- **Rendering:** OpenGL 4.5 + GLFW
- **Matemáticas:** GLM (ya en uso)
- **UI:** ImGui (immediate mode GUI)
- **Física:** Bullet3 (Phase 5+)

### Navball

Esfera que muestra:

- Dirección de vuelo (prograde)
- Dirección opuesta (retrograde)
- Vector normal (radial out)
- Rotación de nave

### HUD

```
┌──────────────────────────────────────┐
│  Vel: 7854 m/s  |  Alt: 400.2 km    │
│  Heading: 90°   |  Pitch: 0°        │
│  Thrust: 85%    |  Fuel: 45%        │
│          [Navball 3D]                │
│                                      │
│  Orbit Elements:                     │
│  • Periapsis: 399.8 km              │
│  • Apoapsis:  401.2 km              │
│  • Inc:        0.0°                  │
└──────────────────────────────────────┘
```

---

## Technical Debt & Considerations

### Phase 1 → 2

- [ ] Usar smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- [ ] Agregar logging framework
- [ ] Implementar event system

### Phase 2 → 3

- [ ] Optimizar cálculo de CoM (cached)
- [ ] Implementar spatial index para colisiones

### Phase 3 → 4

- [ ] Solver ODE más robusto (posiblemente GSL)
- [ ] Manejo de singularidades orbitales

### Phase 4 → 5

- [ ] Collision detection (AABB, sphere)
- [ ] Constraints solver para estructuras

### General

- [ ] Suite de testing (GoogleTest)
- [ ] Benchmarking (Google Benchmark)
- [ ] Documentación Doxygen

---

## Development Guidelines

### Code Style

- **Indentation:** 4 spaces
- **Naming:**
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
  - Members: `lowerCamelCase` or `snake_case_`
- **Headers:** Guards `#pragma once`
- **Namespaces:** `Phoenix::Module::Submodule`

### Commit Convention

```
[PHASE] TYPE: Brief description

PHASE: 1,2,3,4,5,6
TYPE: feat, fix, refactor, docs, test

[1] feat: Add Kepler equation solver
[1] fix: Correct matrix multiplication order
[2] feat: Implement Part hierarchy
```

### Testing

Phase 1 uses manual examples. Future phases:

```cpp
// tests/test_orbit.cpp
#include <gtest/gtest.h>
#include <physics/Orbit.hpp>

TEST(OrbitTest, CircularOrbitEnergy) {
    Orbit circular(7e6, 0.0, 0.0, 0, 0, 0, MU_EARTH);
    double E = circular.mu / (2 * circular.a);
    // Assert energy conservation
}
```

---

## Timeline Estimate

| Phase | Complexity | Est. Time | LOC  |
| ----- | ---------- | --------- | ---- |
| 1     | ★★☆☆☆      | ~2 weeks  | 2500 |
| 2     | ★★★☆☆      | ~3 weeks  | 2000 |
| 3     | ★★★★☆      | ~4 weeks  | 3000 |
| 4     | ★★★★★      | ~5 weeks  | 3500 |
| 5     | ★★★★☆      | ~4 weeks  | 2500 |
| 6     | ★★★★★      | ~6 weeks  | 5000 |

**Total:** ~20 weeks → ~18,500 LOC

---

## References

- Vallado et al. (2007). _Fundamentals of Astrodynamics and Applications_
- Anderson (2017). _Fundamentals of Aerodynamics_. 6th ed.
- Game Engine Architecture by Gregory (for physics integration)
- Real-Time Collision Detection by Akenine-Möller et al.

---

**Last Updated:** Phase 1 Complete  
**Next Review:** Before Phase 2 Start

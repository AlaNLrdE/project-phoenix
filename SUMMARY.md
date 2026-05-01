# PROJECT PHOENIX - PHASE 1 COMPLETION SUMMARY

## 🎯 Overview

**Project Phoenix** es un simulador orbital de escala 1:10 inspirado en Kerbal Space Program, desarrollado en C++20 con GLM.

**Phase 1** implementa los fundamentos del motor de física orbital: propagación Kepleriana analítica, resolución de la ecuación de Kepler, y un sistema integrado de cuerpos celestes y naves.

---

## 📦 What's Included

### Complete Phase 1 Project

```
TTSP/
├── include/                    # Headers públicos
│   ├── math/Constants.hpp      (1.5 KB)   - Constantes, tipos GLM
│   ├── physics/
│   │   ├── Orbit.hpp          (5.5 KB)   - Elementos orbitales (6 parámetros)
│   │   └── CelestialBody.hpp  (3.3 KB)   - Cuerpos celestes
│   ├── vessels/
│   │   └── Vessel.hpp         (3.3 KB)   - Naves espaciales
│   └── world/
│       └── WorldManager.hpp   (3.7 KB)   - Gestor central
│
├── src/                        # Implementaciones
│   ├── main.cpp               (14 KB)    - 5 ejemplos de demostración
│   ├── physics/
│   │   ├── Orbit.cpp          (9.2 KB)  - Ecuación de Kepler + propagación
│   │   └── CelestialBody.cpp  (1.9 KB)
│   ├── vessels/
│   │   └── Vessel.cpp         (3.8 KB)
│   └── world/
│       └── WorldManager.cpp   (3.8 KB)
│
├── .vscode/                    # Configuración VS Code
│   ├── launch.json            - Debugging
│   ├── tasks.json             - Build tasks
│   └── settings.json          - Editor settings
│
├── CMakeLists.txt             # Build system (C++20)
├── build.sh                   # Convenience script
├── setup_glm.sh               # GLM setup helper
├── check_structure.py         # Verificador de estructura
│
├── README.md                  # Documentación completa (~11 KB)
├── INSTALL.md                 # Guía de instalación
├── ROADMAP.md                 # Roadmap Phase 2-6 (~11 KB)
└── .gitignore                 # Git configuration

Total: ~50 KB de código fuente documentado
```

---

## 🔑 Key Components

### 1. **Constants & Types** (`math/Constants.hpp`)

- Parámetros gravitacionales: Earth, Sun
- Escala KSP (1:10)
- Tolerancias numéricas
- Tipos GLM: `dvec3`, `dquat`, `dmat3`, `dmat4`

### 2. **Orbit Class** (`physics/Orbit.hpp/cpp`)

**6 Elementos Orbitales Clásicos:**

- $a$: Semieje mayor
- $e$: Excentricidad
- $i$: Inclinación
- $\Omega$: Nodo ascendente
- $\omega$: Argumento del periapsis
- $\nu$: Anomalía verdadera

**Métodos Principales:**

```cpp
// Constructores
Orbit(a, e, i, Ω, ω, ν, μ, epoch);           // Clásicos
Orbit(position, velocity, μ, epoch);          // Vectores

// Propagación Kepleriana
dvec3 getPositionAtTime(double t);
dvec3 getVelocityAtTime(double t);
void getStateAtTime(double t, dvec3& r, dvec3& v);

// Consultas orbitales
double getPeriod();
double getAltitude(double bodyRadius);
double getDistanceToPeriapsis();
double getDistanceToApoapsis();
bool isStable(), isHyperbolic(), isParabolic();
```

**Propagación Numérica:**

- Ecuación de Kepler: $M = E - e \sin(E)$
- Solver: Newton-Raphson con tolerancia $10^{-10}$ rad
- Transformación: Plano orbital → Marco inercial 3D

### 3. **CelestialBody Class** (`physics/CelestialBody.hpp/cpp`)

**Propiedades:**

- Masa, radio, período de rotación
- Parámetro gravitacional $\mu = GM$
- Atmósfera (altura, densidad básica)
- Órbita relativa al padre

**Operaciones:**

```cpp
void setOrbit(Orbit* orbit, parentName);
dvec3 getPositionAtTime(double t);
dvec3 getVelocityAtTime(double t);
void addSatellite(CelestialBody* satellite);
bool isInAtmosphere(const dvec3& position);
```

### 4. **Vessel Class** (`vessels/Vessel.hpp/cpp`)

**Estado:**

- Nombre, masa seca, combustible
- Órbita actual
- Referencia gravitacional (cuerpo central)

**Métodos:**

```cpp
dvec3 getPosition(double t);
dvec3 getVelocity(double t);
void getState(double t, dvec3& r, dvec3& v);
void applyDeltaV(double dV, const dvec3& direction);
bool consumeFuel(double mass);
double getAltitude();
void printStatus();
```

### 5. **WorldManager Class** (`world/WorldManager.hpp/cpp`)

**Responsabilidades:**

- Registro y gestión de cuerpos celestes
- Registro y gestión de naves
- Avance temporal de simulación
- Time warp
- Desplazamiento flotante (preparado para Phase 4)

```cpp
void registerCelestialBody(CelestialBody* body);
void registerVessel(Vessel* vessel);
void updateSimulation(double deltaTime);
void setTimeWarp(int warp);
void setActiveVessel(const std::string& name);
```

---

## 🚀 Example Usage

### Crear una órbita desde elementos clásicos

```cpp
#include <physics/Orbit.hpp>
using namespace Phoenix::Physics;

// Órbita circular a 7000 km (radio total desde centro)
Orbit leo(
    7000000.0,              // a: semieje mayor (m)
    0.0,                    // e: excentricidad
    0.0,                    // i: inclinación
    0.0, 0.0, 0.0,         // Ω, ω, ν
    Constants::MU_EARTH,    // μ
    0.0                     // epoch
);

// Propagar a t=1 hora
dvec3 pos = leo.getPositionAtTime(3600.0);
double period = leo.getPeriod();  // ~90 minutos
```

### Sistema completo con WorldManager

```cpp
#include <world/WorldManager.hpp>

WorldManager world;

// Crear Tierra
CelestialBody* earth = new CelestialBody(
    "Earth", 5.972e24, 637100 * 0.1, 86164.0,
    Constants::MU_EARTH, 9.81, true, 10000 * 0.1
);
world.registerCelestialBody(earth);

// Crear nave en órbita LEO
Orbit leo(7000000 * 0.1, 0.0, 0.0, 0, 0, 0,
          Constants::MU_EARTH, 0.0);
Vessel* myship = new Vessel("Satellite", 5000.0, leo, "Earth", earth);
world.registerVessel(myship);
world.setActiveVessel("Satellite");

// Simular
for (int i = 0; i < 100; ++i) {
    world.updateSimulation(1.0);  // +1 segundo
    dvec3 pos = myship->getPosition(world.getSimulationTime());
}

world.printVessels();
```

---

## 📊 Examples in main.cpp

El archivo `src/main.cpp` contiene **5 ejemplos de demostración completos**:

1. **Ejemplo 1: Creación de cuerpos celestes**
   - Propiedades físicas de Tierra
   - Escala KSP (1:10)

2. **Ejemplo 2: Órbita circular (LEO)**
   - Construcción desde vectores de estado
   - Cálculo automático de elementos orbitales
   - Visualización de parámetros

3. **Ejemplo 3: Propagación Kepleriana**
   - Seguimiento de posición en el tiempo
   - Un período orbital completo
   - Tabla de propagación

4. **Ejemplo 4: Nave en órbita con WorldManager**
   - Integración sistema completo
   - Simulación de una órbita
   - Output de estado detallado

5. **Ejemplo 5: Maniobra Hohmann básica**
   - Cálculo de ΔV
   - Construcción de órbita de transferencia
   - Parámetros de transferencia

**Ejecución:**

```bash
./build/phoenix
```

Output incluye tablas formateadas, vectores, elementos orbitales, etc.

---

## 🔬 Física Implementada

### Ecuación de Kepler

Resuelta numéricamente con Newton-Raphson:

$$M = E - e \sin(E)$$

Donde:

- $M = n(t - t_0)$: anomalía media
- $E$: anomalía excéntrica
- $n = \sqrt{\mu/a^3}$: movimiento medio

**Convergencia:** Típicamente 3-5 iteraciones para órbitas elípticas estándar.

### Transformación Orbital-Inercial

Rotación secuencial de matrices:

$$\mathbf{R} = R_z(\Omega) \cdot R_x(i) \cdot R_z(\omega)$$

Posición en marco inercial:
$$\mathbf{r}_{inercial} = \mathbf{R} \cdot \mathbf{r}_{orbital}$$

### Cálculos Orbitales

- **Energía orbital:** $\xi = -\mu / (2a)$
- **Período:** $T = 2\pi \sqrt{a^3/\mu}$
- **Velocidad en punto:** $v = \sqrt{\mu(2/r - 1/a)}$
- **Momento angular:** $h = \sqrt{\mu p}$ donde $p = a(1-e^2)$

---

## 🛠️ Compilación & Ejecución

### Requisitos

- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- **GLM** (header-only library)

### Instalación de GLM

**Ubuntu/Debian:**

```bash
sudo apt-get install libglm-dev
```

**macOS:**

```bash
brew install glm
```

**Manual:**

```bash
cd TTSP/extern
git clone https://github.com/g-truc/glm.git
```

### Build

```bash
cd TTSP
./build.sh release        # Build optimizado
./build.sh debug          # Build con símbolos
./build.sh run            # Build + ejecución
./build.sh clean          # Limpiar
```

O manualmente:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./phoenix
```

---

## 📈 Métricas del Proyecto

| Métrica                 | Valor                   |
| ----------------------- | ----------------------- |
| Líneas de código        | ~2,500                  |
| Líneas de documentación | ~2,000                  |
| Namespaces              | 4 (`Phoenix::*`)        |
| Clases principales      | 5                       |
| Métodos publicos        | ~45                     |
| Headers                 | 5                       |
| Implementaciones        | 4                       |
| Ejemplos                | 5                       |
| Configuración           | 3 (CMake, VS Code, Git) |

---

## 📚 Documentación

| Documento         | Contenido                                   | Tamaño      |
| ----------------- | ------------------------------------------- | ----------- |
| **README.md**     | Guía completa Phase 1, API, ejemplos        | 11 KB       |
| **INSTALL.md**    | Instalación por plataforma, troubleshooting | 6 KB        |
| **ROADMAP.md**    | Phase 2-6, diseño, timeline                 | 11 KB       |
| **Code Comments** | Doxygen-ready inline documentation          | 500+ líneas |

---

## 🔄 Flujo de Uso Típico

```
┌─────────────────────────────────────────────────────────┐
│  1. Crear WorldManager                                  │
└────────────────┬────────────────────────────────────────┘
                 │
┌─────────────────────────────────────────────────────────┐
│  2. Registrar CelestialBodies (Earth, Moon, Sun, etc.)   │
└────────────────┬────────────────────────────────────────┘
                 │
┌─────────────────────────────────────────────────────────┐
│  3. Crear Orbits (vectores o elementos clásicos)        │
└────────────────┬────────────────────────────────────────┘
                 │
┌─────────────────────────────────────────────────────────┐
│  4. Crear Vessels con sus órbitas                       │
└────────────────┬────────────────────────────────────────┘
                 │
┌─────────────────────────────────────────────────────────┐
│  5. Registrar Vessels en WorldManager                   │
└────────────────┬────────────────────────────────────────┘
                 │
┌─────────────────────────────────────────────────────────┐
│  6. Loop de simulación:                                 │
│     - updateSimulation(dt)                              │
│     - getPosition/Velocity() de naves                   │
│     - Aplicar maniobras (applyDeltaV)                   │
│     - Consumir combustible                              │
└────────────────┬────────────────────────────────────────┘
                 │
┌─────────────────────────────────────────────────────────┐
│  7. Repetir hasta fin de simulación                     │
└─────────────────────────────────────────────────────────┘
```

---

## 🎓 Conceptos Enseñados

Este proyecto implementa/enseña:

✅ **Astrodinámica orbital clásica**

- Elementos orbitales
- Propagación Kepleriana
- Maniobras orbitales

✅ **Programación moderna en C++**

- C++20 features (std::format, constexpr, ranges)
- GLM para matemáticas 3D
- Gestión de memoria y punteros

✅ **Ingeniería de software**

- Arquitectura modular
- Separación de responsabilidades
- Documentación completa

✅ **Métodos numéricos**

- Resolución de ecuaciones (Newton-Raphson)
- Tolerancias y convergencia
- Precisión de punto flotante

---

## 🚀 Próximas Fases

### Phase 2: Arquitectura de Partes (~3 semanas)

- Jerarquía de partes (Part nodes)
- Centro de Masa dinámico
- Ruptura de naves

### Phase 3: Propulsión (~4 semanas)

- Motores vectorizados
- RK4 bajo empuje
- Consumo de recursos

### Phase 4: Esferas de Influencia (~5 semanas)

- Detección de SoI
- Propagación multi-cuerpo
- Origin floating system

### Phase 5: Aerodinámica (~4 semanas)

- Modelo de drag
- Reentrada térmica
- Efectos de presión atmosférica

### Phase 6: UI y Visualización (~6 semanas)

- Rendering OpenGL
- Navball 3D
- HUD y instrumentos

**Timeline total:** ~22 semanas → ~18,500 LOC

---

## 📋 Checklist Phase 1

✅ Estructura de directorios modular
✅ Clase Orbit con 6 elementos clásicos
✅ Ecuación de Kepler (Newton-Raphson)
✅ Conversión vectores ↔ elementos
✅ Clase CelestialBody
✅ Clase Vessel
✅ WorldManager
✅ Sistema de time warp
✅ 5 ejemplos de demostración
✅ CMakeLists.txt (C++20)
✅ Configuración VS Code (.vscode/)
✅ Documentación README completa
✅ Guía de instalación (INSTALL.md)
✅ Roadmap detallado (ROADMAP.md)
✅ Build script (build.sh)
✅ Verificador de estructura (check_structure.py)

---

## 🎯 Próximos Pasos

1. **Instalar dependencias:**
   - Consultar [INSTALL.md](INSTALL.md)

2. **Compilar proyecto:**

   ```bash
   ./build.sh release
   ```

3. **Ejecutar ejemplos:**

   ```bash
   ./build/phoenix
   ```

4. **Explorar código:**
   - Empezar por `src/main.cpp`
   - Revisar `include/physics/Orbit.hpp`

5. **Comenzar Phase 2:**
   - Consultar [ROADMAP.md](ROADMAP.md)
   - Planificar arquitectura de partes

---

## 📞 References & Resources

- **Curtis, H. D.** (2013). _Orbital Mechanics for Engineering Students_. 3rd ed.
- **Vallado, D. A.** (2007). _Fundamentals of Astrodynamics and Applications_. 3rd ed.
- **GLM Documentation:** https://glm.g-truc.net/
- **KSP Physics:** https://wiki.kerbalspaceprogram.com/

---

## 📝 License

Proyecto académico/experimental. Uso libre para fines educativos.

---

**Status:** ✅ **PHASE 1 COMPLETE**  
**Version:** 0.1.0  
**Last Updated:** 2026-05-01  
**Next Phase:** Phase 2 - Part Hierarchy

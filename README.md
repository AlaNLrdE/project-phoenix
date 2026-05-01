# PROJECT PHOENIX - PHASE 1: El Planetario y Propagación

**Versión:** 0.1.0  
**Lenguaje:** C++20  
**Escala:** 1:10 (Kerbal Space Program)  
**Librería de Matemáticas:** GLM (glm::dvec3, glm::dquat)

---

## 📋 Descripción General

Project Phoenix es un clon de **Kerbal Space Program** a escala 1:10 desarrollado en C++20. Phase 1 implementa los fundamentos del motor de física orbital: propagación Kepleriana, resolución de la ecuación de Kepler y un sistema integrado de cuerpos celestes y naves.

### Objetivos de Phase 1

✓ Definición de elementos orbitales clásicos (6 parámetros)  
✓ Propagación analítica mediante ecuación de Kepler  
✓ Conversión entre vectores de estado (r, v) y elementos orbitales  
✓ Sistema de cuerpos celestes con jerárquía orbital  
✓ Gestión central mediante WorldManager  
✓ Simulación temporal con time warp

---

## 📁 Estructura de Directorios

```
TTSP/
├── CMakeLists.txt                 # Configuración CMake
├── README.md                       # Este archivo
├── include/                        # Headers públicos
│   ├── math/
│   │   └── Constants.hpp          # Constantes astrodinámica, tipos GLM
│   ├── physics/
│   │   ├── Orbit.hpp              # Elementos orbitales, propagación
│   │   └── CelestialBody.hpp      # Cuerpos celestes
│   ├── vessels/
│   │   └── Vessel.hpp             # Naves espaciales
│   └── world/
│       └── WorldManager.hpp       # Gestor central del universo
└── src/                           # Implementaciones
    ├── main.cpp                   # Programa de demostración
    ├── physics/
    │   ├── Orbit.cpp              # Impl. propagación Kepleriana
    │   └── CelestialBody.cpp      # Impl. cuerpos celestes
    ├── vessels/
    │   └── Vessel.cpp             # Impl. naves
    └── world/
        └── WorldManager.cpp       # Impl. gestor
```

---

## 🔧 Compilación

### Requisitos

- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- GLM (header-only, normalmente en `apt-cache search glm`)

### En Linux/macOS

```bash
cd TTSP
mkdir build
cd build
cmake ..
make
./phoenix
```

### En Windows (MSVC)

```bash
cd TTSP
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
Release\phoenix.exe
```

---

## 📖 Conceptos Clave

### 1. **Elementos Orbitales Clásicos**

Una órbita se define completamente mediante 6 parámetros:

| Parámetro           | Símbolo  | Rango       | Unidad | Descripción                         |
| ------------------- | -------- | ----------- | ------ | ----------------------------------- |
| Semieje mayor       | $a$      | $> 0$       | m      | Radio medio orbital                 |
| Excentricidad       | $e$      | $[0, 2)$    | -      | Forma: 0=circular, $<1$=elíptica    |
| Inclinación         | $i$      | $[0, \pi]$  | rad    | Ángulo respecto al plano ecuatorial |
| Nodo ascendente     | $\Omega$ | $[0, 2\pi)$ | rad    | Dirección del nodo ascendente       |
| Argumento periapsis | $\omega$ | $[0, 2\pi)$ | rad    | Rotación en plano orbital           |
| Anomalía verdadera  | $\nu$    | $[0, 2\pi)$ | rad    | Posición actual en órbita           |

### 2. **Ecuación de Kepler**

Relaciona la anomalía media ($M$) con la anomalía excéntrica ($E$):

$$M = E - e \sin(E)$$

**Proceso de propagación:**

1. Calcular anomalía media: $M = n(t - t_0)$ donde $n = \sqrt{\mu/a^3}$
2. Resolver ecuación de Kepler (Newton-Raphson)
3. Convertir a anomalía verdadera: $\nu = 2 \arctan\left(\sqrt{\frac{1+e}{1-e}} \tan\frac{E}{2}\right)$
4. Calcular vectores de estado en plano orbital
5. Transformar a marco inercial 3D

### 3. **Transformación a Coordenadas 3D**

Los vectores en el plano orbital se transforman mediante:

$$\mathbf{R} = R_z(\Omega) \cdot R_x(i) \cdot R_z(\omega)$$

Donde $R_z$ y $R_x$ son rotaciones alrededor del eje Z e X respectivamente.

---

## 🔬 Ejemplo de Uso

### Crear una órbita desde elementos clásicos

```cpp
#include <physics/Orbit.hpp>

using namespace Phoenix::Physics;
using namespace Phoenix::Math;

// Crear órbita circular (e=0) ecuatorial (i=0)
Orbit orbit(
    7000000.0,          // a: semieje mayor (7,000 km)
    0.0,                // e: excentricidad
    0.0,                // i: inclinación
    0.0,                // Ω: nodo ascendente
    0.0,                // ω: argumento periapsis
    0.0,                // ν: anomalía verdadera
    Constants::MU_EARTH // μ: parámetro gravitacional
);

// Obtener posición y velocidad en t=0
dvec3 pos = orbit.getPositionAtTime(0.0);
dvec3 vel = orbit.getVelocityAtTime(0.0);
```

### Crear una órbita desde vectores de estado

```cpp
// Posición y velocidad iniciales
dvec3 position(6378137.0, 0.0, 0.0);  // Radio ecuatorial Tierra
dvec3 velocity(0.0, 7800.0, 0.0);     // ~7.8 km/s para órbita circular

// Construir órbita (calcula elementos automáticamente)
Orbit leo(position, velocity, Constants::MU_EARTH, 0.0);

// Acceder a elementos
std::cout << "Excentricidad: " << leo.e << "\n";
std::cout << "Período: " << leo.getPeriod() / 60.0 << " minutos\n";
```

### Propagación temporal

```cpp
// Propagar durante 1 hora
double t = 3600.0;

dvec3 pos_later = leo.getPositionAtTime(t);
dvec3 vel_later = leo.getVelocityAtTime(t);

// O ambas a la vez (más eficiente)
leo.getStateAtTime(t, pos_later, vel_later);
```

### Sistema con WorldManager

```cpp
#include <world/WorldManager.hpp>

WorldManager world;

// Crear Tierra
CelestialBody* earth = new CelestialBody(
    "Earth",
    5.972e24,                    // masa
    6371000.0 * 0.1,             // radio (escala 1:10)
    86164.0,                     // período rotación
    Constants::MU_EARTH,         // μ
    9.81,                        // gravedad superficial
    true,                        // tiene atmósfera
    100000.0 * 0.1               // altura atmosférica
);

world.registerCelestialBody(earth);

// Crear nave
Orbit leo(position, velocity, Constants::MU_EARTH, 0.0);
Vessel* satellite = new Vessel("MyCanSat", 5000.0, leo, "Earth", earth);

world.registerVessel(satellite);
world.setActiveVessel("MyCanSat");

// Simular
for (int i = 0; i < 100; ++i) {
    world.updateSimulation(1.0);  // Avanzar 1 segundo

    dvec3 pos = satellite->getPosition(world.getSimulationTime());
    double alt = satellite->getAltitude();
}

world.printVessels();
```

---

## 🔢 Precisión Numérica

### Tolerancia de Kepler

La resolución de la ecuación de Kepler usa método de Newton-Raphson con:

- **Tolerancia:** $10^{-10}$ radianes
- **Máx iteraciones:** 100
- **Convergencia típica:** 3-5 iteraciones para órbitas elípticas comunes

### Rango de validez

- **Órbitas elípticas:** $e \in [0, 1)$
- **Órbitas parabólicas:** $e \approx 1$ (detectadas automáticamente)
- **Órbitas hiperbólicas:** $e > 1$ (sin período definido)
- **Alitude mínima:** $> 1$ metro (para evitar singularidades)

---

## 📊 Constantes Astrodinámica

### Parámetros estándar terrestres

| Constante                      | Símbolo      | Valor                  | Unidad |
| ------------------------------ | ------------ | ---------------------- | ------ |
| Parámetro gravitacional Tierra | $\mu_\oplus$ | $3.986 \times 10^{14}$ | m³/s²  |
| Radio ecuatorial Tierra        | $R_\oplus$   | $6,378,137$            | m      |
| Gravedad superficial           | $g_0$        | $9.81$                 | m/s²   |

### Escala KSP (1:10)

En Project Phoenix, todos los cuerpos utilizan `KSP_SCALE = 0.1`:

- Radios: $R' = 0.1 \times R_{real}$
- Densidades: Compensadas para mantener $g \approx g_{real}$
- Altitudes: Expresadas en escala 1:10

---

## 🚀 Funcionalidades por Fases

### ✅ Phase 1 (Actual)

- Propagación Kepleriana
- 6 elementos orbitales
- Resolución numérica de ecuación de Kepler
- Cuerpos celestes estáticos
- WorldManager básico
- Time warp

### 🔄 Phase 2 (Próxima)

- Jerarquía de partes (Part nodes)
- Cálculo dinámico de Centro de Masa (CoM)
- Ruptura de naves (split into 2 vessels)

### ⏳ Phase 3

- Sistema de propulsión
- Consumo de recursos (LiquidFuel/Oxidizer)
- Propagación numérica (RK4) bajo empuje
- Gimbal vectorial

### ⭕ Phase 4

- Esferas de Influencia (SoI)
- Detección de transiciones
- Propagación multi-cuerpo
- Origin floating system

### 🌡️ Phase 5

- Aerodinámica y drag
- Reentrada atmosférica
- Calor y deformación

### 🎨 Phase 6

- UI y visualización
- Navball
- Predicción de trayectorias
- Mapa de órbitas

---

## 📝 API Principal

### `Orbit` class

```cpp
// Constructores
Orbit();
Orbit(a, e, i, Ω, ω, ν, μ, epoch);
Orbit(position, velocity, μ, epoch);

// Propagación
dvec3 getPositionAtTime(double t) const;
dvec3 getVelocityAtTime(double t) const;
void getStateAtTime(double t, dvec3& r, dvec3& v) const;

// Consultas
double getPeriod() const;
double getAltitude(double bodyRadius) const;
double getDistanceToPeriapsis() const;
double getDistanceToApoapsis() const;
bool isStable() const;
bool isHyperbolic() const;
```

### `CelestialBody` class

```cpp
// Constructor
CelestialBody(name, mass, radius, rotPeriod, μ, surfGrav, hasAtm, atmHeight);

// Operaciones
void setOrbit(Orbit* orbit, parentName);
void addSatellite(CelestialBody* satellite);

// Consultas
dvec3 getPositionAtTime(double t) const;
dvec3 getVelocityAtTime(double t) const;
double getOrbitalPeriodDays() const;
bool isInAtmosphere(const dvec3& position) const;
```

### `Vessel` class

```cpp
// Constructor
Vessel(name, dryMass, orbit, refBodyName, refBody);

// Estado
dvec3 getPosition(double t) const;
dvec3 getVelocity(double t) const;
void getState(double t, dvec3& r, dvec3& v) const;

// Maniobras
void applyDeltaV(double deltaV, const dvec3& direction);
bool consumeFuel(double mass);

// Consultas
double getTotalMass() const;
double getAltitude() const;
dvec3 getRelativeVelocity() const;
```

### `WorldManager` class

```cpp
// Gestión
void registerCelestialBody(CelestialBody* body);
void registerVessel(Vessel* vessel);
CelestialBody* getCelestialBody(const std::string& name) const;
Vessel* getVessel(const std::string& name) const;

// Simulación
void updateSimulation(double deltaTime);
void setSimulationTime(double t);
double getSimulationTime() const;

// Time warp
void setTimeWarp(int warp);
int getTimeWarp() const;

// Debug
void printCelestialBodies() const;
void printVessels() const;
```

---

## 🧪 Ejecución de Ejemplos

El archivo `main.cpp` contiene 5 ejemplos demostrando:

1. **Ejemplo 1:** Creación de cuerpos celestes
2. **Ejemplo 2:** Órbita circular (LEO)
3. **Ejemplo 3:** Propagación Kepleriana en tiempo
4. **Ejemplo 4:** Nave en órbita con WorldManager
5. **Ejemplo 5:** Maniobra básica Hohmann

```bash
./phoenix
```

Output esperado: Tablas de propagación, elementos orbitales y estados de naves.

---

## 🐛 Consideraciones de Debug

### Validación de órbitas

```cpp
if (!orbit.isStable()) {
    // Órbita hiperbólica o parabólica - sin período definido
}

if (orbit.getAltitude(bodyRadius) < 0) {
    // La nave está bajo tierra
}
```

### Verificación de precisión

```cpp
// Comprobar conservación de energía orbital
double E = orbit.mu / (2 * orbit.a);  // Energía específica

// Comprobar momento angular
dvec3 h = glm::cross(pos, vel);       // Debe ser constante
```

---

## 📚 Referencias

- Curtis, H. D. (2013). _Orbital Mechanics for Engineering Students_. 3rd ed.
- Vallado, D. A. (2007). _Fundamentals of Astrodynamics and Applications_. 3rd ed.
- [Kerbal Space Program Wiki](https://wiki.kerbalspaceprogram.com/)
- [GLM Documentation](https://glm.g-truc.net/0.9.9/index.html)

---

## 📄 Licencia

Proyecto académico/experimental. Uso libre para fines educativos.

---

**Autor:** Senior Software Engineer & Astrodynamics Specialist  
**Última actualización:** Phase 1 Complete  
**Próxima actualización:** Phase 2 - Part Hierarchy

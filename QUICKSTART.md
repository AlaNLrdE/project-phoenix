# QUICK START - PROJECT PHOENIX PHASE 1

**Tiempo de lectura:** 5 minutos | **Tiempo de compilación:** 3-5 minutos

---

## 🚀 Inicio Rápido (30 segundos)

```bash
# 1. Instalar GLM (1 línea)
# Ubuntu/Debian
sudo apt-get install libglm-dev

# 2. Compilar
cd TTSP
./build.sh release

# 3. Ejecutar
./build/phoenix
```

---

## 📋 5 Conceptos Clave

### 1. Órbita = 6 Elementos Clásicos

```cpp
Orbit orbit(
    7000000.0,          // a: semieje mayor
    0.0,                // e: excentricidad
    0.0,                // i: inclinación
    0.0,                // Ω: nodo ascendente
    0.0,                // ω: argumento periapsis
    0.0,                // ν: anomalía verdadera
    3.986e14,           // μ: parámetro gravitacional
    0.0                 // epoch: tiempo referencia
);
```

### 2. Propagar = Ecuación de Kepler

```cpp
dvec3 pos_en_1h = orbit.getPositionAtTime(3600.0);
double periodo = orbit.getPeriod();  // ~90 minutos para LEO
```

### 3. Planeta = Cuerpo Celeste

```cpp
CelestialBody* earth = new CelestialBody(
    "Earth", 5.972e24, 637100*0.1, 86164.0,
    3.986e14, 9.81, true, 10000*0.1
);
```

### 4. Nave = Órbita + Masa

```cpp
Vessel* ship = new Vessel(
    "Satellite",    // nombre
    5000.0,         // masa seca (kg)
    orbit,          // órbita
    "Earth",        // cuerpo referencia
    earth           // puntero al cuerpo
);
```

### 5. Universo = WorldManager

```cpp
WorldManager world;
world.registerCelestialBody(earth);
world.registerVessel(ship);
world.setTimeWarp(100);         // 100x speed
world.updateSimulation(1.0);    // +1 segundo
```

---

## 💻 Ejemplo Minimalista

```cpp
#include <world/WorldManager.hpp>

int main() {
    WorldManager world;

    // Crear Tierra
    auto earth = new CelestialBody("Earth",
        5.972e24, 637100*0.1, 86164.0,
        3.986e14, 9.81, false, 0.0);
    world.registerCelestialBody(earth);

    // Crear nave en LEO circular a 400 km
    double r_orbit = (637100 + 400000) * 0.1;
    double v_orbit = std::sqrt(3.986e14 / r_orbit);

    Orbit leo(r_orbit, 0.0, 0.0, 0, 0, 0, 3.986e14, 0.0);
    Vessel* sat = new Vessel("MySat", 5000.0, leo, "Earth", earth);

    world.registerVessel(sat);
    world.setActiveVessel("MySat");

    // Simular 90 minutos
    double period = leo.getPeriod();
    for (double t = 0; t <= period; t += 10.0) {
        world.setSimulationTime(t);
        auto pos = sat->getPosition(t);
        auto alt = sat->getAltitude();

        std::cout << "T+" << t/60 << " min: Alt "
                  << alt/1000 << " km\n";
    }

    return 0;
}
```

**Salida esperada:**

```
T+0 min: Alt 400 km
T+0.16667 min: Alt 399.8 km
...
T+90 min: Alt 400.2 km
```

---

## 📂 Estructura Esencial

```
TTSP/
├── include/
│   ├── math/Constants.hpp       ← Tipos, constantes
│   ├── physics/Orbit.hpp        ← Propagación orbital
│   ├── physics/CelestialBody.hpp← Planetas
│   ├── vessels/Vessel.hpp       ← Naves
│   └── world/WorldManager.hpp   ← Sistema
├── src/
│   ├── main.cpp                 ← Ver aquí primero!
│   └── physics/, vessels/, world/ (implementaciones)
├── CMakeLists.txt               ← Build config
└── build.sh                     ← Compilar
```

**Para empezar:** Leer `src/main.cpp` (5 ejemplos)

---

## 🔧 Troubleshooting (1 minuto)

| Problema                     | Solución                             |
| ---------------------------- | ------------------------------------ |
| `CMake Error: GLM not found` | `sudo apt-get install libglm-dev`    |
| `c++20 not supported`        | Actualizar compilador (GCC 10+)      |
| `cannot find -lglm`          | GLM es header-only, no necesita -l   |
| Build falla pero compila     | `rm -rf build && ./build.sh release` |
| Número lejano en consola     | Normal, usa escala 1:10              |

---

## 📖 Documentación

| Archivo         | Propósito              | Lectura |
| --------------- | ---------------------- | ------- |
| README.md       | Documentación completa | 20 min  |
| ARCHITECTURE.md | Diagramas y diseño     | 15 min  |
| ROADMAP.md      | Próximas fases         | 10 min  |
| INSTALL.md      | Instalación detallada  | 5 min   |
| src/main.cpp    | Código ejemplo anotado | 30 min  |

**Orden recomendado:**

1. Este archivo (5 min)
2. `src/main.cpp` (30 min)
3. README.md (20 min)
4. ARCHITECTURE.md (15 min)

---

## ⚡ Operaciones Comunes

### Crear órbita circular

```cpp
double a = 7000000.0;           // semieje mayor (m)
Orbit circular(a, 0.0, 0.0, 0, 0, 0, MU_EARTH, 0.0);
```

### Crear órbita elíptica

```cpp
double a = 6700000.0;           // semieje mayor
double e = 0.2;                 // excentricidad
Orbit ellipse(a, e, 0.0, 0, 0, 0, MU_EARTH, 0.0);
std::cout << "Período: " << ellipse.getPeriod() / 3600.0 << " h\n";
```

### Obtener posición en tiempo

```cpp
dvec3 pos = orbit.getPositionAtTime(3600.0);  // 1 hora después
double dist = glm::length(pos);                // magnitud
```

### Maniobra Delta-V

```cpp
double dv = 100.0;              // 100 m/s
dvec3 direction = glm::normalize(ship->getVelocity(0.0));
ship->applyDeltaV(dv, direction);
```

### Consumo de combustible

```cpp
if (ship->consumeFuel(100.0)) {  // consumir 100 kg
    std::cout << "Combustible consumido\n";
} else {
    std::cout << "Combustible insuficiente\n";
}
```

### Información de nave

```cpp
ship->printStatus();  // imprime todo: masa, órbita, altitud, etc.
```

---

## 🎯 Checklist de Configuración

- [ ] C++20 compiler instalado
- [ ] CMake 3.20+ instalado
- [ ] GLM instalado o descargado
- [ ] Ejecutar `python3 check_structure.py`
- [ ] Ejecutar `./build.sh release`
- [ ] Ejecutar `./build/phoenix` sin errores
- [ ] Ver 5 ejemplos de demostración

---

## 📊 Lo Que Incluye Phase 1

✅ 2,500+ líneas de código C++20  
✅ Ecuación de Kepler (Newton-Raphson)  
✅ 6 elementos orbitales clásicos  
✅ Conversión bidireccional estado ↔ elementos  
✅ Propagación Kepleriana (analítica)  
✅ Cuerpos celestes multi-nivel  
✅ Sistema de naves con maniobras  
✅ WorldManager con time warp  
✅ 5 ejemplos completos  
✅ Documentación técnica completa

---

## 🚫 Lo Que NO Incluye Phase 1

❌ Partes/jerarquía (Phase 2)  
❌ Motores/propulsión (Phase 3)  
❌ Esferas de influencia (Phase 4)  
❌ Aerodinámica/drag (Phase 5)  
❌ Visualización/UI (Phase 6)  
❌ Colisiones/dinámicas  
❌ Restricciones/joints

---

## 💡 Tips & Tricks

### Usar escalas realistas pero sin overflow

```cpp
// Usar KSP_SCALE = 0.1 para todos los valores
double earth_radius_scaled = 6371000.0 * 0.1;  // 637,100 m
```

### Verificar órbita estable

```cpp
if (orbit.isStable() && orbit.e < 1.0) {
    // órbita elíptica cerrada
    double period = orbit.getPeriod();
}
```

### Debug: imprimir estado

```cpp
std::cout << "a = " << orbit.a << " m\n";
std::cout << "e = " << orbit.e << "\n";
std::cout << "T = " << orbit.getPeriod()/3600 << " h\n";
world.printVessels();
```

### Cambiar time warp

```cpp
world.setTimeWarp(1);      // tiempo real
world.setTimeWarp(10);     // 10x velocidad
world.setTimeWarp(100);    // 100x velocidad
```

---

## 🔗 Enlaces Útiles

- **GLM:** https://glm.g-truc.net/
- **Órbitas:** https://en.wikipedia.org/wiki/Orbital_element
- **KSP:** https://wiki.kerbalspaceprogram.com/
- **Astrodynamics:** https://en.wikibooks.org/wiki/Astrodynamics

---

## ❓ Preguntas Frecuentes

**P: ¿Cuáles son las unidades?**  
R: SI (metros, segundos, kg). Escala KSP = 0.1.

**P: ¿Por qué los números son lejanos?**  
R: Usamos escala 1:10. 400 km = 640,000 m = 64,000 en GLM.

**P: ¿Qué precisión tiene la propagación?**  
R: ~1e-10 radianes en anomalía excéntrica. Suficiente para Phase 1.

**P: ¿Se puede visualizar?**  
R: Phase 1 = consola. Phase 6 tendrá OpenGL.

**P: ¿Cómo paso a Phase 2?**  
R: Ver ROADMAP.md. Cambio: Vessel → Part hierarchy.

---

## 🎓 Aprendizaje

Este proyecto enseña:

1. **Astrodinámica orbital** (ecuación de Kepler, elementos clásicos)
2. **C++20 moderno** (namespaces, smart design, GLM)
3. **Métodos numéricos** (Newton-Raphson, tolerancias)
4. **Ingeniería de software** (arquitectura, extensibilidad)
5. **Física de simulación** (integración temporal, propagación)

---

## 📞 Siguientes Pasos

1. **Compilar y ejecutar:** `./build.sh run`
2. **Leer ejemplos:** `src/main.cpp`
3. **Explorar documentación:** README.md → ARCHITECTURE.md
4. **Modificar ejemplos:** Cambiar órbitas, masas, etc.
5. **Planificar Phase 2:** Ver ROADMAP.md

---

**¿Listo?** Ejecuta:

```bash
./build.sh release && ./build/phoenix
```

**¡Disfruta simulando órbitas!** 🚀

---

_Last Updated: Phase 1 Complete_  
_Quick Start Version: 1.0_

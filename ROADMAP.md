# PROJECT PHOENIX — ROADMAP

## Resumen de fases

| Fase | Estado | Descripción                      |
| ---- | ------ | -------------------------------- |
| 1    | ✅     | Mecánica orbital Kepleriana      |
| 2    | ✅     | Jerarquía de partes              |
| 3    | ✅     | Propulsión (Engine, Tsiolkovsky) |
| 4    | ✅     | Esferas de influencia (SoI)      |
| 5    | ✅     | Aerodinámica y reentrada         |
| 6    | ✅     | Visualización ASCII / HUD        |
| 7    | ✅     | Visualización 3D (Raylib 5)      |
| 8    | 🔄     | Launch to orbit (staged)         |

---

## ✅ Fase 1 — Mecánica orbital Kepleriana

**Commit:** `7c16032`

- ✅ Clase `Orbit` con los 6 elementos orbitales clásicos (a, e, i, Ω, ω, ν)
- ✅ Campo `M0` (anomalía media en la época) para propagación correcta
- ✅ Propagación temporal: `getPositionAtTime(t)`, `getVelocityAtTime(t)`
- ✅ Resolución de la ecuación de Kepler (Newton-Raphson, tol=1e-10)
- ✅ Constructor desde vectores de estado (r, v, μ) → elementos orbitales
- ✅ Transformación plano orbital → marco inercial (ECI)
- ✅ `CelestialBody` con propiedades físicas (masa, radio, μ, atmósfera)
- ✅ 3 ejemplos de demostración (examples 1-3 en main.cpp)

---

## ✅ Fase 2 — Jerarquía de partes

**Commit:** `ab82ad5`

- ✅ Clase `Part` con árbol jerárquico (parent/children)
- ✅ `PartType` enum: Command, FuelTank, Engine, Decoupler, Parachute, etc.
- ✅ Centro de masa dinámico (`getCenterOfMass()`) calculado recursivamente
- ✅ Masa total recursiva: dry mass + fuel de todo el árbol
- ✅ Sistema de staging: `decouple()`, `getActiveParts()`
- ✅ Clase `Vessel` con árbol de partes y órbita Kepleriana
- ✅ Docking/undocking entre naves (`dock()`, `undock()`)
- ✅ Ejemplo 4 en main.cpp

---

## ✅ Fase 3 — Propulsión

**Commit:** `724a9c3`

- ✅ Clase `Engine : public Part`
- ✅ Parámetros: `maxThrust` (N), `Isp` (s), `throttle` [0,1]
- ✅ `ignite()`, `shutdown()`, `setThrottle()`
- ✅ `getCurrentThrust()` = throttle × maxThrust (N)
- ✅ `getMassFlowRate()` = F / (Isp × g0) (kg/s)
- ✅ `getExhaustVelocity()` = Isp × g0 (m/s)
- ✅ `computeDeltaV(m_fuel, m_total)` — ecuación de Tsiolkovsky
- ✅ `computeBurnTime(m_fuel)` — tiempo de quemado a throttle dado
- ✅ `Vessel::executeBurn(dv, dt)` — integración Euler con consumo de combustible
- ✅ `Vessel::computeAvailableDeltaV()` — ΔV total de todos los motores activos
- ✅ Ejemplo 5 en main.cpp

---

## ✅ Fase 4 — Esferas de influencia (Patched-Conics)

**Commit:** `72110d4`

- ✅ `SphereOfInfluence::computeRadius(sma, m_child, m_parent)` — fórmula SOI
- ✅ `SphereOfInfluence::isInside(relPos, soiR)` — test de pertenencia
- ✅ `SphereOfInfluence::checkTransition(vessel, bodies, t)` — detección de transición
  - Caso 1: Entrada en SoI de un cuerpo hijo
  - Caso 2: Salida de la SoI del cuerpo actual hacia el padre
- ✅ `TransitionResult` struct con nuevo cuerpo, posición y velocidad en nuevo marco
- ✅ `CelestialBody::getWorldPosition(t)` — recursivo hasta la raíz (estrella)
- ✅ `CelestialBody::getWorldVelocity(t)` — idem
- ✅ `CelestialBody::getSoIRadius()` — a × (m/m_parent)^0.4
- ✅ `WorldManager::buildBodyHierarchy()` — enlaza punteros `parentBody`
- ✅ `WorldManager::updateSoITransitions(t)` — loop de detección + aplicación
- ✅ `WorldManager::applySoITransition(vessel, result, t)` — recalcula órbita
- ✅ `WorldManager::updateFloatingOrigin(threshold)` — reposiciona para evitar pérdida de precisión
- ✅ Corrección bug `Orbit::M0`: propagación ahora siempre devuelve la posición correcta en `t=epoch`
- ✅ Ejemplos 6, 7, 8 en main.cpp
  - example6: Órbita Tierra-Luna, posición de la Luna
  - example7: WorldManager completo con jerarquía Tierra-Luna
  - example8: Transición SoI detectada correctamente (Tierra → Luna)

---

## 🔄 Fase 5 — Aerodinámica y reentrada

**Commit:** (pendiente)

- ✅ Modelo de atmósfera exponencial por capas (`Atmosphere`, 5 capas ISA)
- ✅ Presión dinámica: D = ½ρv²CdA
- ✅ Flujo de calor de Chapman: q = 1.83×10⁻⁴ √(ρ/R) v³ [W/m²]
- ✅ Integrador numérico RK4 de 4to orden (gravedad + arrastre)
- ✅ Simulación de trayectoria de reentrada completa (`AeroForces::simulate`)
- ✅ `Vessel::simulateReentry()` integrado con la nave
- ✅ Ejemplo 9: cápsula de reentrada desde órbita elíptica de deórbita

---

## 🔄 Fase 6 — Visualización / UI

**Commit:** (pendiente)

- ✅ `AsciiRenderer`: canvas 2D de caracteres con viewport configurable
- ✅ Dibujado de órbitas en el plano perifocal (`drawOrbit`)
- ✅ Dibujado del cuerpo central como círculo sólido (`drawBody`)
- ✅ Marcadores de periapsis (P) y apoapsis (A)
- ✅ Corrección de relación de aspecto de caracteres monoespaciados
- ✅ Perfil gráfico altitud-velocidad de reentrada (`displayReentryProfile`)
- ✅ `MissionDisplay`: HUD de telemetría de vuelo en terminal
  - Panel completo de control de misión (`printDashboard`)
  - Tabla de elementos orbitales Keplerianos (`printOrbitalElements`)
  - Informe de combustible con barra de progreso (`printFuelReport`)
  - Nodo de maniobra con ΔV y tiempo de quemado (`printManeuverNode`)
- ✅ Ejemplo 10: demostración de mapas orbitales y HUD de misión

---

## ✅ Fase 7 — Visualización 3D (Raylib 5)

**Commit:** `78b6ab8`

- ✅ Integración Raylib 5.5.0 con detección automática en CMake
- ✅ Escena 3D orbital con Tierra KSP-scale y atmósfera con anillos de brillo
- ✅ Starfield procedural: 2500 estrellas con brillo variable
- ✅ 3 trayectorias simultáneas: LEO 200km, Hohmann, ISS-like 51.6°
- ✅ Nave Phoenix-1 con marcador visual y flecha de velocidad
- ✅ Trayectoria de reentrada pre-calculada (gradiente color naranja→rojo)
- ✅ Cámara orbital interactiva: drag ratón, scroll zoom, reset (R)
- ✅ Time warp: ×1, ×10, ×100, ×500 + pausa
- ✅ Toggles de capas visuales (L/H/I/E/A/G)
- ✅ HUD de telemetría en tiempo real (MET, altitud, velocidad, elementos)
- ✅ Panel de controles en pantalla
- ✅ Leyenda de órbitas
- ✅ FPS monitor
- ✅ Fix macOS ARM: removido FLAG_MSAA_4X_HINT (causaba crash GLFW)
- ✅ Ejemplo 11 (demo_3d.cpp): visualizador completamente funcional

---

## 🔄 Fase 8 — Launch to Orbit (Staged Rockets)

**Commit:** (en progreso)

### 8A — Vehicle Modeling
- [ ] `Stage` class: motor, tanque, estructura
- [ ] `LaunchVehicle`: vector de stages, CoM/Isp agregados
- [ ] Fuel distribution: transferencia entre tanques por gravedad
- [ ] Thrust curve y empuje disponible por stage
- [ ] Masa de estructura (dry mass) por stage
- [ ] Tracking de centro de masa dinámico durante quemado

### 8B — Launch Sequence
- [ ] `LaunchController`: secuencia de ignición
- [ ] Separation logic: detector de motor apagado, decoupler automático
- [ ] Collision physics: separación segura entre stages (distancia mínima)
- [ ] Launch clamps: restricción inicial de posición/velocidad
- [ ] Countdown y hold-down firing

### 8C — Ascent Guidance
- [ ] Gravity turn profile: inicio de pitch at 50m/s, progresión temporal
- [ ] Max-Q detection: búsqueda dinámica de punto de máxima presión dinámica
- [ ] Throttle management: limitación por Max-Q
- [ ] Target orbit specification: altitud, inclinación, argumento del nodo
- [ ] Pitch program: cálculo de ángulo requerido vs. vuelo actual

### 8D — Autopilot (Thrust Vector Control)
- [ ] Gimbal control: vector thrust hacia proa deseada
- [ ] PID controller: error de pitch/yaw/roll → gimbal angle
- [ ] Fin control: aeroaletas para estabilidad (si aplica)
- [ ] Throttle servo: rampa de empuje suave
- [ ] g-limit: protección contra sobrecarga estructural

### 8E — Orbital Insertion
- [ ] Coast phase detection: apogeo → periapsis fijos
- [ ] Circularization burn: cálculo ΔV, tiempo de ignición
- [ ] Burn execution: timing, throttle profile
- [ ] Achieve orbit: validación de elemento a circular

### 8F — Launch Simulation UI
- [ ] Real-time telemetry: altitud, velocidad, ángulo de vuelo, aceleración
- [ ] Stage info: masa actual, empuje, ΔV disponible
- [ ] Abort modes: emergency staging, chutes, reentrada
- [ ] Flight director: guía visual de pitch/yaw/roll
- [ ] Replay system: grabación y reproducción de vuelo

---

## ⏳ Fases futuras

- Phase 9: Multi-vessel coordination (rendezvous, docking automático)
- Phase 10: Mission planning (maneuver node calculator)
- Phase 11: Multiplayer networking (shared orbits, physics sync)

---

## Línea de tiempo estimada

```
Ph1 ──────── Ph2 ──────── Ph3 ──────── Ph4 ──────── Ph5 ──────── Ph6 ──── Ph7 ──────── Ph8
  ✅            ✅            ✅            ✅          ✅          ✅        ✅        🔄
Orbital      Partes      Propulsión     SoI      Aerodinámica  ASCII/HUD  3D/Raylib  Launch
```

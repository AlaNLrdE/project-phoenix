# PROJECT PHOENIX — ROADMAP

## Resumen de fases

| Fase | Estado | Descripción                           |
|------|--------|---------------------------------------|
| 1    | ✅      | Mecánica orbital Kepleriana           |
| 2    | ✅      | Jerarquía de partes                   |
| 3    | ✅      | Propulsión (Engine, Tsiolkovsky)       |
| 4    | ✅      | Esferas de influencia (SoI)           |
| 5    | ✅      | Aerodinámica y reentrada              |
| 6    | 🔄     | Visualización / UI                    |

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

## ⏳ Fase 6 — Visualización / UI (Futura)

- [ ] Renderer de órbitas (curvas de Keplero en 2D/3D)
- [ ] Proyección de trayectorias futuras
- [ ] Indicadores de ΔV, periapsis/apoapsis dinámico
- [ ] Modo mapa (Map View estilo KSP)
- [ ] Posible backend: Dear ImGui / SDL2 / OpenGL

---

## Línea de tiempo estimada

```
Ph1 ──────── Ph2 ──────── Ph3 ──────── Ph4 ──────── Ph5 ──────── Ph6
  ✅            ✅            ✅            ✅          🔄 (actual)    ⏳
Orbital      Partes      Propulsión     SoI        Aerodinámica  Visualiz.
```

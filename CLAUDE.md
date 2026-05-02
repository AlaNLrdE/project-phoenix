# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
./build.sh release          # compile all targets
./build/phoenix             # console demos (Phases 1–8)
./build/phoenix_3d          # 3D orbital viewer (Phase 7, Raylib)
./build/phoenix_launch      # 3D launch demo (Phase 8, Raylib)

rm -rf build && ./build.sh release   # clean rebuild
python3 check_structure.py           # verify file layout
```

Requires: C++20, CMake 3.20+, GLM (header-only), Raylib 5+ (3D targets only).  
macOS: `brew install glm raylib`. GLM can also live in `extern/glm/`.

## Architecture

KSP-style orbital mechanics simulator at **1:10 scale** (`KSP_SCALE = 0.1`).  
All altitudes, radii, and orbital distances in the code are 10× smaller than real Earth.

**Namespace hierarchy:**

| Namespace | Classes |
|---|---|
| `Phoenix::Math` | `Constants`, GLM typedefs (`dvec3`, `dquat`, …) |
| `Phoenix::Physics` | `Orbit`, `CelestialBody`, `Atmosphere`, `AeroForces` |
| `Phoenix::Parts` | `Part`, `Engine` |
| `Phoenix::Vessels` | `Vessel` |
| `Phoenix::World` | `WorldManager`, `SphereOfInfluence` |
| `Phoenix::UI` | `AsciiRenderer`, `MissionDisplay` |
| `Phoenix::Launch` | `StageConfig`, `LaunchVehicle`, `AscentIntegrator`, `GuidanceLaw` |

**Key algorithms:**
- Kepler equation solved with Newton-Raphson (tolerance 1e-10, typically 3–5 iterations).
- `AscentIntegrator` uses RK4 (dt ≈ 0.05 s) with forces: thrust + point-mass gravity + atmospheric drag.
- `GuidanceLaw` drives pitch: vertical below `hKick`, smoothstep 0°→45° up to `hTurn`, prograde tracking above.
- ΔV via Tsiolkovsky: `m0` = wet mass from stage _i_ upward; `mf = m0 − propellant_i`.

**Data flow for a launch simulation:**
```
StageConfig(s) → LaunchVehicle → AscentIntegrator(+GuidanceLaw) → AscentResult
                                                                        ↓
                                                               state vector at cutoff
                                                               → Orbit (circularization, Phase 8D+)
```

## Phase 8 conventions (sub-phases 8A–8F)

- Each sub-phase adds to **`src/demo_launch.cpp`** → `phoenix_launch` executable (never a new file).
- After each sub-phase: (1) update `.agent/SESSION.yaml` (`status`, `commit_hash`, `phases_complete`, `checklist`, `next_phase`), (2) extend `demo_launch.cpp`, (3) commit both together.
- Current status: **8C complete** (commit `e2bd705`). Next: **Phase 8D — OrbitalManeuvers**.

### Phase 8D tasks
1. `CircularizationBurn` — ΔV at apoapsis to raise periapsis to circular orbit.
2. Integrate with `AscentResult` — hand off state vector at cutoff to `Orbit` class.
3. Visualize circularized orbit in `demo_launch.cpp` (second orbit ring after insertion).
4. Console demo: show circularization ΔV in the Phase 8 example output.

## Session state

Canonical session file: `.agent/SESSION.yaml`.  
Resume protocol: `cat .agent/SESSION.yaml` → `python3 check_structure.py` → `./build.sh release`.

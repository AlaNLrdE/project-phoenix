/**
 * PROJECT PHOENIX — Demo Launch 3D (Phase 8)
 * ===========================================
 * Visualizador 3D del vehículo de lanzamiento y simulación de ascenso.
 *
 * Phase 8A — Vehicle stack viewer (tecla V):
 *   - Cilindros coloreados por etapa, marcador de CoM, HUD de rendimiento
 *   - Exploded view (E), selección de etapa (Tab/1-3)
 *
 * Phase 8B — Launch animation (tecla L):
 *   - Ascenso simulado con AscentIntegrator (RK4 + gravity turn)
 *   - Trail de trayectoria, separación de etapas, cámara de seguimiento
 *   - HUD de telemetría en tiempo real (altitud, velocidad, etapa activa)
 *
 * Phase 8C — Ascent guidance (mismo modo L, guiado mejorado):
 *   - GuidanceLaw: pitch program altitud-dependiente, Max-Q throttle
 *   - El motor se corta cuando apoapsis = 40 km (400 km real @ KSP scale)
 *   - Anillo de órbita objetivo visible en 3D
 *   - Marcador Max-Q en el trail (punto azul brillante)
 *   - Flash de "ORBIT INSERTION" al alcanzar la apoapsis objetivo
 *
 * Phase 8D — Orbital maneuvers (mismo modo L):
 *   - OrbitalManeuvers::circularize(): ΔV de circularización en apoapsis
 *   - Elipse de inserción visible en 3D (segmentos sobre la superficie)
 *   - Anillo de órbita circular (cian) sobre el anillo objetivo (verde)
 *   - ΔV de circularización en el panel de órbita del HUD
 *
 * Controles:
 *   V                              — modo stack viewer (Phase 8A)
 *   L                              — modo launch animation (Phase 8B/8C)
 *   Raton boton izq. + arrastrar   — rotar camara
 *   Scroll                         — zoom
 *   R                              — reset camara
 *   ESPACIO                        — pausar/reanudar (modo launch)
 *   1/2/3/4                        — velocidad de simulacion x1/x5/x20/x100
 *   E                              — exploded view (modo stack)
 *   Tab / 0-2                      — seleccionar etapa (modo stack)
 *   ESC                            — salir
 */

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <launch/LaunchVehicle.hpp>
#include <launch/AscentIntegrator.hpp>
#include <launch/GuidanceLaw.hpp>
#include <launch/OrbitalManeuvers.hpp>
#include <physics/CelestialBody.hpp>
#include <physics/Atmosphere.hpp>
#include <math/Constants.hpp>

#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

using namespace Phoenix::Math;
using namespace Phoenix::Physics;
using namespace Phoenix::Launch;

// ─── Helpers de formato ───────────────────────────────────────────────────────

static std::string fmt(double v, int prec = 1) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(prec) << v;
    return s.str();
}

// ─── Starfield ────────────────────────────────────────────────────────────────

struct Star { Vector3 pos; float brightness; };

static std::vector<Star> makeStars(int n = 1500, float r = 300.0f) {
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> ang(0.0f, 6.2832f);
    std::uniform_real_distribution<float> ct(-1.0f, 1.0f);
    std::uniform_real_distribution<float> br(0.4f, 1.0f);
    std::vector<Star> out; out.reserve(n);
    for (int i = 0; i < n; ++i) {
        float c = ct(rng), s = std::sqrt(1.0f - c * c), a = ang(rng);
        out.push_back({{ r*s*std::cos(a), r*c, r*s*std::sin(a) }, br(rng)});
    }
    return out;
}

// ─── HUD helpers ─────────────────────────────────────────────────────────────

static void hudBox(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, { 0, 0, 0, 170 });
    DrawRectangleLines(x, y, w, h, { 60, 180, 255, 200 });
}

static void hudRow(int x, int y, const char *key, const std::string &val,
                   Color vc = WHITE) {
    DrawText(key, x, y, 13, { 120, 200, 255, 220 });
    DrawText(val.c_str(), x + 145, y, 13, vc);
}

// ─── Colores por etapa ────────────────────────────────────────────────────────
//  0 = booster, 1 = core, 2+ = upper

static Color stageColor(int i, int total, bool selected) {
    Color base;
    if (i == 0)          base = { 220, 100, 40,  255 };  // booster — naranja
    else if (i == total-1) base = { 200, 230, 255, 255 }; // upper   — blanco
    else                 base = { 230, 200, 50,  255 };  // mid     — amarillo
    if (selected) {
        base.r = (unsigned char)std::min(255, base.r + 40);
        base.g = (unsigned char)std::min(255, base.g + 40);
        base.b = (unsigned char)std::min(255, base.b + 40);
    }
    return base;
}

// ─── Geometría de la pila ─────────────────────────────────────────────────────

struct StageVis {
    float base;    // Y de la base de esta etapa en render units
    float height;  // altura de la etapa en render units
    float radius;  // radio del cilindro
    Color col;
    std::string name;
    double dv, twr, isp, wetTons;
};

static std::vector<StageVis> buildStack(const LaunchVehicle &lv, int selIdx) {
    // 1 render unit = 1 metre (escala humana para el cohete)
    // stageHeight() ya devuelve metros
    const int N = lv.stageCount();
    std::vector<StageVis> vis(N);

    float base = 0.0f;
    for (int i = 0; i < N; ++i) {
        const auto &s = lv.getStage(i);
        // reutilizamos la misma heurística que LaunchVehicle::stageHeight()
        float propT = (float)(s.getPropellantMass() / 1000.0);
        float engL  = s.engines.empty() ? 0.0f : 1.2f;
        float h     = 0.5f + propT * 0.15f + engL;

        // Radio proporcional a la raíz cuadrada de la masa (cohete cilíndrico)
        float massT = (float)(s.getWetMass() / 1000.0);
        float rad   = 0.3f + 0.03f * std::sqrt(massT);

        vis[i].base    = base;
        vis[i].height  = h;
        vis[i].radius  = rad;
        vis[i].col     = stageColor(i, N, i == selIdx);
        vis[i].name    = s.name;
        vis[i].dv      = lv.getStageDeltaV(i);
        vis[i].twr     = lv.getStageTWR(i);
        vis[i].isp     = s.getWeightedIsp();
        vis[i].wetTons = s.getWetMass() / 1000.0;
        base += h;
    }
    return vis;
}

// ─── Escala física → render (usada en modo launch) ───────────────────────────

static constexpr double PHYS_EARTH_R  = 6371000.0 * Constants::KSP_SCALE; // 637100 m
static constexpr double RENDER_EARTH_R = 1.5;  // render units
static constexpr double P2R = RENDER_EARTH_R / PHYS_EARTH_R;

static Vector3 p2r(const dvec3 &v) {
    return { (float)(v.x * P2R),
             (float)(v.z * P2R),
             (float)(v.y * P2R) };
}

// ─── Rocket 3D model (simple cylinder+cone en render space) ──────────────────

static void drawRocketAt(Vector3 pos, Vector3 velDir, float scale, int activeStage,
                          int totalStages)
{
    // Alinear el eje Y del cilindro con la dirección de vuelo
    // Raylib DrawCylinder no soporta rotación arbitraria, usar DrawLine para simplificar
    // El cohete se dibuja como una pequeña esfera+línea en modo launch
    Color c = { 220, 220, 240, 255 };
    DrawSphere(pos, scale * 0.3f, c);
    // Llama al motor (punto brillante)
    if (totalStages > 0 && activeStage < totalStages) {
        Vector3 tailPos = {
            pos.x - velDir.x * scale * 0.5f,
            pos.y - velDir.y * scale * 0.5f,
            pos.z - velDir.z * scale * 0.5f
        };
        DrawSphere(tailPos, scale * 0.2f, { 255, 160, 50, 230 });
        // pluma del motor
        for (int k = 0; k < 3; ++k) {
            float t = (float)k / 3.0f;
            Vector3 flame = {
                tailPos.x - velDir.x * scale * (0.3f + t * 0.5f),
                tailPos.y - velDir.y * scale * (0.3f + t * 0.5f),
                tailPos.z - velDir.z * scale * (0.3f + t * 0.5f)
            };
            unsigned char alpha = (unsigned char)(200 * (1.0f - t));
            DrawSphere(flame, scale * (0.15f - t * 0.1f), { 255, 100, 30, alpha });
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  MAIN
// ──────────────────────────────────────────────────────────────────────────────

int main()
{
    // ── Launch vehicle — Atlas-K (TWR ~1.37, sequential staging, Phase 8C) ──
    // Low TWR gives a longer ~147s Stage-0 burn, giving the pitch program time
    // to build enough horizontal velocity so the engine-cutoff condition fires.
    LaunchVehicle soyuz("Atlas-K (KSP 0.1)");
    {
        StageConfig s0; s0.name="Booster"; s0.strutMass=100; s0.decouplerMass=50;
        s0.hasDecoupler=true;
        s0.engines.push_back({"RD-180K", 200, 180000, 300, 1});
        s0.tanks.push_back({"S1-Tank", 600, 9000});
        soyuz.addStage(s0);
    }
    {
        StageConfig s1; s1.name="UpperStage"; s1.strutMass=30; s1.hasDecoupler=false;
        s1.engines.push_back({"RL10K", 100, 90000, 350, 1});
        s1.tanks.push_back({"S2-Tank", 300, 2500});
        s1.tanks.push_back({"Capsule", 500, 0});
        soyuz.addStage(s1);
    }

    // ── Pre-computar trayectoria de ascenso (Phase 8B) ────────────────────────
    CelestialBody earth("Earth", 5.972e24,
        6371000.0 * Constants::KSP_SCALE, 86164.0,
        Constants::MU_EARTH * Constants::KSP_SCALE * Constants::KSP_SCALE,
        9.81, true, 70000.0 * Constants::KSP_SCALE);
    Atmosphere atm = Atmosphere::makeEarthLike(Constants::KSP_SCALE);

    // ── Phase 8C: guided ascent to 40 km apoapsis ───────────────────────────
    GuidanceLaw guidance;
    guidance.hKick          = 200.0;
    guidance.hTurn          = 6000.0;
    guidance.maxQLimit      = 35000.0;
    guidance.maxQThrottle   = 0.75;
    guidance.targetApoapsis = 40000.0;   // 40 km (= 400 km real @ KSP 0.1 scale)

    AscentIntegrator integrator(soyuz, earth, &atm);
    integrator.setThrottle(1.0);
    integrator.setRecordInterval(10);
    integrator.setGuidance(guidance);
    AscentResult ascent = integrator.simulate(0.05, 900.0, 0.0);

    // Convertir trayectoria a render space
    std::vector<Vector3> trailRender;
    trailRender.reserve(ascent.trajectory.size());
    for (const auto &pt : ascent.trajectory)
        trailRender.push_back(p2r(pt.position));

    // Encontrar índice de Max-Q en la trayectoria muestreada
    int maxQIdx = 0;
    double maxQVal = 0.0;
    for (int i = 0; i < (int)ascent.trajectory.size(); ++i) {
        if (ascent.trajectory[i].dynPressure > maxQVal) {
            maxQVal = ascent.trajectory[i].dynPressure;
            maxQIdx = i;
        }
    }

    // Índice de engine cutoff (inserción orbital)
    int cutoffIdx = -1;
    for (int i = 0; i < (int)ascent.trajectory.size(); ++i) {
        if (ascent.trajectory[i].time >= ascent.cutoffTime && ascent.orbitInserted) {
            cutoffIdx = i; break;
        }
    }

    // Radio del anillo de órbita objetivo en render space
    float targetOrbitR = (float)((earth.radius + guidance.targetApoapsis) * P2R);

    // ── Phase 8D: circularización ─────────────────────────────────────────────
    CircularizationResult circ;
    if (ascent.orbitInserted) {
        circ = OrbitalManeuvers::circularize(ascent, earth);
    }

    // Elipse de inserción pre-muestreada (segmentos sobre superficie solamente)
    struct OrbitSeg { Vector3 a, b; };
    std::vector<OrbitSeg> insertionSegs;
    if (circ.valid) {
        const int   segs  = 120;
        const double T    = circ.insertionOrbit.getPeriod();
        const double tEpoch = ascent.cutoffTime;
        for (int k = 0; k < segs; ++k) {
            double ta = tEpoch + (double)k       / segs * T;
            double tb = tEpoch + (double)(k + 1) / segs * T;
            dvec3 pa = circ.insertionOrbit.getPositionAtTime(ta);
            dvec3 pb = circ.insertionOrbit.getPositionAtTime(tb);
            // Solo dibujar segmentos donde ambos puntos estén sobre la superficie
            if (glm::length(pa) >= earth.radius && glm::length(pb) >= earth.radius)
                insertionSegs.push_back({ p2r(pa), p2r(pb) });
        }
    }

    // Radio del anillo de la órbita circular (= apoapsis de inserción)
    float circOrbitR = circ.valid
        ? (float)(circ.circularOrbit.a * P2R)
        : targetOrbitR;

    // ── Window ───────────────────────────────────────────────────────────────
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Project Phoenix — Launch Viewer (8A+8B+8C+8D)");
    SetTargetFPS(60);

    // ── Estado de UI ─────────────────────────────────────────────────────────
    enum class Mode { Stack, Launch } mode = Mode::Stack;
    int   selStage   = -1;
    bool  exploded   = false;
    float explodGap  = 2.0f;

    // Launch animation state
    double simTime    = 0.0;
    double timeScale  = 1.0;
    bool   paused     = false;
    int    curPtIdx   = 0;   // índice en trajectory
    float  stagingFlash    = 0.0f;
    int    lastStagingIdx  = 0;
    float  insertionFlash  = 0.0f;  // flash de "ORBIT INSERTION" (8C)
    bool   insertionShown  = false;

    // ── Cámara ────────────────────────────────────────────────────────────────
    Camera3D camera = {};
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float camYaw    =  30.0f;
    float camPitch  =  20.0f;
    float camDist   =  40.0f;
    float camTgtX   =  0.0f;
    float camTgtY   =  15.0f;
    float camTgtZ   =  0.0f;

    auto updateCamera = [&]() {
        float yaw   = camYaw   * DEG2RAD;
        float pitch = camPitch * DEG2RAD;
        camera.position = {
            camTgtX + camDist * std::cos(pitch) * std::sin(yaw),
            camTgtY + camDist * std::sin(pitch),
            camTgtZ + camDist * std::cos(pitch) * std::cos(yaw)
        };
        camera.target = { camTgtX, camTgtY, camTgtZ };
        camera.up     = { 0.0f, 1.0f, 0.0f };
    };
    updateCamera();

    Vector2 lastMouse = {0, 0};
    bool    dragging  = false;

    // ── Starfield ─────────────────────────────────────────────────────────────
    auto stars = makeStars(1500, 300.0f);

    float earthR = (float)(earth.radius * P2R);

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // ── Input global ──────────────────────────────────────────────────────
        if (IsKeyPressed(KEY_V)) { mode = Mode::Stack; }
        if (IsKeyPressed(KEY_L)) {
            mode = Mode::Launch;
            simTime = 0.0; curPtIdx = 0; paused = false;
            stagingFlash = 0.0f; lastStagingIdx = 0;
            insertionFlash = 0.0f; insertionShown = false;
            // Ajustar cámara para vista orbital
            camDist = 5.0f; camPitch = 10.0f; camYaw = 30.0f;
        }
        if (IsKeyPressed(KEY_R)) {
            camYaw = 30.0f; camPitch = 20.0f; camDist = 40.0f;
            camTgtX = 0.0f; camTgtY = 15.0f; camTgtZ = 0.0f;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            camDist -= wheel * (mode == Mode::Launch ? 0.3f : 2.0f);
            camDist  = Clamp(camDist, 0.1f, 150.0f);
        }
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  { dragging = true;  lastMouse = mouse; }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) { dragging = false; }
        if (dragging) {
            Vector2 d = { mouse.x - lastMouse.x, mouse.y - lastMouse.y };
            camYaw   -= d.x * 0.3f;
            camPitch += d.y * 0.3f;
            camPitch  = Clamp(camPitch, -85.0f, 85.0f);
            lastMouse = mouse;
        }

        // ── Input modo Stack ──────────────────────────────────────────────────
        if (mode == Mode::Stack) {
            if (IsKeyPressed(KEY_TAB))   selStage = (selStage + 2) % (soyuz.stageCount()+1) - 1;
            if (IsKeyPressed(KEY_ZERO))  selStage = -1;
            if (IsKeyPressed(KEY_ONE))   selStage = 0;
            if (IsKeyPressed(KEY_TWO))   selStage = 1;
            if (IsKeyPressed(KEY_THREE)) selStage = 2;
            if (IsKeyPressed(KEY_E))     exploded = !exploded;
        }

        // ── Input modo Launch ─────────────────────────────────────────────────
        if (mode == Mode::Launch) {
            if (IsKeyPressed(KEY_SPACE)) paused = !paused;
            if (IsKeyPressed(KEY_ONE))   timeScale =   1.0;
            if (IsKeyPressed(KEY_TWO))   timeScale =   5.0;
            if (IsKeyPressed(KEY_THREE)) timeScale =  20.0;
            if (IsKeyPressed(KEY_FOUR))  timeScale = 100.0;

            if (!paused && !ascent.trajectory.empty()) {
                simTime += (double)dt * timeScale;
                // Avanzar índice de trayectoria
                while (curPtIdx + 1 < (int)ascent.trajectory.size() &&
                       ascent.trajectory[curPtIdx + 1].time <= simTime)
                    curPtIdx++;
                // Detectar staging
                if (lastStagingIdx < (int)ascent.stagingTimes.size() &&
                    simTime >= ascent.stagingTimes[lastStagingIdx]) {
                    stagingFlash = 2.5f;
                    lastStagingIdx++;
                }
                stagingFlash -= dt;

                // Detectar engine cutoff / orbit insertion (Phase 8C)
                if (!insertionShown && ascent.orbitInserted &&
                    simTime >= ascent.cutoffTime) {
                    insertionFlash = 3.5f;
                    insertionShown = true;
                }
                insertionFlash -= dt;
            }

            // Cámara sigue al cohete en modo launch
            if (curPtIdx < (int)trailRender.size()) {
                Vector3 rp = trailRender[curPtIdx];
                float smooth = std::min(1.0f, dt * 3.0f);
                camTgtX += (rp.x - camTgtX) * smooth;
                camTgtY += (rp.y - camTgtY) * smooth;
                camTgtZ += (rp.z - camTgtZ) * smooth;
            }
        }

        updateCamera();

        // ── Render ───────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground({ 3, 3, 12, 255 });
        BeginMode3D(camera);

        // Starfield (siempre)
        for (const auto &st : stars) {
            unsigned char b = (unsigned char)(st.brightness * 190);
            DrawPoint3D(st.pos, { b, b, b, b });
        }

        if (mode == Mode::Stack) {
            // ── MODO STACK VIEWER ─────────────────────────────────────────────
            DrawPlane({ 0, 0, 0 }, { 60, 60 }, { 30, 40, 30, 80 });
            for (int g = -5; g <= 5; ++g) {
                DrawLine3D({(float)g*5, 0.01f, -25}, {(float)g*5, 0.01f, 25}, { 50, 70, 50, 100 });
                DrawLine3D({-25, 0.01f, (float)g*5}, { 25, 0.01f, (float)g*5}, { 50, 70, 50, 100 });
            }
            DrawCylinder({ 0, 0, 0 }, 2.5f, 2.5f, 1.0f, 12, { 60, 60, 60, 255 });
            DrawCylinderWires({ 0, 0, 0 }, 2.5f, 2.5f, 1.0f, 12, { 80, 80, 80, 200 });

            auto stack = buildStack(soyuz, selStage);
            const int N = (int)stack.size();
            float comH = (float)soyuz.getCoMHeight();
            if (exploded) {
                for (int i = 1; i < N; ++i)
                    for (int j = i; j < N; ++j) stack[j].base += explodGap;
                float tm = 0.0f, nm = 0.0f;
                for (int j = 0; j < N; ++j) {
                    float m = (float)soyuz.getStage(j).getWetMass();
                    nm += m * (stack[j].base + stack[j].height/2.0f); tm += m;
                }
                if (tm > 0.0f) comH = nm/tm;
            }
            for (int i = 0; i < N; ++i) {
                const auto &sv = stack[i];
                Vector3 bpos = { 0.0f, sv.base, 0.0f };
                Vector3 cent = { 0.0f, sv.base + sv.height/2.0f, 0.0f };
                DrawCylinder(bpos, sv.radius*.95f, sv.radius*.95f, sv.height, 16, sv.col);
                DrawCylinderWires(bpos, sv.radius*.95f, sv.radius*.95f, sv.height, 16, {30,30,30,200});
                if (i == N-1) {
                    Vector3 top = { 0.0f, sv.base+sv.height, 0.0f };
                    DrawCylinder(top, sv.radius*.95f, 0.0f, sv.radius*1.2f, 12, {220,220,240,255});
                }
                int eCount=0;
                for (const auto &e : soyuz.getStage(i).engines) eCount += e.count;
                float nr = sv.radius*.18f, ring = sv.radius*.55f;
                for (int k = 0; k < std::min(eCount,8); ++k) {
                    float ang = (float)k/std::min(eCount,8) * 6.2832f;
                    DrawSphere({ring*std::cos(ang), sv.base-nr*.5f, ring*std::sin(ang)}, nr, {50,50,60,255});
                }
                if (i == selStage)
                    DrawCylinderWires(bpos, sv.radius+.05f, sv.radius+.05f, sv.height+.05f, 16, {255,255,100,255});
                DrawLine3D(cent, {cent.x+sv.radius+.5f, cent.y, cent.z}, {180,180,180,130});
            }
            // CoM
            DrawSphere({0,comH,0}, .2f, {255,60,60,220});
            DrawLine3D({-4,comH,0},{4,comH,0},{255,60,60,160});
            DrawLine3D({0,comH,-4},{0,comH,4},{255,60,60,160});

        } else {
            // ── MODO LAUNCH ANIMATION ─────────────────────────────────────────

            // Tierra
            DrawSphere({0,0,0}, earthR, {40, 80, 160, 255});
            DrawSphereWires({0,0,0}, earthR, 12, 12, {60, 110, 200, 60});
            // Atmósfera (anillo semitransparente)
            float atmR = (float)((earth.radius + atm.atmosphereHeight) * P2R);
            DrawSphereWires({0,0,0}, atmR, 8, 8, {80, 140, 220, 40});

            // Anillo de órbita objetivo (Phase 8C) — círculo en el plano ecuatorial
            {
                const int  segs  = 64;
                const float twoPi = 6.2832f;
                for (int k = 0; k < segs; ++k) {
                    float a0 = (float)k       / segs * twoPi;
                    float a1 = (float)(k + 1) / segs * twoPi;
                    Vector3 p0 = { targetOrbitR * std::cos(a0), 0.0f, targetOrbitR * std::sin(a0) };
                    Vector3 p1 = { targetOrbitR * std::cos(a1), 0.0f, targetOrbitR * std::sin(a1) };
                    DrawLine3D(p0, p1, { 80, 220, 80, 50 });
                }
            }

            // Phase 8D — elipse de inserción (naranja tenue, solo porción visible)
            for (const auto &sg : insertionSegs)
                DrawLine3D(sg.a, sg.b, { 220, 150, 40, 70 });

            // Phase 8D — anillo de órbita circular post-circularización (cian)
            if (circ.valid) {
                const int  segs  = 64;
                const float twoPi = 6.2832f;
                for (int k = 0; k < segs; ++k) {
                    float a0 = (float)k       / segs * twoPi;
                    float a1 = (float)(k + 1) / segs * twoPi;
                    Vector3 p0 = { circOrbitR * std::cos(a0), 0.0f, circOrbitR * std::sin(a0) };
                    Vector3 p1 = { circOrbitR * std::cos(a1), 0.0f, circOrbitR * std::sin(a1) };
                    DrawLine3D(p0, p1, { 60, 220, 240, 110 });
                }
            }

            // Trail de trayectoria (hasta el punto actual)
            int drawTo = std::min(curPtIdx + 1, (int)trailRender.size());
            for (int i = 1; i < drawTo; ++i) {
                float t = (float)i / (float)trailRender.size();
                unsigned char alpha = (unsigned char)(80 + 150 * t);
                int stIdx = ascent.trajectory[i].stageIndex;
                Color tc = stageColor(stIdx, soyuz.stageCount(), false);
                tc.a = alpha;
                DrawLine3D(trailRender[i-1], trailRender[i], tc);
            }

            // Marcador Max-Q (Phase 8C) — punto azul en el trail
            if (maxQIdx > 0 && maxQIdx < (int)trailRender.size()) {
                DrawSphere(trailRender[maxQIdx], earthR * 0.025f, { 60, 140, 255, 220 });
            }

            // Marcador engine cutoff / orbit insertion
            if (cutoffIdx > 0 && cutoffIdx < (int)trailRender.size()) {
                DrawSphere(trailRender[cutoffIdx], earthR * 0.03f, { 80, 255, 120, 220 });
            }

            // Posición actual del cohete
            if (curPtIdx < (int)trailRender.size()) {
                Vector3 rpos = trailRender[curPtIdx];
                // Dirección de vuelo (normalizada)
                Vector3 velDir = {0,1,0};
                if (curPtIdx > 0) {
                    Vector3 dp = {
                        rpos.x - trailRender[curPtIdx-1].x,
                        rpos.y - trailRender[curPtIdx-1].y,
                        rpos.z - trailRender[curPtIdx-1].z
                    };
                    float len = std::sqrt(dp.x*dp.x + dp.y*dp.y + dp.z*dp.z);
                    if (len > 1e-6f) velDir = {dp.x/len, dp.y/len, dp.z/len};
                }
                float rocketScale = earthR * 0.04f;
                int stIdx = ascent.trajectory[curPtIdx].stageIndex;
                drawRocketAt(rpos, velDir, rocketScale, stIdx, soyuz.stageCount());
            }
        }

        EndMode3D();

        // ── HUD 2D ────────────────────────────────────────────────────────────

        if (mode == Mode::Stack) {
            // Panel de rendimiento
            int px = 1280-280, py=10;
            hudBox(px,py,268,200);
            DrawText("LAUNCH VEHICLE", px+8, py+6, 14, {80,200,255,255});
            DrawText(soyuz.getName().c_str(), px+8, py+22, 12, WHITE);
            int ry=py+44;
            hudRow(px+8,ry,"Total mass",    fmt(soyuz.getTotalMass()/1000.,1)+" t"); ry+=18;
            hudRow(px+8,ry,"Total DV",      fmt(soyuz.getTotalDeltaV(),0)+" m/s",{100,255,100,255}); ry+=18;
            hudRow(px+8,ry,"Payload frac",  fmt(soyuz.getPayloadFraction()*100.,1)+" %"); ry+=18;
            hudRow(px+8,ry,"CoM height",    fmt(soyuz.getCoMHeight(),2)+" m"); ry+=18;
            hudRow(px+8,ry,"Payload mass",  fmt(soyuz.getPayloadMass()/1000.,2)+" t");

            // Etapa seleccionada
            if (selStage >= 0 && selStage < soyuz.stageCount()) {
                const auto &s = soyuz.getStage(selStage);
                hudBox(px, 220, 268, 200);
                DrawText(("STAGE "+std::to_string(selStage)+" "+s.name).c_str(),
                         px+8,226,13, stageColor(selStage, soyuz.stageCount(), false));
                ry=246;
                hudRow(px+8,ry,"Wet mass",  fmt(s.getWetMass()/1000.,1)+" t"); ry+=18;
                hudRow(px+8,ry,"Propellant",fmt(s.getPropellantMass()/1000.,1)+" t"); ry+=18;
                hudRow(px+8,ry,"DV",        fmt(soyuz.getStageDeltaV(selStage),0)+" m/s",{100,255,100,255}); ry+=18;
                hudRow(px+8,ry,"TWR",       fmt(soyuz.getStageTWR(selStage),2)); ry+=18;
                hudRow(px+8,ry,"Isp",       fmt(s.getWeightedIsp(),1)+" s"); ry+=18;
            }

            // Controles
            hudBox(10, 720-80, 480, 68);
            DrawText("CONTROLES", 18, 644, 12, {120,200,255,200});
            DrawText("Arrastrar: rotar | Scroll: zoom | R: reset | E: exploded", 18, 660, 12, {200,200,200,200});
            DrawText("Tab/1/2/3: etapa | 0: deselec | L: modo lanzamiento", 18, 676, 12, {200,200,200,200});

            // Leyenda
            for (int i = soyuz.stageCount()-1; i >= 0; --i) {
                Color c = stageColor(i, soyuz.stageCount(), i == selStage);
                DrawRectangle(10, 10+(soyuz.stageCount()-1-i)*22, 16, 16, c);
                DrawText(("S"+std::to_string(i)+" "+soyuz.getStage(i).name.substr(0,10)).c_str(),
                         30, 12+(soyuz.stageCount()-1-i)*22, 12, c);
            }
            if (exploded)
                DrawText("EXPLODED VIEW  [E]", 10, 720-100, 13, {255,220,60,220});

        } else {
            // ── HUD lanzamiento ───────────────────────────────────────────────
            int px = 1280-260, py=10;
            hudBox(px, py, 248, 240);
            DrawText("ASCENT TELEMETRY", px+8, py+6, 14, {80,200,255,255});
            int ry = py+26;
            if (curPtIdx < (int)ascent.trajectory.size()) {
                const auto &pt = ascent.trajectory[curPtIdx];
                hudRow(px+8,ry,"Mission time", fmt(pt.time,1)+" s"); ry+=18;
                hudRow(px+8,ry,"Altitude",     fmt(pt.altitude/1000.,2)+" km"); ry+=18;
                hudRow(px+8,ry,"Speed",        fmt(pt.speed,1)+" m/s"); ry+=18;
                hudRow(px+8,ry,"Dyn. Press.",  fmt(pt.dynPressure/1000.,1)+" kPa",
                    pt.dynPressure > guidance.maxQLimit
                        ? Color{255,80,80,255} : Color{200,200,200,220}); ry+=18;
                hudRow(px+8,ry,"Mass",         fmt(pt.mass/1000.,1)+" t"); ry+=18;
                hudRow(px+8,ry,"Thrust",       fmt(pt.thrust/1000.,1)+" kN"); ry+=18;
                hudRow(px+8,ry,"Drag",         fmt(pt.drag/1000.,2)+" kN"); ry+=18;
                std::string stName = pt.stageIndex < soyuz.stageCount()
                    ? soyuz.getStage(pt.stageIndex).name : "Coast";
                hudRow(px+8,ry,"Active stage", stName,
                       stageColor(pt.stageIndex, soyuz.stageCount(), false)); ry+=18;
                hudRow(px+8,ry,"Stages jett.", std::to_string(lastStagingIdx)); ry+=18;
            }
            hudRow(px+8,ry,"Time scale", "x"+fmt(timeScale,0));

            // Panel órbita (Phase 8C/8D)
            hudBox(px, py+250, 248, 120);
            DrawText(ascent.orbitInserted ? "ORBIT INSERTION" : "FINAL ORBIT",
                     px+8, py+256, 13,
                     ascent.orbitInserted ? Color{100,255,100,255} : Color{200,200,200,220});
            int ry2 = py+274;
            hudRow(px+8,ry2,"Target apo",  fmt(guidance.targetApoapsis/1000.,0)+" km"); ry2+=18;
            hudRow(px+8,ry2,"Apoapsis",    fmt(ascent.apoapsis/1000.,1)+" km",
                             ascent.orbitInserted ? Color{100,255,100,255} : WHITE); ry2+=18;
            hudRow(px+8,ry2,"Periapsis",   fmt(ascent.periapsis/1000.,1)+" km"); ry2+=18;
            hudRow(px+8,ry2,"Max-Q",       fmt(ascent.maxDynPressure/1000.,1)+" kPa @ "
                             +fmt(ascent.maxDynPressureAlt/1000.,1)+" km"); ry2+=18;
            // Phase 8D: circularization ΔV
            if (circ.valid) {
                hudRow(px+8,ry2,"Circ. DV",
                       fmt(circ.deltaV,1)+" m/s",
                       Color{60,220,240,255});
            }

            // Flash de separación de etapa
            if (stagingFlash > 0.0f) {
                unsigned char alpha = (unsigned char)(std::min(1.0f, stagingFlash) * 230);
                int sIdx = lastStagingIdx - 1;
                std::string msg = "  STAGING! " +
                    (sIdx < soyuz.stageCount() ? soyuz.getStage(sIdx).name : "stage") +
                    " jettisoned";
                DrawRectangle(300, 310, 680, 38, {30,30,30,(unsigned char)(alpha*0.7f)});
                DrawText(msg.c_str(), 320, 320, 22, {255, 200, 60, alpha});
            }

            // Flash de inserción orbital (Phase 8C/8D)
            if (insertionFlash > 0.0f) {
                unsigned char alpha = (unsigned char)(std::min(1.0f, insertionFlash) * 255);
                std::string apo = fmt(ascent.apoapsis / 1000.0, 1) + " km";
                std::string dvStr = circ.valid
                    ? ("  circ.DV=" + fmt(circ.deltaV, 1) + " m/s")
                    : "";
                std::string msg = "  ORBIT INSERTION  apo=" + apo + dvStr + "  ENGINE CUTOFF";
                DrawRectangle(200, 355, 880, 42, {10,30,10,(unsigned char)(alpha*0.85f)});
                DrawRectangleLines(200, 355, 880, 42, {80, 255, 120, alpha});
                DrawText(msg.c_str(), 220, 366, 20, {120, 255, 140, alpha});
            }

            // Controles launch
            hudBox(10, 720-60, 560, 48);
            DrawText("SPACE: pausa | 1/2/3/4: x1/x5/x20/x100 | R: reset cam | V: stack",
                     18, 720-52, 11, {200,200,200,200});
            DrawText((paused ? "PAUSADO" : ("x"+fmt(timeScale,0)+" speed")).c_str(),
                     18, 720-36, 13, paused ? Color{255,100,100,220} : Color{100,255,100,220});
        }

        DrawFPS(1280-80, 720-20);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

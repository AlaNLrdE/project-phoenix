/**
 * PROJECT PHOENIX — Demo Launch 3D (Phase 8)
 * ===========================================
 * Visualizador 3D del modelo de vehículo de lanzamiento (LaunchVehicle).
 *
 * Phase 8A — Vehicle stack viewer:
 *   - Cilindros coloreados por etapa (alturas y masas reales del modelo)
 *   - Marcador de CoM (esfera roja con línea de referencia)
 *   - HUD de rendimiento: ΔV, TWR, Isp, masa por etapa
 *   - Modo "exploded view" para inspeccionar etapas separadas
 *   - Starfield de fondo + suelo con grid
 *
 * Controles:
 *   Ratón botón izq. + arrastrar  — rotar cámara
 *   Scroll                         — zoom
 *   R                              — reset cámara
 *   E                              — toggle exploded view
 *   Tab / número 0-2               — seleccionar etapa (resaltar)
 *   ESC                            — salir
 */

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <launch/LaunchVehicle.hpp>
#include <math/Constants.hpp>

#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>

using namespace Phoenix::Math;
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

// ──────────────────────────────────────────────────────────────────────────────
//  MAIN
// ──────────────────────────────────────────────────────────────────────────────

int main()
{
    // ── Launch vehicle (mismo cohete que demo Phase 8A en main.cpp) ──────────
    LaunchVehicle soyuz("Soyuz-FG (KSP 0.1)");
    {
        StageConfig booster;
        booster.name          = "Boosters";
        booster.strutMass     = 800.0;
        booster.decouplerMass = 120.0;
        booster.hasDecoupler  = true;
        booster.engines.push_back({"RD-107A", 1100.0, 838800.0, 310.7, 4});
        booster.tanks.push_back(  {"BoosterTank", 3200.0, 39600.0});
        soyuz.addStage(booster);
    }
    {
        StageConfig core;
        core.name          = "CoreStage";
        core.strutMass     = 600.0;
        core.decouplerMass = 80.0;
        core.hasDecoupler  = true;
        core.engines.push_back({"RD-108A", 1250.0, 792400.0, 314.2, 1});
        core.tanks.push_back(  {"CoreTank", 6900.0, 91400.0});
        soyuz.addStage(core);
    }
    {
        StageConfig upper;
        upper.name          = "UpperStage";
        upper.strutMass     = 200.0;
        upper.hasDecoupler  = false;
        upper.engines.push_back({"RD-0110",   410.0, 297900.0, 326.0, 1});
        upper.tanks.push_back(  {"UpperTank",  2355.0, 22000.0});
        upper.tanks.push_back(  {"SoyuzCapsule", 2800.0, 0.0});
        soyuz.addStage(upper);
    }

    // ── Window ───────────────────────────────────────────────────────────────
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Project Phoenix — Launch Vehicle Viewer (Phase 8A)");
    SetTargetFPS(60);

    // ── Estado de UI ─────────────────────────────────────────────────────────
    int  selStage   = -1;     // etapa seleccionada (-1 = ninguna)
    bool exploded   = false;  // separar etapas visualmente
    float explodGap = 2.0f;   // separación en modo exploded (m)

    // ── Cámara ────────────────────────────────────────────────────────────────
    // El cohete está centrado en X=0, Z=0, va de Y=0 hasta ~Y=30m.
    // Cámara inicial: vista lateral a ~40m de distancia.
    Camera3D camera = {};
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float camYaw   =  30.0f;
    float camPitch =  15.0f;
    float camDist  =  40.0f;
    float camTargetY = 15.0f;  // apunta al centro del cohete

    auto updateCamera = [&]() {
        float yaw   = camYaw   * DEG2RAD;
        float pitch = camPitch * DEG2RAD;
        camera.position = {
            camDist * std::cos(pitch) * std::sin(yaw),
            camTargetY + camDist * std::sin(pitch),
            camDist * std::cos(pitch) * std::cos(yaw)
        };
        camera.target = { 0.0f, camTargetY, 0.0f };
        camera.up     = { 0.0f, 1.0f, 0.0f };
    };
    updateCamera();

    Vector2 lastMouse = {0, 0};
    bool    dragging  = false;

    // ── Starfield ─────────────────────────────────────────────────────────────
    auto stars = makeStars(1500, 300.0f);

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!WindowShouldClose())
    {
        // ── Input ────────────────────────────────────────────────────────────
        float dt = GetFrameTime(); (void)dt;

        // Teclas de selección de etapa
        if (IsKeyPressed(KEY_TAB)) {
            selStage = (selStage + 2) % (soyuz.stageCount() + 1) - 1;
        }
        if (IsKeyPressed(KEY_ZERO)) selStage = -1;
        if (IsKeyPressed(KEY_ONE))  selStage = 0;
        if (IsKeyPressed(KEY_TWO))  selStage = 1;
        if (IsKeyPressed(KEY_THREE)) selStage = 2;

        // Toggle exploded
        if (IsKeyPressed(KEY_E)) exploded = !exploded;

        // Reset cámara
        if (IsKeyPressed(KEY_R)) {
            camYaw = 30.0f; camPitch = 15.0f; camDist = 40.0f;
        }

        // Zoom con scroll
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            camDist -= wheel * 2.0f;
            camDist  = Clamp(camDist, 5.0f, 150.0f);
        }

        // Rotar con botón izquierdo
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dragging = true; lastMouse = mouse;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging = false;
        if (dragging) {
            Vector2 delta = { mouse.x - lastMouse.x, mouse.y - lastMouse.y };
            camYaw   -= delta.x * 0.3f;
            camPitch += delta.y * 0.3f;
            camPitch  = Clamp(camPitch, -80.0f, 80.0f);
            lastMouse = mouse;
        }

        updateCamera();

        // ── Construir geometría del stack ────────────────────────────────────
        auto stack = buildStack(soyuz, selStage);
        const int N = (int)stack.size();

        // CoM height
        float comH = (float)soyuz.getCoMHeight();

        // En modo exploded, desplazar cada etapa hacia arriba
        if (exploded) {
            for (int i = 1; i < N; ++i) {
                for (int j = i; j < N; ++j)
                    stack[j].base += explodGap;
            }
            // Recalcular CoM aproximado en exploded (sólo visual)
            float totalM = 0.0f, num = 0.0f;
            for (int j = 0; j < N; ++j) {
                float m = (float)soyuz.getStage(j).getWetMass();
                num    += m * (stack[j].base + stack[j].height / 2.0f);
                totalM += m;
            }
            if (totalM > 0.0f) comH = num / totalM;
        }

        // ── Render ───────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground({ 5, 5, 15, 255 });

        BeginMode3D(camera);

            // Starfield
            for (const auto &st : stars) {
                unsigned char b = (unsigned char)(st.brightness * 200);
                DrawPoint3D(st.pos, { b, b, b, b });
            }

            // Suelo
            DrawPlane({ 0, 0, 0 }, { 60, 60 }, { 30, 40, 30, 80 });
            // Grid lines
            for (int g = -5; g <= 5; ++g) {
                DrawLine3D({(float)g*5, 0.01f, -25}, {(float)g*5, 0.01f, 25},
                           { 50, 70, 50, 120 });
                DrawLine3D({-25, 0.01f, (float)g*5}, { 25, 0.01f, (float)g*5},
                           { 50, 70, 50, 120 });
            }

            // Launchpad (cilindro base)
            DrawCylinder({ 0, 0, 0 }, 2.5f, 2.5f, 1.0f, 12, { 60, 60, 60, 255 });
            DrawCylinderWires({ 0, 0, 0 }, 2.5f, 2.5f, 1.0f, 12, { 80, 80, 80, 255 });

            // Etapas del cohete
            for (int i = 0; i < N; ++i) {
                const auto &sv = stack[i];
                Vector3 center = { 0.0f, sv.base + sv.height / 2.0f, 0.0f };
                Vector3 bpos   = { 0.0f, sv.base, 0.0f };

                // Cuerpo principal
                DrawCylinder(bpos, sv.radius * 0.95f, sv.radius * 0.95f,
                             sv.height, 16, sv.col);
                DrawCylinderWires(bpos, sv.radius * 0.95f, sv.radius * 0.95f,
                                  sv.height, 16, { 30, 30, 30, 200 });

                // Cono de nariz (solo etapa superior)
                if (i == N - 1) {
                    Vector3 top = { 0.0f, sv.base + sv.height, 0.0f };
                    DrawCylinder(top, sv.radius * 0.95f, 0.0f, sv.radius * 1.2f,
                                 12, { 220, 220, 240, 255 });
                }

                // Toberas de motor (esferas pequeñas en la base)
                if (!soyuz.getStage(i).engines.empty()) {
                    int eCount = 0;
                    for (const auto &e : soyuz.getStage(i).engines)
                        eCount += e.count;
                    float nozzleR = sv.radius * 0.18f;
                    float nozzleRing = sv.radius * 0.55f;
                    int nDraw = std::min(eCount, 8);
                    for (int k = 0; k < nDraw; ++k) {
                        float ang = (float)k / nDraw * 6.2832f;
                        Vector3 np = {
                            nozzleRing * std::cos(ang),
                            sv.base - nozzleR * 0.5f,
                            nozzleRing * std::sin(ang)
                        };
                        DrawSphere(np, nozzleR, { 50, 50, 60, 255 });
                    }
                }

                // Resaltado si seleccionado: contorno más grueso
                if (i == selStage) {
                    DrawCylinderWires(bpos, sv.radius + 0.05f, sv.radius + 0.05f,
                                      sv.height + 0.05f, 16, { 255, 255, 100, 255 });
                }

                // Etiqueta 3D flotante a la derecha del cilindro
                Vector3 labelPos = { center.x + sv.radius + 0.5f, center.y, center.z };
                DrawLine3D(center, labelPos, { 180, 180, 180, 160 });
                DrawSphere(labelPos, 0.06f, { 200, 200, 200, 200 });
            }

            // Marcador de CoM (esfera roja + línea horizontal)
            Vector3 comPos = { 0.0f, comH, 0.0f };
            DrawSphere(comPos, 0.2f, { 255, 60, 60, 220 });
            // línea horizontal indicadora
            DrawLine3D({ -4.0f, comH, 0.0f }, { 4.0f, comH, 0.0f },
                       { 255, 60, 60, 180 });
            DrawLine3D({ 0.0f, comH, -4.0f }, { 0.0f, comH, 4.0f },
                       { 255, 60, 60, 180 });

        EndMode3D();

        // ── HUD — panel lateral derecho: rendimiento del vehículo ────────────
        {
            int px = 1280 - 280, py = 10, pw = 268, ph = 200;
            hudBox(px, py, pw, ph);
            DrawText("LAUNCH VEHICLE", px + 8, py + 6, 14, { 80, 200, 255, 255 });
            DrawText(soyuz.getName().c_str(), px + 8, py + 22, 12, WHITE);
            int ry = py + 44;
            hudRow(px+8, ry, "Total mass",
                   fmt(soyuz.getTotalMass()/1000.0, 1) + " t"); ry += 18;
            hudRow(px+8, ry, "Total DV",
                   fmt(soyuz.getTotalDeltaV(), 0) + " m/s",
                   { 100, 255, 100, 255 }); ry += 18;
            hudRow(px+8, ry, "Payload frac",
                   fmt(soyuz.getPayloadFraction()*100.0, 1) + " %"); ry += 18;
            hudRow(px+8, ry, "CoM height",
                   fmt(soyuz.getCoMHeight(), 2) + " m"); ry += 18;
            hudRow(px+8, ry, "Stages",
                   std::to_string(soyuz.stageCount())); ry += 18;
            hudRow(px+8, ry, "Payload mass",
                   fmt(soyuz.getPayloadMass()/1000.0, 2) + " t");
        }

        // ── HUD — panel lateral derecho: etapa seleccionada ──────────────────
        if (selStage >= 0 && selStage < soyuz.stageCount()) {
            const auto &s = soyuz.getStage(selStage);
            int px = 1280 - 280, py = 220, pw = 268, ph = 210;
            hudBox(px, py, pw, ph);
            std::string title = "STAGE " + std::to_string(selStage)
                              + " — " + s.name;
            DrawText(title.c_str(), px+8, py+6, 13, stageColor(selStage, soyuz.stageCount(), false));
            int ry = py + 26;
            hudRow(px+8, ry, "Wet mass",
                   fmt(s.getWetMass()/1000.0,1) + " t"); ry += 18;
            hudRow(px+8, ry, "Dry mass",
                   fmt(s.getDryMass()/1000.0,1) + " t"); ry += 18;
            hudRow(px+8, ry, "Propellant",
                   fmt(s.getPropellantMass()/1000.0,1) + " t"); ry += 18;
            hudRow(px+8, ry, "DV",
                   fmt(soyuz.getStageDeltaV(selStage),0) + " m/s",
                   { 100, 255, 100, 255 }); ry += 18;
            hudRow(px+8, ry, "TWR",
                   fmt(soyuz.getStageTWR(selStage),2)); ry += 18;
            hudRow(px+8, ry, "Isp",
                   fmt(s.getWeightedIsp(),1) + " s"); ry += 18;
            hudRow(px+8, ry, "Thrust",
                   fmt(s.getMaxThrust()/1000.0,1) + " kN"); ry += 18;
            double shift = soyuz.getCoMShiftDuringBurn(selStage);
            std::string shiftStr = (shift >= 0 ? "+" : "") + fmt(shift, 2) + " m";
            hudRow(px+8, ry, "CoM shift",
                   shiftStr, shift > 0 ? Color{255,180,60,255} : Color{100,200,255,255});
        }

        // ── HUD — panel inferior: controles ──────────────────────────────────
        {
            int px = 10, py = 720 - 80, pw = 480, ph = 68;
            hudBox(px, py, pw, ph);
            DrawText("CONTROLES", px+8, py+6, 12, { 120, 200, 255, 200 });
            DrawText("Arrastrar: rotar  |  Scroll: zoom  |  R: reset camara",
                     px+8, py+22, 12, { 200, 200, 200, 220 });
            DrawText("E: exploded view  |  Tab/1/2/3: selec. etapa  |  0: deselec.",
                     px+8, py+38, 12, { 200, 200, 200, 220 });
            DrawText("ESC: salir",
                     px+8, py+54, 12, { 200, 200, 200, 200 });
        }

        // ── HUD — marcadores de CoM y leyenda ────────────────────────────────
        {
            DrawCircle(12, 12, 6, { 255, 60, 60, 220 });
            DrawText("CoM", 22, 6, 13, { 255, 60, 60, 220 });

            int lx = 10, ly = 10;
            for (int i = soyuz.stageCount()-1; i >= 0; --i) {
                Color c = stageColor(i, soyuz.stageCount(), i == selStage);
                DrawRectangle(lx, ly + (soyuz.stageCount()-1-i)*22,
                              16, 16, c);
                std::string lbl = "S" + std::to_string(i) + " " +
                                  soyuz.getStage(i).name.substr(0,10);
                DrawText(lbl.c_str(),
                         lx + 20, ly + (soyuz.stageCount()-1-i)*22 + 2,
                         12, c);
            }
        }

        // ── Modo exploded indicator ───────────────────────────────────────────
        if (exploded)
            DrawText("EXPLODED VIEW [E para salir]", 10, 720 - 100, 13,
                     { 255, 220, 60, 220 });

        // ── FPS ──────────────────────────────────────────────────────────────
        DrawFPS(1280 - 80, 720 - 20);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

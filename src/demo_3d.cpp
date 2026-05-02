/**
 * PROJECT PHOENIX — Demo 3D (Phase 7)
 * ====================================
 * Visualizador 3D orbital usando Raylib 5.
 *
 * Escena:
 *   - Cuerpo central (Tierra KSP-scale) como esfera azul
 *   - Starfield de 2000 estrellas de fondo
 *   - Hasta 3 órbitas simultáneas con sus trayectorias
 *   - Nave Phoenix-1 animada en tiempo real (escala visual ampliada)
 *   - HUD de telemetría sobreimpreso (altitud, velocidad, tiempo)
 *   - Cámara orbital controlable con ratón y teclado
 *
 * Controles:
 *   Ratón botón izq. + arrastrar  — rotar cámara
 *   Scroll                        — zoom
 *   R                             — reset cámara
 *   1 / 2 / 3                     — cambiar velocidad de simulación (×1, ×10, ×100)
 *   ESPACIO                       — pausar / reanudar
 *   ESC                           — salir
 */

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

// Phoenix engine headers (only physics / math, no UI)
#include <physics/Orbit.hpp>
#include <physics/CelestialBody.hpp>
#include <physics/Atmosphere.hpp>
#include <physics/AeroForces.hpp>
#include <math/Constants.hpp>

#include <cmath>
#include <vector>
#include <string>
#include <array>
#include <random>
#include <sstream>
#include <iomanip>

// ─── Aliases ─────────────────────────────────────────────────────────────────

using namespace Phoenix::Math;
using namespace Phoenix::Physics;

// ─── Scale helpers ───────────────────────────────────────────────────────────

// We render at a visual scale independent of KSP physics scale.
// 1 render unit = RENDER_SCALE metres (physics).
// Earth radius in physics = 6371000 * KSP_SCALE = 637100 m
// We want Earth to appear ~1.5 render units.
static constexpr double PHYSICS_EARTH_RADIUS = 6371000.0 * Constants::KSP_SCALE; // 637100 m
static constexpr double RENDER_EARTH_RADIUS  = 1.5;     // render units
static constexpr double PHYSICS_TO_RENDER    = RENDER_EARTH_RADIUS / PHYSICS_EARTH_RADIUS;

static Vector3 toRender(const dvec3 &v) {
    return { (float)(v.x * PHYSICS_TO_RENDER),
             (float)(v.z * PHYSICS_TO_RENDER),  // Z → Y (OpenGL up)
             (float)(v.y * PHYSICS_TO_RENDER) };
}

static float renderRadius(double physicsRadius) {
    return (float)(physicsRadius * PHYSICS_TO_RENDER);
}

// ─── Orbit path cache ─────────────────────────────────────────────────────────

struct OrbitTrail {
    std::vector<Vector3> points;
    Color                color;
    std::string          label;
};

static OrbitTrail buildOrbitTrail(const Orbit &orbit, Color col,
                                  const std::string &label, int samples = 360)
{
    OrbitTrail trail;
    trail.color = col;
    trail.label = label;
    trail.points.reserve(samples + 1);
    double period = orbit.getPeriod();
    for (int i = 0; i <= samples; ++i) {
        double t = (double)i / samples * period;
        dvec3 pos = orbit.getPositionAtTime(t);
        trail.points.push_back(toRender(pos));
    }
    return trail;
}

// ─── Starfield ────────────────────────────────────────────────────────────────

struct Star { Vector3 pos; float brightness; };

static std::vector<Star> generateStars(int count = 2000, float radius = 80.0f)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> angle(0.0f, 6.2832f);
    std::uniform_real_distribution<float> cosTheta(-1.0f, 1.0f);
    std::uniform_real_distribution<float> bright(0.4f, 1.0f);

    std::vector<Star> stars;
    stars.reserve(count);
    for (int i = 0; i < count; ++i) {
        float ct = cosTheta(rng);
        float st = std::sqrt(1.0f - ct * ct);
        float a  = angle(rng);
        stars.push_back({
            { radius * st * std::cos(a), radius * ct, radius * st * std::sin(a) },
            bright(rng)
        });
    }
    return stars;
}

// ─── Atmosphere glow (rings) ──────────────────────────────────────────────────

static void drawAtmosphereGlow(float bodyRadius, float atmRadius)
{
    int rings = 16;
    for (int r = 0; r < rings; ++r) {
        float t    = (float)r / rings;
        float rad  = bodyRadius + (atmRadius - bodyRadius) * t;
        float alpha = (1.0f - t) * 50.0f;     // fade out
        Color col  = { 80, 140, 220, (unsigned char)alpha };
        DrawCircle3D({0, 0, 0}, rad, {1, 0, 0}, 90.0f, col);
        DrawCircle3D({0, 0, 0}, rad, {0, 0, 1}, 0.0f,  col);
        DrawCircle3D({0, 0, 0}, rad, {0, 1, 0}, 0.0f,  col);
    }
}

// ─── HUD helpers ──────────────────────────────────────────────────────────────

static std::string fmtDouble(double v, int prec = 1) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(prec) << v;
    return s.str();
}

static void drawHudBox(int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, { 0, 0, 0, 160 });
    DrawRectangleLines(x, y, w, h, { 60, 160, 255, 200 });
}

static void hudLabel(int x, int y, const char *key, const std::string &val, Color vc = WHITE)
{
    DrawText(key, x, y, 14, { 120, 200, 255, 220 });
    DrawText(val.c_str(), x + 130, y, 14, vc);
}

// ─── Reentry trail ────────────────────────────────────────────────────────────

static std::vector<Vector3> buildReentryTrail(const AeroTrajectoryResult &result)
{
    std::vector<Vector3> pts;
    pts.reserve(result.points.size());
    for (const auto &p : result.points)
        pts.push_back(toRender(p.position));
    return pts;
}

// ─────────────────────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    // ── Window ──────────────────────────────────────────────────────────────
    const int SCREEN_W = 1280;
    const int SCREEN_H = 720;
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "Project Phoenix — 3D Orbital Viewer");
    SetTargetFPS(60);

    // ── Physics setup ────────────────────────────────────────────────────────
    const double earthMu     = Constants::MU_EARTH;
    const double earthRadius = PHYSICS_EARTH_RADIUS;
    const double atmHeight   = 100000.0 * Constants::KSP_SCALE; // 10 km

    // LEO — Phoenix-1
    const double alt_leo = 200000.0;
    const double r_leo   = earthRadius + alt_leo;
    const double v_leo   = std::sqrt(earthMu / r_leo);
    Orbit leo(dvec3(r_leo, 0, 0), dvec3(0, v_leo, 0), earthMu, 0.0);

    // Hohmann transfer orbit (LEO → MEO 400 km)
    const double alt_meo = 400000.0;
    const double r_meo   = earthRadius + alt_meo;
    const double a_hoh   = (r_leo + r_meo) / 2.0;
    const double v_hoh   = std::sqrt(earthMu * (2.0 / r_leo - 1.0 / a_hoh));
    Orbit hohmann(dvec3(r_leo, 0, 0), dvec3(0, v_hoh, 0), earthMu, 0.0);

    // Inclined orbit (ISS-like, 51.6°)
    Orbit inclined(
        earthRadius + 200000.0,  // a  (LEO)
        0.001,                   // e
        glm::radians(51.6),      // i
        glm::radians(30.0),      // Ω
        0.0,                     // ω
        0.0,                     // ν
        earthMu
    );

    // ── Reentry simulation (pre-computed) ────────────────────────────────────
    Atmosphere atm = Atmosphere::makeEarthLike(Constants::KSP_SCALE);
    const double r_apo  = earthRadius + 400000.0;
    const double r_peri = earthRadius + 2000.0;
    const double a_deo  = 0.5 * (r_apo + r_peri);
    const double ecc    = (r_apo - r_peri) / (r_apo + r_peri);
    const double r_ent  = earthRadius + atmHeight;
    const double v_ent  = std::sqrt(earthMu * (2.0 / r_ent - 1.0 / a_deo));
    const double h_ang  = std::sqrt(earthMu * a_deo * (1.0 - ecc * ecc));
    const double v_tang = h_ang / r_ent;
    const double v_rad  = -std::sqrt(std::max(0.0, v_ent * v_ent - v_tang * v_tang));
    dvec3 r0_re(r_ent, 0.0, 0.0);
    dvec3 v0_re(v_rad, v_tang, 0.0);
    auto reentryResult = AeroForces::simulate(
        r0_re, v0_re, 3000.0, 12.0, 1.5, 2.0,
        earthRadius, earthMu, atm, 0.05, 100000, 3);

    // ── Build visual assets ───────────────────────────────────────────────────
    auto stars = generateStars(2500, 80.0f);

    // Orbit trails
    std::array<OrbitTrail, 3> trails = {
        buildOrbitTrail(leo,      { 80, 200, 255, 255 }, "LEO 200km"),
        buildOrbitTrail(hohmann,  { 255, 200, 60, 255  }, "Hohmann"),
        buildOrbitTrail(inclined, { 100, 255, 120, 255 }, "ISS-like 51.6°"),
    };

    std::vector<Vector3> reentryTrail = buildReentryTrail(reentryResult);

    // ── Render scales ─────────────────────────────────────────────────────────
    float bodyR   = renderRadius(earthRadius);
    float atmR    = renderRadius(earthRadius + atmHeight);
    float vesselR = bodyR * 0.025f;  // vessel marker — visually enlarged

    // ── Camera ────────────────────────────────────────────────────────────────
    Camera3D camera = {};
    camera.position   = { 0.0f, 6.0f, 10.0f };
    camera.target     = { 0.0f, 0.0f,  0.0f };
    camera.up         = { 0.0f, 1.0f,  0.0f };
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Camera control state
    float   camYaw    =  45.0f;   // degrees
    float   camPitch  =  25.0f;
    float   camDist   =  10.0f;
    Vector2 lastMouse = { 0, 0 };
    bool    dragging  = false;

    // ── Simulation state ──────────────────────────────────────────────────────
    double simTime    = 0.0;
    double timeScale  = 1.0;
    bool   paused     = false;

    // Toggles
    bool showLeo      = true;
    bool showHohmann  = true;
    bool showInclined = true;
    bool showReentry  = true;
    bool showAtm      = true;
    bool showGrid     = false;

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!WindowShouldClose())
    {
        // ── Input ──────────────────────────────────────────────────────────
        float dt = GetFrameTime();

        // Simulation speed
        if (IsKeyPressed(KEY_ONE))   timeScale = 1.0;
        if (IsKeyPressed(KEY_TWO))   timeScale = 10.0;
        if (IsKeyPressed(KEY_THREE)) timeScale = 100.0;
        if (IsKeyPressed(KEY_FOUR))  timeScale = 500.0;
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;

        // Toggles
        if (IsKeyPressed(KEY_L)) showLeo      = !showLeo;
        if (IsKeyPressed(KEY_H)) showHohmann  = !showHohmann;
        if (IsKeyPressed(KEY_I)) showInclined = !showInclined;
        if (IsKeyPressed(KEY_E)) showReentry  = !showReentry;
        if (IsKeyPressed(KEY_A)) showAtm      = !showAtm;
        if (IsKeyPressed(KEY_G)) showGrid     = !showGrid;

        // Camera reset
        if (IsKeyPressed(KEY_R)) {
            camYaw = 45.0f; camPitch = 25.0f; camDist = 10.0f;
        }

        // Camera zoom (scroll)
        float wheel = GetMouseWheelMove();
        camDist -= wheel * 0.5f;
        camDist = Clamp(camDist, 2.5f, 60.0f);

        // Camera drag
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dragging  = true;
            lastMouse = mouse;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging = false;
        if (dragging) {
            camYaw   -= (mouse.x - lastMouse.x) * 0.4f;
            camPitch -= (mouse.y - lastMouse.y) * 0.4f;
            camPitch  = Clamp(camPitch, -89.0f, 89.0f);
            lastMouse = mouse;
        }

        // Update camera position from spherical coords
        float yawR   = camYaw   * DEG2RAD;
        float pitchR = camPitch * DEG2RAD;
        camera.position = {
            camDist * std::cos(pitchR) * std::sin(yawR),
            camDist * std::sin(pitchR),
            camDist * std::cos(pitchR) * std::cos(yawR)
        };

        // Advance simulation time
        if (!paused)
            simTime += dt * timeScale;

        // Current vessel positions
        dvec3 leoPos  = leo.getPositionAtTime(simTime);
        dvec3 hohPos  = hohmann.getPositionAtTime(simTime);
        dvec3 incPos  = inclined.getPositionAtTime(simTime);

        dvec3 leoVel  = leo.getVelocityAtTime(simTime);

        Vector3 leoR  = toRender(leoPos);
        Vector3 hohR  = toRender(hohPos);
        Vector3 incR  = toRender(incPos);

        double leoAlt = (glm::length(leoPos) - earthRadius) / 1000.0;
        double leoSpd = glm::length(leoVel);

        // ── Draw ───────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground({ 2, 3, 12, 255 });

        BeginMode3D(camera);

            // Stars
            for (const auto &s : stars) {
                unsigned char b = (unsigned char)(s.brightness * 200);
                DrawPoint3D(s.pos, { b, b, b, 255 });
            }

            // Atmosphere glow
            if (showAtm) drawAtmosphereGlow(bodyR, atmR);

            // Earth (body)
            DrawSphere({ 0, 0, 0 }, bodyR, { 20, 60, 140, 255 });
            DrawSphereWires({ 0, 0, 0 }, bodyR, 12, 12, { 40, 80, 180, 120 });

            // Optional reference grid
            if (showGrid)
                DrawGrid(20, renderRadius(earthRadius + 200000.0) / 10.0f);

            // ── Orbit trails ──────────────────────────────────────────────
            auto drawTrail = [](const OrbitTrail &trail, bool show) {
                if (!show || trail.points.size() < 2) return;
                for (size_t i = 0; i + 1 < trail.points.size(); ++i)
                    DrawLine3D(trail.points[i], trail.points[i + 1], trail.color);
            };

            drawTrail(trails[0], showLeo);
            drawTrail(trails[1], showHohmann);
            drawTrail(trails[2], showInclined);

            // ── Reentry trail ──────────────────────────────────────────────
            if (showReentry && reentryTrail.size() > 1) {
                for (size_t i = 0; i + 1 < reentryTrail.size(); ++i) {
                    // Colour by index: from orange (entry) to red (impact)
                    float t  = (float)i / reentryTrail.size();
                    Color col = {
                        255,
                        (unsigned char)(160 * (1.0f - t)),
                        0, 255
                    };
                    DrawLine3D(reentryTrail[i], reentryTrail[i + 1], col);
                }
            }

            // ── Vessel markers ─────────────────────────────────────────────
            if (showLeo) {
                DrawSphere(leoR,  vesselR,         { 80, 220, 255, 255 });
                DrawSphereWires(leoR, vesselR * 1.6f, 6, 6, { 80, 220, 255, 160 });
                // velocity arrow (scaled)
                dvec3 leoVelN = glm::normalize(leoVel);
                Vector3 arrowEnd = {
                    leoR.x + (float)(leoVelN.x * vesselR * 4.0f),
                    leoR.y + (float)(leoVelN.z * vesselR * 4.0f),
                    leoR.z + (float)(leoVelN.y * vesselR * 4.0f)
                };
                DrawLine3D(leoR, arrowEnd, { 255, 255, 80, 220 });
            }

            if (showHohmann)
                DrawSphere(hohR, vesselR * 0.8f, { 255, 200, 60, 255 });

            if (showInclined)
                DrawSphere(incR, vesselR * 0.8f, { 100, 255, 120, 255 });

        EndMode3D();

        // ── HUD ────────────────────────────────────────────────────────────
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        // Title bar
        DrawRectangle(0, 0, sw, 28, { 0, 0, 0, 200 });
        DrawText("PROJECT PHOENIX — 3D Orbital Viewer", 10, 7, 14, { 80, 200, 255, 255 });
        std::string speedStr = paused ? "PAUSE" : ("x" + fmtDouble(timeScale, 0));
        DrawText(speedStr.c_str(), sw - 80, 7, 14, paused ? YELLOW : GREEN);

        // Telemetry panel (bottom-left)
        int px = 10, py = sh - 210;
        drawHudBox(px, py, 280, 200);
        DrawText("PHOENIX-1  (LEO)", px + 8, py + 8, 15, { 80, 200, 255, 255 });
        DrawLine(px + 8, py + 27, px + 272, py + 27, { 60, 120, 200, 200 });

        double T = simTime;
        int hours = (int)(T / 3600) % 24;
        int mins  = (int)(T / 60) % 60;
        int secs  = (int)T % 60;
        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "%02dh %02dm %02ds", hours, mins, secs);
        hudLabel(px + 8, py + 34, "Tiempo MET:", timeBuf);
        hudLabel(px + 8, py + 54, "Altitud:", fmtDouble(leoAlt, 1) + " km");
        hudLabel(px + 8, py + 74, "Velocidad:", fmtDouble(leoSpd, 1) + " m/s");
        hudLabel(px + 8, py + 94, "Período:", fmtDouble(leo.getPeriod() / 60.0, 2) + " min");
        hudLabel(px + 8, py + 114, "e:", fmtDouble(leo.e, 6));
        hudLabel(px + 8, py + 134, "Apo.:", fmtDouble((leo.a * (1 + leo.e) - earthRadius) / 1000.0, 1) + " km");
        hudLabel(px + 8, py + 154, "Peri.:", fmtDouble((leo.a * (1 - leo.e) - earthRadius) / 1000.0, 1) + " km");
        hudLabel(px + 8, py + 174, "Escala x:", fmtDouble(timeScale, 0));

        // Controls panel (top-right)
        int cx = sw - 230, cy = 36;
        drawHudBox(cx, cy, 220, 210);
        DrawText("CONTROLES", cx + 8, cy + 8, 14, { 80, 200, 255, 255 });
        DrawLine(cx + 8, cy + 26, cx + 212, cy + 26, { 60, 120, 200, 200 });
        int ly = cy + 32;
        auto ctrl = [&](const char *k, const char *desc) {
            DrawText(k, cx + 8, ly, 12, { 255, 200, 60, 255 });
            DrawText(desc, cx + 65, ly, 12, { 200, 200, 200, 255 });
            ly += 16;
        };
        ctrl("Drag",    "Rotar camara");
        ctrl("Scroll",  "Zoom");
        ctrl("R",       "Reset camara");
        ctrl("1/2/3/4", "Velocidad sim.");
        ctrl("ESPACIO", "Pausa");
        ctrl("L",       showLeo      ? "[ON]  LEO"     : "[OFF] LEO");
        ctrl("H",       showHohmann  ? "[ON]  Hohmann"  : "[OFF] Hohmann");
        ctrl("I",       showInclined ? "[ON]  ISS-like" : "[OFF] ISS-like");
        ctrl("E",       showReentry  ? "[ON]  Reentrada": "[OFF] Reentrada");
        ctrl("A",       showAtm      ? "[ON]  Atmosfera": "[OFF] Atmosfera");
        ctrl("G",       showGrid     ? "[ON]  Grid"     : "[OFF] Grid");
        ctrl("ESC",     "Salir");

        // Legend (orbit colours) — bottom-right
        int lx = sw - 230, ly2 = sh - 110;
        drawHudBox(lx, ly2, 220, 100);
        DrawText("LEYENDA", lx + 8, ly2 + 8, 14, { 80, 200, 255, 255 });
        DrawLine(lx + 8, ly2 + 26, lx + 212, ly2 + 26, { 60, 120, 200, 200 });
        DrawRectangle(lx + 8, ly2 + 34, 18, 10, { 80, 200, 255, 255 });
        DrawText("LEO 200 km",     lx + 32, ly2 + 32, 13, WHITE);
        DrawRectangle(lx + 8, ly2 + 52, 18, 10, { 255, 200, 60, 255 });
        DrawText("Hohmann ->MEO",  lx + 32, ly2 + 50, 13, WHITE);
        DrawRectangle(lx + 8, ly2 + 70, 18, 10, { 100, 255, 120, 255 });
        DrawText("ISS-like 51.6", lx + 32, ly2 + 68, 13, WHITE);

        // FPS
        DrawFPS(sw - 70, sh - 22);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#pragma once

namespace Phoenix::Launch {

/**
 * @struct GuidanceLaw
 * @brief Pitch program, Max-Q limit and orbit-insertion target for AscentIntegrator.
 *
 * Pitch model (altitude-based — more robust than time-based):
 *   alt < hKick           — radial (vertical)
 *   hKick ≤ alt < hTurn   — smoothstep pitch 0° → 45° off-vertical (toward east)
 *   alt ≥ hTurn           — prograde tracking (gravity turn continues naturally)
 *
 * Max-Q throttle:
 *   When dynamic pressure q = ½ρv² > maxQLimit (Pa), throttle is clamped
 *   to maxQThrottle fraction (overrides the integrator's global throttle).
 *
 * Orbit insertion:
 *   When targetApoapsis > 0 and the instantaneous apoapsis ≥ targetApoapsis
 *   the engine is cut.  The integrator then coasts until apoapsis is reached
 *   (or maxTime expires) and records the final orbital state.
 */
struct GuidanceLaw {
    // ── Pitch program ─────────────────────────────────────────────────────────
    double hKick  = 200.0;    ///< m — altitude at which pitch-over begins
    double hTurn  = 6000.0;   ///< m — altitude at which pitch reaches 45° (prograde tracking begins)

    // ── Max-Q throttle limit ─────────────────────────────────────────────────
    double maxQLimit    = 35000.0; ///< Pa — dynamic pressure threshold
    double maxQThrottle = 0.75;    ///< throttle fraction applied when q > maxQLimit

    // ── Orbit insertion ──────────────────────────────────────────────────────
    double targetApoapsis = 0.0;   ///< m above surface; 0 = burn until empty
};

} // namespace Phoenix::Launch

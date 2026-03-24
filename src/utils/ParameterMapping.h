#pragma once
// ParameterMapping.h   Pure psychoacoustic mapping functions
// distanceNorm d in [0,1] -> all DSP parameter values.
// All functions: [[nodiscard]], noexcept, safe on audio thread.
//
// CUE 1: Gain    inverse-square law (ISO free-field model)
// CUE 2: HF Shelf air absorption (ISO 9613-1, hyper-real)
// CUE 3: Mid Tilt Fletcher-Munson equal-loudness compensation
// CUE 4: Reverb Wet DRR model (Zahorik 2002), sqrt curve
// CUE 5: Pre-delay  geometric room reflection model

#include <cmath>
#include <algorithm>

namespace DistanceMapping {
    inline constexpr float R_REF=1.f, R_MAX=20.f;
    inline constexpr float WET_MAX=0.45f, PRE_MAX_MS=30.f;
    inline constexpr float SHELF_DB=-14.f, TILT_DB=2.5f;
    inline constexpr float SHELF_FMAX=8000.f, SHELF_FMIN=2500.f;

    // CUE 1: -20*log10(1 + d*19)  [0 dB -> -26.0 dB at d=1]
    [[nodiscard]] inline float gainDb(float d) noexcept {
        d=std::clamp(d,0.f,1.f);
        return -20.f*std::log10(1.f+d*(R_MAX/R_REF-1.f)); }

    // CUE 2a: Linear shelf gain  [0 -> -14 dB]
    [[nodiscard]] inline float hfShelfGainDb(float d) noexcept {
        return SHELF_DB*std::clamp(d,0.f,1.f); }

    // CUE 2b: Log-interp shelf freq  [8kHz -> 2.5kHz]
    [[nodiscard]] inline float hfShelfFreqHz(float d) noexcept {
        return SHELF_FMAX*std::pow(SHELF_FMIN/SHELF_FMAX,std::clamp(d,0.f,1.f)); }

    // CUE 3: +2.5dB @ 300Hz at d=1 (Fletcher-Munson compensation)
    [[nodiscard]] inline float midTiltDb(float d) noexcept {
        return TILT_DB*std::clamp(d,0.f,1.f); }

    // CUE 4: sqrt curve for perceptual DRR linearity  [0 -> 45%]
    [[nodiscard]] inline float reverbWet(float d) noexcept {
        return WET_MAX*std::sqrt(std::clamp(d,0.f,1.f)); }

    // CUE 5: Linear pre-delay  [0 -> 30ms]
    [[nodiscard]] inline float preDelayMs(float d) noexcept {
        return PRE_MAX_MS*std::clamp(d,0.f,1.f); }

    // FDN room size scale  [0.20 -> 0.85]
    [[nodiscard]] inline float roomSize(float d) noexcept {
        return 0.20f+0.65f*std::clamp(d,0.f,1.f); }

    // HRTF: sigmoid S-curve centred at d=0.5
    [[nodiscard]] inline float hrtfWet(float d) noexcept {
        d=std::clamp(d,0.f,1.f);
        return 1.f/(1.f+std::exp(-10.f*(d-0.5f))); }

    // FDN damping coefficient  [0.30 -> 0.85]
    [[nodiscard]] inline float reverbDamping(float d) noexcept {
        return 0.30f+0.55f*std::clamp(d,0.f,1.f); }

    struct Mapped {
        float gainDb,hfShelfGainDb,hfShelfFreqHz;
        float midTiltDb,reverbWet,preDelayMs;
        float roomSize,hrtfWet,reverbDamping; };

    [[nodiscard]] inline Mapped compute(float d) noexcept {
        return { gainDb(d),hfShelfGainDb(d),hfShelfFreqHz(d),
                 midTiltDb(d),reverbWet(d),preDelayMs(d),
                 roomSize(d),hrtfWet(d),reverbDamping(d) }; }
} // namespace DistanceMapping

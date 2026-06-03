#pragma once
// DistanceChain.h  Master DSP orchestrator.
//
// Signal flow:
//   Gain -> Spectral -> direct
//      |-- direct ---------> [optional HRTF: mono->L/R ear FIR] --> directOut
//      \-- direct -> Reverb (stereo, NOT binauralised) ---------> wet
//   out = sqrt(1-w)*directOut + sqrt(w)*wet      (constant-power crossfade)
//
// The HRTF (optional headphone externalisation) is applied to the DIRECT path
// only. The reverberant field stays stereo/non-directional and is summed in
// afterwards, so the room is never collapsed onto the direct-sound direction.
//
// Each mapped parameter has its own SmoothedValue (10ms ramp).
// getNextValue() called exactly once per block (block-rate), except the
// dry/wet crossfade which is smoothed per-sample.

#include "GainStage.h"
#include "SpectralStage.h"
#include "ReverbStage.h"
#include "HrtfStage.h"
#include "../utils/ParameterMapping.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

class DistanceChain {
public:
    static constexpr double SMOOTH=0.010;
    void prepare(const juce::dsp::ProcessSpec&) noexcept;
    void process(juce::dsp::AudioBlock<float>&,float d,bool hrtf,float roomScale,
                 bool bypEQ=false,bool bypRev=false) noexcept;
    void reset() noexcept;
    HrtfStage& getHrtfStage() noexcept { return hrtf; }
private:
    GainStage gain; SpectralStage spectral;
    ReverbStage reverb; HrtfStage hrtf;
    double sr{44100.};
    int blockSize{512};
    using SV=juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear>;
    SV sGain{0.f},sSG{0.f},sSF{8000.f},sTilt{0.f};
    SV sWet{0.f},sPre{0.f},sRoom{0.3f},sDamp{0.3f};
    SV sCfWet{0.f};                       // per-sample dry/wet crossfade smoother
    juce::AudioBuffer<float> wetScratch;  // pre-allocated reverb-only buffer
    JUCE_LEAK_DETECTOR(DistanceChain)
};

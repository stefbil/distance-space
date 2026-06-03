#pragma once
// ReverbStage.h -- Reverb using JUCE's built-in Freeverb algorithm.
// Pre-delay ring buffer (max 50ms) + Freeverb processing.
//
// This stage produces the REVERB-ONLY (wet) signal. It does NOT mix dry/wet.
// The direct/wet crossfade is performed by DistanceChain AFTER the optional
// HRTF stage, so that the (stereo, non-directional) reverberant field is never
// collapsed onto the single direction of the binauralised direct path.
//
// Real-time safety: the wet output is written into a caller-provided buffer
// (pre-allocated by DistanceChain). No allocation occurs on the audio thread.

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

class ReverbStage {
public:
    static constexpr int MAX_PRE=9600;
    static constexpr double RAMP=0.010;

    void prepare(const juce::dsp::ProcessSpec&) noexcept;

    // Renders the wet (reverb-only) signal of `input` into `wetOut`.
    // `input` is left untouched. `wetOut` must have >= input's channel count
    // and >= input's sample count. No dry/wet mixing happens here.
    void processWet(const juce::dsp::AudioBlock<float>& input,
                    juce::dsp::AudioBlock<float>& wetOut,
                    float preMs,float roomScale,float damp) noexcept;
    void reset() noexcept;

private:
    double sampleRate{44100.};
    int numChannels{2};

    // Pre-delay ring buffer
    std::vector<float> preBufL, preBufR;
    int preW{0};

    juce::Reverb reverb;
    juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear> sPre;

    JUCE_LEAK_DETECTOR(ReverbStage)
};

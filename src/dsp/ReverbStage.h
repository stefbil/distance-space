#pragma once
// ReverbStage.h -- Reverb using JUCE's built-in Freeverb algorithm.
// Pre-delay ring buffer (max 50ms) + Freeverb processing.
// Constant-power sqrt wet/dry crossfade.

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

class ReverbStage {
public:
    static constexpr int MAX_PRE=9600;
    static constexpr double RAMP=0.010;

    void prepare(const juce::dsp::ProcessSpec&) noexcept;
    void process(juce::dsp::AudioBlock<float>&,float wet,float preMs,
                 float roomScale,float damp) noexcept;
    void reset() noexcept;

private:
    double sampleRate{44100.};
    int numChannels{2};

    // Pre-delay ring buffer
    std::vector<float> preBufL, preBufR;
    int preW{0};

    juce::Reverb reverb;
    juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear> sWet,sPre;

    JUCE_LEAK_DETECTOR(ReverbStage)
};

#pragma once
// GainStage.h   Inverse-square-law gain with 10ms ramp.
// juce::dsp::Gain handles dB->linear conversion + AVX2 SIMD multiply.
#include <juce_dsp/juce_dsp.h>
class GainStage {
public:
    static constexpr double RAMP=0.010;
    void prepare(const juce::dsp::ProcessSpec& s) noexcept
    { gain.prepare(s); gain.setRampDurationSeconds(RAMP); gain.setGainDecibels(0.f); }
    void setGainDb(float dB) noexcept { gain.setGainDecibels(dB); }
    void process(juce::dsp::AudioBlock<float>& b) noexcept
    { auto ctx=juce::dsp::ProcessContextReplacing<float>(b); gain.process(ctx); }
    void reset() noexcept { gain.reset(); }
private:
    juce::dsp::Gain<float> gain;
    JUCE_LEAK_DETECTOR(GainStage)
};

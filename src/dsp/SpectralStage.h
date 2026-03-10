#pragma once
// SpectralStage.h
// Filter 1: High shelf  -- air absorption. Butterworth (Q=0.707).
//   Gain: 0 -> -14 dB.  Freq: 8kHz -> 2.5kHz.
// Filter 2: Mid peak    -- Fletcher-Munson compensation.
//   Freq: 300 Hz. Q=1.0. Gain: 0 -> +2.5 dB.
// Coefficient updates throttled: skip if params unchanged > threshold.
// Per-channel filter instances (avoid shared-state aliasing).

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

class SpectralStage {
public:
    static constexpr float FT=2.f, GT=0.05f;   // Hz / dB thresholds
    static constexpr float MF=300.f, MQ=1.f;   // mid-peak constants
    static constexpr int   MC=2;

    void prepare(const juce::dsp::ProcessSpec& s) noexcept {
        sr=s.sampleRate;
        nc=static_cast<int>(juce::jmin((juce::uint32)MC,s.numChannels));
        for(int c=0;c<nc;++c){sh[c].prepare(s);pk[c].prepare(s);}
        lF=-1.f; updateCoefficients(8000.f,0.f,0.f); }

    void updateCoefficients(float sHz,float sDb,float tDb) noexcept {
        if(std::abs(sHz-lF)<=FT && std::abs(sDb-lSG)<=GT &&
           std::abs(tDb-lTG)<=GT) return;
        lF=sHz; lSG=sDb; lTG=tDb;
        using CF=juce::dsp::IIR::Coefficients<float>;
        // JUCE filter factories expect LINEAR gain, not dB
        const double shelfGain=juce::Decibels::decibelsToGain((double)sDb);
        const double peakGain =juce::Decibels::decibelsToGain((double)tDb);
        auto sc=CF::makeHighShelf(sr,(double)sHz,0.707,shelfGain);
        auto pc=CF::makePeakFilter(sr,(double)MF,(double)MQ,peakGain);
        for(int c=0;c<nc;++c){*sh[c].coefficients=*sc;*pk[c].coefficients=*pc;} }

    void process(juce::dsp::AudioBlock<float>& b) noexcept {
        for(int c=0;c<nc;++c){
            auto s=b.getSingleChannelBlock((size_t)c);
            auto ctx=juce::dsp::ProcessContextReplacing<float>(s);
            sh[c].process(ctx); pk[c].process(ctx);} }

    void reset() noexcept
    { for(int c=0;c<nc;++c){sh[c].reset();pk[c].reset();} }

private:
    std::array<juce::dsp::IIR::Filter<float>,MC> sh,pk;
    double sr{44100.}; int nc{2};
    float lF{-1.f},lSG{0.f},lTG{0.f};
    JUCE_LEAK_DETECTOR(SpectralStage)
};

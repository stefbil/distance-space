#pragma once
// HrtfStage.h  --  HRTF binaural convolution (Phase 2 feature)
// Thread safety:
//   loadImpulseResponse() -- MESSAGE thread only. Sets isLoaded with RELEASE.
//   process()             -- AUDIO thread only. Checks isLoaded with ACQUIRE.
//   scratch pre-allocated in prepare(). NEVER setSize() in process().

#include <juce_dsp/juce_dsp.h>
#include <atomic>

class HrtfStage {
public:
    static constexpr double XF=0.050; // 50ms crossfade
    static constexpr int    PS=512;   // partition size

    void prepare(const juce::dsp::ProcessSpec& s) noexcept {
        sr=s.sampleRate;
        convL.prepare(s); convR.prepare(s);
        sm.reset(sr,XF); sm.setCurrentAndTargetValue(0.f);
        // Allocate scratch ONCE here. Never in process().
        scratch.setSize(2,(int)s.maximumBlockSize,false,true,false); }

    // Message thread only loads separate left/right ear IRs from binary data
    void loadImpulseResponse(const void* irL, size_t irLSize,
                             const void* irR, size_t irRSize) {
        convL.loadImpulseResponse(irL,irLSize,
                                  juce::dsp::Convolution::Stereo::no,
                                  juce::dsp::Convolution::Trim::yes,PS);
        convR.loadImpulseResponse(irR,irRSize,
                                  juce::dsp::Convolution::Stereo::no,
                                  juce::dsp::Convolution::Trim::yes,PS);
        isLoaded.store(true,std::memory_order_release); }

    // Audio thread only. Uses pre-allocated scratch (no allocation).
    //
    // Routing: the stereo input is treated as a single source. It is downmixed
    // to mono and convolved with the LEFT-ear and RIGHT-ear IRs separately, so
    // both ears receive the same source filtered by its own ear transfer
    // function. This is the correct single-source binaural rendering, NOT a
    // diagonal L->L / R->R pass. `wet` controls the externalisation amount via
    // a constant-power blend with the original (un-binauralised) direct signal.
    void process(juce::dsp::AudioBlock<float>& block,float wet) noexcept {
        if(!isLoaded.load(std::memory_order_acquire)) return;
        sm.setTargetValue(wet);
        const float w=sm.getNextValue(); if(w<1e-4f) return;
        const int n=(int)block.getNumSamples();
        jassert(n<=scratch.getNumSamples());

        // Mono downmix of the source into BOTH scratch channels.
        auto* s0=scratch.getWritePointer(0);
        auto* s1=scratch.getWritePointer(1);
        const auto* inL=block.getChannelPointer(0);
        const auto* inR=block.getChannelPointer(1);
        for(int i=0;i<n;++i){ const float mono=0.5f*(inL[i]+inR[i]); s0[i]=mono; s1[i]=mono; }

        // Convolve the mono source with each ear's IR independently.
        auto hb=juce::dsp::AudioBlock<float>(scratch).getSubBlock(0,(size_t)n);
        auto ch0=hb.getSingleChannelBlock(0);
        auto ch1=hb.getSingleChannelBlock(1);
        {auto ctx=juce::dsp::ProcessContextReplacing<float>(ch0);
         convL.process(ctx);}
        {auto ctx=juce::dsp::ProcessContextReplacing<float>(ch1);
         convR.process(ctx);}

        // Constant-power blend: (1-w) original direct + w binaural source.
        const float dG=std::sqrt(1.f-w), wG=std::sqrt(w);
        auto* dL=block.getChannelPointer(0);
        auto* dR=block.getChannelPointer(1);
        const auto* bL=scratch.getReadPointer(0);
        const auto* bR=scratch.getReadPointer(1);
        for(int i=0;i<n;++i){ dL[i]=dL[i]*dG+bL[i]*wG; dR[i]=dR[i]*dG+bR[i]*wG; } }

    void reset() noexcept
    { convL.reset(); convR.reset(); sm.setCurrentAndTargetValue(0.f); }

private:
    juce::dsp::Convolution     convL,convR;
    juce::SmoothedValue<float> sm{0.f};
    std::atomic<bool>          isLoaded{false};
    juce::AudioBuffer<float>   scratch;
    double                     sr{44100.};
    JUCE_LEAK_DETECTOR(HrtfStage)
};

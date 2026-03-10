#include "ReverbStage.h"

void ReverbStage::prepare(const juce::dsp::ProcessSpec& s) noexcept {
    sampleRate=s.sampleRate;
    numChannels=juce::jmin((int)s.numChannels,2);
    preBufL.assign(MAX_PRE,0.f);
    preBufR.assign(MAX_PRE,0.f);
    preW=0;
    reverb.setSampleRate(s.sampleRate);
    reverb.reset();
    sWet.reset(sampleRate,RAMP); sPre.reset(sampleRate,RAMP);
    sWet.setCurrentAndTargetValue(0.f); sPre.setCurrentAndTargetValue(0.f);
}

void ReverbStage::reset() noexcept {
    std::fill(preBufL.begin(),preBufL.end(),0.f);
    std::fill(preBufR.begin(),preBufR.end(),0.f);
    preW=0;
    reverb.reset();
    sWet.setCurrentAndTargetValue(0.f); sPre.setCurrentAndTargetValue(0.f);
}

void ReverbStage::process(juce::dsp::AudioBlock<float>& block,
    float wet,float preMs,float roomScale,float damp) noexcept
{
    sWet.setTargetValue(wet); sPre.setTargetValue(preMs);
    const int ns=(int)block.getNumSamples();
    const int nc=juce::jmin((int)block.getNumChannels(),2);

    // Update Freeverb parameters wetLevel/dryLevel set to 1/0 to
    // handle crossfade for constant-power behaviour.
    juce::Reverb::Parameters params;
    params.roomSize  =juce::jlimit(0.f,1.f,roomScale);
    params.damping   =juce::jlimit(0.f,1.f,damp);
    params.wetLevel  =1.f;
    params.dryLevel  =0.f;
    params.width     =1.f;
    params.freezeMode=0.f;
    reverb.setParameters(params);

    // Allocate scratch for the reverb-only (wet) signal
    juce::AudioBuffer<float> wetBuf(nc,ns);
    wetBuf.clear();

    // Fill wetBuf with pre-delayed input
    for(int s = 0; s < ns; ++s)
    {
        // Fractional delay in samples
        const float delaySamples = sPre.getNextValue() * 0.001f * (float)sampleRate;

        // integer part, clamped so ps2 = ps1+1 never exceeds buffer bounds
        const int   ps1  = juce::jlimit(0, MAX_PRE - 2, (int)delaySamples);
        const float frac = delaySamples - (float)ps1;
        const int   ps2  = ps1 + 1;

        // Read left channel (interpolated) — read BEFORE writing to ring buffer
        const float rL1 = preBufL[(preW - ps1 + MAX_PRE) % MAX_PRE];
        const float rL2 = preBufL[(preW - ps2 + MAX_PRE) % MAX_PRE];
        wetBuf.setSample(0, s, rL1 + frac * (rL2 - rL1));

        // Read right channel (interpolated)
        if(nc >= 2)
        {
            const float rR1 = preBufR[(preW - ps1 + MAX_PRE) % MAX_PRE];
            const float rR2 = preBufR[(preW - ps2 + MAX_PRE) % MAX_PRE];
            wetBuf.setSample(1, s, rR1 + frac * (rR2 - rR1));
        }

        // Write current input into ring buffer (AFTER reads)
        preBufL[preW] = block.getSample(0, s);
        if(nc >= 2)
            preBufR[preW] = block.getSample(1, s);

        // Advance write pointer
        if(++preW >= MAX_PRE) preW = 0;
    }

    // Run Freeverb on pre-delayed signal (100% wet internally)
    if(nc>=2)
        reverb.processStereo(wetBuf.getWritePointer(0),
                             wetBuf.getWritePointer(1),ns);
    else
        reverb.processMono(wetBuf.getWritePointer(0),ns);

    // Constant-power crossfade: dry*sqrt(1-w) + wet*sqrt(w)
    for(int s=0;s<ns;++s){
        const float w=juce::jlimit(0.f,1.f,sWet.getNextValue());
        const float dG=std::sqrt(1.f-w), wG=std::sqrt(w);
        for(int c=0;c<nc;++c){
            const float dry=block.getSample(c,s);
            const float rev=wetBuf.getSample(c,s);
            block.setSample(c,s,dry*dG+rev*wG);
        }
    }
}

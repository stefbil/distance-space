#include "ReverbStage.h"

void ReverbStage::prepare(const juce::dsp::ProcessSpec& s) noexcept {
    sampleRate=s.sampleRate;
    numChannels=juce::jmin((int)s.numChannels,2);
    preBufL.assign(MAX_PRE,0.f);
    preBufR.assign(MAX_PRE,0.f);
    preW=0;
    reverb.setSampleRate(s.sampleRate);
    reverb.reset();
    sPre.reset(sampleRate,RAMP);
    sPre.setCurrentAndTargetValue(0.f);
}

void ReverbStage::reset() noexcept {
    std::fill(preBufL.begin(),preBufL.end(),0.f);
    std::fill(preBufR.begin(),preBufR.end(),0.f);
    preW=0;
    reverb.reset();
    sPre.setCurrentAndTargetValue(0.f);
}

void ReverbStage::processWet(const juce::dsp::AudioBlock<float>& input,
    juce::dsp::AudioBlock<float>& wetOut,
    float preMs,float roomScale,float damp) noexcept
{
    sPre.setTargetValue(preMs);
    const int ns=(int)input.getNumSamples();
    const int nc=juce::jmin((int)input.getNumChannels(),2);

    // Freeverb runs 100% wet; the dry/wet crossfade is done by DistanceChain.
    juce::Reverb::Parameters params;
    params.roomSize  =juce::jlimit(0.f,1.f,roomScale);
    params.damping   =juce::jlimit(0.f,1.f,damp);
    params.wetLevel  =1.f;
    params.dryLevel  =0.f;
    params.width     =1.f;
    params.freezeMode=0.f;
    reverb.setParameters(params);

    // Fill wetOut with the pre-delayed input (fractional ring-buffer read).
    for(int s = 0; s < ns; ++s)
    {
        const float delaySamples = sPre.getNextValue() * 0.001f * (float)sampleRate;

        // integer part, clamped so ps2 = ps1+1 never exceeds buffer bounds
        const int   ps1  = juce::jlimit(0, MAX_PRE - 2, (int)delaySamples);
        const float frac = delaySamples - (float)ps1;
        const int   ps2  = ps1 + 1;

        // Read left channel (interpolated) — read BEFORE writing to ring buffer
        const float rL1 = preBufL[(preW - ps1 + MAX_PRE) % MAX_PRE];
        const float rL2 = preBufL[(preW - ps2 + MAX_PRE) % MAX_PRE];
        wetOut.setSample(0, s, rL1 + frac * (rL2 - rL1));

        // Read right channel (interpolated)
        if(nc >= 2)
        {
            const float rR1 = preBufR[(preW - ps1 + MAX_PRE) % MAX_PRE];
            const float rR2 = preBufR[(preW - ps2 + MAX_PRE) % MAX_PRE];
            wetOut.setSample(1, s, rR1 + frac * (rR2 - rR1));
        }

        // Write current input into ring buffer (AFTER reads)
        preBufL[preW] = input.getSample(0, s);
        if(nc >= 2)
            preBufR[preW] = input.getSample(1, s);

        // Advance write pointer
        if(++preW >= MAX_PRE) preW = 0;
    }

    // Run Freeverb on the pre-delayed signal (100% wet, written in place).
    if(nc>=2)
        reverb.processStereo(wetOut.getChannelPointer(0),
                             wetOut.getChannelPointer(1),ns);
    else
        reverb.processMono(wetOut.getChannelPointer(0),ns);
}

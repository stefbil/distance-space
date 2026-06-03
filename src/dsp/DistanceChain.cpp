#include "DistanceChain.h"
void DistanceChain::prepare(const juce::dsp::ProcessSpec& s) noexcept {
    sr=s.sampleRate;
    blockSize = (int)s.maximumBlockSize;
    gain.prepare(s); spectral.prepare(s); reverb.prepare(s); hrtf.prepare(s);
    auto init=[&](SV& sv,float v){sv.reset(sr,SMOOTH);sv.setCurrentAndTargetValue(v);};
    init(sGain,0.f); init(sSG,0.f); init(sSF,8000.f); init(sTilt,0.f);
    init(sWet,0.f);  init(sPre,0.f); init(sRoom,0.3f); init(sDamp,0.3f);
    init(sCfWet,0.f);
    // Pre-allocate reverb-only scratch ONCE (never on the audio thread).
    wetScratch.setSize(2,blockSize,false,true,false);
    // Reset all stages after prepare to ensure clean state
    gain.reset(); spectral.reset(); reverb.reset(); hrtf.reset(); }

void DistanceChain::process(juce::dsp::AudioBlock<float>& block,
    float d,bool hrtfOn,float roomScale,bool bypEQ,bool bypRev) noexcept
{
    const auto m=DistanceMapping::compute(d);
    sGain.setTargetValue(m.gainDb);      sSG.setTargetValue(m.hfShelfGainDb);
    sSF  .setTargetValue(m.hfShelfFreqHz);sTilt.setTargetValue(m.midTiltDb);
    sWet .setTargetValue(m.reverbWet);   sPre.setTargetValue(m.preDelayMs);
    sRoom.setTargetValue(m.roomSize*roomScale); sDamp.setTargetValue(m.reverbDamping);
    // Advance all smoothers ONE step per block
    const int   bs   = (int)block.getNumSamples();
    const float gDb  = sGain.skip(bs);
    const float sg   = sSG  .skip(bs);
    const float sf   = sSF  .skip(bs);
    const float tilt = sTilt .skip(bs);
    const float wet  = sWet .skip(bs);
    const float pre  = sPre .skip(bs);
    const float rm   = sRoom.skip(bs);
    const float dmp  = sDamp.skip(bs);
    gain.setGainDb(gDb);
    gain.process(block);
    if(!bypEQ) {
        spectral.updateCoefficients(sf,sg,tilt);
        spectral.process(block);
    }

    // `block` now holds the DIRECT (post gain + EQ) signal.
    const int n  = (int)block.getNumSamples();
    const int nc = juce::jmin((int)block.getNumChannels(),2);

    if(!bypRev) {
        // Render reverb-only from the (pre-HRTF) direct signal into wetScratch.
        // The reverb is rendered ONCE and kept stereo/non-directional.
        jassert(n<=wetScratch.getNumSamples());
        auto wetBlock=juce::dsp::AudioBlock<float>(wetScratch).getSubBlock(0,(size_t)n);
        reverb.processWet(block,wetBlock,pre,rm,dmp);
    }

    // Binauralise the DIRECT path only (optional headphone externalisation).
    if(hrtfOn) hrtf.process(block,m.hrtfWet);

    if(!bypRev) {
        // Constant-power crossfade: directOut*sqrt(1-w) + wet*sqrt(w).
        sCfWet.setTargetValue(wet);
        for(int s=0;s<n;++s){
            const float w  = juce::jlimit(0.f,1.f,sCfWet.getNextValue());
            const float dG = std::sqrt(1.f-w), wG = std::sqrt(w);
            for(int c=0;c<nc;++c){
                const float dry = block.getSample(c,s);
                const float rev = wetScratch.getSample(c,s);
                block.setSample(c,s,dry*dG+rev*wG);
            }
        }
    }
}

void DistanceChain::reset() noexcept
{ gain.reset(); spectral.reset(); reverb.reset(); hrtf.reset();
  sCfWet.setCurrentAndTargetValue(0.f); }

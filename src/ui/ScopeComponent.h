#pragma once
// ScopeComponent.h Oscilloscope. Reads AudioBufferFifo at 30Hz.
// memmove ring: O(N) shift, no heap allocation in timerCallback.
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../utils/AudioBufferFifo.h"
#include "../utils/ParameterMapping.h"
#include <atomic>
#include <cstring>
class ScopeComponent : public juce::Component, public juce::Timer {
public:
    static constexpr int N=2048;
    explicit ScopeComponent(AudioBufferFifo<2,4096>& f):af(f)
    {db.setSize(2,N);db.clear();startTimerHz(30);}
    ~ScopeComponent() override { stopTimer(); }
    void setDistance(float d) noexcept { dn.store(d,std::memory_order_relaxed); }
    void timerCallback() override {
        const int av=af.getNumReady();if(!av)return;
        const int dr=juce::jmin(av,N);
        juce::AudioBuffer<float> tmp(2,dr);
        af.pull(tmp,dr);
        const int keep=N-dr;
        for(int c=0;c<2;++c){auto* d=db.getWritePointer(c);
            if(keep>0)std::memmove(d,d+dr,(size_t)keep*sizeof(float));
            std::memcpy(d+keep,tmp.getReadPointer(c),(size_t)dr*sizeof(float));}
        repaint();}
    void paint(juce::Graphics& g) override {
        const auto b=getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xFF0D1020));g.fillRoundedRectangle(b,4.f);
        g.setColour(juce::Colour(0x10FFFFFF));
        for(int i=1;i<4;++i)g.drawHorizontalLine((int)(b.getY()+b.getHeight()*i/4.f),b.getX(),b.getRight());
        g.setColour(juce::Colour(0x22FFFFFF));g.drawHorizontalLine((int)b.getCentreY(),b.getX(),b.getRight());
        const float d=dn.load(std::memory_order_relaxed);
        const float gn=std::pow(10.f,DistanceMapping::gainDb(d)/20.f);
        const float wt=DistanceMapping::reverbWet(d)/0.45f;
        const uint8_t gr=(uint8_t)(160+(int)(60.f*(1.f-d)));
        wave(g,0,b,gn,0,juce::Colour::fromRGBA(255u,gr,20u,(uint8_t)(200-(int)(60*d))));
        if(wt>0.01f){const int off=(int)(DistanceMapping::preDelayMs(d)*.001f*44100.f*N/2048.f);
            wave(g,1,b,gn*wt*.6f,off,juce::Colour::fromRGBA(80u,160u,255u,(uint8_t)(wt*160.f)));}}
private:
    AudioBufferFifo<2,4096>& af;
    juce::AudioBuffer<float> db;
    std::atomic<float> dn{0.f};
    void wave(juce::Graphics& g,int ch,const juce::Rectangle<float>& b,
              float sc,int off,juce::Colour col) {
        if(db.getNumSamples()<N)return;
        const float* d=db.getReadPointer(juce::jlimit(0,db.getNumChannels()-1,ch));
        const float cy=b.getCentreY(),amp=b.getHeight()*.44f*sc,dx=b.getWidth()/float(N);
        juce::Path p;
        for(int i=0;i<N;++i){const int s=juce::jlimit(0,N-1,i-off);
            const float px=b.getX()+i*dx,py=cy-d[s]*amp;
            i==0?p.startNewSubPath(px,py):p.lineTo(px,py);}
        g.setColour(col);
        g.strokePath(p,juce::PathStrokeType(1.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
    JUCE_LEAK_DETECTOR(ScopeComponent)
};

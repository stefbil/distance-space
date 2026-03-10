#pragma once
// CueActivityComponent.h 5 psychoacoustic cue activity bars.
// CUES defined in .cpp (juce::Colour is not constexpr-constructible).
#include <juce_gui_basics/juce_gui_basics.h>
#include "../utils/ParameterMapping.h"
#include <array>
#include <cmath>
class CueActivityComponent : public juce::Component {
public:
    struct CueDef { const char* name; juce::Colour colour; };
    static const std::array<CueDef,5> CUES; // defined in .cpp
    void setParameters(float d,bool h) noexcept
    {dn.store(d,std::memory_order_relaxed);ht.store(h,std::memory_order_relaxed);}
    void tick() noexcept { repaint(); }
    void paint(juce::Graphics& g) override {
        const float d=dn.load(std::memory_order_relaxed);
        const bool  h=ht.load(std::memory_order_relaxed);
        const std::array<float,5> v{
            std::pow(10.f,DistanceMapping::gainDb(d)/20.f),
            1.f-DistanceMapping::hfShelfFreqHz(d)/8000.f,
            DistanceMapping::reverbWet(d)/0.45f,
            DistanceMapping::preDelayMs(d)/30.f,
            h?DistanceMapping::hrtfWet(d):0.f};
        const auto b=getLocalBounds().toFloat();
        const float rH=b.getHeight()/(float)CUES.size(),lW=78.f,vW=34.f;
        const float bX=b.getX()+lW,bW=b.getWidth()-lW-vW-8.f;
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),9.f,juce::Font::plain));
        for(size_t i=0;i<CUES.size();++i){
            const float y=b.getY()+(float)i*rH,cy=y+rH*.5f;
            const float val=juce::jlimit(0.f,1.f,v[i]);
            g.setColour(juce::Colour(0x77FFFFFF));
            g.drawText(CUES[i].name,(int)b.getX(),(int)(cy-6),(int)(lW-4),13,juce::Justification::centredRight);
            g.setColour(juce::Colour(0x14FFFFFF));g.fillRoundedRectangle(bX,cy-2.5f,bW,5.f,2.5f);
            if(val>0.005f){g.setColour(CUES[i].colour);g.fillRoundedRectangle(bX,cy-2.5f,bW*val,5.f,2.5f);}
            g.setColour(juce::Colour(0xAAFFFFFF));
            g.drawText(juce::String((int)(val*100))+"%",(int)(bX+bW+4),(int)(cy-6),32,13,juce::Justification::centredLeft);}}
private:
    std::atomic<float> dn{0.f}; std::atomic<bool> ht{false};
    JUCE_LEAK_DETECTOR(CueActivityComponent)
};

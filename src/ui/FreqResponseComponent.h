#pragma once
// FreqResponseComponent.h EQ curve display at 30Hz.
// juce::Path cached; rebuilt only when |d - lastD| > 0.002.
#include <juce_gui_basics/juce_gui_basics.h>
#include "../utils/ParameterMapping.h"
#include <atomic>
#include <cmath>
class FreqResponseComponent : public juce::Component, public juce::Timer {
public:
    FreqResponseComponent()  { startTimerHz(30); }
    ~FreqResponseComponent() override { stopTimer(); }
    void setDistance(float d) noexcept { dn.store(d,std::memory_order_relaxed); }
    void timerCallback() override {
        const float d=dn.load(std::memory_order_relaxed);
        if(std::abs(d-lD)>0.002f){lD=d;rebuild();repaint();} }
    void paint(juce::Graphics& g) override {
        const auto b=getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xFF0E1220));g.fillRoundedRectangle(b,4.f);
        grid(g,b);
        g.setColour(juce::Colour(0x22FFFFFF));
        g.drawHorizontalLine((int)dbY(0.f,b),b.getX()+PL,b.getRight()-PR);
        if(!fill.isEmpty()){juce::ColourGradient fg(juce::Colour(0x30FFA028),0.f,b.getY(),
             juce::Colours::transparentBlack,0.f,b.getBottom(),false);
             g.setGradientFill(fg);g.fillPath(fill);}
        if(!cur.isEmpty()){const uint8_t gr=(uint8_t)(160+(int)(60.f*(1.f-lD)));
             g.setColour(juce::Colour::fromRGBA(255u,gr,20u,220u));
             g.strokePath(cur,juce::PathStrokeType(2.2f,
                 juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
        labels(g,b);}
    void resized() override { lD=-1.f; }
private:
    static constexpr float FMIN=20,FMAX=20000,DMIN=-18,DMAX=6,PL=36,PR=8,PT=10,PB=22;
    static constexpr int NP=256;
    juce::Path cur,fill; std::atomic<float> dn{0.f}; float lD{-1.f};
    float fX(float f,const juce::Rectangle<float>& b) const noexcept
    {return b.getX()+PL+std::log10(f/FMIN)/std::log10(FMAX/FMIN)*(b.getWidth()-PL-PR);}
    float dbY(float db,const juce::Rectangle<float>& b) const noexcept
    {return b.getY()+PT+(1.f-(db-DMIN)/(DMAX-DMIN))*(b.getHeight()-PT-PB);}
    static float shM(float f,float fc,float g)
    {return g/(1.f+std::pow(f/fc,-4.f));}
    static float pkM(float f,float g,float fc=300.f,float Q=1.f)
    {float w=f/fc;return g/(1.f+Q*Q*(w-1.f/w)*(w-1.f/w));}
    void rebuild() {
        const auto b=getLocalBounds().toFloat();if(b.isEmpty())return;
        const float sf=DistanceMapping::hfShelfFreqHz(lD);
        const float sg=DistanceMapping::hfShelfGainDb(lD);
        const float tg=DistanceMapping::midTiltDb(lD);
        cur.clear();fill.clear();float fx0=0,fxN=0;bool first=true;
        for(int i=0;i<NP;++i){
            const float f=FMIN*std::pow(FMAX/FMIN,(float)i/(NP-1));
            const float db=shM(f,sf,sg)+pkM(f,tg);
            const float px=fX(f,b),py=dbY(db,b);
            if(first){cur.startNewSubPath(px,py);fx0=px;first=false;}
            else cur.lineTo(px,py);fxN=px;}
        fill=cur;fill.lineTo(fxN,dbY(DMIN,b));
        fill.lineTo(fx0,dbY(DMIN,b));fill.closeSubPath();}
    void grid(juce::Graphics& g,const juce::Rectangle<float>& b) {
        g.setColour(juce::Colour(0x14FFFFFF));
        for(float f:{50.f,100.f,200.f,500.f,1000.f,2000.f,5000.f,10000.f})
            g.drawVerticalLine((int)fX(f,b),b.getY()+PT,b.getBottom()-PB);
        for(float db:{-12.f,-6.f,0.f,3.f})
            g.drawHorizontalLine((int)dbY(db,b),b.getX()+PL,b.getRight()-PR);}
    void labels(juce::Graphics& g,const juce::Rectangle<float>& b) {
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),9.f,juce::Font::plain));
        g.setColour(juce::Colour(0x66FFFFFF));
        for(auto[f,l]:std::initializer_list<std::pair<float,const char*>>{
            {100,"100"},{500,"500"},{1000,"1k"},{5000,"5k"},{20000,"20k"}})
            g.drawText(l,(int)fX(f,b)-12,(int)(b.getBottom()-PB),24,14,juce::Justification::centred);
        for(float db:{-12.f,-6.f,0.f})
            g.drawText(juce::String((int)db),(int)b.getX(),(int)(dbY(db,b)-7),
                       (int)PL-2,14,juce::Justification::centredRight);}
    JUCE_LEAK_DETECTOR(FreqResponseComponent)
};

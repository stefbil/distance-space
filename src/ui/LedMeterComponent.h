#pragma once
// LedMeterComponent.h LED bar + 2s peak hold.
// std::function<float()> value provider decoupled from data source.
// tick() called by parent editor's 30Hz timer. No internal timer.
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <algorithm>
class LedMeterComponent : public juce::Component {
public:
    static constexpr int SEGS=16;
    static constexpr float HOLD=2.f, FALL=0.012f;
    struct Config {
        juce::Colour nC{juce::Colour(0xFFFFAA28)},wC{juce::Colour(0xFFFF8800)},pC{juce::Colour(0xFFFF3030)};
        float wAt{0.75f},pAt{0.90f}; juce::String label; };
    LedMeterComponent(std::function<float()> p,Config c={})
        :vp(std::move(p)),cfg(std::move(c)){}
    void tick() noexcept {
        const float lv=juce::jlimit(0.f,1.f,vp()); cur=lv;
        if(lv>=pk){pk=lv;ph=0;}
        else if(++ph>(int)(HOLD*30.f)) pk=std::max(0.f,pk-FALL);
        repaint();}
    void paint(juce::Graphics& g) override {
        const auto b=getLocalBounds();
        const int sH=(b.getHeight()-18)/SEGS,sW=b.getWidth()-4;
        const int lit=(int)(cur*SEGS),pkS=(int)(pk*SEGS);
        for(int i=0;i<SEGS;++i){
            const int idx=SEGS-1-i; const float nm=(float)idx/SEGS; const int y=2+i*sH;
            juce::Colour col=nm>=cfg.pAt?cfg.pC:nm>=cfg.wAt?cfg.wC:cfg.nC;
            const bool iL=(idx<lit),iP=(idx==pkS&&pk>0.01f);
            g.setColour(iL?col:(iP?col.withAlpha(.8f):juce::Colour(0x14FFFFFF)));
            g.fillRoundedRectangle(2.f,(float)y,(float)sW,(float)(sH-1),1.5f);}
        if(cfg.label.isNotEmpty()){
            g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),9.f,juce::Font::plain));
            g.setColour(juce::Colour(0x77FFFFFF));
            g.drawText(cfg.label,b.getX(),b.getBottom()-16,b.getWidth(),14,juce::Justification::centred);}}
private:
    std::function<float()> vp; Config cfg;
    float cur{0.f},pk{0.f}; int ph{0};
    JUCE_LEAK_DETECTOR(LedMeterComponent)
};

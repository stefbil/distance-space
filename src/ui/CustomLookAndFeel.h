#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
class DistanceLookAndFeel : public juce::LookAndFeel_V4 {
public:
    DistanceLookAndFeel() {
        setColour(juce::Slider::backgroundColourId,    juce::Colour(0xFF151822));
        setColour(juce::ComboBox::backgroundColourId,  juce::Colour(0xFF1E2236));
        setColour(juce::ComboBox::outlineColourId,     juce::Colour(0x55FFFFFF));
        setColour(juce::ComboBox::textColourId,        juce::Colour(0xEEFFD278));
        setColour(juce::TextButton::buttonColourId,    juce::Colour(0xFF222840));
        setColour(juce::TextButton::buttonOnColourId,  juce::Colour(0x44FFA028));
        setColour(juce::TextButton::textColourOffId,   juce::Colour(0xBBFFFFFF));
        setColour(juce::TextButton::textColourOnId,    juce::Colour(0xFFFFD278));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xFF1E2236));
        setColour(juce::PopupMenu::textColourId,       juce::Colour(0xEEFFFFFF)); }

    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
                          float pos,float sa,float ea,juce::Slider&) override {
        const float cx=x+w*.5f,cy=y+h*.5f,r=juce::jmin(w,h)*.42f;
        const float ang=sa+pos*(ea-sa);
        {juce::Path t;t.addArc(cx-r*.88f,cy-r*.88f,r*1.76f,r*1.76f,sa,ea,true);
         g.setColour(juce::Colour(0x1AFFFFFF));
         g.strokePath(t,juce::PathStrokeType(5.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
        if(pos>0.001f){juce::Path v;v.addArc(cx-r*.88f,cy-r*.88f,r*1.76f,r*1.76f,sa,ang,true);
         const uint8_t gr=(uint8_t)(160+(int)(60.f*(1.f-pos)));
         g.setColour(juce::Colour::fromRGBA(255u,gr,20u,240u));
         g.strokePath(v,juce::PathStrokeType(4.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
        for(int i=0;i<=10;++i){const float ta=sa+i*(ea-sa)/10.f;const bool mj=(i%5==0);
         g.setColour(mj?juce::Colour(0xBBFFA028):juce::Colour(0x33FFFFFF));
         g.drawLine(cx+std::cos(ta)*r*(mj?.70f:.78f),cy+std::sin(ta)*r*(mj?.70f:.78f),
                    cx+std::cos(ta)*r*.84f,cy+std::sin(ta)*r*.84f,mj?2.f:1.f);}
        juce::ColourGradient grad(juce::Colour(0xFF606878),cx-r*.15f,cy-r*.15f,
                                  juce::Colour(0xFF1A1E28),cx+r*.5f,cy+r*.5f,true);
        g.setGradientFill(grad); g.fillEllipse(cx-r*.61f,cy-r*.61f,r*1.22f,r*1.22f);
        g.setColour(juce::Colour(0x28FFFFFF));g.drawEllipse(cx-r*.61f,cy-r*.61f,r*1.22f,r*1.22f,1.5f);
        const float pAng=ang-juce::MathConstants<float>::halfPi;
        g.setColour(juce::Colour(0xFFFFD278));
        g.drawLine(cx+std::cos(pAng)*r*.22f,cy+std::sin(pAng)*r*.22f,
                   cx+std::cos(pAng)*r*.56f,cy+std::sin(pAng)*r*.56f,2.8f);
        g.setColour(juce::Colour(0xDDFFB840));g.fillEllipse(cx-4.5f,cy-4.5f,9.f,9.f);}

    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
                          float pos,float,float,const juce::Slider::SliderStyle,juce::Slider&) override {
        const float ty=y+h*.5f,th=4.f,tr=7.f;
        g.setColour(juce::Colour(0x22FFFFFF));g.fillRoundedRectangle((float)x,ty-th*.5f,(float)w,th,2.f);
        g.setColour(juce::Colour(0xCCFFA028));g.fillRoundedRectangle((float)x,ty-th*.5f,pos-x,th,2.f);
        g.setColour(juce::Colour(0xFFFFB840));g.fillEllipse(pos-tr,ty-tr,tr*2.f,tr*2.f);}

    void drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox&) override {
        g.setColour(findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(0.f,0.f,(float)w,(float)h,5.f);
        g.setColour(findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(.5f,.5f,w-1.f,h-1.f,5.f,1.f);}

    void drawButtonBackground(juce::Graphics& g,juce::Button& btn,const juce::Colour&,bool hi,bool) override {
        const auto b=btn.getLocalBounds().toFloat();const bool on=btn.getToggleState();
        if(btn.getClickingTogglesState()){
            g.setColour(on?juce::Colour(0x44FFA028):(hi?juce::Colour(0x30FFFFFF):juce::Colour(0x1CFFFFFF)));
            g.fillRoundedRectangle(b,5.f);
            g.setColour(on?juce::Colour(0x99FFA028):juce::Colour(0x44FFFFFF));
            g.drawRoundedRectangle(b.reduced(.5f),5.f,1.f);
        } else {
            g.setColour(hi?juce::Colour(0xFF2A3050):juce::Colour(0xFF222840));
            g.fillRoundedRectangle(b,5.f);
            g.setColour(hi?juce::Colour(0xBBFFA028):juce::Colour(0x88FFA028));
            g.drawRoundedRectangle(b.reduced(.5f),5.f,1.2f);
        }}

    void drawButtonText(juce::Graphics& g,juce::TextButton& btn,bool,bool) override {
        const bool toggle=btn.getClickingTogglesState();
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),toggle?10.f:10.5f,juce::Font::plain));
        if(toggle)
            g.setColour(btn.getToggleState()?findColour(juce::TextButton::textColourOnId)
                                            :findColour(juce::TextButton::textColourOffId));
        else
            g.setColour(juce::Colour(0xFFFFD278));
        g.drawFittedText(btn.getButtonText(),btn.getLocalBounds(),juce::Justification::centred,1);}
};

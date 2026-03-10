#include "PluginEditor.h"
const std::vector<DistancePluginEditor::Preset> DistancePluginEditor::PRESETS{
    {"Reference",0.00f,1,false},{"Intimate",0.08f,0,false},
    {"Stage",0.28f,1,false},    {"Hall",0.62f,2,false},
    {"Distant",1.00f,2,false},  {"HRTF Near",0.15f,0,true},
    {"HRTF Deep",0.75f,2,true}};

DistancePluginEditor::DistancePluginEditor(DistancePluginProcessor& p)
    : AudioProcessorEditor(&p),proc(p),scope(p.getScopeFifo()),
      meterL([&p]{return p.getMeterLeft();},
             LedMeterComponent::Config{juce::Colour(0xFFFFAA28),juce::Colour(0xFFFF8800),juce::Colour(0xFFFF3030),0.75f,0.90f,"L"}),
      meterR([&p]{return p.getMeterRight();},
             LedMeterComponent::Config{juce::Colour(0xFFFFAA28),juce::Colour(0xFFFF8800),juce::Colour(0xFFFF3030),0.75f,0.90f,"R"}),
      meterW([&p]{return p.getMeterWet();},
             LedMeterComponent::Config{juce::Colour(0xFF60B0FF),juce::Colour(0xFF4090EE),juce::Colour(0xFF2060CC),0.75f,0.90f,"WET"})
{
    setLookAndFeel(&laf); setSize(W,H); setResizable(false,false);
    distKnob.setSliderStyle(juce::Slider::Rotary);
    distKnob.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    distKnob.setRotaryParameters(juce::degreesToRadians(210.f),juce::degreesToRadians(510.f),true);
    addAndMakeVisible(distKnob); kAtt=std::make_unique<SA>(proc.apvts,"distance",distKnob);
    distScrub.setSliderStyle(juce::Slider::LinearHorizontal);
    distScrub.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    addAndMakeVisible(distScrub); sAtt=std::make_unique<SA>(proc.apvts,"distance",distScrub);
    roomCombo.addItemList({"Small","Medium","Large"},1);
    addAndMakeVisible(roomCombo); rAtt=std::make_unique<CA>(proc.apvts,"room_size",roomCombo);
    hrtfBtn.setClickingTogglesState(true); bypassBtn.setClickingTogglesState(true);
    bypEQBtn.setClickingTogglesState(true); bypRevBtn.setClickingTogglesState(true);
    addAndMakeVisible(hrtfBtn);   hAtt=std::make_unique<BA>(proc.apvts,"hrtf_on",hrtfBtn);
    addAndMakeVisible(bypassBtn); bAtt=std::make_unique<BA>(proc.apvts,"bypass", bypassBtn);
    addAndMakeVisible(bypEQBtn);  eqBypAtt=std::make_unique<BA>(proc.apvts,"byp_eq",bypEQBtn);
    addAndMakeVisible(bypRevBtn); revBypAtt=std::make_unique<BA>(proc.apvts,"byp_reverb",bypRevBtn);
    distLabel.setFont(juce::Font(juce::Font::getDefaultSerifFontName(),34.f,juce::Font::bold));
    distLabel.setColour(juce::Label::textColourId,juce::Colour(0xFFFFD278));
    distLabel.setJustificationType(juce::Justification::centred);
    distLabel.setText("< 1m",juce::dontSendNotification);
    addAndMakeVisible(distLabel);
    addAndMakeVisible(freqResp); addAndMakeVisible(scope);
    addAndMakeVisible(cueBar);   addAndMakeVisible(meterL);
    addAndMakeVisible(meterR);   addAndMakeVisible(meterW);
    for(size_t i=0;i<PRESETS.size();++i){
        auto b=std::make_unique<juce::TextButton>(PRESETS[i].name);
        const size_t idx=i;
        b->onClick=[this,idx]{applyPreset(PRESETS[idx]);};
        addAndMakeVisible(*b); pBtns.push_back(std::move(b));}

    addAndMakeVisible(loadIrBtn); addAndMakeVisible(defaultIrBtn);
    loadIrBtn.onClick=[this]{
        fileChooser=std::make_unique<juce::FileChooser>(
            "Select HRTF Impulse Response",juce::File{},"*.wav;*.aiff;*.flac");
        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc){
                auto f=fc.getResult();
                if(f.existsAsFile()){
                    proc.loadCustomHrtf(f);
                    irNameLabel.setText(proc.getCustomIrName(),juce::dontSendNotification);
                }
            });};
    defaultIrBtn.onClick=[this]{
        proc.loadDefaultHrtf();
        irNameLabel.setText(proc.getCustomIrName(),juce::dontSendNotification);};
    irNameLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),9.f,juce::Font::plain));
    irNameLabel.setColour(juce::Label::textColourId,juce::Colour(0x99FFFFFF));
    irNameLabel.setJustificationType(juce::Justification::centredLeft);
    irNameLabel.setText(proc.getCustomIrName().isEmpty()?"No IR loaded":proc.getCustomIrName(),
                        juce::dontSendNotification);
    addAndMakeVisible(irNameLabel);
    if(proc.getCustomIrName().isEmpty()) proc.loadDefaultHrtf();
    irNameLabel.setText(proc.getCustomIrName(),juce::dontSendNotification);
    startTimerHz(HZ); }

DistancePluginEditor::~DistancePluginEditor()
{ stopTimer(); setLookAndFeel(nullptr); }

void DistancePluginEditor::timerCallback() {
    const float d=proc.apvts.getRawParameterValue("distance")->load(std::memory_order_relaxed);
    const bool hOn=proc.apvts.getRawParameterValue("hrtf_on")->load(std::memory_order_relaxed)>0.5f;
    freqResp.setDistance(d); scope.setDistance(d); cueBar.setParameters(d,hOn);
    distLabel.setText(fmtDist(d),juce::dontSendNotification);
    meterL.tick(); meterR.tick(); meterW.tick(); cueBar.tick(); }

void DistancePluginEditor::paint(juce::Graphics& g) {
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF1C2034),0.f,0.f,
        juce::Colour(0xFF141828),(float)W,(float)H,false));
    g.fillAll();
    g.setColour(juce::Colour(0x06000000));
    for(int y=0;y<H;y+=4) g.drawHorizontalLine(y,0.f,(float)W);
    g.setColour(juce::Colour(0x30FFA028)); g.drawHorizontalLine(52,0.f,(float)W);
    g.setColour(juce::Colour(0x18FFFFFF)); g.drawVerticalLine(228,56.f,(float)(H-46));
    g.drawHorizontalLine(H-46,0.f,(float)W);

    const auto s20=juce::Font(juce::Font::getDefaultSerifFontName(),20.f,juce::Font::bold);
    const auto m9 =juce::Font(juce::Font::getDefaultMonospacedFontName(),9.f,juce::Font::plain);
    g.setFont(s20);
    g.setColour(juce::Colour(0xFFFFD278));g.drawText("DISTANCE",16,14,120,24,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0x66FFFFFF));g.drawText("/",140,14,20,24,juce::Justification::centred);
    g.setColour(juce::Colour(0xBBB0DFFF));g.drawText("SPACE",164,14,80,24,juce::Justification::centredLeft);
    g.setFont(m9); g.setColour(juce::Colour(0x44FFFFFF));
    g.drawText("Immersive Sonic Experiences | Stefanos Biliousis",256,20,220,14,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0x77FFA028));
    g.drawText("SPECTRAL RESPONSE",236,58,200,12,juce::Justification::centredLeft);
    g.drawText("SIGNAL SCOPE",      236,220,200,12,juce::Justification::centredLeft);
    g.drawText("CUE ACTIVITY",      236,332,200,12,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0x66FFFFFF));
    g.drawText("ROOM",8,348,40,14,juce::Justification::centredLeft);

    g.setColour(juce::Colour(0x77FFA028));
    g.drawText("HRTF IR",8,448,60,12,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0x10FFFFFF));
    g.fillRoundedRectangle(234.f,438.f,620.f,30.f,4.f);
    g.setColour(juce::Colour(0x88FFA028));
    g.drawText("PRESETS",240,442,66,22,juce::Justification::centredLeft); }

void DistancePluginEditor::resized() {
    bypEQBtn .setBounds(478,14,68,24); bypRevBtn.setBounds(550,14,68,24);
    hrtfBtn  .setBounds(622,14,68,24); bypassBtn.setBounds(694,14,68,24);
    distLabel.setBounds(8,56,210,44);  distKnob.setBounds(8,100,210,210);
    distScrub.setBounds(18,316,190,20);roomCombo.setBounds(50,344,168,24);
    meterL.setBounds(8,374,58,80);     meterR.setBounds(72,374,58,80);
    meterW.setBounds(136,374,58,80);
    loadIrBtn  .setBounds(8,460,80,20);
    defaultIrBtn.setBounds(92,460,68,20);
    irNameLabel.setBounds(164,460,62,20);
    freqResp.setBounds(236,70,616,142);scope.setBounds(236,232,616,92);
    cueBar.setBounds(236,344,616,90);
    int px=310,py=442;
    for(auto& b:pBtns){b->setBounds(px,py,72,22);px+=76;} }

void DistancePluginEditor::applyPreset(const Preset& p) {
    proc.apvts.getParameter("distance") ->setValueNotifyingHost(p.dist);
    proc.apvts.getParameter("room_size")->setValueNotifyingHost(p.room*0.5f);
    proc.apvts.getParameter("hrtf_on") ->setValueNotifyingHost(p.hrtf?1.f:0.f); }

juce::String DistancePluginEditor::fmtDist(float d) const {
    const float m=1.f+d*19.f;
    if(m<1.5f)return "< 1m";
    if(m<3.f) return juce::String(m,1)+"m";
    return juce::String(juce::roundToInt(m))+"m"; }

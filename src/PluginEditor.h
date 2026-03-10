#pragma once
// PluginEditor.h  860x520 editor. Single 30Hz timer.
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/FreqResponseComponent.h"
#include "ui/ScopeComponent.h"
#include "ui/LedMeterComponent.h"
#include "ui/CueActivityComponent.h"
class DistancePluginEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    static constexpr int W=860,H=520,HZ=30;
    explicit DistancePluginEditor(DistancePluginProcessor&);
    ~DistancePluginEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
private:
    DistancePluginProcessor& proc;
    DistanceLookAndFeel laf;
    juce::Label distLabel;
    juce::Slider distKnob,distScrub;
    juce::ComboBox roomCombo;
    juce::TextButton hrtfBtn{"HRTF"},bypassBtn{"BYPASS"},bypEQBtn{"EQ BYP"},bypRevBtn{"REV BYP"};
    FreqResponseComponent freqResp;
    ScopeComponent scope;
    CueActivityComponent cueBar;
    LedMeterComponent meterL,meterR,meterW;
    using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> kAtt,sAtt;
    std::unique_ptr<BA> hAtt,bAtt,eqBypAtt,revBypAtt;
    std::unique_ptr<CA> rAtt;
    struct Preset{juce::String name;float dist;int room;bool hrtf;};
    static const std::vector<Preset> PRESETS;
    std::vector<std::unique_ptr<juce::TextButton>> pBtns;
    void applyPreset(const Preset&);
    juce::String fmtDist(float) const;
    juce::TextButton loadIrBtn{"LOAD IR"},defaultIrBtn{"DEFAULT"};
    juce::Label irNameLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistancePluginEditor)
};

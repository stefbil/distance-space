#pragma once
// PluginProcessor.h
// AUDIO THREAD: processBlock() no alloc, no locks, no JUCE msg calls.
// UI THREAD:    get*()  atomic relaxed reads. createEditor() msg thread.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include "dsp/DistanceChain.h"
#include "utils/AudioBufferFifo.h"

class DistancePluginProcessor : public juce::AudioProcessor {
public:
    static constexpr const char* ID_DIST="distance", *ID_BYP="bypass",
                                *ID_HRTF="hrtf_on",  *ID_ROOM="room_size",
                                *ID_BYP_EQ="byp_eq", *ID_BYP_REV="byp_reverb";
    static inline constexpr std::array<float,3> ROOM_SCALES{0.65f,1.0f,1.45f};

    DistancePluginProcessor();
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Distance/Space"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int    getNumPrograms()    override { return 1; }
    int    getCurrentProgram() override { return 0; }
    void   setCurrentProgram(int) override {}
    const  juce::String getProgramName(int) override { return {}; }
    void   changeProgramName(int,const juce::String&) override {}
    void   getStateInformation(juce::MemoryBlock&) override;
    void   setStateInformation(const void*,int) override;

    [[nodiscard]] float getMeterLeft()  const noexcept { return mL.load(std::memory_order_relaxed); }
    [[nodiscard]] float getMeterRight() const noexcept { return mR.load(std::memory_order_relaxed); }
    [[nodiscard]] float getMeterWet()   const noexcept { return mW.load(std::memory_order_relaxed); }
    AudioBufferFifo<2,4096>& getScopeFifo() noexcept { return scopeFifo; }
    void loadDefaultHrtf();
    void loadCustomHrtf(const juce::File& irFile);
    juce::String getCustomIrName() const { return customIrName; }
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
private:
    std::atomic<float> *pDist{},*pByp{},*pHrtf{},*pRoom{},*pBypEQ{},*pBypRev{};
    DistanceChain chain;
    juce::String customIrPath, customIrName;
    AudioBufferFifo<2,4096> scopeFifo;
    std::atomic<float> mL{0.f},mR{0.f},mW{0.f};
    float envL{0.f},envR{0.f};
    static constexpr float ATK=0.9f, REL=0.9997f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistancePluginProcessor)
};

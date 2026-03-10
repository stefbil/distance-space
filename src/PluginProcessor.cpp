#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "utils/ParameterMapping.h"
#include "BinaryData.h"
#include <juce_audio_formats/juce_audio_formats.h>

juce::AudioProcessorValueTreeState::ParameterLayout
DistancePluginProcessor::createParameterLayout() {
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ID_DIST,2},"Distance / Space",
        NormalisableRange<float>(0.f,1.f,0.001f),
        0.f,AudioParameterFloatAttributes{}.withLabel("dist")));
    p.push_back(std::make_unique<AudioParameterBool>(ParameterID{ID_BYP,2},"Bypass",false));
    p.push_back(std::make_unique<AudioParameterBool>(ParameterID{ID_HRTF,2},"HRTF",false));
    p.push_back(std::make_unique<AudioParameterChoice>(ParameterID{ID_ROOM,2},"Room Size",
        StringArray{"Small","Medium","Large"},1));
    p.push_back(std::make_unique<AudioParameterBool>(ParameterID{ID_BYP_EQ,2},"Bypass EQ",false));
    p.push_back(std::make_unique<AudioParameterBool>(ParameterID{ID_BYP_REV,2},"Bypass Reverb",false));
    return {p.begin(),p.end()}; }

DistancePluginProcessor::DistancePluginProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input",  juce::AudioChannelSet::stereo(),true)
          .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      apvts(*this,nullptr,"Parameters",createParameterLayout())
{
    pDist  =apvts.getRawParameterValue(ID_DIST);
    pByp   =apvts.getRawParameterValue(ID_BYP);
    pHrtf  =apvts.getRawParameterValue(ID_HRTF);
    pRoom  =apvts.getRawParameterValue(ID_ROOM);
    pBypEQ =apvts.getRawParameterValue(ID_BYP_EQ);
    pBypRev=apvts.getRawParameterValue(ID_BYP_REV); }

void DistancePluginProcessor::prepareToPlay(double sr,int bs) {
    const juce::dsp::ProcessSpec spec{sr,(juce::uint32)bs,(juce::uint32)getTotalNumOutputChannels()};
    chain.prepare(spec); scopeFifo.reset();
    mL=mR=mW=0.f; envL=envR=0.f; }

void DistancePluginProcessor::releaseResources() { chain.reset(); }

bool DistancePluginProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo(); }

void DistancePluginProcessor::processBlock(juce::AudioBuffer<float>& buf,juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    for(int i=getTotalNumInputChannels();i<getTotalNumOutputChannels();++i)
        buf.clear(i,0,buf.getNumSamples());
    const float d   =pDist->load(std::memory_order_relaxed);
    const bool  byp =pByp ->load(std::memory_order_relaxed)>0.5f;
    const bool  hrtf=pHrtf->load(std::memory_order_relaxed)>0.5f;
    const float rs    =ROOM_SCALES[juce::jlimit(0,2,(int)pRoom->load(std::memory_order_relaxed))];
    const bool  bypEQ =pBypEQ ->load(std::memory_order_relaxed)>0.5f;
    const bool  bypRev=pBypRev->load(std::memory_order_relaxed)>0.5f;
    scopeFifo.push(buf,0,buf.getNumSamples()); // push DRY before DSP
    if(byp) return;
    auto block=juce::dsp::AudioBlock<float>(buf);
    chain.process(block,d,hrtf,rs,bypEQ,bypRev);
    const int n=buf.getNumSamples();
    float pkL=0.f,pkR=0.f;
    for(int s=0;s<n;++s){
        pkL=std::max(pkL,std::abs(buf.getSample(0,s)));
        pkR=std::max(pkR,std::abs(buf.getSample(1,s))); }
    envL=pkL>envL?envL+ATK*(pkL-envL):envL*REL;
    envR=pkR>envR?envR+ATK*(pkR-envR):envR*REL;
    mL.store(envL,std::memory_order_relaxed);
    mR.store(envR,std::memory_order_relaxed);
    mW.store(DistanceMapping::reverbWet(d)/0.45f,std::memory_order_relaxed); }

void DistancePluginProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state=apvts.copyState();
    if(customIrPath.isNotEmpty())
        state.setProperty("customIrPath",customIrPath,nullptr);
    auto xml=state.createXml(); copyXmlToBinary(*xml,dest); }

void DistancePluginProcessor::setStateInformation(const void* d,int sz) {
    auto xml=getXmlFromBinary(d,sz);
    if(xml&&xml->hasTagName(apvts.state.getType())){
        auto state=juce::ValueTree::fromXml(*xml);
        customIrPath=state.getProperty("customIrPath","").toString();
        apvts.replaceState(state);
        if(customIrPath.isNotEmpty()){
            juce::File f(customIrPath);
            if(f.existsAsFile()) loadCustomHrtf(f);
        }
    } }

void DistancePluginProcessor::loadDefaultHrtf() {
    chain.getHrtfStage().loadImpulseResponse(
        BinaryData::H0e000a_LEFT_wav,  BinaryData::H0e000a_LEFT_wavSize,
        BinaryData::H0e000a_RIGHT_wav, BinaryData::H0e000a_RIGHT_wavSize);
    customIrPath.clear(); customIrName="Default (KEMAR)"; }

void DistancePluginProcessor::loadCustomHrtf(const juce::File& irFile) {
    juce::AudioFormatManager afm;
    afm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(irFile));
    if(!reader||reader->lengthInSamples==0) return;
    const int len=(int)reader->lengthInSamples;
    const int nch=(int)reader->numChannels;
    juce::AudioBuffer<float> full(nch,len);
    reader->read(&full,0,len,0,true,nch>1);
    const double fileSR=reader->sampleRate;
    auto toWav=[&](int ch) -> juce::MemoryBlock {
        juce::AudioBuffer<float> mono(1,len);
        mono.copyFrom(0,0,full,juce::jmin(ch,nch-1),0,len);
        juce::MemoryBlock mb;
        auto* mos=new juce::MemoryOutputStream(mb,false);
        juce::WavAudioFormat wav;
        auto* w=wav.createWriterFor(mos,fileSR,1,16,{},0);
        if(w){ w->writeFromAudioSampleBuffer(mono,0,len); delete w; }
        else delete mos;
        return mb; };
    auto memL=toWav(0), memR=toWav(nch>1?1:0);
    chain.getHrtfStage().loadImpulseResponse(
        memL.getData(),memL.getSize(),memR.getData(),memR.getSize());
    customIrPath=irFile.getFullPathName();
    customIrName=irFile.getFileName(); }

juce::AudioProcessorEditor* DistancePluginProcessor::createEditor()
{ return new DistancePluginEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{ return new DistancePluginProcessor(); }

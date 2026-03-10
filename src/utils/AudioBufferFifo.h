#pragma once
// AudioBufferFifo.h
// Lock-free SPSC ring buffer.  Producer=audio thread, Consumer=UI timer.
// Zero allocation after construction.  Zero locks on hot path.
// Built on juce::AbstractFifo (atomic read/write).

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cstring>

template <int Channels=2, int Capacity=4096>
class AudioBufferFifo {
public:
    void reset() noexcept {
        fifo.reset();
        for (auto& ch : buf) ch.fill(0.f); }

    void push(const juce::AudioBuffer<float>& src,
              int start, int n) noexcept {
        int s1,n1,s2,n2;
        fifo.prepareToWrite(n,s1,n1,s2,n2);
        const int ch=juce::jmin(Channels,src.getNumChannels());
        for (int c=0;c<ch;++c) {
            if(n1>0) std::memcpy(&buf[c][s1],src.getReadPointer(c,start),
                                 (size_t)n1*sizeof(float));
            if(n2>0) std::memcpy(&buf[c][s2],src.getReadPointer(c,start+n1),
                                 (size_t)n2*sizeof(float)); }
        fifo.finishedWrite(n1+n2); }

    int pull(juce::AudioBuffer<float>& dst, int n) noexcept {
        int s1,n1,s2,n2;
        fifo.prepareToRead(n,s1,n1,s2,n2);
        const int total=n1+n2; if(!total) return 0;
        const int ch=juce::jmin(Channels,dst.getNumChannels());
        for (int c=0;c<ch;++c) {
            if(n1>0) std::memcpy(dst.getWritePointer(c),
                                 &buf[c][s1],(size_t)n1*sizeof(float));
            if(n2>0) std::memcpy(dst.getWritePointer(c,n1),
                                 &buf[c][s2],(size_t)n2*sizeof(float)); }
        fifo.finishedRead(total); return total; }

    [[nodiscard]] int getNumReady() const noexcept
    { return fifo.getNumReady(); }

private:
    juce::AbstractFifo                               fifo{Capacity};
    std::array<std::array<float,Capacity>,Channels>  buf{};
};

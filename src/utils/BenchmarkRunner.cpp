#include "utils/BenchmarkRunner.h"
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>

namespace distance_benchmark {
namespace {

void fillStereoWhiteNoise(juce::AudioBuffer<float>& buf, std::mt19937& gen) {
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    for (int c = 0; c < buf.getNumChannels(); ++c)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(c, i, dist(gen));
}

void applyScenario(DistancePluginProcessor& proc, bool hrtfOn, float d) noexcept {
    proc.apvts.getRawParameterValue(DistancePluginProcessor::ID_DIST)->store(d, std::memory_order_relaxed);
    proc.apvts.getRawParameterValue(DistancePluginProcessor::ID_HRTF)
        ->store(hrtfOn ? 1.f : 0.f, std::memory_order_relaxed);
    proc.apvts.getRawParameterValue(DistancePluginProcessor::ID_BYP)->store(0.f, std::memory_order_relaxed);
}

struct Timings {
    double mean_us{};
    double cpu_pct{};
    double worst_us{};
};

Timings measureProcessBlock(DistancePluginProcessor& proc, int blockSize, juce::AudioBuffer<float>& buf,
                            juce::MidiBuffer& midi) {
    proc.prepareToPlay(48000.0, blockSize);
    const double block_budget_us = (static_cast<double>(blockSize) / 48000.0) * 1e6;

    // Warm-up 
    constexpr int kWarmup = 200;
    for (int i = 0; i < kWarmup; ++i)
        proc.processBlock(buf, midi);

    using clock = std::chrono::high_resolution_clock;
    double sum_us = 0.0;
    double worst_us = 0.0;

    constexpr int kIterations = 10000;
    for (int i = 0; i < kIterations; ++i) {
        const auto t0 = clock::now();
        proc.processBlock(buf, midi);
        const auto t1 = clock::now();
        const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        sum_us += us;
        worst_us = std::max(worst_us, us);
    }

    const double mean_us = sum_us / static_cast<double>(kIterations);
    return {mean_us, (mean_us / block_budget_us) * 100.0, worst_us};
}

} // namespace

void runAndPrintCsvToStdout() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    DistancePluginProcessor proc;
    proc.loadDefaultHrtf();

    std::mt19937 gen(5489u);
    juce::MidiBuffer midi;

    static constexpr int blockSizes[] = {64, 128, 256, 512};
    static constexpr float distances[] = {0.f, 1.f};
    constexpr double sampleRate = 48000.0;

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "# Warm-up: 200 untimed processBlock() calls after prepareToPlay() before each "
                   "10000-iteration timed measurement.\n";
    std::cout << "# Real-time block budget at 48 kHz: budget_us = (block_size / 48000.0) * 1e6\n";
    std::cout << "# block_size,budget_us\n";
    for (int bs : blockSizes)
        std::cout << "# " << bs << ',' << (static_cast<double>(bs) / sampleRate) * 1e6 << '\n';
    std::cout << "block_size,hrtf_on,d,mean_us,cpu_pct,worst_us\n";

    for (int blockSize : blockSizes) {
        juce::AudioBuffer<float> buf(2, blockSize);
        fillStereoWhiteNoise(buf, gen);

        for (bool hrtfOn : {false, true}) {
            for (float d : distances) {
                applyScenario(proc, hrtfOn, d);
                const Timings t = measureProcessBlock(proc, blockSize, buf, midi);
                std::cout << blockSize << ',' << (hrtfOn ? 1 : 0) << ',' << d << ',' << t.mean_us << ','
                          << t.cpu_pct << ',' << t.worst_us << '\n';
            }
        }
    }
}

} // namespace distance_benchmark

int main() {
    distance_benchmark::runAndPrintCsvToStdout();
    return 0;
}

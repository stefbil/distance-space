# Distance Space

Distance Space is a JUCE-based stereo audio effect plugin that simulates source distance and room depth with spectral shaping, HRTF cues, and reverb.

## Features

- Distance control (`< 1m` to far field) with linked spectral/reverb behavior
- Optional HRTF spatialization with embedded default KEMAR impulse responses
- Room size selection (`Small`, `Medium`, `Large`)
- Independent bypass controls for full processing, EQ stage, and reverb stage
- Built-in visual feedback:
  - Spectral response display
  - Signal scope
  - Cue activity meter
  - L/R/Wet LED meters
- Custom HRTF IR loading from `.wav`, `.aiff`, or `.flac`

## Repository Layout

- `src/PluginProcessor.*` – audio processing, parameter layout, state handling
- `src/PluginEditor.*` – plugin UI, controls, preset buttons, metering
- `src/dsp/` – DSP stages and distance processing chain
- `src/ui/` – custom visualization and meter components
- `src/utils/BenchmarkRunner.*` – console benchmark entry and timing runner
- `HRTF IR/` – embedded default left/right IR files

## Build

This project uses CMake + JUCE and currently expects a local JUCE checkout in `CMakeLists.txt`:

```cmake
add_subdirectory("/path/to/JUCE" build_juce)
```

Before building on your machine, update that path (or switch to the commented `FetchContent` setup in `CMakeLists.txt`).

### Configure and build (example)

```bash
cmake -S . -B build
cmake --build build --config Release
```

`CMakePresets.json` includes Windows Visual Studio presets:

- `windows-debug`
- `windows-release`

## Benchmark App

The repository also defines a console target, `DistanceBenchmark`, that benchmarks `processBlock()` across block sizes and scenarios and prints CSV to stdout.

After configuring/building, run the benchmark binary from your build output directory.

## License

This project is distributed under the terms in [`LICENSE`](./LICENSE).

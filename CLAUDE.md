# MACHINE LORD RAP STATION: Claude System Brief 🏭

## Project Identity
**MACHINE LORD RAP STATION** is a standalone procedural production suite. It is designed for "Spontaneous Banger Manufacturing"—a workflow optimized for rappers and producers to build arrangements, record vocals per section, and export 24-bit WAV masters in a single session.

## System Architecture

### 1. The Generator (Logic Layer)
Located in `Generator.h`, the engine uses a non-linear arrangement model. 
- **Section Matching**: Sections of the same type share a "Core Motif" (DNA) but feature unique procedural "Performances" (fills, ghost notes).
- **Harmonic Intelligence**: Logic determines chord progressions (I-IV-V variants) and ensures lead melodies follow melodic contours within the selected scale.

### 2. The Audio Engine (DSP Layer)
Located in `Instruments.h`, the synthesis uses a custom `SynthesiserVoice` hierarchy.
- **Master FX Chain**: Features a soft-clipping master bus and a global **Glue Reverb** (`juce::dsp::Reverb`) to unify the pure DSP oscillators.
- **Vox Lab Pre-Amp**: Microphone input is boosted by 2.0x and captured *pre-synthesiser* to ensure high-fidelity recording regardless of output volume.

### 3. The Interface (UI/UX Layer)
Built with a custom `FactoryLookAndFeel`.
- **Timeline Producer**: A visual viewport containing color-coded `SectionComponents`.
- **MPC Control Grid**: A high-density layout of rotary knobs and selectors for BPM, Swing, and Drum specialty (Fill/Ghost/Drama).
- **Visual Feedback**: Real-time `AudioVisualizer` (waveform) and a responsive `LevelMeter` for input monitoring.

## Key Technical Specifications
- **Framework**: JUCE 7+
- **Language**: C++17 (with local patches for Apple Clang compatibility)
- **Audio Depth**: 24-bit / 44.1kHz
- **Recording Buffer**: 8 seconds per section (dynamic map allocation)
- **Entitlements**: Required for macOS Sandbox Microphone access.

## Implementation Quirks
- **Reference Counting**: Uses `CustomSound` with section IDs to ensure the synthesiser pulls the correct buffer from the `std::map<juce::String, juce::AudioBuffer<float>>` during playback.
- **Sample-Accurate Export**: The `exportToWav` function uses an offline rendering loop, bypassing the real-time audio callback to ensure no buffer dropouts in the final file.

---
*Documented for architectural continuity.*

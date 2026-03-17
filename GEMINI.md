# MACHINE LORD RAP STATION: Gemini Dev Log ⚡️

## Project Overview
The **MACHINE LORD RAP STATION** (formerly *Human Banger Factory*) is a high-end procedural audio workstation built with **C++** and the **JUCE** framework. Its purpose is to bridge the gap between algorithmic composition and spontaneous rap performance, allowing a "Machine Lord" to manufacture complete, harmonically-matched hip-hop tracks instantly.

## The "Machine Lord" Philosophy
Unlike traditional DAWs that rely on static loops, this station is driven by **Rhythmic and Tonal DNA**. The goal was to create a "living" instrument where the user defines the arrangement and the engine provides the "performance."

## Core Features 🚀

### 1. Pure DSP Synthesis Engine
- **No Samples**: Every sound is generated mathematically in real-time.
- **808 Kick**: Sine wave with fast pitch-sweep and tanh-saturation.
- **Metallic Hats**: Six detuned square-wave oscillators passed through a high-pass filter.
- **Polyphonic Chords**: Lush sawtooth pads with a 24dB/oct ladder filter.
- **Harmonic Matching**: All instruments share a global "Key" and "Scale" (Minor, Phrygian, Dorian) state.

### 2. Algorithmic Composition
- **Harmony**: Uses Circle of Fifths logic to select root keys and valid scale degrees.
- **Style DNA**: 
    - **Trap**: Constant 16th-note hat runs and syncopated kicks.
    - **Boom Bap**: 8th-note bounce with reinforced kick-drum presence and swing.
    - **Lo-Fi**: Relaxed rhythms with dusty ghost notes.
- **Drama Logic**: Procedural "dropouts" and silences are calculated based on a user-defined probability slider.

### 3. The Vox Lab (Modular Recording)
- **Per-Section Recording**: Capture unique 8-second takes for every block in the timeline (Verse 1, Hook, Verse 2, etc.).
- **Procedural Chopping**: The engine automatically syncs and "chops" vocal takes based on section IDs.
- **The Mangler**: Real-time "Chop & Screw," "Stutter," and "Reverse Ghost" algorithms for vocal destruction.

## Technical Notes & Architecture 🛠

### Thread-Safe Mutation
To allow the user to click **REGEN** or **MUTATE** while the song is playing, the engine utilizes a `juce::CriticalSection` mutex. The UI thread locks the song data during mutation, while the Audio thread uses a `ScopedTryLock` to avoid audio glitches or concurrency crashes.

### The "Staccato" Sequencer
A major breakthrough was the implementation of a "Sequencer Kill-Switch." To prevent the ambient "long note" feel, the engine sends a `noteOff` to the synthesizer at every step before triggering the next `noteOn`, ensuring punchy, hip-hop timing.

### macOS Hardware Permissions
Due to modern macOS security, the build requires a `Microphone.entitlements` file and specific `Info.plist` strings. Without these, the system will block microphone access at the kernel level without notifying the user.

---
*Developed in collaboration with Gemini CLI.*

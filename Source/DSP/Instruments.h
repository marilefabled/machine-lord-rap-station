#pragma once
#include <JuceHeader.h>

// --- ENHANCED DNA ---
struct SynthPatch {
    float attack = 0.01f, decay = 0.4f, sustain = 0.5f, release = 0.2f;
    float cutoff = 2000.0f, resonance = 1.0f, drive = 1.0f;
    int oscType = 0; 
};

struct DrumKitDNA {
    struct Kick { float decay = 0.4f; float click = 100.0f; float pitch = 50.0f; float drive = 1.5f; } kick;
    struct Snare { float snap = 0.1f; float tone = 180.0f; float noiseColor = 2000.0f; } snare;
    struct Hats { float cutoff = 7000.0f; float decay = 0.05f; } hats;
};

class CustomSound : public juce::SynthesiserSound {
public:
    CustomSound(int n, juce::String sid = "") : note(n), sectionId(sid) {}
    bool appliesToNote (int n) override { return n == note || (note >= 48 && n >= note); }
    bool appliesToChannel (int) override { return true; }
    int note;
    juce::String sectionId;
};

class BaseVoice : public juce::SynthesiserVoice {
public:
    void stopNote (float, bool allowTailOff) override { 
        ampEnv.noteOff(); 
        if (!allowTailOff) { clearCurrentNote(); ampEnv.reset(); }
    }
    void pitchWheelMoved(int){} void controllerMoved(int,int){}
protected:
    juce::ADSR ampEnv;
    float level = 0.0f;
};

//==============================================================================
class KickVoice : public BaseVoice {
public:
    KickVoice() { osc.initialise([](float x){return std::sin(x);}); pitchEnv.setParameters({0.001f,0.05f,0.0f,0.001f}); }
    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<CustomSound*>(s) && dynamic_cast<CustomSound*>(s)->note == 36; }
    void startNote (int, float v, juce::SynthesiserSound*, int) override { level = v; ampEnv.noteOn(); pitchEnv.noteOn(); }
    void prepareToPlay (double sR, int, int) { osc.prepare({sR,512,2}); ampEnv.setSampleRate(sR); pitchEnv.setSampleRate(sR); }
    void renderNextBlock (juce::AudioBuffer<float>& b, int s, int n) override {
        if (!ampEnv.isActive()) { clearCurrentNote(); return; }
        for (int i=s; i<s+n; ++i) {
            osc.setFrequency(dna.pitch + (dna.click * pitchEnv.getNextSample()));
            float smp = std::tanh(osc.processSample(0.0f) * dna.drive) * ampEnv.getNextSample() * level;
            for (int c=0; c<b.getNumChannels(); ++c) b.addSample(c, i, smp);
        }
    }
    void applyDNA(const DrumKitDNA::Kick& k) { dna = k; ampEnv.setParameters({0.001f, dna.decay, 0.0f, 0.1f}); }
private:
    juce::dsp::Oscillator<float> osc; juce::ADSR pitchEnv; DrumKitDNA::Kick dna;
};

//==============================================================================
class PolySynthVoice : public BaseVoice {
public:
    PolySynthVoice(int targetNote) : assignedNote(targetNote) { filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); }
    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<CustomSound*>(s) && dynamic_cast<CustomSound*>(s)->note == assignedNote; }
    void startNote(int n,float v,juce::SynthesiserSound*,int) override {
        level=v; float f=juce::MidiMessage::getMidiNoteInHertz(n);
        osc1.setFrequency(f); osc2.setFrequency(f*1.005f);
        ampEnv.noteOn();
    }
    void prepareToPlay(double sR,int,int){ 
        osc1.prepare({sR,512,2}); osc2.prepare({sR,512,2}); filter.prepare({sR,512,2}); 
        ampEnv.setSampleRate(sR);
    }
    void renderNextBlock(juce::AudioBuffer<float>& b, int s, int n) override {
        if(!ampEnv.isActive()){clearCurrentNote();return;}
        for(int i=s; i<s+n; ++i){
            float raw = (osc1.processSample(0.0f) + osc2.processSample(0.0f)) * 0.5f;
            float smp = std::tanh(filter.processSample(0, raw) * patch.drive) * ampEnv.getNextSample() * level * 0.3f;
            for (int c=0;c<b.getNumChannels();++c) b.addSample(c,i,smp);
        }
    }
    void applyPatch(const SynthPatch& p) {
        patch = p;
        auto lambda = [this](float x) {
            if (patch.oscType == 1) return x / juce::MathConstants<float>::pi; 
            if (patch.oscType == 2) return x < 0 ? -1.0f : 1.0f; 
            return std::sin(x); 
        };
        osc1.initialise(lambda); osc2.initialise(lambda);
        ampEnv.setParameters({patch.attack, patch.decay, patch.sustain, patch.release});
        filter.setCutoffFrequency(patch.cutoff); filter.setResonance(patch.resonance);
    }
private:
    juce::dsp::Oscillator<float> osc1, osc2; juce::dsp::StateVariableTPTFilter<float> filter; SynthPatch patch; int assignedNote;
};

//==============================================================================
class VoxStationVoice : public BaseVoice {
public:
    VoxStationVoice(std::map<juce::String, juce::AudioBuffer<float>>& buffers) : voxBuffers(buffers) { 
        ampEnv.setParameters({0.01f, 0.1f, 1.0f, 0.1f}); 
    }
    bool canPlaySound(juce::SynthesiserSound* s) override { 
        auto* cs = dynamic_cast<CustomSound*>(s);
        return cs && cs->note == 84; 
    }
    void startNote(int n, float v, juce::SynthesiserSound* s, int) override {
        auto* cs = dynamic_cast<CustomSound*>(s);
        if (cs && voxBuffers.count(cs->sectionId) && voxBuffers[cs->sectionId].getNumSamples() > 0) {
            currentBuffer = &voxBuffers[cs->sectionId];
            level = v;
            currentSampleIndex = (float)startOffset;
            ampEnv.noteOn();
        } else {
            clearCurrentNote();
        }
    }
    void prepareToPlay(double sR, int, int) { ampEnv.setSampleRate(sR); }
    void renderNextBlock(juce::AudioBuffer<float>& b, int s, int n) override {
        if (!ampEnv.isActive() || currentBuffer == nullptr) { clearCurrentNote(); return; }
        for (int i=s; i<s+n; ++i) {
            int idx = (int)currentSampleIndex;
            if (idx >= currentBuffer->getNumSamples()) { clearCurrentNote(); break; }
            float smp = currentBuffer->getSample(0, idx) * ampEnv.getNextSample() * level;
            for (int c=0; c<b.getNumChannels(); ++c) b.addSample(c, i, smp);
            currentSampleIndex += playbackSpeed;
        }
    }
    void setChop(int offset, float speed) { startOffset = offset; playbackSpeed = speed; }
private:
    std::map<juce::String, juce::AudioBuffer<float>>& voxBuffers;
    juce::AudioBuffer<float>* currentBuffer = nullptr;
    float currentSampleIndex = 0;
    int startOffset = 0;
    float playbackSpeed = 1.0f;
};

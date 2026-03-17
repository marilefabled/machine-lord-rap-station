#pragma once
#include <JuceHeader.h>

// --- PATCH DNA ---
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
class SnareVoice : public BaseVoice {
public:
    SnareVoice() { toneOsc.initialise([](float x){return std::sin(x);}); filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass); }
    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<CustomSound*>(s) && dynamic_cast<CustomSound*>(s)->note == 38; }
    void startNote (int, float v, juce::SynthesiserSound*, int) override { level = v; ampEnv.noteOn(); noiseEnv.noteOn(); }
    void prepareToPlay (double sR, int, int) { toneOsc.prepare({sR,512,2}); filter.prepare({sR,512,2}); ampEnv.setSampleRate(sR); noiseEnv.setSampleRate(sR); }
    void renderNextBlock (juce::AudioBuffer<float>& b, int s, int n) override {
        if (!ampEnv.isActive() && !noiseEnv.isActive()) { clearCurrentNote(); return; }
        for (int i=s; i<s+n; ++i) {
            float nz = filter.processSample(0, ((juce::Random::getSystemRandom().nextFloat()*2.0f)-1.0f)*noiseEnv.getNextSample());
            float smp = (toneOsc.processSample(0.0f)*ampEnv.getNextSample() + nz)*level * 0.8f;
            for (int c=0; c<b.getNumChannels(); ++c) b.addSample(c, i, smp);
        }
    }
    void applyDNA(const DrumKitDNA::Snare& sn) { dna=sn; toneOsc.setFrequency(dna.tone); filter.setCutoffFrequency(dna.noiseColor); noiseEnv.setParameters({0.001f,dna.snap,0.0f,0.001f}); ampEnv.setParameters({0.001f,0.1f,0.0f,0.001f}); }
private:
    juce::dsp::Oscillator<float> toneOsc; juce::dsp::StateVariableTPTFilter<float> filter; juce::ADSR noiseEnv; DrumKitDNA::Snare dna;
};

//==============================================================================
class HiHatVoice : public BaseVoice {
public:
    HiHatVoice() { float f[6]={245,306,384,496,638,822}; for (int i=0;i<6;++i){oscs[i].initialise([](float x){return x<0?-1.0f:1.0f;});oscs[i].setFrequency(f[i]);} filter.setType(juce::dsp::StateVariableTPTFilterType::highpass); }
    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<CustomSound*>(s) && dynamic_cast<CustomSound*>(s)->note == 42; }
    void startNote (int,float v,juce::SynthesiserSound*,int) override { level=v; ampEnv.noteOn(); }
    void prepareToPlay(double sR,int,int){ for(int i=0;i<6;++i)oscs[i].prepare({sR,512,2}); filter.prepare({sR,512,2}); ampEnv.setSampleRate(sR); }
    void renderNextBlock(juce::AudioBuffer<float>& b,int s,int n) override {
        if(!ampEnv.isActive()){clearCurrentNote();return;}
        for(int i=s; i<s+n; ++i){
            float sum=0; for(int j=0;j<6;++j)sum+=oscs[j].processSample(0.0f);
            float smp=filter.processSample(0,sum*0.15f)*ampEnv.getNextSample()*level;
            for(int c=0;c<b.getNumChannels();++c) b.addSample(c,i,smp);
        }
    }
    void applyDNA(const DrumKitDNA::Hats& h){ dna=h; filter.setCutoffFrequency(dna.cutoff); ampEnv.setParameters({0.001f,dna.decay,0.0f,0.01f}); }
private:
    juce::dsp::Oscillator<float> oscs[6]; juce::dsp::StateVariableTPTFilter<float> filter; DrumKitDNA::Hats dna;
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
    VoxStationVoice(std::map<juce::String, juce::AudioBuffer<float>>& buffers, juce::String& activeID) 
        : voxBuffers(buffers), currentGlobalID(activeID) { 
        ampEnv.setParameters({0.01f, 0.1f, 1.0f, 0.1f}); 
    }
    bool canPlaySound(juce::SynthesiserSound* s) override { 
        auto* cs = dynamic_cast<CustomSound*>(s);
        return cs && cs->note == 84; 
    }
    void startNote (int n, float v, juce::SynthesiserSound* s, int) override {
        auto* cs = dynamic_cast<CustomSound*>(s);
        juce::String sid = (cs && cs->sectionId != "") ? cs->sectionId : currentGlobalID;
        if (voxBuffers.count(sid) && voxBuffers[sid].getNumSamples() > 0) {
            currentBuffer = &voxBuffers[sid];
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
    juce::String& currentGlobalID;
    juce::AudioBuffer<float>* currentBuffer = nullptr;
    float currentSampleIndex = 0;
    int startOffset = 0;
    float playbackSpeed = 1.0f;
};

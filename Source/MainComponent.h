#pragma once
#include <JuceHeader.h>
#include "DSP/Instruments.h"
#include "DSP/Generator.h"
#include "FactoryLookAndFeel.h"
#include <map>

// --- QOL: Simple Level Meter ---
class LevelMeter : public juce::Component, public juce::Timer {
public:
    LevelMeter() { startTimer(30); }
    void setLevel(float l) { level = l; }
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(level > 0.9f ? juce::Colours::red : juce::Colours::orange);
        auto w = bounds.getWidth() * level;
        g.fillRoundedRectangle(bounds.withWidth(w), 3.0f);
    }
    void timerCallback() override { level *= 0.85f; repaint(); }
private:
    float level = 0.0f;
};

// --- PREMIUM: Audio Visualizer ---
class AudioVisualizer : public juce::Component, public juce::Timer {
public:
    AudioVisualizer() { startTimer(40); }
    void pushBuffer(const juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumSamples() > 0) {
            float level = buffer.getMagnitude(0, 0, buffer.getNumSamples());
            levels.push_back(level);
            if (levels.size() > 100) levels.erase(levels.begin());
        }
    }
    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colours::orange.withAlpha(0.3f));
        auto w = (float)getWidth() / 100.0f;
        for (int i=0; i < (int)levels.size(); ++i) {
            float h = levels[i] * getHeight() * 2.0f;
            g.fillRect((float)i * w, (float)getHeight() - h, w - 1.0f, h);
        }
    }
    void timerCallback() override { repaint(); }
private:
    std::vector<float> levels;
};

class SectionComponent : public juce::Component {
public:
    SectionComponent(BeatGenerator::Section& s, std::function<void()> regen, std::function<void()> record, std::function<void()> regenDrums) 
        : section(s), onRegen(regen), onRecord(record), onRegenDrums(regenDrums) {
        addAndMakeVisible(regenBtn); regenBtn.setButtonText("REGEN ALL"); regenBtn.onClick = [this]{ onRegen(); };
        addAndMakeVisible(recBtn); recBtn.setButtonText("REC"); recBtn.onClick = [this]{ onRecord(); };
        addAndMakeVisible(drumsBtn); drumsBtn.setButtonText("DRUMS"); drumsBtn.onClick = [this]{ onRegenDrums(); };
    }
    void setRecording(bool isRec) { 
        recBtn.setButtonText(isRec ? "RECORDING..." : "REC");
        recBtn.setColour(juce::TextButton::buttonColourId, isRec ? juce::Colours::red : juce::Colour(0xff222222));
    }
    void paint(juce::Graphics& g) override {
        juce::Colour c = juce::Colours::grey;
        if (section.type == BeatGenerator::Intro) c = juce::Colours::teal;
        else if (section.type == BeatGenerator::Verse) c = juce::Colours::orange;
        else if (section.type == BeatGenerator::Hook)  c = juce::Colours::purple;
        else if (section.type == BeatGenerator::Outro) c = juce::Colours::crimson;
        auto area = getLocalBounds().toFloat();
        g.setColour(c.withAlpha(0.15f)); g.fillRoundedRectangle(area, 6.0f);
        g.setColour(c.withAlpha(0.4f)); g.drawRoundedRectangle(area, 6.0f, 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.9f)); g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
        juce::String names[] = {"INTRO", "VERSE", "HOOK", "OUTRO"};
        g.drawText(names[section.type], getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        recBtn.setBounds(r.removeFromBottom(25));
        drumsBtn.setBounds(r.removeFromBottom(25).translated(0, -5));
        regenBtn.setBounds(r.removeFromBottom(25).translated(0, -10));
    }
    BeatGenerator::Section& section;
private:
    juce::TextButton regenBtn, recBtn, drumsBtn;
    std::function<void()> onRegen, onRecord, onRegenDrums;
};

class MainComponent  : public juce::AudioAppComponent, public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;
    void prepareToPlay (int, double) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
    void releaseResources() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    FactoryLookAndFeel factoryLF;
    juce::OwnedArray<SectionComponent> sectionViews;
    juce::Viewport timelineViewport;
    juce::Component timelineContent;

    juce::TextButton generateButton { "NEW SONG DNA" }, playButton { "PLAY / STOP" }, exportWavBtn{"EXPORT WAV"}, stopRecBtn{"STOP ALL REC"}, reGenKitBtn{"NEW KIT DNA"}, settingsBtn{"SETTINGS"};
    juce::Slider tempoSlider, swingSlider, fillSlider, ghostSlider, dramaSlider;
    juce::Slider voxChopSlider, voxPitchSlider, voxWetSlider;
    juce::ComboBox keySelector, scaleSelector, styleSelector, voxMangleSelector;
    juce::Label statusLabel, timeLabel, arrangementLabel, inputLabel;
    juce::Label tempoLabel, swingLabel, fillLabel, ghostLabel, dramaLabel, styleLabel, keyLabel, scaleLabel;
    juce::Label voxChopLabel, voxPitchLabel, voxWetLabel, voxMangleLabel;
    LevelMeter inputMeter;
    AudioVisualizer visualizer;

    // Arrangement Buttons
    juce::TextButton addIntroBtn{"+I"}, addVerseBtn{"+V"}, addHookBtn{"+H"}, addOutroBtn{"+O"}, clearArrBtn{"CLR"};
    std::vector<BeatGenerator::SectionType> currentArrangement;

    std::map<juce::String, juce::AudioBuffer<float>> voxBuffers;
    juce::String activeRecordingId = "";
    juce::String currentlyTriggeringId = "";
    int writePosition = 0;

    juce::Synthesiser synth;
    juce::dsp::Reverb reverb;
    
    BeatGenerator::Song currentSong;
    int currentSectionIndex = 0, songStepCounter = 0;
    bool isPlaying = false;
    DrumKitDNA currentKit;
    juce::AudioBuffer<float> recordedVoxBuffer;

    void updateKitDNA();
    void exportToWav();
    void updateTimeLabel();
    void refreshTimeline();
    void requestPermissions();
    void showAudioSettings();

    juce::CriticalSection songLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
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
    void timerCallback() override { level *= 0.9f; repaint(); }
private:
    float level = 0.0f;
};

class SectionComponent : public juce::Component {
public:
    SectionComponent(BeatGenerator::Section& s, std::function<void()> regen, std::function<void()> record) 
        : section(s), onRegen(regen), onRecord(record) {
        addAndMakeVisible(regenBtn); regenBtn.setButtonText("REGEN"); regenBtn.onClick = [this]{ onRegen(); };
        addAndMakeVisible(recBtn); recBtn.setButtonText("REC"); recBtn.onClick = [this]{ onRecord(); };
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
        g.setColour(juce::Colours::white.withAlpha(0.9f)); g.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        juce::String names[] = {"INTRO", "VERSE", "HOOK", "OUTRO"};
        g.drawText(names[section.type], getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        recBtn.setBounds(r.removeFromBottom(25));
        regenBtn.setBounds(r.removeFromBottom(25).translated(0, -5));
    }
    BeatGenerator::Section& section;
private:
    juce::TextButton regenBtn, recBtn;
    std::function<void()> onRegen, onRecord;
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

    juce::TextButton generateButton { "NEW SONG DNA" }, playButton { "PLAY / STOP" }, exportWavBtn{"EXPORT WAV"}, stopRecBtn{"STOP ALL REC"}, reGenKitBtn{"NEW KIT DNA"};
    juce::Slider tempoSlider, swingSlider, fillSlider, ghostSlider, dramaSlider;
    juce::ComboBox keySelector, scaleSelector, styleSelector;
    juce::Label statusLabel, timeLabel, arrangementLabel, inputLabel;
    juce::Label tempoLabel, swingLabel, fillLabel, ghostLabel, dramaLabel, styleLabel, keyLabel, scaleLabel;
    LevelMeter inputMeter;

    juce::TextButton addIntroBtn{"+I"}, addVerseBtn{"+V"}, addHookBtn{"+H"}, addOutroBtn{"+O"}, clearArrBtn{"CLR"};
    std::vector<BeatGenerator::SectionType> currentArrangement;

    std::map<juce::String, juce::AudioBuffer<float>> voxBuffers;
    juce::String activeRecordingId = "";
    int writePosition = 0;

    juce::Synthesiser synth;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
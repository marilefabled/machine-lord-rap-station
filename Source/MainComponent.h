#pragma once
#include <JuceHeader.h>
#include "DSP/Instruments.h"
#include "DSP/Generator.h"
#include <map>

class SectionComponent : public juce::Component {
public:
    SectionComponent(BeatGenerator::Section& s, std::function<void()> regen, std::function<void()> record) 
        : section(s), onRegen(regen), onRecord(record) {
        addAndMakeVisible(regenBtn); regenBtn.setButtonText("REGEN"); regenBtn.onClick = [this]{ onRegen(); };
        addAndMakeVisible(recBtn); recBtn.setButtonText("REC VOX"); recBtn.onClick = [this]{ onRecord(); };
    }
    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colours::orange.withAlpha(0.2f)); g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
        g.setColour(juce::Colours::white); juce::String names[] = {"INTRO", "VERSE", "HOOK", "OUTRO"};
        g.drawText(names[section.type], getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        recBtn.setBounds(r.removeFromBottom(30));
        regenBtn.setBounds(r.removeFromBottom(30).translated(0, -5));
    }
private:
    BeatGenerator::Section& section;
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
    juce::OwnedArray<SectionComponent> sectionViews;
    juce::Viewport timelineViewport;
    juce::Component timelineContent;

    juce::TextButton generateButton { "NEW SONG DNA" }, playButton { "PLAY / STOP" }, exportWavBtn{"EXPORT WAV"}, stopRecBtn{"STOP ALL REC"};
    juce::Slider tempoSlider, swingSlider, fillSlider, ghostSlider, dramaSlider;
    juce::ComboBox keySelector, scaleSelector, styleSelector;
    juce::Label statusLabel, timeLabel, arrangementLabel;

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

    void updateKitDNA();
    void exportToWav();
    void updateTimeLabel();
    void refreshTimeline();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
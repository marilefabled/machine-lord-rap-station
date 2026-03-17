#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize (1200, 900);

    for(int i=0; i<4; ++i) synth.addVoice(new KickVoice());
    for(int i=0; i<2; ++i) synth.addVoice(new PolySynthVoice(48)); 
    for(int i=0; i<8; ++i) synth.addVoice(new PolySynthVoice(60)); 
    for(int i=0; i<4; ++i) synth.addVoice(new PolySynthVoice(72)); 
    for(int i=0; i<4; ++i) synth.addVoice(new VoxStationVoice(voxBuffers)); 
    
    synth.addSound(new CustomSound(36)); synth.addSound(new CustomSound(48));
    synth.addSound(new CustomSound(60)); synth.addSound(new CustomSound(72));
    synth.addSound(new CustomSound(84));

    auto setupS = [this](juce::Slider& s, float min, float max, float def) {
        addAndMakeVisible(s); s.setRange(min, max); s.setValue(def);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    };

    setupS(tempoSlider, 40, 180, 90); setupS(swingSlider, 0, 1, 0);
    setupS(fillSlider, 0, 1, 0.3f); setupS(ghostSlider, 0, 1, 0.2f); setupS(dramaSlider, 0, 0.3f, 0.05f);

    addAndMakeVisible(keySelector); keySelector.addItemList({"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"}, 1); keySelector.setSelectedItemIndex(0);
    addAndMakeVisible(scaleSelector); scaleSelector.addItemList({"Minor","Phrygian","Dorian"}, 1); scaleSelector.setSelectedItemIndex(0);
    addAndMakeVisible(styleSelector); styleSelector.addItemList({"Boom Bap","Trap","Lo-Fi"}, 1); styleSelector.setSelectedItemIndex(0);

    addAndMakeVisible(timelineViewport);
    timelineViewport.setViewedComponent(&timelineContent, false);
    timelineContent.setSize(2000, 180);

    auto addP = [this](juce::TextButton& b, BeatGenerator::SectionType t) {
        addAndMakeVisible(b); b.onClick = [this,t]{ currentArrangement.push_back(t); updateTimeLabel(); };
    };
    addP(addIntroBtn, BeatGenerator::Intro); addP(addVerseBtn, BeatGenerator::Verse);
    addP(addHookBtn, BeatGenerator::Hook); addP(addOutroBtn, BeatGenerator::Outro);
    addAndMakeVisible(clearArrBtn); clearArrBtn.onClick = [this]{ currentArrangement.clear(); currentSong.sections.clear(); refreshTimeline(); updateTimeLabel(); };

    addAndMakeVisible(generateButton);
    generateButton.onClick = [this] {
        if (currentArrangement.empty()) return;
        auto dParams = BeatGenerator::DrumParams{ (float)fillSlider.getValue(), (float)ghostSlider.getValue(), (float)dramaSlider.getValue() };
        currentSong = BeatGenerator::generateSong(60+keySelector.getSelectedItemIndex(), (BeatGenerator::ScaleType)scaleSelector.getSelectedItemIndex(), (BeatGenerator::RhythmStyle)styleSelector.getSelectedItemIndex(), (float)swingSlider.getValue(), dParams, currentKit, currentArrangement);
        updateKitDNA(); refreshTimeline(); updateTimeLabel();
    };

    addAndMakeVisible(playButton);
    playButton.onClick = [this] {
        if (isPlaying) { stopTimer(); isPlaying = false; synth.allNotesOff(0, true); }
        else {
            if (currentSong.sections.empty()) return;
            currentSectionIndex = 0; songStepCounter = 0;
            startTimer(60000/(tempoSlider.getValue()*4)); isPlaying = true;
        }
    };

    addAndMakeVisible(exportWavBtn); exportWavBtn.onClick = [this]{ exportToWav(); };
    addAndMakeVisible(stopRecBtn); stopRecBtn.onClick = [this]{ activeRecordingId = ""; statusLabel.setText("RECORDING STOPPED", juce::dontSendNotification); };

    addAndMakeVisible(timeLabel); addAndMakeVisible(statusLabel);
    addAndMakeVisible(arrangementLabel);
    currentKit = BeatGenerator::generateRandomKit();
    setAudioChannels (1, 2);
}

MainComponent::~MainComponent() { shutdownAudio(); }

void MainComponent::refreshTimeline() {
    sectionViews.clear();
    int x = 10;
    for (int i=0; i < (int)currentSong.sections.size(); ++i) {
        auto* v = new SectionComponent(currentSong.sections[i], 
            [this, i]{
                currentSong.sections[i] = BeatGenerator::generateSingleSection(currentSong, currentSong.sections[i].type);
                refreshTimeline();
            },
            [this, i]{
                activeRecordingId = currentSong.sections[i].id;
                voxBuffers[activeRecordingId].setSize(1, 44100 * 8); 
                writePosition = 0;
                statusLabel.setText("RECORDING VOX FOR PART " + juce::String(i+1), juce::dontSendNotification);
            });
        timelineContent.addAndMakeVisible(v);
        v->setBounds(x, 10, 150, 160);
        sectionViews.add(v);
        x += 160;
    }
    timelineContent.setSize(juce::jmax(1200, x), 180);
}

void MainComponent::updateTimeLabel() {
    int totalSteps = 0;
    for (auto& s : currentSong.sections) totalSteps += s.numBars * 16;
    if (totalSteps == 0) for (auto t : currentArrangement) totalSteps += (t == BeatGenerator::Verse || t == BeatGenerator::Hook) ? 128 : 64;
    float totalSecs = (totalSteps * 60.0f) / (tempoSlider.getValue() * 4.0f);
    int mins = (int)totalSecs / 60; int secs = (int)totalSecs % 60;
    timeLabel.setText("SONG DURATION: " + juce::String(mins) + ":" + (secs < 10 ? "0" : "") + juce::String(secs), juce::dontSendNotification);
}

void MainComponent::updateKitDNA() {
    for (int i=0; i<synth.getNumVoices(); ++i) {
        if (auto* v = synth.getVoice(i)) {
            if (auto* kv = dynamic_cast<KickVoice*>(v)) kv->applyDNA(currentKit.kick);
            if (auto* pv = dynamic_cast<PolySynthVoice*>(v)) {
                if (pv->canPlaySound(synth.getSound(1).get())) pv->applyPatch(currentSong.bassPatch);
                else if (pv->canPlaySound(synth.getSound(2).get())) pv->applyPatch(currentSong.padPatch);
                else if (pv->canPlaySound(synth.getSound(3).get())) pv->applyPatch(currentSong.leadPatch);
            }
        }
    }
}

void MainComponent::timerCallback() {
    if (currentSectionIndex >= (int)currentSong.sections.size()) { stopTimer(); isPlaying = false; synth.allNotesOff(0, true); return; }
    auto& sec = currentSong.sections[currentSectionIndex];
    for (const auto& e : sec.events) {
        if (e.step == songStepCounter) {
            if (e.timeOffset > 0) juce::Timer::callAfterDelay((int)(e.timeOffset*50), [this,e]{ synth.noteOn(1, e.midiNote, e.velocity); });
            else synth.noteOn(1, e.midiNote, e.velocity);
        }
    }
    songStepCounter++;
    if (songStepCounter >= sec.numBars * 16) {
        songStepCounter = 0; currentSectionIndex++;
        if (currentSectionIndex < (int)currentSong.sections.size()) statusLabel.setText("PLAYING: PART " + juce::String(currentSectionIndex+1), juce::dontSendNotification);
    }
}

void MainComponent::prepareToPlay (int, double sR) {
    synth.setCurrentPlaybackSampleRate (sR);
    for (int i=0; i<synth.getNumVoices(); ++i) {
        if (auto* v = synth.getVoice(i)) {
            if (auto* kv = dynamic_cast<KickVoice*>(v)) kv->prepareToPlay(sR, 512, 2);
            if (auto* pv = dynamic_cast<PolySynthVoice*>(v)) pv->prepareToPlay(sR, 512, 2);
            if (auto* vv = dynamic_cast<VoxStationVoice*>(v)) vv->prepareToPlay(sR, 512, 2);
        }
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& b) {
    if (activeRecordingId != "") {
        auto* in = b.buffer->getReadPointer(0, b.startSample);
        auto& buf = voxBuffers[activeRecordingId];
        for (int i=0; i<b.numSamples; ++i) { 
            if (writePosition < buf.getNumSamples()) buf.setSample(0, writePosition++, in[i]); 
            else activeRecordingId = ""; 
        }
    }
    b.clearActiveBufferRegion(); synth.renderNextBlock (*b.buffer, {}, b.startSample, b.numSamples);
}

void MainComponent::paint (juce::Graphics& g) {
    g.fillAll (juce::Colour(0xff0a0a0a)); g.setColour(juce::Colours::orange); g.drawRect(getLocalBounds().reduced(5), 2);
    g.setColour (juce::Colours::white); g.setFont (32.0f);
    g.drawText ("MACHINE LORD RAP STATION", 30, 30, 600, 40, juce::Justification::left);
}

void MainComponent::resized() {
    auto area = getLocalBounds().reduced(30); area.removeFromTop(80);
    timelineViewport.setBounds(area.removeFromTop(200).reduced(10));
    
    auto createPanel = area.removeFromTop(120);
    auto arrBtns = createPanel.removeFromTop(50);
    int bW = arrBtns.getWidth()/6;
    addIntroBtn.setBounds(arrBtns.removeFromLeft(bW).reduced(5));
    addVerseBtn.setBounds(arrBtns.removeFromLeft(bW).reduced(5));
    addHookBtn.setBounds(arrBtns.removeFromLeft(bW).reduced(5));
    addOutroBtn.setBounds(arrBtns.removeFromLeft(bW).reduced(5));
    clearArrBtn.setBounds(arrBtns.removeFromLeft(bW).reduced(5));
    stopRecBtn.setBounds(arrBtns.reduced(5));
    
    generateButton.setBounds(createPanel.reduced(200, 5));

    auto sliderPanel = area.removeFromTop(200);
    int sW = sliderPanel.getWidth()/5;
    tempoSlider.setBounds(sliderPanel.removeFromLeft(sW).reduced(10));
    swingSlider.setBounds(sliderPanel.removeFromLeft(sW).reduced(10));
    fillSlider.setBounds(sliderPanel.removeFromLeft(sW).reduced(10));
    ghostSlider.setBounds(sliderPanel.removeFromLeft(sW).reduced(10));
    dramaSlider.setBounds(sliderPanel.reduced(10));

    playButton.setBounds(area.removeFromBottom(100).reduced(250, 10));
    exportWavBtn.setBounds(area.removeFromBottom(40).reduced(350, 0));
    timeLabel.setBounds(area.removeFromBottom(30));
    statusLabel.setBounds(area.removeFromBottom(30));
}

void MainComponent::exportToWav() {}
void MainComponent::releaseResources() {}

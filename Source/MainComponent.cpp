#include "MainComponent.h"

MainComponent::MainComponent()
{
    setLookAndFeel(&factoryLF);
    setSize (1200, 900);
    recordedVoxBuffer.setSize(1, 44100 * 8);

    // Setup Synth
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

    auto setupL = [this](juce::Label& l, juce::String text) {
        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
    };

    setupS(tempoSlider, 40, 180, 90); setupL(tempoLabel, "BPM");
    setupS(swingSlider, 0, 1, 0); setupL(swingLabel, "SWING");
    setupS(fillSlider, 0, 1, 0.3f); setupL(fillLabel, "FILLS");
    setupS(ghostSlider, 0, 1, 0.2f); setupL(ghostLabel, "GHOSTS");
    setupS(dramaSlider, 0, 0.3f, 0.05f); setupL(dramaLabel, "DRAMA");

    addAndMakeVisible(keySelector); keySelector.addItemList({"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"}, 1); keySelector.setSelectedItemIndex(0); setupL(keyLabel, "KEY");
    addAndMakeVisible(scaleSelector); scaleSelector.addItemList({"Minor","Phrygian","Dorian"}, 1); scaleSelector.setSelectedItemIndex(0); setupL(scaleLabel, "SCALE");
    addAndMakeVisible(styleSelector); styleSelector.addItemList({"Boom Bap","Trap","Lo-Fi"}, 1); styleSelector.setSelectedItemIndex(0); setupL(styleLabel, "STYLE");

    addAndMakeVisible(timelineViewport);
    timelineViewport.setViewedComponent(&timelineContent, false);

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
        statusLabel.setText("FACTORY READY: " + currentSong.name, juce::dontSendNotification);
    };

    addAndMakeVisible(reGenKitBtn);
    reGenKitBtn.onClick = [this] {
        currentKit = BeatGenerator::generateRandomKit();
        updateKitDNA();
        statusLabel.setText("NEW KIT DNA APPLIED", juce::dontSendNotification);
    };

    addAndMakeVisible(playButton);
    playButton.onClick = [this] {
        if (isPlaying) { stopTimer(); isPlaying = false; synth.allNotesOff(0, true); statusLabel.setText("STOPPED", juce::dontSendNotification); }
        else {
            if (currentSong.sections.empty()) return;
            currentSectionIndex = 0; songStepCounter = 0;
            startTimer(60000/(tempoSlider.getValue()*4)); isPlaying = true;
            statusLabel.setText("PLAYING...", juce::dontSendNotification);
        }
    };

    addAndMakeVisible(exportWavBtn); exportWavBtn.onClick = [this]{ exportToWav(); };
    addAndMakeVisible(stopRecBtn); stopRecBtn.onClick = [this]{ activeRecordingId = ""; for(auto* v : sectionViews) v->setRecording(false); statusLabel.setText("REC STOPPED", juce::dontSendNotification); };

    addAndMakeVisible(inputMeter);
    addAndMakeVisible(inputLabel); inputLabel.setText("MIC IN", juce::dontSendNotification);
    addAndMakeVisible(timeLabel); addAndMakeVisible(statusLabel);
    
    currentKit = BeatGenerator::generateRandomKit();
    requestPermissions();
}

MainComponent::~MainComponent() { setLookAndFeel(nullptr); shutdownAudio(); }

void MainComponent::requestPermissions() {
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio)) {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
            [&] (bool granted) { setAudioChannels (granted ? 1 : 0, 2); });
    } else {
        setAudioChannels (1, 2);
    }
}

void MainComponent::refreshTimeline() {
    sectionViews.clear();
    int x = 10;
    for (int i=0; i < (int)currentSong.sections.size(); ++i) {
        auto* v = new SectionComponent(currentSong.sections[i], 
            [this, i]{ currentSong.sections[i] = BeatGenerator::generateSingleSection(currentSong, currentSong.sections[i].type); refreshTimeline(); },
            [this, i]{ 
                activeRecordingId = currentSong.sections[i].id;
                voxBuffers[activeRecordingId].setSize(1, 44100 * 8); 
                writePosition = 0;
                for(auto* sv : sectionViews) sv->setRecording(false);
                sectionViews[i]->setRecording(true);
                statusLabel.setText("RECORDING PART " + juce::String(i+1), juce::dontSendNotification);
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
    timeLabel.setText("DURATION: " + juce::String(mins) + ":" + (secs < 10 ? "0" : "") + juce::String(secs), juce::dontSendNotification);
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
            auto sound = CustomSound(e.midiNote, e.sectionId);
            if (e.timeOffset > 0) juce::Timer::callAfterDelay((int)(e.timeOffset*50), [this,e,sound]{ synth.noteOn(1, e.midiNote, e.velocity); });
            else synth.noteOn(1, e.midiNote, e.velocity);
        }
    }
    songStepCounter++;
    if (songStepCounter >= sec.numBars * 16) {
        songStepCounter = 0; currentSectionIndex++;
        if (currentSectionIndex < (int)currentSong.sections.size()) statusLabel.setText("PART: " + juce::String(currentSectionIndex+1), juce::dontSendNotification);
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
    if (b.buffer->getNumChannels() > 0) {
        auto* in = b.buffer->getReadPointer(0, b.startSample);
        float maxIn = 0;
        for(int i=0; i<b.numSamples; ++i) maxIn = juce::jmax(maxIn, std::abs(in[i]));
        inputMeter.setLevel(maxIn);
        if (activeRecordingId != "") {
            auto& buf = voxBuffers[activeRecordingId];
            for (int i=0; i<b.numSamples; ++i) { 
                if (writePosition < buf.getNumSamples()) buf.setSample(0, writePosition++, in[i]); 
                else { activeRecordingId = ""; for(auto* sv : sectionViews) sv->setRecording(false); }
            }
        }
    }
    b.clearActiveBufferRegion(); synth.renderNextBlock (*b.buffer, {}, b.startSample, b.numSamples);
}

void MainComponent::paint (juce::Graphics& g) {
    g.fillAll (juce::Colour(0xff0a0a0a)); 
    g.setColour(juce::Colours::orange.withAlpha(0.5f)); g.drawRect(getLocalBounds().reduced(5), 2);
    g.setColour (juce::Colours::white); g.setFont (juce::FontOptions(24.0f).withStyle("Bold"));
    g.drawText ("MACHINE LORD RAP STATION", 30, 30, 600, 40, juce::Justification::left);
}

void MainComponent::resized() {
    auto area = getLocalBounds().reduced(30); area.removeFromTop(80);
    
    // 1. TIMELINE
    timelineViewport.setBounds(area.removeFromTop(200).reduced(10));
    area.removeFromTop(20);

    // 2. CONTROL PANEL
    auto topRow = area.removeFromTop(140);
    
    // Selectors & Labels
    auto selectors = topRow.removeFromLeft(topRow.getWidth() * 0.4f);
    int selW = selectors.getWidth() / 3;
    
    auto s1 = selectors.removeFromLeft(selW); styleLabel.setBounds(s1.removeFromTop(25)); styleSelector.setBounds(s1.reduced(5, 15));
    auto s2 = selectors.removeFromLeft(selW); keyLabel.setBounds(s2.removeFromTop(25)); keySelector.setBounds(s2.reduced(5, 15));
    auto s3 = selectors; scaleLabel.setBounds(s3.removeFromTop(25)); scaleSelector.setBounds(s3.reduced(5, 15));

    // Arrangement Buttons
    auto arrRow = topRow.removeFromLeft(topRow.getWidth() * 0.6f);
    int bW = arrRow.getWidth() / 6;
    addIntroBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 30));
    addVerseBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 30));
    addHookBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 30));
    addOutroBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 30));
    clearArrBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 30));
    stopRecBtn.setBounds(arrRow.reduced(5, 30));

    generateButton.setBounds(area.removeFromTop(60).reduced(250, 5));
    area.removeFromTop(20);

    // 3. KNOB LAB
    auto sliderPanel = area.removeFromTop(220);
    int sW = sliderPanel.getWidth() / 6;
    
    auto sl1 = sliderPanel.removeFromLeft(sW); tempoLabel.setBounds(sl1.removeFromTop(25)); tempoSlider.setBounds(sl1.reduced(5));
    auto sl2 = sliderPanel.removeFromLeft(sW); swingLabel.setBounds(sl2.removeFromTop(25)); swingSlider.setBounds(sl2.reduced(5));
    auto sl3 = sliderPanel.removeFromLeft(sW); fillLabel.setBounds(sl3.removeFromTop(25)); fillSlider.setBounds(sl3.reduced(5));
    auto sl4 = sliderPanel.removeFromLeft(sW); ghostLabel.setBounds(sl4.removeFromTop(25)); ghostSlider.setBounds(sl4.reduced(5));
    auto sl5 = sliderPanel.removeFromLeft(sW); dramaLabel.setBounds(sl5.removeFromTop(25)); dramaSlider.setBounds(sl5.reduced(5));
    
    auto sideActions = sliderPanel.reduced(10, 40);
    reGenKitBtn.setBounds(sideActions.removeFromTop(sideActions.getHeight()/2).reduced(0, 10));
    exportWavBtn.setBounds(sideActions.reduced(0, 10));

    // 4. TRANSPORT & METER
    auto bottom = area.removeFromBottom(120);
    playButton.setBounds(bottom.removeFromTop(70).reduced(250, 0));
    
    auto meterArea = bottom.reduced(150, 10);
    inputLabel.setBounds(meterArea.removeFromLeft(60));
    inputMeter.setBounds(meterArea.reduced(0, 10));

    timeLabel.setBounds(30, getHeight() - 40, 200, 30);
    statusLabel.setBounds(getWidth() - 330, getHeight() - 40, 300, 30);
}

void MainComponent::exportToWav() {}
void MainComponent::releaseResources() {}

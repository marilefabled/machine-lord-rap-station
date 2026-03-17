#include "MainComponent.h"

MainComponent::MainComponent()
{
    setLookAndFeel(&factoryLF);
    setSize (1200, 950);
    recordedVoxBuffer.setSize(1, 44100 * 8);

    // Setup Synth
    for(int i=0; i<4; ++i) synth.addVoice(new KickVoice());
    for(int i=0; i<4; ++i) synth.addVoice(new SnareVoice());
    for(int i=0; i<8; ++i) synth.addVoice(new HiHatVoice());
    for(int i=0; i<2; ++i) synth.addVoice(new PolySynthVoice(48)); 
    for(int i=0; i<8; ++i) synth.addVoice(new PolySynthVoice(60)); 
    for(int i=0; i<4; ++i) synth.addVoice(new PolySynthVoice(72)); 
    for(int i=0; i<4; ++i) synth.addVoice(new VoxStationVoice(voxBuffers, currentlyTriggeringId)); 
    
    synth.addSound(new CustomSound(36)); synth.addSound(new CustomSound(38));
    synth.addSound(new CustomSound(39)); synth.addSound(new CustomSound(42));
    synth.addSound(new CustomSound(48)); synth.addSound(new CustomSound(60));
    synth.addSound(new CustomSound(72)); synth.addSound(new CustomSound(84));

    auto setupS = [this](juce::Slider& s, float min, float max, float def) {
        addAndMakeVisible(s); s.setRange(min, max); s.setValue(def);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    };

    auto setupL = [this](juce::Label& l, juce::String text) {
        addAndMakeVisible(l); l.setText(text, juce::dontSendNotification); l.setJustificationType(juce::Justification::centred);
    };

    setupS(tempoSlider, 40, 180, 90); setupL(tempoLabel, "BPM");
    setupS(swingSlider, 0, 1, 0); setupL(swingLabel, "SWING");
    setupS(fillSlider, 0, 1, 0.3f); setupL(fillLabel, "FILLS");
    setupS(ghostSlider, 0, 1, 0.2f); setupL(ghostLabel, "GHOSTS");
    setupS(dramaSlider, 0, 0.3f, 0.05f); setupL(dramaLabel, "DRAMA");

    setupS(voxChopSlider, 0, 1, 0.5f); setupL(voxChopLabel, "VOX CHOP");
    setupS(voxPitchSlider, 0.5f, 2.0f, 1.0f); setupL(voxPitchLabel, "VOX PITCH");
    setupS(voxWetSlider, 0, 1, 0.8f); setupL(voxWetLabel, "VOX WET");

    addAndMakeVisible(keySelector); keySelector.addItemList({"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"}, 1); keySelector.setSelectedItemIndex(0); setupL(keyLabel, "KEY");
    addAndMakeVisible(scaleSelector); scaleSelector.addItemList({"Minor","Phrygian","Dorian"}, 1); scaleSelector.setSelectedItemIndex(0); setupL(scaleLabel, "SCALE");
    addAndMakeVisible(styleSelector); styleSelector.addItemList({"Boom Bap","Trap","Lo-Fi"}, 1); styleSelector.setSelectedItemIndex(0); setupL(styleLabel, "STYLE");
    addAndMakeVisible(voxMangleSelector); voxMangleSelector.addItemList({"Standard", "Chop & Screw", "Stutter", "Double-Up", "Reverse Ghost"}, 1); voxMangleSelector.setSelectedItemIndex(1); setupL(voxMangleLabel, "VOX MANGLE");

    addAndMakeVisible(timelineViewport);
    timelineViewport.setViewedComponent(&timelineContent, false);

    auto addP = [this](juce::TextButton& b, BeatGenerator::SectionType t) {
        addAndMakeVisible(b); b.onClick = [this,t]{ const juce::ScopedLock sl(songLock); currentArrangement.push_back(t); updateTimeLabel(); };
    };
    addP(addIntroBtn, BeatGenerator::Intro); addP(addVerseBtn, BeatGenerator::Verse);
    addP(addHookBtn, BeatGenerator::Hook); addP(addOutroBtn, BeatGenerator::Outro);
    addAndMakeVisible(clearArrBtn); clearArrBtn.onClick = [this]{ const juce::ScopedLock sl(songLock); currentArrangement.clear(); currentSong.sections.clear(); refreshTimeline(); updateTimeLabel(); };

    addAndMakeVisible(generateButton);
    generateButton.onClick = [this] {
        if (currentArrangement.empty()) return;
        const juce::ScopedLock sl(songLock);
        auto dParams = BeatGenerator::DrumParams{ (float)fillSlider.getValue(), (float)ghostSlider.getValue(), (float)dramaSlider.getValue() };
        currentSong = BeatGenerator::generateSong(60+keySelector.getSelectedItemIndex(), (BeatGenerator::ScaleType)scaleSelector.getSelectedItemIndex(), (BeatGenerator::RhythmStyle)styleSelector.getSelectedItemIndex(), (float)swingSlider.getValue(), dParams, currentKit, currentArrangement);
        updateKitDNA(); refreshTimeline(); updateTimeLabel();
        statusLabel.setText("FACTORY READY", juce::dontSendNotification);
    };

    addAndMakeVisible(reGenKitBtn); reGenKitBtn.onClick = [this] { currentKit = BeatGenerator::generateRandomKit(); updateKitDNA(); statusLabel.setText("NEW KIT DNA", juce::dontSendNotification); };
    addAndMakeVisible(settingsBtn); settingsBtn.onClick = [this] { showAudioSettings(); };

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

    addAndMakeVisible(inputMeter); addAndMakeVisible(visualizer);
    addAndMakeVisible(inputLabel); inputLabel.setText("MIC IN", juce::dontSendNotification);
    addAndMakeVisible(timeLabel); addAndMakeVisible(statusLabel);
    
    currentKit = BeatGenerator::generateRandomKit();
    requestPermissions();
}

MainComponent::~MainComponent() { setLookAndFeel(nullptr); shutdownAudio(); }

void MainComponent::showAudioSettings() {
    auto* selector = new juce::AudioDeviceSelectorComponent(deviceManager, 1, 1, 2, 2, false, false, true, false);
    selector->setSize(500, 450);
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "AUDIO SETTINGS";
    options.componentToCentreAround = this;
    options.launchAsync();
}

void MainComponent::requestPermissions() {
    juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
        [&] (bool granted) { if (granted) setAudioChannels (1, 2); });
}

void MainComponent::exportToWav() {
    const juce::ScopedLock sl(songLock);
    if (currentSong.sections.empty()) return;
    statusLabel.setText("RENDERING WAV...", juce::dontSendNotification);
    auto file = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile(currentSong.name + ".wav");
    juce::WavAudioFormat wavFormat;
    if (auto outStream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream())) {
        if (auto writer = std::unique_ptr<juce::AudioFormatWriter>(wavFormat.createWriterFor(outStream.get(), 44100, 2, 24, {}, 0))) {
            outStream.release();
            int stepSize = (int)((60.0f * 44100.0f) / (tempoSlider.getValue() * 4.0f));
            juce::AudioBuffer<float> renderBuffer(2, stepSize);
            synth.allNotesOff(0, false);
            for (auto& section : currentSong.sections) {
                currentlyTriggeringId = section.id;
                for (int step=0; step < section.numBars * 16; ++step) {
                    renderBuffer.clear();
                    // KILL OLD NOTES FIRST
                    synth.allNotesOff(0, false);
                    for (auto& e : section.events) { if (e.step == step) synth.noteOn(1, e.midiNote, e.velocity); }
                    synth.renderNextBlock(renderBuffer, {}, 0, stepSize);
                    writer->writeFromAudioSampleBuffer(renderBuffer, 0, stepSize);
                }
            }
            statusLabel.setText("EXPORTED TO DESKTOP!", juce::dontSendNotification);
        }
    }
}

void MainComponent::refreshTimeline() {
    sectionViews.clear();
    int x = 10;
    for (int i=0; i < (int)currentSong.sections.size(); ++i) {
        auto* v = new SectionComponent(currentSong.sections[i], 
            [this, i]{ const juce::ScopedLock sl(songLock); currentSong.sections[i] = BeatGenerator::generateSingleSection(currentSong, currentSong.sections[i].type); refreshTimeline(); },
            [this, i]{ 
                activeRecordingId = currentSong.sections[i].id;
                voxBuffers[activeRecordingId].setSize(1, 44100 * 8); voxBuffers[activeRecordingId].clear();
                writePosition = 0;
                for(auto* sv : sectionViews) sv->setRecording(false);
                sectionViews[i]->setRecording(true);
                statusLabel.setText("RECORDING VOX...", juce::dontSendNotification);
            },
            [this, i]{ const juce::ScopedLock sl(songLock); BeatGenerator::mutateDrums(currentSong.sections[i], currentSong); });
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
            if (auto* sv = dynamic_cast<SnareVoice*>(v)) sv->applyDNA(currentKit.snare);
            if (auto* hv = dynamic_cast<HiHatVoice*>(v)) hv->applyDNA(currentKit.hats);
            if (auto* pv = dynamic_cast<PolySynthVoice*>(v)) {
                if (pv->canPlaySound(synth.getSound(3).get())) pv->applyPatch(currentSong.bassPatch);
                else if (pv->canPlaySound(synth.getSound(4).get())) pv->applyPatch(currentSong.padPatch);
                else if (pv->canPlaySound(synth.getSound(5).get())) pv->applyPatch(currentSong.leadPatch);
            }
            if (auto* vv = dynamic_cast<VoxStationVoice*>(v)) {
                auto& r = juce::Random::getSystemRandom();
                int tmpl = voxMangleSelector.getSelectedItemIndex();
                vv->setChop(r.nextInt(22050), (tmpl == 1) ? 0.7f : 1.0f * (float)voxPitchSlider.getValue());
            }
        }
    }
}

void MainComponent::timerCallback() {
    const juce::ScopedTryLock sl(songLock);
    if (!sl.isLocked()) return;
    if (currentSectionIndex >= (int)currentSong.sections.size()) { stopTimer(); isPlaying = false; synth.allNotesOff(0, true); return; }
    auto& sec = currentSong.sections[currentSectionIndex];
    currentlyTriggeringId = sec.id;

    // --- CRITICAL: Kill old notes every step for rhythm ---
    synth.allNotesOff(0, false);

    for (const auto& e : sec.events) {
        if (e.step == songStepCounter) {
            if (e.timeOffset > 0) juce::Timer::callAfterDelay((int)(e.timeOffset*50), [this,e]{ synth.noteOn(1, e.midiNote, e.velocity); });
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
    juce::dsp::ProcessSpec spec { sR, 512, 2 };
    reverb.prepare(spec); reverb.setParameters({0.5f, 0.5f, 0.1f, 0.1f, 0.1f, 0.1f});
    for (int i=0; i<synth.getNumVoices(); ++i) {
        if (auto* v = synth.getVoice(i)) {
            if (auto* kv = dynamic_cast<KickVoice*>(v)) kv->prepareToPlay(sR, 512, 2);
            if (auto* sv = dynamic_cast<SnareVoice*>(v)) sv->prepareToPlay(sR, 512, 2);
            if (auto* hv = dynamic_cast<HiHatVoice*>(v)) hv->prepareToPlay(sR, 512, 2);
            if (auto* pv = dynamic_cast<PolySynthVoice*>(v)) pv->prepareToPlay(sR, 512, 2);
            if (auto* vv = dynamic_cast<VoxStationVoice*>(v)) vv->prepareToPlay(sR, 512, 2);
        }
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& b) {
    if (b.buffer->getNumChannels() > 0) {
        auto* in = b.buffer->getReadPointer(0, b.startSample);
        float maxIn = 0;
        for(int i=0; i<b.numSamples; ++i) {
            float smp = std::abs(in[i]);
            maxIn = juce::jmax(maxIn, smp);
            // RECORDING PRE-AMP BOOST
            if (activeRecordingId != "") {
                auto& buf = voxBuffers[activeRecordingId];
                if (writePosition < buf.getNumSamples()) 
                    buf.setSample(0, writePosition++, in[i] * 2.0f); // 2x boost
                else { activeRecordingId = ""; }
            }
        }
        inputMeter.setLevel(maxIn * 4.0f); // High visibility
    }
    b.clearActiveBufferRegion(); synth.renderNextBlock (*b.buffer, {}, b.startSample, b.numSamples);
    juce::dsp::AudioBlock<float> block(*b.buffer);
    reverb.process(juce::dsp::ProcessContextReplacing<float>(block));
    visualizer.pushBuffer(*b.buffer);
}

void MainComponent::paint (juce::Graphics& g) {
    g.fillAll (juce::Colour(0xff0a0a0a)); g.setColour(juce::Colours::orange.withAlpha(0.5f)); g.drawRect(getLocalBounds().reduced(5), 2);
    g.setColour (juce::Colours::white); g.setFont (juce::FontOptions(24.0f).withStyle("Bold"));
    g.drawText ("MACHINE LORD RAP STATION", 30, 30, 600, 40, juce::Justification::left);
}

void MainComponent::resized() {
    auto area = getLocalBounds().reduced(30); area.removeFromTop(80);
    visualizer.setBounds(area.removeFromTop(60).reduced(10));
    timelineViewport.setBounds(area.removeFromTop(200).reduced(10));
    area.removeFromTop(10);
    auto controlRow = area.removeFromTop(130);
    auto selectors = controlRow.removeFromLeft(controlRow.getWidth() * 0.45f);
    int selW = selectors.getWidth() / 4;
    auto s1 = selectors.removeFromLeft(selW); styleLabel.setBounds(s1.removeFromTop(25)); styleSelector.setBounds(s1.reduced(5, 10));
    auto s2 = selectors.removeFromLeft(selW); keyLabel.setBounds(s2.removeFromTop(25)); keySelector.setBounds(s2.reduced(5, 10));
    auto s3 = selectors.removeFromLeft(selW); scaleLabel.setBounds(s3.removeFromTop(25)); scaleSelector.setBounds(s3.reduced(5, 10));
    auto s4 = selectors; voxMangleLabel.setBounds(s4.removeFromTop(25)); voxMangleSelector.setBounds(s4.reduced(5, 10));
    auto arrRow = controlRow;
    int bW = arrRow.getWidth() / 6;
    addIntroBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 25)); addVerseBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 25));
    addHookBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 25)); addOutroBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 25));
    clearArrBtn.setBounds(arrRow.removeFromLeft(bW).reduced(5, 25)); stopRecBtn.setBounds(arrRow.reduced(5, 25));
    generateButton.setBounds(area.removeFromTop(60).reduced(300, 0));
    area.removeFromTop(10);
    auto sliderPanel = area.removeFromTop(220);
    int sWidth = sliderPanel.getWidth() / 8;
    auto sl1 = sliderPanel.removeFromLeft(sWidth); tempoLabel.setBounds(sl1.removeFromTop(25)); tempoSlider.setBounds(sl1.reduced(5));
    auto sl2 = sliderPanel.removeFromLeft(sWidth); swingLabel.setBounds(sl2.removeFromTop(25)); swingSlider.setBounds(sl2.reduced(5));
    auto sl3 = sliderPanel.removeFromLeft(sWidth); fillLabel.setBounds(sl3.removeFromTop(25)); fillSlider.setBounds(sl3.reduced(5));
    auto sl4 = sliderPanel.removeFromLeft(sWidth); ghostLabel.setBounds(sl4.removeFromTop(25)); ghostSlider.setBounds(sl4.reduced(5));
    auto sl5 = sliderPanel.removeFromLeft(sWidth); dramaLabel.setBounds(sl5.removeFromTop(25)); dramaSlider.setBounds(sl5.reduced(5));
    auto sl6 = sliderPanel.removeFromLeft(sWidth); voxChopLabel.setBounds(sl6.removeFromTop(25)); voxChopSlider.setBounds(sl6.reduced(5));
    auto sl7 = sliderPanel.removeFromLeft(sWidth); voxPitchLabel.setBounds(sl7.removeFromTop(25)); voxPitchSlider.setBounds(sl7.reduced(5));
    auto sl8 = sliderPanel; voxWetLabel.setBounds(sl8.removeFromTop(25)); voxWetSlider.setBounds(sl8.reduced(5));
    auto bottom = area.removeFromBottom(140);
    playButton.setBounds(bottom.removeFromTop(70).reduced(250, 0));
    auto sideActions = bottom.reduced(50, 10);
    reGenKitBtn.setBounds(sideActions.removeFromLeft(120).reduced(5, 10)); exportWavBtn.setBounds(sideActions.removeFromRight(120).reduced(5, 10));
    settingsBtn.setBounds(sideActions.removeFromLeft(120).reduced(5, 10));
    auto meterArea = sideActions.reduced(50, 5);
    inputLabel.setBounds(meterArea.removeFromLeft(60)); inputMeter.setBounds(meterArea.reduced(0, 10));
    timeLabel.setBounds(30, getHeight() - 40, 200, 30); statusLabel.setBounds(getWidth() - 330, getHeight() - 40, 300, 30);
}

void MainComponent::releaseResources() {}

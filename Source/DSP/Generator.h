#pragma once
#include <JuceHeader.h>
#include "Instruments.h"
#include <vector>

class BeatGenerator
{
public:
    enum RhythmStyle { BoomBap, Trap, LoFi };
    enum ScaleType { Minor, Phrygian, Dorian };
    enum SectionType { Intro, Verse, Hook, Outro };

    struct DrumParams { float fillChance = 0.3f; float ghostNoteProb = 0.2f; float dropNoteProb = 0.05f; };
    struct NoteEvent { int step; int midiNote; float velocity; float timeOffset = 0.0f; juce::String sectionId; };
    struct Section { SectionType type; std::vector<NoteEvent> events; int numBars = 4; juce::String id; };

    struct Song {
        juce::String name; int rootKey; ScaleType scaleType; RhythmStyle style; float swing = 0.0f;
        DrumKitDNA kit; SynthPatch bassPatch, padPatch, leadPatch;
        DrumParams drumParams; std::vector<Section> sections;
    };

    static Song generateSong(int root, ScaleType scale, RhythmStyle style, float swing, DrumParams dParams, DrumKitDNA kit, const std::vector<SectionType>& arrangement)
    {
        Song song; song.name = "Banger_" + juce::String(juce::Time::getCurrentTime().toMilliseconds());
        song.rootKey = root; song.scaleType = scale; song.style = style; song.swing = swing; song.drumParams = dParams; song.kit = kit;
        song.bassPatch = generateRandomPatch("Bass"); song.padPatch = generateRandomPatch("Pad"); song.leadPatch = generateRandomPatch("Lead");
        for (auto type : arrangement) song.sections.push_back(generateSingleSection(song, type));
        return song;
    }

    static Section generateSingleSection(const Song& song, SectionType type) {
        Section s; s.type = type; s.numBars = (type == Verse || type == Hook) ? 8 : 4;
        s.id = "SID_" + juce::String(juce::Random::getSystemRandom().nextInt64());
        auto& r = juce::Random::getSystemRandom();
        std::vector<int> scale; int intervals[] = {0,2,3,5,7,8,10};
        for(int i : intervals) scale.push_back(song.rootKey + i);

        for (int bar = 0; bar < s.numBars; ++bar) {
            int offset = bar * 16;
            for (int step = 0; step < 16; ++step) {
                int gs = offset + step; float sw = (step%2!=0)?song.swing*0.5f:0;
                if (r.nextFloat() < song.drumParams.dropNoteProb) continue;

                if (step == 0 || (song.style == Trap && step == 6)) s.events.push_back({gs, 36, 1.0f, sw, s.id});
                if (step == 4 || step == 12) s.events.push_back({gs, 38, 0.9f, sw, s.id});
                if (step % 2 == 0) s.events.push_back({gs, 42, 0.5f, sw, s.id});

                // Algorithmically matching Vox triggers
                if (type == Hook && step % 8 == 0) s.events.push_back({gs, 84, 0.8f, sw, s.id});
            }
            if (type != Intro) {
                int chordRoot = scale[(bar/2)%3 == 1 ? 3 : ((bar/2)%3 == 2 ? 4 : 0)];
                s.events.push_back({offset, chordRoot - 12, 0.7f, 0, s.id});
                if (type == Hook) {
                    s.events.push_back({offset, chordRoot, 0.4f, 0, s.id});
                    s.events.push_back({offset, chordRoot+3, 0.3f, 0, s.id});
                    s.events.push_back({offset, chordRoot+7, 0.3f, 0, s.id});
                    // Melodic Intelligence: scale-aware lead
                    s.events.push_back({offset+8, scale[r.nextInt(5)] + 12, 0.5f, 0, s.id});
                }
            }
        }
        return s;
    }

    static SynthPatch generateRandomPatch(juce::String type) {
        SynthPatch p; auto& r = juce::Random::getSystemRandom();
        if (type == "Bass") { p.attack = 0.01f; p.decay = 0.3f; p.cutoff = 300; p.oscType = r.nextInt(2); }
        else if (type == "Pad") { p.attack = 0.8f; p.release = 1.5f; p.cutoff = 600; p.oscType = 1; }
        else { p.attack = 0.05f; p.decay = 0.2f; p.cutoff = 2000; p.resonance = 3.0f; p.oscType = r.nextInt(3); }
        p.drive = r.nextFloat()*2+1; return p;
    }

    static DrumKitDNA generateRandomKit() {
        DrumKitDNA dna; auto& r = juce::Random::getSystemRandom();
        dna.kick = { 0.4f, 100, 50, 1.5f };
        dna.snare = { 0.1f, 180, 2000 };
        dna.hats = { 7000, 0.05f };
        return dna;
    }
};

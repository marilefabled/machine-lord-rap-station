cmake_minimum_required(VERSION 3.15)
project(ProceduralBeatMaker VERSION 1.0.0)

# Include the JUCE framework from the Downloads folder
add_subdirectory("/Users/omarilocal/Downloads/JUCE" juce_build)

# Create an audio application
juce_add_gui_app(ProceduralBeatMaker
    PRODUCT_NAME "MACHINE LORD HUMAN BANGER FACTORY"
    VERSION 1.0.0)

# Add our source files
target_sources(ProceduralBeatMaker PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp)

# Compile requirements
target_compile_features(ProceduralBeatMaker PRIVATE cxx_std_14)
target_compile_definitions(ProceduralBeatMaker PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

juce_generate_juce_header(ProceduralBeatMaker)

# Link against JUCE modules (including juce_dsp for synths)
target_link_libraries(ProceduralBeatMaker PRIVATE
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors)
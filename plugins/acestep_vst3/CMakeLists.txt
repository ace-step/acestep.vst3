cmake_minimum_required(VERSION 3.22)

project(ACEStepVST3Shell VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
)

FetchContent_MakeAvailable(juce)

juce_add_plugin(
    acestep_vst3
    COMPANY_NAME "ACE-Step"
    PLUGIN_MANUFACTURER_CODE ACST
    PLUGIN_CODE ASTP
    PRODUCT_NAME "ACE-Step VST3 Shell"
    FORMATS VST3
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument
)

juce_generate_juce_header(acestep_vst3)

target_sources(
    acestep_vst3
    PRIVATE
        src/PluginBackendClient.cpp
        src/CompositionLaneComponent.cpp
        src/PluginEditor.cpp
        src/PluginEditorState.cpp
        src/PluginEditorPreview.cpp
        src/PluginEnums.cpp
        src/PluginPreview.cpp
        src/PluginProcessor.cpp
        src/PluginState.cpp
        src/PreviewDeckComponent.cpp
        src/ResultDeckComponent.cpp
        src/StatusStripComponent.cpp
        src/SynthPanelComponent.cpp
        src/TapeTransportComponent.cpp
        src/V2Chrome.cpp
        src/V2LookAndFeel.cpp
)

target_compile_features(acestep_vst3 PRIVATE cxx_std_20)

target_compile_definitions(
    acestep_vst3
    PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(
    acestep_vst3
    PRIVATE
        juce::juce_audio_utils
)

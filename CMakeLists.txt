cmake_minimum_required(VERSION 3.15)
project(DistancePlugin VERSION 1.0.0)

#Juce Path
add_subdirectory("E:\\University\\Medialogy\\JUCE" build_juce)

# include(FetchContent)
# FetchContent_Declare(JUCE
#     GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
#     GIT_TAG        8.0.8
#     GIT_SHALLOW    TRUE)
# FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DistancePlugin
    COMPANY_NAME            "StefBil"
    PLUGIN_MANUFACTURER_CODE "Stef"
    PLUGIN_CODE             "DstS"
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Distance Space"
    PLUGIN_NAME             "Distance Space"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES         "Fx" "Spatial"
    COPY_PLUGIN_AFTER_BUILD TRUE)
juce_add_binary_data(HrtfBinaryData
    SOURCES
        "HRTF IR/H0e000a_LEFT.wav"
        "HRTF IR/H0e000a_RIGHT.wav")

target_sources(DistancePlugin PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/dsp/ReverbStage.cpp
    src/dsp/DistanceChain.cpp
    src/ui/CueActivityComponent.cpp)
target_compile_definitions(DistancePlugin PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(DistancePlugin PRIVATE
    HrtfBinaryData
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_core
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# if (MSVC)
#     target_compile_options(DistancePlugin PRIVATE
#         /W4 /WX /fp:fast /arch:AVX2 /GL /permissive-)
#     target_link_options(DistancePlugin PRIVATE /LTCG)
# endif()

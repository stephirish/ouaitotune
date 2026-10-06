cmake_minimum_required(VERSION 3.22)
project(Ouaitotune VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

# JUCE: use a local checkout (-DJUCE_DIR=...) or download it
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(OUAI_FORMATS VST3)
if(APPLE)
    list(APPEND OUAI_FORMATS AU)
endif()

juce_add_plugin(Ouaitotune
    COMPANY_NAME "Ouai Stephane"
    BUNDLE_ID "com.ouaistephane.ouaitotune"
    PLUGIN_MANUFACTURER_CODE Ouai
    PLUGIN_CODE Otun
    FORMATS ${OUAI_FORMATS}
    PRODUCT_NAME "Ouaitotune"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Pitch Shift
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Ouaitotune PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

juce_add_binary_data(OuaiAssets SOURCES
    Assets/Ouai_Long_loop.wav
    Assets/face_m0.jpg Assets/face_m1.jpg Assets/face_m2.jpg
    Assets/face_m3.jpg Assets/face_m4.jpg Assets/face_m5.jpg
    Assets/panel.jpg)

target_compile_definitions(Ouaitotune PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(UNIX AND NOT APPLE)
    # lets the plugin build on minimal Linux boxes (only used to check the code compiles)
    target_compile_definitions(Ouaitotune PUBLIC JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(Ouaitotune
    PRIVATE
        OuaiAssets
        juce::juce_audio_processors
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# optional: renders the editor to a PNG (used to check the UI without a DAW)
option(OUAI_TESTS "Build UI snapshot test" OFF)
if(OUAI_TESTS)
    juce_add_console_app(ui_snapshot PRODUCT_NAME "ui_snapshot")
    target_sources(ui_snapshot PRIVATE Tests/ui_snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(ui_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Build_Standalone=0)
    target_link_libraries(ui_snapshot PRIVATE OuaiAssets juce::juce_audio_processors juce::juce_audio_formats)
endif()

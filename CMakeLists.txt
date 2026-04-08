cmake_minimum_required(VERSION 3.22)
project(BZideChannelStrip VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS: Universal binary (Apple Silicon + Intel)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
endif()

# JUCE
add_subdirectory(JUCE)

# B-Zide Channel Strip Plugin
juce_add_plugin(BZideChannelStrip
    COMPANY_NAME "Dawmuze"
    BUNDLE_ID "com.dawmuze.bzide"
    PLUGIN_MANUFACTURER_CODE Dwmz
    PLUGIN_CODE Bzid
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "B-Zide Channel Strip"
    ICON_BIG "${CMAKE_SOURCE_DIR}/Assets/icon.ico"
    ICON_SMALL "${CMAKE_SOURCE_DIR}/Assets/icon.ico"
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE
    VST3_CATEGORIES "Fx" "Channel Strip" "EQ" "Compressor"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Source files
target_sources(BZideChannelStrip PRIVATE
    Source/BZideProcessor.cpp
    Source/BZideEditor.cpp
    Source/DSP/LA2ACompressor.cpp
    Source/UI/VUMeterFace.cpp
    Source/UI/MeterBallistics.cpp
    Source/UI/MeterChannel.cpp
    Source/UI/RotaryKnob.cpp
)

# Binary assets (meter face, knob images)
juce_add_binary_data(BZideAssets SOURCES
    Assets/meter_face_white_hires.png
    Assets/knob.png
    Assets/back.jpg
    Assets/knob_texture.jpg
    Assets/meter_face_gold.png
    Assets/bzide_logo.jpg
    Assets/fader_thumb.jpg
)

target_compile_definitions(BZideChannelStrip PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(BZideChannelStrip PRIVATE
    BZideAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_cryptography
)

# Security flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(BZideChannelStrip PRIVATE -Wall -Wextra)
endif()

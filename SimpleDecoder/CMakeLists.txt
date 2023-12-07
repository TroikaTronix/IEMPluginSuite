set (PLUGIN_TARGET "")

if (IEM_FORMATS)
    juce_add_plugin (SimpleDecoder
        PLUGIN_MANUFACTURER_CODE IEMA
        PLUGIN_CODE SiDe
        ICON_BIG "../resources/Standalone/IEMPluginSuiteSmall.png"
        ICON_SMALL "../resources/Standalone/IEMPluginSuiteSmall.png"
        COMPANY_NAME "IEM"
        PRODUCT_NAME "SimpleDecoder"
        FORMATS ${IEM_FORMATS}
        VERSION ${SimpleDecoder_VERSION}
        LV2URI "http://plugins.iem.at/SimpleDecoder"
        VST2_CATEGORY kPlugSurroundFx kPlugCategSpacializer
        VST3_CATEGORIES Spatial Surround Up-Downmix)
    
    target_compile_definitions (SimpleDecoder PRIVATE
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    list (APPEND PLUGIN_TARGET SimpleDecoder)
endif()
if (IEM_BUILD_AAX)
    juce_add_plugin (SimpleDecoderAAX
        PLUGIN_MANUFACTURER_CODE IEMA
        PLUGIN_CODE SiDe
        ICON_BIG "../resources/Standalone/IEMPluginSuiteSmall.png"
        ICON_SMALL "../resources/Standalone/IEMPluginSuiteSmall.png"
        COMPANY_NAME "IEM"
        PRODUCT_NAME "SimpleDecoder"
        FORMATS AAX
        VERSION ${SimpleDecoder_VERSION}
        AAX_CATEGORY AAX_ePlugInCategory_SoundField)
    
    target_compile_definitions (SimpleDecoderAAX PRIVATE
        JUCE_DISPLAY_SPLASH_SCREEN=1)

    list (APPEND PLUGIN_TARGET SimpleDecoderAAX)
endif()

juce_add_binary_data (SimpleDecoderPresets
    HEADER_NAME "Presets.h"
    NAMESPACE Presets
    SOURCES
    Source/Presets/5point1.json
    Source/Presets/5point1point4.json
    Source/Presets/7point1.json
    Source/Presets/7point1point4.json
    Source/Presets/22_2_NHK.json
    Source/Presets/Cube_8ch.json
    Source/Presets/CUBE.json
    Source/Presets/MSDecoder.json
    Source/Presets/Produktionsstudio.json
    Source/Presets/Quadraphonic.json)

foreach (TARGET IN LISTS PLUGIN_TARGET)
    juce_generate_juce_header(${TARGET})

    target_sources(${TARGET} PRIVATE
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/DecoderInfoBox.h

        ../resources/OSC/OSCInputStream.h
        ../resources/OSC/OSCParameterInterface.cpp
        ../resources/OSC/OSCParameterInterface.h
        ../resources/OSC/OSCStatus.cpp
        ../resources/OSC/OSCStatus.h
        ../resources/OSC/OSCUtilities.h

        ../resources/efficientSHvanilla.cpp
        )

    target_compile_definitions(${TARGET} PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_LAME_AUDIO_FORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_include_directories(${TARGET} PRIVATE Source)

    target_link_libraries(${TARGET} PRIVATE
        LAF_fonts
        IEM_logo
        SimpleDecoderPresets
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_osc
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
endforeach()

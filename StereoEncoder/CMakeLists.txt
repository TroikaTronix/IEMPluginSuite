set (PLUGIN_TARGET "")

if (IEM_FORMATS)
    juce_add_plugin (StereoEncoder
        PLUGIN_MANUFACTURER_CODE IEMA
        PLUGIN_CODE StEn
        ICON_BIG "../resources/Standalone/IEMPluginSuiteSmall.png"
        ICON_SMALL "../resources/Standalone/IEMPluginSuiteSmall.png"
        COMPANY_NAME "IEM"
        PRODUCT_NAME "StereoEncoder"
        FORMATS ${IEM_FORMATS}
        VERSION ${StereoEncoder_VERSION}
        LV2URI "http://plugins.iem.at/StereoEncoder"
        VST2_CATEGORY kPlugSurroundFx kPlugCategSpacializer
        VST3_CATEGORIES Spatial Surround Up-Downmix)

    target_compile_definitions (StereoEncoder PRIVATE
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    list (APPEND PLUGIN_TARGET StereoEncoder)
endif()
if (IEM_BUILD_AAX)
    juce_add_plugin (StereoEncoderAAX
        PLUGIN_MANUFACTURER_CODE IEMA
        PLUGIN_CODE StEn
        ICON_BIG "../resources/Standalone/IEMPluginSuiteSmall.png"
        ICON_SMALL "../resources/Standalone/IEMPluginSuiteSmall.png"
        COMPANY_NAME "IEM"
        PRODUCT_NAME "StereoEncoder"
        FORMATS AAX
        VERSION ${StereoEncoder_VERSION}
        AAX_CATEGORY AAX_ePlugInCategory_SoundField)

    target_compile_definitions (StereoEncoderAAX PRIVATE
        JUCE_DISPLAY_SPLASH_SCREEN=1)

    list (APPEND PLUGIN_TARGET StereoEncoderAAX)
endif()

foreach (TARGET IN LISTS PLUGIN_TARGET)
    juce_generate_juce_header (${TARGET})

    target_sources (${TARGET} PRIVATE
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h

        ../resources/OSC/OSCInputStream.h
        ../resources/OSC/OSCParameterInterface.cpp
        ../resources/OSC/OSCParameterInterface.h
        ../resources/OSC/OSCStatus.cpp
        ../resources/OSC/OSCStatus.h
        ../resources/OSC/OSCUtilities.h

        ../resources/efficientSHvanilla.cpp
        )

    target_compile_definitions (${TARGET} PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_LAME_AUDIO_FORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_include_directories (${TARGET} PRIVATE Source)

    target_link_libraries (${TARGET} PRIVATE
        LAF_fonts
        IEM_logo
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_osc
        juce::juce_opengl
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
endforeach()

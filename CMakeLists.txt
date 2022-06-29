cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 14)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)

set(PROJECT_NAME unity_audio_plugin_demo_juce)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_CXX_FLAGS "/EHsc /MD")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# It's "bad form" to set this globally, but it's convenient.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_definitions(JUCE_MODAL_LOOPS_PERMITTED=1)

project(AudioPluginDemo)

add_subdirectory(helm/concurrentqueue)
add_subdirectory(helm/mopo)

list(APPEND CMAKE_PREFIX_PATH "C:/JUCE/install")
find_package(JUCE CONFIG REQUIRED)

function(juce_add_shared_library target)
    add_library(${target} SHARED
            AudioPluginUtil.cpp
            Plugin_Test.cpp
            )

    target_compile_definitions(${target} PRIVATE JUCE_STANDALONE_APPLICATION=1)
    _juce_initialise_target(${target} ${ARGN})
    _juce_set_output_name(${target} "$<TARGET_PROPERTY:${target},JUCE_PRODUCT_NAME>")
    set_target_properties(${target} PROPERTIES JUCE_TARGET_KIND_STRING "App")
endfunction()

juce_add_shared_library(${PROJECT_NAME}
        VERSION     0.0.1
        )

juce_generate_juce_header(${PROJECT_NAME})

#file(GLOB ${PROJECT_SOURCE_DIR}/helm/src/editor_components
#        "*.cpp"
#        )

target_sources(${PROJECT_NAME} PRIVATE
        Plugin_Test.cpp

#        # Mopo
#        helm/mopo/src/alias.cpp
#        helm/mopo/src/arpeggiator.cpp
#        helm/mopo/src/biquad_filter.cpp
#        helm/mopo/src/bit_crush.cpp
#        helm/mopo/src/bypass_router.cpp
#        helm/mopo/src/delay.cpp
#        helm/mopo/src/distortion.cpp
#        helm/mopo/src/envelope.cpp
#        helm/mopo/src/feedback.cpp
#        helm/mopo/src/formant_manager.cpp
#        helm/mopo/src/ladder_filter.cpp
#        helm/mopo/src/linear_slope.cpp
#        helm/mopo/src/magnitude_lookup.cpp
#        helm/mopo/src/memory.cpp
#        helm/mopo/src/midi_lookup.cpp
#        helm/mopo/src/mono_panner.cpp
#        helm/mopo/src/mopo.h
#        helm/mopo/src/operators.cpp
#        helm/mopo/src/oscillator.cpp
#        helm/mopo/src/portamento_slope.cpp
#        helm/mopo/src/processor.cpp
#        helm/mopo/src/processor_router.cpp
#        helm/mopo/src/resonance_lookup.cpp
#        helm/mopo/src/reverb.cpp
#        helm/mopo/src/reverb_all_pass.cpp
#        helm/mopo/src/reverb_comb.cpp
#        helm/mopo/src/sample_decay_lookup.cpp
#        helm/mopo/src/simple_delay.cpp
#        helm/mopo/src/smooth_filter.cpp
#        helm/mopo/src/smooth_value.cpp
#        helm/mopo/src/state_variable_filter.cpp
#        helm/mopo/src/step_generator.cpp
#        helm/mopo/src/stutter.cpp
#        helm/mopo/src/trigger_operators.cpp
#        helm/mopo/src/value.cpp
#        helm/mopo/src/voice_handler.cpp

        # Helm common
#        helm/src/common/border_bounds_constrainer.cpp
#        helm/src/common/file_list_box_model.cpp
        helm/src/common/helm_common.cpp
        helm/src/common/load_save.cpp
        helm/src/common/midi_manager.cpp
        helm/src/common/startup.cpp
        helm/src/common/synth_base.cpp

        # Helm synthesis
        helm/src/synthesis/dc_filter.cpp
        helm/src/synthesis/detune_lookup.cpp
        helm/src/synthesis/fixed_point_oscillator.cpp
        helm/src/synthesis/fixed_point_wave.cpp
        helm/src/synthesis/gate.cpp
        helm/src/synthesis/helm_engine.cpp
        helm/src/synthesis/helm_lfo.cpp
        helm/src/synthesis/helm_module.cpp
        helm/src/synthesis/helm_oscillators.cpp
        helm/src/synthesis/helm_voice_handler.cpp
        helm/src/synthesis/noise_oscillator.cpp
        helm/src/synthesis/peak_meter.cpp
        helm/src/synthesis/resonance_cancel.cpp
        helm/src/synthesis/trigger_random.cpp
        helm/src/synthesis/value_switch.cpp

        # Plugin
        helm/src/plugin/helm_plugin.cpp
        )

target_include_directories(${PROJECT_NAME} PUBLIC
        helm/src/common
        helm/src/synthesis
        helm/src/plugin
        #        helm/src/look_and_feel
        #        helm/src/editor_sections
        #        helm/src/editor_components
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_ALLOW_STATIC_NULL_VARIABLES=0
        JUCE_LOG_ASSERTIONS=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_USE_XRANDR=0
        )

juce_add_binary_data(
        MyBinaryData SOURCES
        "helm/patches/Factory Presets/Keys/CM Bells.helm"
)

if(${IS_HOLOLENS_2})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/comsuppw.lib)
    target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC\\14.29.30133\\lib\\arm64\\msvcprt.lib")
    target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC\\14.29.30133\\lib\\arm64\\vcruntime.lib")

    target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC\\14.29.30133\\lib\\arm64\\msvcrt.lib")

    #target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.22000.0\\ucrt\\arm64\\libucrt.lib")
    #target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC\\14.29.30133\\lib\\arm64\\libcpmt.lib")
    #target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.22000.0\\ucrt\\arm64\\ucrt.lib")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
        concurrentqueue
        mopo

        MyBinaryData
        juce::juce_analytics
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_box2d
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra

        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags

#        juce::juce_cryptography
#        juce::juce_opengl
#        juce::juce_osc
#        juce::juce_video
#        juce::juce_product_unlocking
        )



if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC winmm.lib )
endif(WIN32)

# Copy to unity plugin dir
if(${IS_HOLOLENS_2})
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "C:/XRMIFrameworkUnity/Assets/Plugins/WSA/ARM64/AudioPluginDemo.dll")
else()
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "C:/XRMIFrameworkUnity/Assets/Plugins/x86_64/AudioPluginDemo.dll")
endif()

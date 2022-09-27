cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 14)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)

set(PROJECT_NAME unity_audio_plugin_demo_juce)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
if(${IS_DEBUG})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MDd")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MD")
endif()

add_compile_definitions(JUCE_MODAL_LOOPS_PERMITTED=1)

project(AudioPluginDemo)

if(${IS_HOLOLENS_2})
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_M_ARM64 /D__arm64__ /D__ARM_NEON /Zc:arm64-aliased-neon-types-")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D__ARM_NEON /Zc:arm64-aliased-neon-types-")
else()
    #ADD_COMPILE_DEFINITIONS(/arch:SSE2)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D__SSE2__")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D__SSE2__")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# It's "bad form" to set this globally, but it's convenient.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

list(APPEND CMAKE_PREFIX_PATH "C:/JUCE/install")
find_package(JUCE CONFIG REQUIRED)

include(FetchContent)

FetchContent_Declare(
        readerwriterqueue
        GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
        GIT_TAG           master
)

FetchContent_MakeAvailable(readerwriterqueue)

function(juce_add_shared_library target)
    add_library(${target} SHARED
            AudioPluginUtil.cpp
            Plugin_Test_Vital.cpp
            )

    target_compile_definitions(${target} PRIVATE JUCE_STANDALONE_APPLICATION=1)
    _juce_initialise_target(${target} ${ARGN})
    _juce_set_output_name(${target} "$<TARGET_PROPERTY:${target},JUCE_PRODUCT_NAME>")
    set_target_properties(${target} PROPERTIES JUCE_TARGET_KIND_STRING "App")
endfunction()

juce_add_shared_library(${PROJECT_NAME}
        VERSION     1.0.7
        )

juce_generate_juce_header(${PROJECT_NAME})

include_directories(${PROJECT_SOURCE_DIR}/vital/src/common)

file(GLOB COMMON_SRC ${PROJECT_SOURCE_DIR}/vital/src/common/*.cpp)
file(GLOB COMMON_SRC_H ${PROJECT_SOURCE_DIR}/vital/src/common/*.h)

list(REMOVE_ITEM COMMON_SRC "${PROJECT_SOURCE_DIR}/vital/src/common/border_bounds_constrainer.cpp")
#list(REMOVE_ITEM COMMON_SRC "${PROJECT_SOURCE_DIR}/vital/src/common/line_generator.cpp")
list(REMOVE_ITEM COMMON_SRC_H "${PROJECT_SOURCE_DIR}/vital/src/common/border_bounds_constrainer.h")
#list(REMOVE_ITEM COMMON_SRC_H "${PROJECT_SOURCE_DIR}/vital/src/common/line_generator.h")

file(GLOB WAVETABLE_SRC ${PROJECT_SOURCE_DIR}/vital/src/common/wavetable/*.cpp)

file(GLOB_RECURSE SYNTHESIS_SRC ${PROJECT_SOURCE_DIR}/vital/src/synthesis/*.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
        vital/third_party/kissfft
        vital/third_party/json
        vital/third_party/concurrentqueue
        COMMON_SRC_H
        vital/src/common/wavetable
        vital/src/plugin
        vital/src/synthesis
        vital/src/synthesis/lookups
        vital/src/synthesis/effects
        vital/src/synthesis/effects_engine
        vital/src/synthesis/filters
        vital/src/synthesis/framework
        vital/src/synthesis/modulators
        vital/src/synthesis/modules
        vital/src/synthesis/producers
        vital/src/synthesis/synth_engine
        vital/src/synthesis/utilities
        vital/src/interface/look_and_feel

        )

target_sources(${PROJECT_NAME} PRIVATE
        vital/src/synthesis/lookups/memory.h
        vital/third_party/kissfft/kissfft.h
        vital/third_party/concurrentqueue/concurrentqueue.h
        vital/third_party/json/json.h
        ${COMMON_SRC}
        ${WAVETABLE_SRC}
        vital/src/plugin/synth_plugin.cpp
        ${SYNTHESIS_SRC}
        vital/src/synthesis/framework/poly_values.h
        vital/src/interface/look_and_feel/synth_strings.h
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



target_link_libraries(${PROJECT_NAME} PUBLIC
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
#        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra

        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags

        readerwriterqueue
        )


if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC winmm.lib )
endif(WIN32)

if(${IS_HOLOLENS_2})
    if(${IS_DEBUG})
        target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.32.31326\\lib\\arm64\\comsuppwd.lib")
        target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.32.31326\\lib\\arm64\\msvcprtd.lib")
        target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.32.31326\\lib\\arm64\\vcruntimed.lib")
        target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.32.31326\\lib\\arm64\\msvcrtd.lib")
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.32.31326\\lib\\arm64\\comsuppw.lib")
        target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.32.31326\\lib\\arm64\\msvcprt.lib")
        target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.32.31326\\lib\\arm64\\vcruntime.lib")
        target_link_libraries(${PROJECT_NAME} PUBLIC "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.32.31326\\lib\\arm64\\msvcrt.lib")
    endif()
endif()

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

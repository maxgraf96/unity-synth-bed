//
// Created by Max on 27/06/2022.
//

#pragma once

//#include <readerwriterqueue.h>
#include "AudioPluginInterface.h"
#include "AudioPluginUtil.h"
#include "synth_plugin.h"
#include "load_save.h"

using namespace std;

namespace JuceSynthVital {
    enum Param {
        P_FREQ,
        P_MIX,
        P_NUM
    };

    unique_ptr<AudioBuffer<float>> g_juceBuffer;
    unique_ptr<MidiBuffer> g_midiReadBuffer;
    unique_ptr<MidiBuffer> g_midiWriteBuffer;
    unique_ptr<SynthPlugin> g_vitalPlugin;
    Array<File> presets_;
    CriticalSection lock;
//    moodycamel::ReaderWriterQueue<MidiMessage> queue(100);

    struct EffectData {
        float p[P_NUM]; // Parameters
    };

    inline uint8 initialByte (const int type, const int channel) noexcept
    {
        return (uint8) (type | jlimit (0, 15, channel - 1));
    }

    int InternalRegisterEffectDefinition(UnityAudioEffectDefinition &definition) {
        int numparams = P_NUM;
        definition.paramdefs = new UnityAudioParameterDefinition[numparams];
        RegisterParameter(definition, "Frequency", "Hz",
                          0.0f, kMaxSampleRate, 1000.0f,
                          1.0f, 3.0f,
                          P_FREQ);
        RegisterParameter(definition, "Mix amount", "%",
                          0.0f, 1.0f, 0.5f,
                          100.0f, 1.0f,
                          P_MIX);
        return numparams;
    }

    void swapMidiBuffers(){
        const ScopedLock sl(lock);

        // Clear read buffer
        g_midiReadBuffer->clear();
        // Swap read with write buffer
        g_midiReadBuffer->swapWith(*g_midiWriteBuffer);
    }

    // ------------------------------------------------ Main ------------------------------------------------
    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(
            UnityAudioEffectState *state,
            float *inbuffer,
            float *outbuffer,
            unsigned int length,
            int inchannels,
            int outchannels) {
        // ---------------------------- AUDIO ----------------------------
        if (state->flags & (UnityAudioEffectStateFlags_IsMuted | UnityAudioEffectStateFlags_IsPaused))
            return UNITY_AUDIODSP_OK;

        swapMidiBuffers();

//        g_midiReadBuffer->clear();
//        bool popSuccess = true;
//        while(popSuccess){
//            MidiMessage current;
//            popSuccess = queue.try_dequeue(current);
//            if(popSuccess)
//                g_midiReadBuffer->addEvent(current, 0);
//        }

        // Process audio and MIDI read buffer
        g_vitalPlugin->myProcessBlock(*g_juceBuffer, *g_midiReadBuffer);

        auto leftReader = g_juceBuffer->getReadPointer(0);
        auto rightReader = g_juceBuffer->getReadPointer(1);

        for (auto sample = 0; sample < g_juceBuffer->getNumSamples(); sample++)
        {
            outbuffer[sample * outchannels + 0] = leftReader[sample];
            outbuffer[sample * outchannels + 1] = rightReader[sample];
        }


        return UNITY_AUDIODSP_OK;
    }
    // ------------------------------------------------ </Main> ---------------------------------------------

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(
            UnityAudioEffectState *state) {

        EffectData *effectdata = new EffectData;
        state->effectdata = effectdata;

        g_vitalPlugin = make_unique<SynthPlugin>();
        g_juceBuffer = make_unique<AudioBuffer<float>>(2, state->dspbuffersize);
        g_midiReadBuffer = make_unique<MidiBuffer>();
        g_midiWriteBuffer = make_unique<MidiBuffer>();

        g_vitalPlugin->prepareToPlay(static_cast<double>(state->samplerate), state->dspbuffersize);

        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback(
            UnityAudioEffectState *state) {
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT
    UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback(UnityAudioEffectState *state, int index, float value) {
        EffectData *data = state->GetEffectData<EffectData>();
        if (index >= P_NUM)
            return UNITY_AUDIODSP_ERR_UNSUPPORTED;

        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT
    UNITY_AUDIODSP_CALLBACK GetFloatParameterCallback(UnityAudioEffectState *state, int index, float *value,
                                                      char *valuestr) {
        return UNITY_AUDIODSP_OK;
    }

    int UNITY_AUDIODSP_CALLBACK GetFloatBufferCallback(UnityAudioEffectState *state, const char *name, float *buffer,
                                                       int numsamples) {
        return UNITY_AUDIODSP_OK;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_UpdateConfig(int bufferSize, int sampleRate) {
        g_vitalPlugin->pauseProcessing(true);
        g_juceBuffer->setSize(2, bufferSize);
        g_vitalPlugin->prepareToPlay(sampleRate, bufferSize);
        g_vitalPlugin->pauseProcessing(false);
    }

    std::string patchStr;
    std::string error;

    extern "C" UNITY_AUDIODSP_EXPORT_API const char* Synthesizer_GetAllPatches(){
        patchStr = "";
        LoadSave::getAllPresets(presets_);
        for (auto && patch : presets_){
            patchStr += patch.getFullPathName().toStdString() + ";";
        }

        return patchStr.data();
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API const char* Synthesizer_LoadPatch(int index){
        error = "";
        File file = presets_[index];
        g_vitalPlugin->pauseProcessing(true);
        g_vitalPlugin->getEngine()->allSoundsOff();
        g_vitalPlugin->loadFromFile(file, error);
        g_vitalPlugin->pauseProcessing(false);

        return error.data();
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_SetStreamingAssetsPath(const char* dataPathStr)
    {
        LoadSave::UnityStreamingAssetsPath = std::string(dataPathStr);
        // Reload patches
        LoadSave::getAllPresets(presets_);
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_SetLogPath(const char* consoleLogDir){
        std::string logDir = std::string(consoleLogDir);
        juce::Logger::setCurrentLogger(FileLogger::createDateStampedLogger (logDir, "VitalLog", ".txt", "Hello"));
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API int Synthesizer_AddMessage(int note, int channel, bool isNoteOn, float velocity) {
        const ScopedLock sl(lock);

        if(isNoteOn){
//            queue.enqueue(juce::MidiMessage::noteOn(channel, note, velocity));
            g_midiWriteBuffer->addEvent(juce::MidiMessage::noteOn(channel, note, velocity), 0);
        } else {
            // lift velocity!!!
//            queue.enqueue(juce::MidiMessage::noteOff(channel, note, velocity));
            g_midiWriteBuffer->addEvent(juce::MidiMessage::noteOff(channel, note, velocity), 0);
        }
        return 200;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API int Synthesizer_AllNotesOff() {
        const ScopedLock sl(lock);
        for(int channel = 1; channel < 17; channel++){
//            queue.enqueue(juce::MidiMessage::allNotesOff(channel));
            g_midiWriteBuffer->addEvent(juce::MidiMessage::allNotesOff(channel), 0);
        }
        return 200;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_PitchBendMessage(int channel, int bend){
        const ScopedLock sl(lock);
        g_midiWriteBuffer->addEvent(juce::MidiMessage::pitchWheel(channel, bend), 0);
//        queue.enqueue(juce::MidiMessage::pitchWheel(channel, bend));
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_ChannelAfterTouchMessage(int channel, int note, int value){
        const ScopedLock sl(lock);
        g_midiWriteBuffer->addEvent(juce::MidiMessage::aftertouchChange(channel, note, value), 0);
//        queue.enqueue(juce::MidiMessage::aftertouchChange(channel, note, value));
    }
}
//
// Created by Max on 27/06/2022.
//

#pragma once

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

    struct EffectData {
        float p[P_NUM]; // Parameters
    };

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

        // Clear read buffer
        g_midiReadBuffer->clear();
        // Swap read with write buffer
        g_midiReadBuffer->swapWith(*g_midiWriteBuffer);

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

        g_juceBuffer = make_unique<AudioBuffer<float>>(2, state->dspbuffersize);
        g_vitalPlugin = make_unique<SynthPlugin>();
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
        g_vitalPlugin->loadFromFile(file, error);

        return error.data();
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_SetStreamingAssetsPath(const char* dataPathStr)
    {
        LoadSave::UnityStreamingAssetsPath = std::string(dataPathStr);
        // Reload patches
        LoadSave::getAllPresets(presets_);
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API int Synthesizer_AddMessage(int note, int channel, bool isNoteOn) {
        if(isNoteOn){
            g_midiWriteBuffer->addEvent(juce::MidiMessage::noteOn(channel, note, 0.5f), 0);
        } else {
            // lift velocity!!!
            float lift = 0.5;
            g_midiWriteBuffer->addEvent(juce::MidiMessage::noteOff(channel, note, lift), 0);
        }
        return 200;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API int Synthesizer_AllNotesOff() {
        for(int channel = 1; channel < 17; channel++){
            g_midiWriteBuffer->addEvent(juce::MidiMessage::allNotesOff(channel), 0);
        }
        return 200;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_PitchBendMessage(int channel, int bend){
        g_midiWriteBuffer->addEvent(juce::MidiMessage::pitchWheel(channel, bend), 0);
//        g_vitalPlugin->getEngine()->setPitchWheel(bend, channel);
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_ChannelAfterTouchMessage(int channel, int note, int value){
        g_midiWriteBuffer->addEvent(juce::MidiMessage::aftertouchChange(channel, note, value), 0);
//        g_vitalPlugin->getEngine()->setChannelAftertouch(channel, value, 0);
    }
}
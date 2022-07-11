//
// Created by Max on 27/06/2022.
//

#pragma once

#include "AudioPluginInterface.h"
#include "AudioPluginUtil.h"
#include "synth_plugin.h"
#include "load_save.h"
#include "sound_engine.h"

namespace JuceSynthVital {
    enum Param {
        P_FREQ,
        P_MIX,
        P_NUM
    };

    std::unique_ptr<AudioBuffer<float>> g_juceBuffer;
    std::unique_ptr<MidiBuffer> g_midiBuffer;
    std::unique_ptr<SynthPlugin> g_vitalPlugin;
    Array<File> presets_;
    Mutex lock;

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

    double currentSampleRate = 0.0, currentAngle = 0.0, angleDelta = 0.0;
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

        // Process audio and MIDI
        g_vitalPlugin->myProcessBlock(*g_juceBuffer, *g_midiBuffer);
        auto level = 0.125f;

//        g_vitalPlugin->processBlock(*g_juceBuffer, *g_midiBuffer);

        auto leftReader = g_juceBuffer->getReadPointer(0);
        auto rightReader = g_juceBuffer->getReadPointer(1);

        for (auto sample = 0; sample < g_juceBuffer->getNumSamples(); sample++)
        {
            outbuffer[sample * outchannels + 0] = leftReader[sample] * level;
            outbuffer[sample * outchannels + 1] = rightReader[sample] * level;
        }

        return UNITY_AUDIODSP_OK;
    }
    // ------------------------------------------------ </Main> ---------------------------------------------

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(
            UnityAudioEffectState *state) {
        g_juceBuffer = std::make_unique<AudioBuffer<float>>(2, state->dspbuffersize);

        EffectData *effectdata = new EffectData;
        state->effectdata = effectdata;

        g_vitalPlugin = std::make_unique<SynthPlugin>();
        g_vitalPlugin->prepareToPlay(static_cast<double>(state->samplerate), state->dspbuffersize);

        g_vitalPlugin->getEngine()->allNotesOff(0);

        g_midiBuffer = std::make_unique<MidiBuffer>();


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

        switch (index) {
            case 0:
//                data->processor->m_frequency = value;
                break;
        }
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

    extern "C" UNITY_AUDIODSP_EXPORT_API int Synthesizer_AddMessage(int note, int channel, bool isNoteOn) {
        if(isNoteOn){
            g_vitalPlugin->getEngine()->noteOn(note, 0.5f, 0, channel);
        } else {
            // lift velocity!!!
            float lift = 0.5;
            g_vitalPlugin->getEngine()->noteOff(note, lift, 0, channel);
        }
        return 200;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API int Synthesizer_AllNotesOff() {
        g_vitalPlugin->getEngine()->allNotesOff(0);

        return 200;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_UpdateConfig(int bufferSize, int sampleRate) {
        do {}
        while (!lock.TryLock());
        g_juceBuffer = std::make_unique<AudioBuffer<float>>(2, bufferSize);
        g_vitalPlugin->prepareToPlay(sampleRate, bufferSize);
        lock.Unlock();
    }

    std::string patchStr;

    extern "C" UNITY_AUDIODSP_EXPORT_API const char* Synthesizer_GetAllPatches(){
        patchStr = "";
        LoadSave::getAllPresets(presets_);
        for (auto && patch : presets_){
            patchStr += patch.getFullPathName().toStdString() + ";";
        }

        return patchStr.data();
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_LoadPatch(int index){
        File file = presets_[index];
        std::string error;
        g_vitalPlugin->loadFromFile(file, error);
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_SetStreamingAssetsPath(const char* dataPathStr)
    {
//        LoadSave::UnityStreamingAssetsPath = std::string(dataPathStr);
//         Reload patches
//        g_vitalPlugin->loadPatches();
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_PitchBendMessage(int channel, double bend){
        g_vitalPlugin->getEngine()->setPitchWheel(bend, channel);
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_ModWheelMessage(int channel, double value){
        g_vitalPlugin->getEngine()->setModWheel(value, channel);
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_ChannelAfterTouchMessage(int channel, double value){
        g_vitalPlugin->getEngine()->setChannelAftertouch(channel, value, 0);
    }
}
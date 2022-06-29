//
// Created by Max on 27/06/2022.
//

#pragma once

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED

#include "AudioPluginInterface.h"
#include "AudioPluginUtil.h"
#include "helm_plugin.h"

namespace MIDI {
    struct MidiEvent {
        int channel;
        int note;
        bool isNoteOn;
    };

    static RingBuffer<8192, MidiEvent> MIDI_DATA;
}

namespace JuceSynth {
    enum Param {
        P_FREQ,
        P_MIX,
        P_NUM
    };

    std::unique_ptr<AudioBuffer<float>> g_juceBuffer;
    std::unique_ptr<MidiBuffer> g_midiBuffer;
    std::unique_ptr<HelmPlugin> g_helmPlugin;
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

//    static void HandleEvent(MIDI::MidiEvent event, int samplenumber) {
//        uint8 velocity = 100;
//        juce::MidiMessage message;
//
//        // Special case: all notes off
//        if (event.note == -1 && event.channel == -1 && !event.isNoteOn) {
//            for (int i = 0; i < 16; i++) {
//                message = juce::MidiMessage::allNotesOff(i);
//                g_midiBuffer->addEvent(message, 0);
//            }
//        }
//
//        if (event.isNoteOn) {
//            message = juce::MidiMessage::noteOn(event.channel, event.note, velocity);
//        } else {
//            message = juce::MidiMessage::noteOff(event.channel, event.note);
//        }
//
//        g_midiBuffer->addEvent(message, 0);
//    }

    double currentSampleRate = 0.0, currentAngle = 0.0, angleDelta = 0.0;
    // ------------------------------------------------ Main ------------------------------------------------
    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(
            UnityAudioEffectState *state,
            float *inbuffer,
            float *outbuffer,
            unsigned int length,
            int inchannels,
            int outchannels) {
        // ---------------------------- MIDI ----------------------------
//        int dspTick = state->currdsptick;
//        MIDI::MidiEvent ev;
//        while (MIDI::MIDI_DATA.Read(ev)) {
//            HandleEvent(ev, dspTick);
//        }

        // ---------------------------- AUDIO ----------------------------
        if (state->flags & (UnityAudioEffectStateFlags_IsMuted | UnityAudioEffectStateFlags_IsPaused))
            return UNITY_AUDIODSP_OK;

        // Process audio and MIDI
        g_helmPlugin->myProcessBlock(*g_juceBuffer, *g_midiBuffer);

//        const auto cyclesPerSample = 500.0 / 44100.0;
//        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;

        auto bufferToFill = *g_juceBuffer;
        auto level = 0.125f;

        assert(g_juceBuffer->getNumSamples() == length);

        for (auto sample = 0; sample < length; ++sample)
        {
//            auto currentSample = (float) std::sin (currentAngle);
//            currentAngle += angleDelta;

            auto currentSample = g_juceBuffer->getSample(0, sample);

            outbuffer[sample * outchannels + 0] = currentSample * level;
            outbuffer[sample * outchannels + 1] = currentSample * level;
        }

        // Write to Unity buffer
//        for (unsigned int n = 0; n < length; n++) {
//            for (int i = 0; i < outchannels; i++) {
//                outbuffer[n * outchannels + i] = g_juceBuffer->getSample(i, n);
//            }
//        }

        return UNITY_AUDIODSP_OK;
    }
    // ------------------------------------------------ </Main> ---------------------------------------------

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(
            UnityAudioEffectState *state) {
        g_juceBuffer = std::make_unique<AudioBuffer<float>>(2, state->dspbuffersize);

        EffectData *effectdata = new EffectData;
        state->effectdata = effectdata;

        g_helmPlugin = std::make_unique<HelmPlugin>();
        g_helmPlugin->prepareToPlay(state->samplerate, state->dspbuffersize);
        g_helmPlugin->loadInitPatch();

        g_helmPlugin->getEngine()->allNotesOff(0);
//        g_helmPlugin->getEngine()->noteOn(60, 0.5f, 0, 2);
//        g_helmPlugin->getEngine()->noteOn(63, 0.5f, 0, 2);
//        g_helmPlugin->getEngine()->noteOn(67, 0.5f, 0, 2);

        g_midiBuffer = std::make_unique<MidiBuffer>();


        // Load init patch

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
            g_helmPlugin->getEngine()->noteOn(note, 0.5f, 0, channel);
        } else {
            g_helmPlugin->getEngine()->noteOff(note, 0);
        }
        return 200;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API int Synthesizer_AllNotesOff() {
        g_helmPlugin->getEngine()->allNotesOff(0);

        return 200;
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_UpdateConfig(int bufferSize, int sampleRate) {
        do {}
        while (!lock.TryLock());
        g_juceBuffer = std::make_unique<AudioBuffer<float>>(2, bufferSize);
        g_helmPlugin->prepareToPlay(sampleRate, bufferSize);
        lock.Unlock();
    }

    extern "C" UNITY_AUDIODSP_EXPORT_API void Synthesizer_GetBufferData(int channel, float buffer[], int numSamples, int numAudioChannels)
    {

    }
}
/*
  ZynAddSubFX - a software synthesizer

  AlsaEngine.h - ALSA Driver
  Copyright (C) 2009 Alan Calvert
  Copyright (C) 2014 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ALSA_ENGINE_H
#define ALSA_ENGINE_H

#include <pthread.h>
#include <string>
#include <alsa/asoundlib.h>
#include <queue>

#include "AudioOut.h"
#include "MidiIn.h"
#include "OutMgr.h"
#include "../Misc/Stereo.h"

#include <jack/midiport.h>
#ifdef JACK_HAS_METADATA_API
# include <jack/metadata.h>
#endif // JACK_HAS_METADATA_API
#include "jack_osc.h"

namespace zyn {

class AlsaEngine:public AudioOut, MidiIn
{
    public:
        AlsaEngine(const SYNTH_T &synth);
        ~AlsaEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;
        void setMidiEn(bool nval);
        bool getMidiEn() const;

    protected:
        void *AudioThread();
        static void *_AudioThread(void *arg);
        void *MidiThread();
        static void *_MidiThread(void *arg);

    private:
        bool openMidi();
        void stopMidi();
        bool openAudio();
        void stopAudio();

        short *interleave(const Stereo<float *> &smps);

        struct {
            std::string device;
            snd_seq_t  *handle;
            snd_seq_port_info_t  *pinfo;
            int alsaId;
            bool exiting;
            pthread_t pThread;
        } midi;

        struct {
            snd_pcm_t *handle;
            snd_pcm_hw_params_t *params;
            snd_pcm_sw_params_t *swparams;
            snd_pcm_status_t    *status;
            snd_pcm_t           *pcm;
            unsigned int        sampleRate;
            snd_pcm_uframes_t   frames;
            unsigned int        periods;
            short    *buffer;
            pthread_t pThread;
            float peaks[1];
        } audio;

        void *processAudio();
        
        unsigned int bufferindex = 0;
        unsigned int sampleoffset = 0;
        unsigned long framecount = 0;

        snd_htimestamp_t htstampnow; 
        snd_htimestamp_t htstamptrigger; 
        snd_htimestamp_t htstampaudio;

        unsigned long tstamptrigger;
        unsigned long ttrigger, tmidi;
        snd_pcm_audio_tstamp_config_t audio_tstamp_config;

};

}

#endif

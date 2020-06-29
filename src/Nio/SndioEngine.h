/*
  ZynAddSubFX - a software synthesizer

  SndioEngine.h - SNDIO Driver
  Copyright (C) 2020 Kinichiro Inoguchi

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef SNDIO_ENGINE_H
#define SNDIO_ENGINE_H

#include <pthread.h>
#include <queue>
#include <sndio.h>
#include <string>

#include "AudioOut.h"
#include "MidiIn.h"
#include "OutMgr.h"
#include "../Misc/Stereo.h"

namespace zyn {

class SndioEngine:public AudioOut, MidiIn
{
    public:
        SndioEngine(const SYNTH_T &synth);
        ~SndioEngine();

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
        bool openAudio();
        void stopAudio();
        bool openMidi();
        void stopMidi();

        void *processAudio();
        void *processMidi();
        short *interleave(const Stereo<float *> &smps);
	void showAudioInfo(struct sio_hdl *handle);

        struct {
            struct sio_hdl *handle;
            struct sio_par params;
            short *buffer;
	    size_t buffer_size;
            pthread_t pThread;
            float peaks[1];
        } audio;

        struct {
            std::string device;
            struct mio_hdl *handle;
            bool exiting;
            pthread_t pThread;
        } midi;
};

}

#endif

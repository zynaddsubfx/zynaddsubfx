/*
  ZynAddSubFX - a software synthesizer

  JackEngine.h - Jack Driver
  Copyright (C) 2009 Alan Calvert
  Copyright (C) 2014 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef JACK_ENGINE_H
#define JACK_ENGINE_H

#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <jack/jack.h>
#include <pthread.h>

#include "MidiIn.h"
#include "AudioOut.h"

namespace zyn {

typedef jack_default_audio_sample_t jsample_t;

class JackEngine:public AudioOut, MidiIn
{
    public:
        JackEngine(const SYNTH_T &synth);
        ~JackEngine() { }

        bool Start();
        void Stop();

        void setMidiEn(bool nval);
        bool getMidiEn() const;

        void setAudioEn(bool nval);
        bool getAudioEn() const;

        int getBuffersize() { return audio.jackNframes; }

        std::string clientName();
        int clientId();

    protected:

        int processCallback(jack_nframes_t nframes);
        static int _processCallback(jack_nframes_t nframes, void *arg);
        int bufferSizeCallback(jack_nframes_t nframes);
        static int _bufferSizeCallback(jack_nframes_t nframes, void *arg);
        static void _errorCallback(const char *msg);
        static void _infoCallback(const char *msg);
        static int _xrunCallback(void *arg);

    private:
        bool connectServer(std::string server);
        bool connectJack();
        void disconnectJack();
        bool openAudio();
        void stopAudio();
        bool processAudio(jack_nframes_t nframes);
        bool openMidi();
        void stopMidi();

        jack_client_t *jackClient;
        struct audio {
            unsigned int jackSamplerate;
            unsigned int jackNframes;
            jack_port_t *ports[2];
            jsample_t   *portBuffs[2];
            float peaks[1];
        } audio;
        struct osc {
            jack_port_t *oscport;
        } osc;
        struct midi {
            jack_port_t *inport;
            bool         jack_sync;
        } midi;

        void handleMidi(unsigned long frames);
};

}

#endif

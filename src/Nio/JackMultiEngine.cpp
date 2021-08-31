/*
  ZynAddSubFX - a software synthesizer

  JackMultiEngine.cpp - Channeled Audio output JACK
  Copyright (C) 2012-2012 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <jack/jack.h>
#include <string>
#include <cstring>
#include <err.h>
#include <cstdio>
#include <cassert>

#include "Nio.h"
#include "Compressor.h"
#include "../Misc/Util.h"
#include "../Misc/Master.h"
#include "../Misc/Part.h"
#include "../Misc/MiddleWare.h"

#include "JackMultiEngine.h"

extern zyn::MiddleWare *middleware;

namespace zyn {

using std::string;

struct jack_multi
{
    jack_port_t *ports[NUM_MIDI_PARTS * 2 + 2];
    float peaks[NUM_MIDI_PARTS + 1];
    jack_client_t *client;
    bool running;
};

JackMultiEngine::JackMultiEngine(const SYNTH_T &synth)
    :AudioOut(synth), impl(new jack_multi())
{
    impl->running = false;
    impl->client  = NULL;
    memset(impl->peaks, 0, sizeof(impl->peaks));

    name = "JACK-MULTI";
}

JackMultiEngine::~JackMultiEngine(void)
{
    delete impl;
}

void JackMultiEngine::setAudioEn(bool nval)
{
    if(nval)
        Start();
    else
        Stop();
}

bool JackMultiEngine::getAudioEn() const
{
    return impl->running;
}



bool JackMultiEngine::Start(void)
{
    if(impl->client)
        return true;

    string clientname = "zynaddsubfx";
    string postfix    = Nio::getPostfix();
    if(!postfix.empty())
        clientname += "_" + postfix;
    if(Nio::pidInClientName)
        clientname += "_" + os_pid_as_padded_string();
    jack_status_t jackstatus;

    impl->client = jack_client_open(clientname.c_str(), JackNullOption, &jackstatus);

    if(!impl->client)
        errx(1, "failed to connect to jack...");


    //Create the set of jack ports
    char portName[20];
    memset(portName,0,sizeof(portName));

#define JACK_REGISTER(x) jack_port_register(impl->client, x, \
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0)
    //Create the master wet port

    impl->ports[0] = JACK_REGISTER("out-L");
    impl->ports[1] = JACK_REGISTER("out-R");

    //Create all part's outputs
    for(int i = 0; i < NUM_MIDI_PARTS * 2; i += 2) {
        snprintf(portName, 19, "part%d/out-L", i / 2);
        impl->ports[2 + i] = JACK_REGISTER(portName);
        snprintf(portName, 19, "part%d/out-R", i / 2);
        impl->ports[3 + i] = JACK_REGISTER(portName);
    }

    //verify that all sample rate and buffer_size are the same in jack.
    //This insures that the connection can be made with no resampling or
    //buffering
    if(synth.samplerate != jack_get_sample_rate(impl->client))
        errx(1, "jack must have the same sample rate!");
    if(synth.buffersize != (int) jack_get_buffer_size(impl->client))
        errx(1, "jack must have the same buffer size");

    jack_set_process_callback(impl->client, _processCallback, this);

    //run
    if(jack_activate(impl->client))
        errx(1, "failed at starting the jack client");
    impl->running = true;
    return true;
}

int JackMultiEngine::_processCallback(jack_nframes_t nframes, void *arg)
{
    return static_cast<JackMultiEngine *>(arg)->processAudio(nframes);
}

int JackMultiEngine::processAudio(jack_nframes_t nframes)
{
    //Gather all buffers
    float *buffers[NUM_MIDI_PARTS * 2 + 2];

    for(int i = 0; i < NUM_MIDI_PARTS * 2 + 2; ++i) {
        //Abort if ports are only partially initialized
        if(!impl->ports[i])
            return false;

        buffers[i] =
            (float *)jack_port_get_buffer(impl->ports[i], nframes);
        assert(buffers[i]);
    }

    //Get the wet samples from OutMgr
    Stereo<float *> smp = getNext();
    memcpy(buffers[0], smp.l, synth.bufferbytes);
    memcpy(buffers[1], smp.r, synth.bufferbytes);

    const int maxFrames = (synth.bufferbytes / sizeof(float));

    //Make sure the audio output doesn't overflow
    if(isOutputCompressionEnabled)
        for(int frame = 0; frame != maxFrames; ++frame) {
            float &p = impl->peaks[0];
            float &l = buffers[0][frame];
            float &r = buffers[1][frame];
            stereoCompressor(synth.samplerate, p, l, r);
        }

    //Gather other samples from individual parts
    Master &master = *middleware->spawnMaster();
    for(int i = 0; i < NUM_MIDI_PARTS; ++i) {
        float &p = impl->peaks[i + 1];

        memcpy(buffers[2*i + 2], master.part[i]->partoutl, synth.bufferbytes);
        memcpy(buffers[2*i + 3], master.part[i]->partoutr, synth.bufferbytes);

        //Make sure the audio output doesn't overflow
        if(isOutputCompressionEnabled)
            for(int frame = 0; frame != maxFrames; ++frame) {
                float &l = buffers[2*i + 2][frame];
                float &r = buffers[2*i + 3][frame];
                stereoCompressor(synth.samplerate, p, l, r);
            }
    }

    return false;
}

void JackMultiEngine::Stop()
{
    for(int i = 0; i < NUM_MIDI_PARTS * 2 + 2; ++i) {
        jack_port_t *port = impl->ports[i];
        impl->ports[i] = NULL;
        if(port)
            jack_port_unregister(impl->client, port);
    }

    if(impl->client)
        jack_client_close(impl->client);
    impl->client = NULL;

    impl->running = false;
}

}

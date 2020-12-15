/*
  ZynAddSubFX - a software synthesizer

  SndioEngine.cpp - SNDIO Driver
  Copyright (C) 2020 Kinichiro Inoguchi

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <iostream>
#include <poll.h>
#include <stdlib.h>

#include "Compressor.h"
#include "InMgr.h"
#include "Nio.h"
#include "SndioEngine.h"
#include "../Misc/Config.h"
#include "../Misc/Util.h"

using namespace std;

namespace zyn {

SndioEngine::SndioEngine(const SYNTH_T &synth)
    :AudioOut(synth)
{
    name = "SNDIO";
    audio.handle = NULL;
    audio.buffer = new short[synth.buffersize * 2];
    audio.buffer_size = synth.buffersize * 2 * sizeof(short);
    audio.peaks[0] = 0;
    audio.pThread = 0;

    midi.handle  = NULL;
    midi.pThread = 0;
}

SndioEngine::~SndioEngine()
{
    Stop();
    delete[] audio.buffer;
}

bool SndioEngine::Start()
{
    return openAudio() && openMidi();
}

void SndioEngine::Stop()
{
    if(getMidiEn())
        setMidiEn(false);
    if(getAudioEn())
        setAudioEn(false);
}

void SndioEngine::setAudioEn(bool nval)
{
    if(nval)
        openAudio();
    else
        stopAudio();
}

bool SndioEngine::getAudioEn() const
{
    return audio.handle;
}

void SndioEngine::setMidiEn(bool nval)
{
    if(nval)
        openMidi();
    else
        stopMidi();
}

bool SndioEngine::getMidiEn() const
{
    return midi.handle;
}

void *SndioEngine::AudioThread()
{
    set_realtime();
    return processAudio();
}

void *SndioEngine::_AudioThread(void *arg)
{
    return (static_cast<SndioEngine *>(arg))->AudioThread();
}

void *SndioEngine::MidiThread(void)
{
    set_realtime();
    return processMidi();
}

void *SndioEngine::_MidiThread(void *arg)
{
    return static_cast<SndioEngine *>(arg)->MidiThread();
}

bool SndioEngine::openAudio()
{
    int rc;

    if(getAudioEn())
        return true;

    audio.handle = NULL;

    if((audio.handle = sio_open(SIO_DEVANY, SIO_PLAY, 0)) == NULL) {
        fprintf(stderr, "unable to open sndio audio device\n");
        return false;
    }

    sio_initpar(&audio.params);
    audio.params.rate = synth.samplerate;
    audio.params.appbufsz = audio.params.rate * 0.05;
    audio.params.xrun = SIO_SYNC;

    rc = sio_setpar(audio.handle, &audio.params);
    if(rc != 1) {
        fprintf(stderr, "unable to set sndio audio parameters");
        return false;
    }

    showAudioInfo(audio.handle);

    rc = sio_start(audio.handle);
    if(rc != 1) {
        fprintf(stderr, "unable to start sndio audio");
        return false;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&audio.pThread, &attr, _AudioThread, this);
    return true;
}

void SndioEngine::stopAudio()
{
    struct sio_hdl *handle = audio.handle;
    int rc;

    if(!getAudioEn())
        return;

    audio.handle = NULL;

    pthread_join(audio.pThread, NULL);

    rc = sio_stop(handle);
    if(rc != 1)
        fprintf(stderr, "unable to stop sndio audio");

    sio_close(handle);
}

bool SndioEngine::openMidi()
{
    if(getMidiEn())
        return true;

    midi.handle = NULL;

    if((midi.handle = mio_open(MIO_PORTANY, MIO_IN, 1)) == NULL) {
        fprintf(stderr, "unable to open sndio midi device\n");
        return false;
    }

    midi.exiting = false;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&midi.pThread, &attr, _MidiThread, this);
    return true;
}

void SndioEngine::stopMidi()
{
    struct mio_hdl *handle = midi.handle;

    if(!getMidiEn())
        return;

    if((midi.handle != NULL) && midi.pThread) {
        midi.exiting = true;
        pthread_join(midi.pThread, 0);
    }

    midi.handle = NULL;

    if(handle)
        mio_close(handle);
}

void *SndioEngine::processAudio()
{
    size_t len;
    struct sio_hdl *handle;

    while(audio.handle) {
        audio.buffer = interleave(getNext());
        handle = audio.handle;
        len = sio_write(handle, audio.buffer, audio.buffer_size);
        if(len == 0) // write error according to sndio examples
            cerr << "sio_write error" << endl;
    }
    return NULL;
}

void *SndioEngine::processMidi()
{
    int n;
    int nfds;
    struct pollfd *pfd;
    int rc;
    int revents;
    size_t len;
    unsigned char buf[3];

    n = mio_nfds(midi.handle);
    if(n <= 0) {
        cerr << "mio_nfds error" << endl;
        return NULL;
    }

    pfd = (struct pollfd *) calloc(n, sizeof(struct pollfd));
    if(pfd == NULL) {
        cerr << "calloc error" << endl;
        return NULL;
    }

    while(1) {
        if(midi.exiting)
            break;

        nfds = mio_pollfd(midi.handle, pfd, POLLIN);

        rc = poll(pfd, nfds, 1000);
        if(rc < 0 && rc != EAGAIN && rc != EINTR) {
            cerr << "poll error" << endl;
            break;
        }

        revents = mio_revents(midi.handle, pfd);
        if(revents & POLLHUP) {
            cerr << "mio_revents catches POLLHUP" << endl;
            continue;
        }
        if(!(revents & POLLIN))
            continue;

        memset(buf, 0, sizeof(buf));
        len = mio_read(midi.handle, buf, sizeof(buf));
        if(len == 0) {
            // since mio_read is non-blocking, this must indicate an error
            // so stop processing all MIDI
            break;
        } else if(len > sizeof(buf)) {
            fprintf(stderr, "mio_read invalid len = %zu\n", len);
            continue;
        }

        midiProcess(buf[0], buf[1], buf[2]);
    }
    free(pfd);
    return NULL;
}

short *SndioEngine::interleave(const Stereo<float *> &smps)
{
    short *shortInterleaved;
    int frame, idx;
    float l, r;
    double scaled;

    shortInterleaved = audio.buffer;
    memset(shortInterleaved, 0, audio.buffer_size);

    for(frame = idx = 0; frame < synth.buffersize; ++frame) {
        l = smps.l[frame];
        r = smps.r[frame];
        stereoCompressor(synth.samplerate, audio.peaks[0], l, r);

        scaled = l * (8.0f * 0x10000000);
        shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
        scaled = r * (8.0f * 0x10000000);
        shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
    }
    return shortInterleaved;
}

void SndioEngine::showAudioInfo(struct sio_hdl *handle)
{
    int rc;
    struct sio_par par;
    struct sio_cap cap;
    unsigned int i;

    rc = sio_getpar(handle, &par);
    if(rc != 1) {
        fprintf(stderr, "unable to get sndio audio parameters");
        return;
    }

    fprintf(stderr, "sndio audio parameters:\n");
    fprintf(stderr,
        "  bits = %u bps = %u sig = %u le = %u msb = %u rchan = %u pchan = %u\n"
        "  rate = %u appbufsz = %u bufsz = %u round = %u xrun = %u\n",
        par.bits, par.bps, par.sig, par.le, par.msb, par.rchan, par.pchan,
        par.rate, par.appbufsz, par.bufsz, par.round, par.xrun);

    rc = sio_getcap(handle, &cap);
    if(rc != 1) {
        fprintf(stderr, "unable to get sndio audio capabilities");
        return;
    }

    fprintf(stderr, "sndio audio capabilities:\n");
    fprintf(stderr, "  supported encodings:\n");
    for(i = 0; i < SIO_NENC; ++i)
        fprintf(stderr,
            "    [%d] bits = %u bps = %u sig = %u le = %u msb = %u\n",
            i, cap.enc[i].bits, cap.enc[i].bps, cap.enc[i].sig,
            cap.enc[i].le, cap.enc[i].msb);

    fprintf(stderr, "  supported channel numbers of recording:\n");
    for(i = 0; i < SIO_NCHAN; ++i)
        fprintf(stderr, "    [%d] rchan = %u\n", i, cap.rchan[i]);

    fprintf(stderr, "  supported channel numbers of playback:\n");
    for(i = 0; i < SIO_NCHAN; ++i)
        fprintf(stderr, "    [%d] pchan = %u\n", i, cap.pchan[i]);

    fprintf(stderr, "  suported sample rates:\n");
    for(i = 0; i < SIO_NRATE; ++i)
        fprintf(stderr, "    [%d] rate = %u\n", i, cap.rate[i]);

    fprintf(stderr, "  available configurations:\n");
    for(i = 0; i < cap.nconf; ++i)
        fprintf(stderr,
            "    [%d] enc = %x rchan = %x pchan = %x rate = %x\n",
            i, cap.confs[i].enc, cap.confs[i].rchan, cap.confs[i].pchan,
            cap.confs[i].rate);
}

}

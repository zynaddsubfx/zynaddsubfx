/*
  ZynAddSubFX - a software synthesizer

  OutMgr.cpp - Audio Output Manager
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "OutMgr.h"
#include <algorithm>
#include <iostream>
#include <cassert>
#include "AudioOut.h"
#include "Engine.h"
#include "EngineMgr.h"
#include "InMgr.h"
#include "WavEngine.h"
#include "../Misc/Master.h"
#include "../Misc/Util.h" //for set_realtime()
using namespace std;

namespace zyn {

OutMgr &OutMgr::getInstance(const SYNTH_T *synth)
{
    static OutMgr instance(synth);
    return instance;
}

#if HAVE_BG_SYNTH_THREAD
void *
OutMgr::_refillThread(void *arg)
{
    return static_cast<OutMgr *>(arg)->refillThread();
}

void *
OutMgr::refillThread()
{
    refillLock();
    while (bgSynthEnabled) {
        refillSmps(stales + synth.buffersize);
        refillWakeup();
        refillWait();
    }
    refillUnlock();
    return 0;
}

void
OutMgr::setBackgroundSynth(bool enable)
{
    void *dummy;

    if (bgSynthEnabled == enable)
        return;
    if (bgSynthEnabled) {
        refillLock();
        bgSynthEnabled = false;
        refillWakeup();
        refillUnlock();

        pthread_join(bgSynthThread, &dummy);
    } else {
        refillLock();
        bgSynthEnabled = true;
        refillUnlock();

        pthread_create(&bgSynthThread, 0, &_refillThread, this);
    }
}
#endif

OutMgr::OutMgr(const SYNTH_T *synth_)
    :wave(new WavEngine(*synth_)),
      priBuf(new float[FRAME_SIZE_MAX],
             new float[FRAME_SIZE_MAX]),
      priBuffCurrent(priBuf), master(NULL), stales(0), synth(*synth_)
{
    assert(synth_);
    currentOut = NULL;

    //init samples
    outr = new float[synth.buffersize];
    outl = new float[synth.buffersize];
    memset(outl, 0, synth.bufferbytes);
    memset(outr, 0, synth.bufferbytes);

#if HAVE_BG_SYNTH_THREAD
    pthread_mutex_init(&bgSynthMtx, 0);
    pthread_cond_init(&bgSynthCond, 0);
    bgSynthEnabled = false;
#endif
    midiFlushOffset = 0;

    /* at any stales value, there should be space for synth.buffersize samples */
    maxStoredSmps = FRAME_SIZE_MAX - (FRAME_SIZE_MAX % synth.buffersize);
    assert(maxStoredSmps > (unsigned int)synth.buffersize);
    maxStoredSmps -= synth.buffersize;
}

OutMgr::~OutMgr()
{
#if HAVE_BG_SYNTH_THREAD
    setBackgroundSynth(false);
#endif

    delete wave;
    delete [] priBuf.l;
    delete [] priBuf.r;
    delete [] outr;
    delete [] outl;
#if HAVE_BG_SYNTH_THREAD
    pthread_cond_destroy(&bgSynthCond);
    pthread_mutex_destroy(&bgSynthMtx);
#endif
}

void OutMgr::refillSmps(unsigned int smpsLimit)
{
    InMgr &midi = InMgr::getInstance();

    while(smpsLimit > curStoredSmps()) {
        refillUnlock();
        if(!midi.empty() &&
           !midi.flush(midiFlushOffset, midiFlushOffset + synth.buffersize)) {
          midiFlushOffset += synth.buffersize;
        } else {
          midiFlushOffset = 0;
        }
        master->AudioOut(outl, outr);
        refillLock();
        addSmps(outl, outr);
    }
}

/* Sequence of a tick
 * 1) Lets remove old/stale samples
 * 2) Apply applicable MIDI events
 * 3) Lets see if we need to generate samples
 * 4) Lets generate some
 * 5) Goto 2 if more are needed
 * 6) Lets return those samples to the primary and secondary outputs
 * 7) Lets wait for another tick
 */
 
 void OutMgr::setMidiParameterFeedbackQueue(RTQueue<std::tuple<char, int, int>, MIDI_QUEUE_LENGTH> *midiQueue)
 {
     master->setMidiParameterFeedbackQueue(midiQueue);
 }
Stereo<float *> OutMgr::tick(unsigned int frameSize)
{
    auto retval = priBuf;
    //SysEv->execute();
    refillLock();
    /* cleanup stales, if any */
    if(frameSize + stales > maxStoredSmps)
        removeStaleSmps();
#if HAVE_BG_SYNTH_THREAD
    /* check if backround sampling is enabled */
    if(bgSynthEnabled) {
        assert(frameSize <= (unsigned int)synth.buffersize);
        /* wait for background samples to complete, if any */
        while(frameSize + stales > curStoredSmps())
            refillWait();
    } else {
#endif
        /* check if drivers ask for too many samples */
        assert(frameSize + stales <= maxStoredSmps);
        /* produce samples foreground, if any */
        refillSmps(frameSize + stales);
#if HAVE_BG_SYNTH_THREAD
    }
#endif
    retval.l += stales;
    retval.r += stales;
    stales += frameSize;
#if HAVE_BG_SYNTH_THREAD
    if(bgSynthEnabled) {
        /* start refill thread again, if any */
        refillWakeup();
    }
#endif
    refillUnlock();
    return retval;
}

AudioOut *OutMgr::getOut(string name)
{
    return dynamic_cast<AudioOut *>(EngineMgr::getInstance().getEng(name));
}

string OutMgr::getDriver() const
{
    return currentOut->name;
}

bool OutMgr::setSink(string name)
{
    AudioOut *sink = getOut(name);

    if(!sink)
        return false;

    if(currentOut)
        currentOut->setAudioEn(false);

    currentOut = sink;
    currentOut->setAudioEn(true);

    bool success = currentOut->getAudioEn();

    //Keep system in a valid state (aka with a running driver)
    if(!success)
        (currentOut = getOut("NULL"))->setAudioEn(true);

    return success;
}

string OutMgr::getSink() const
{
    if(currentOut)
        return currentOut->name;
    else {
        cerr << "BUG: No current output in OutMgr " << __LINE__ << endl;
        return "ERROR";
    }
    return "ERROR";
}

void OutMgr::setAudioCompressor(bool isEnabled)
{
    currentOut->isOutputCompressionEnabled=isEnabled;
}

bool OutMgr::getAudioCompressor(void)
{
    return currentOut->isOutputCompressionEnabled;
}

void OutMgr::setMaster(Master *master_)
{
    master=master_;
}

void OutMgr::applyOscEventRt(const char *msg)
{
    master->applyOscEvent(msg);
}

//perform a cheap linear interpolation for resampling
//This will result in some distortion at frame boundaries
//returns number of samples produced
static size_t resample(float *dest,
                       const float *src,
                       float s_in,
                       float s_out,
                       size_t elms)
{
    size_t out_elms = elms * s_out / s_in;
    float  r_pos    = 0.0f;
    for(int i = 0; i < (int)out_elms; ++i, r_pos += s_in / s_out)
        dest[i] = interpolate(src, elms, r_pos);

    return out_elms;
}

void OutMgr::addSmps(float *l, float *r)
{
    //allow wave file to syphon off stream
    wave->push(Stereo<float *>(l, r), synth.buffersize);

    const int s_out = currentOut->getSampleRate(),
              s_sys = synth.samplerate;

    if(s_out != s_sys) { //we need to resample
        const size_t steps = resample(priBuffCurrent.l,
                                      l,
                                      s_sys,
                                      s_out,
                                      synth.buffersize);
        resample(priBuffCurrent.r, r, s_sys, s_out, synth.buffersize);

        priBuffCurrent.l += steps;
        priBuffCurrent.r += steps;
    }
    else { //just copy the samples
        memcpy(priBuffCurrent.l, l, synth.bufferbytes);
        memcpy(priBuffCurrent.r, r, synth.bufferbytes);
        priBuffCurrent.l += synth.buffersize;
        priBuffCurrent.r += synth.buffersize;
    }
}

void OutMgr::removeStaleSmps()
{
    if(!stales)
        return;

    const int leftover = curStoredSmps() - stales;

    assert(leftover > -1);

    //leftover samples [seen at very low latencies]
    if(leftover) {
        memmove(priBuf.l, priBuffCurrent.l - leftover, leftover * sizeof(float));
        memmove(priBuf.r, priBuffCurrent.r - leftover, leftover * sizeof(float));
        priBuffCurrent.l = priBuf.l + leftover;
        priBuffCurrent.r = priBuf.r + leftover;
    }
    else
        priBuffCurrent = priBuf;

    stales = 0;
}

}

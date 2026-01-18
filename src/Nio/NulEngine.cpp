/*
  ZynAddSubFX - a software synthesizer

  OSSaudiooutput.C - Audio output for Open Sound System
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "NulEngine.h"
#include "../globals.h"

#include <chrono>
#include <iostream>
using namespace std;

namespace zyn {

NulEngine::NulEngine(const SYNTH_T &synth_)
    :AudioOut(synth_)
{
    name = "NULL";
}

void NulEngine::AudioThread()
{
    using duration = chrono::microseconds;
    const duration increase(synth.buffersize * 1'000'000 / synth.samplerate);

    while(thread.joinable()) {
        getNext();

        time_point now = chrono::steady_clock::now();
        if(playing_until == time_point()) {
            playing_until = now;
        }
        else {
            using namespace std::chrono_literals;
            duration remaining = std::chrono::duration_cast<duration>(playing_until - now);
            if(remaining > 10ms) //Don't x() less than 10ms.
                //This will add latency...
                this_thread::sleep_for(remaining  - 10ms);
            else if(remaining < 0us) {
                playing_until -= remaining;
                cerr << "WARNING - too late" << endl;
            }
        }
        playing_until += increase;
    }
}

NulEngine::~NulEngine()
{}

bool NulEngine::Start()
{
    setAudioEn(true);
    return getAudioEn();
}

void NulEngine::Stop()
{
    setAudioEn(false);
}

void NulEngine::setAudioEn(bool nval)
{
    if(nval) {
        if(!getAudioEn()) {
            thread = std::thread(&NulEngine::AudioThread, this);
        }
    }
    else
    if(getAudioEn()) {
        std::thread tmpthread = std::move(thread);
        tmpthread.join();
    }
}

bool NulEngine::getAudioEn() const
{
    return thread.joinable();
}

}

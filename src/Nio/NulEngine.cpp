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
#include "../Misc/Util.h"

#include <chrono>
#include <iostream>
using namespace std;

namespace zyn {

NulEngine::NulEngine(const SYNTH_T &synth_)
    :AudioOut(synth_), running(false)
{
    static_assert(std::atomic<bool>::is_always_lock_free,
        "std::atomic<bool> is not always lock-free on this platform; "
        "NulEngine requires lock-free atomics for safe thread signaling");
    name = "NULL";
}

void NulEngine::AudioThread()
{
    using duration = chrono::microseconds;
    const duration increase(synth.buffersize * 1'000'000 / synth.samplerate);

    while(running.load(std::memory_order_relaxed)) {
        getNext();

        time_point now = chrono::steady_clock::now();
        if(playing_until == time_point()) {
            playing_until = now;
        }
        else {
            using namespace std::chrono_literals;
            duration remaining = std::chrono::duration_cast<duration>(playing_until - now);
            if(remaining > 10ms) //Don't sleep_for() less than 10ms.
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
            running.store(true, std::memory_order_relaxed);
            thread = std::thread(&NulEngine::AudioThread, this);
        }
    }
    else
    if(getAudioEn()) {
        running.store(false, std::memory_order_relaxed);
        thread.join();
    }
}

bool NulEngine::getAudioEn() const
{
    return thread.joinable();
}

}

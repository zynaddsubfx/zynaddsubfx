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

#include <iostream>
#include <thread>
#include <chrono>

using namespace std;
using namespace std::chrono;

namespace zyn {

NulEngine::NulEngine(const SYNTH_T &synth_)
    : AudioOut(synth_)
{
    name = "NULL";
    playing_until = steady_clock::now();
}

void NulEngine::AudioThread()
{
    auto us_per_buffer = static_cast<long long>(synth.buffersize) * 1'000'000 / synth.samplerate;

    while (running.load()) {
        getNext();

        auto now = steady_clock::now();

        // Initialize if first run
        if (playing_until == time_point<steady_clock>{})
            playing_until = now;

        auto remaining = duration_cast<microseconds>(playing_until - now).count();

        if (remaining > 10'000) {
            // Sleep slightly less than the buffer duration
            this_thread::sleep_for(microseconds(remaining - 10'000));
        } else if (remaining < 0) {
            cerr << "WARNING - too late" << endl;
        }

        playing_until += microseconds(us_per_buffer);
    }
}

NulEngine::~NulEngine()
{
    Stop();
}

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
    if (nval) {
        if (!running.load()) {
            running.store(true);
            audio_thread = std::thread(&NulEngine::AudioThread, this);
        }
    } else {
        if (running.load()) {
            running.store(false);
            if (audio_thread.joinable())
                audio_thread.join();
        }
    }
}

bool NulEngine::getAudioEn() const
{
    return running.load();
}

}

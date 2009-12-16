/*
  ZynAddSubFX - a software synthesizer

  OSSaudiooutput.C - Audio output for Open Sound System
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "NulEngine.h"
#include "../globals.h"

#include <iostream>

using namespace std;

NulEngine::NulEngine(OutMgr *out)
    :AudioOut(out)
{
    playing_until.tv_sec  = 0;
    playing_until.tv_usec = 0;
}



void *NulEngine::_AudioThread(void *arg)
{
    return (static_cast<NulEngine*>(arg))->AudioThread();
}


void *NulEngine::AudioThread()
{  
    while (!threadStop())
    {
        const Stereo<Sample> smps = getNext();
        dummyOut();
    }
    return NULL;
}

void NulEngine::dummyOut()
{
    struct timeval now;
    int remaining = 0;
    gettimeofday(&now, NULL);
    if((playing_until.tv_usec == 0) && (playing_until.tv_sec == 0)) {
        playing_until.tv_usec = now.tv_usec;
        playing_until.tv_sec  = now.tv_sec;
    }
    else  {
        remaining = (playing_until.tv_usec - now.tv_usec)
            + (playing_until.tv_sec - now.tv_sec) * 1000000;
        if(remaining > 10000) //Don't sleep() less than 10ms.
            //This will add latency...
            usleep(remaining - 10000);
        if(remaining < 0)
            cerr << "WARNING - too late" << endl;
    }
    playing_until.tv_usec += SOUND_BUFFER_SIZE * 1000000 / SAMPLE_RATE;
    if(remaining < 0)
        playing_until.tv_usec -= remaining;
    playing_until.tv_sec  += playing_until.tv_usec / 1000000;
    playing_until.tv_usec %= 1000000;
    return;
}


NulEngine::~NulEngine()
{
}


bool NulEngine::Start()
{
    pthread_attr_t attr;
    threadStop = false;
    enabled = true;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pThread, &attr, _AudioThread, this);

    return true;
}

void NulEngine::Stop()
{
    threadStop = true;
    enabled    = false;
}


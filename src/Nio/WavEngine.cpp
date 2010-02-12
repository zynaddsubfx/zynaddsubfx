/*
  Copyright (C) 2006 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include "WavEngine.h"
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include "SafeQueue.h"
#include "../Misc/Util.h"

using namespace std;

WavEngine::WavEngine(OutMgr *out, string filename, int samplerate, int channels)
    :AudioOut(out), file(filename, samplerate, channels),
    enabled(false)
{
}

WavEngine::~WavEngine()
{
    Stop();
}

bool WavEngine::openAudio()
{
    return file.good();
}

bool WavEngine::Start()
{
    if(enabled())
        return true;
    pthread_attr_t attr;
    enabled = true;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&pThread, &attr, _AudioThread, this);

    return true;
}

void WavEngine::Stop()
{
    if(!enabled())
        return;
    enabled = false;

    pthread_join(pThread, NULL);
}

void *WavEngine::_AudioThread(void *arg)
{
    return (static_cast<WavEngine*>(arg))->AudioThread();
}

void *WavEngine::AudioThread()
{
    short int *recordbuf_16bit = new short int [SOUND_BUFFER_SIZE*2];
    int size = SOUND_BUFFER_SIZE;


    while (enabled())
    {
        const Stereo<Sample> smps = getNext(true);
        for(int i = 0; i < size; i++) {
            recordbuf_16bit[i*2]   = limit((int)(smps.l()[i] * 32767.0), -32768, 32767);
            recordbuf_16bit[i*2+1] = limit((int)(smps.r()[i] * 32767.0), -32768, 32767);
        }
        file.writeStereoSamples(size, recordbuf_16bit);
    }
    pthread_exit(NULL);
}


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

using namespace std;

WavEngine::WavEngine(OutMgr *out, string filename, int samplerate, int channels)
    :AudioOut(out), file(filename, samplerate, channels)
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

    //put something in the queue
    pthread_mutex_lock(&outBuf_mutex);
    outBuf.push(Stereo<Sample>(Sample(1,0.0),Sample(1,0.0)));
    pthread_mutex_unlock(&outBuf_mutex);

    //make sure it moves
    pthread_cond_signal(&outBuf_cv);
    pthread_mutex_unlock(&outBuf_mutex);
    pthread_join(pThread, NULL);
}

//lazy getter
const Stereo<Sample> WavEngine::getNext()
{
    Stereo<Sample> ans;
    pthread_mutex_lock(&outBuf_mutex);
    bool isEmpty = outBuf.empty();
    pthread_mutex_unlock(&outBuf_mutex);
    if(isEmpty)//wait for samples
    {
        pthread_mutex_lock(&outBuf_mutex);
        pthread_cond_wait(&outBuf_cv, &outBuf_mutex);
        pthread_mutex_unlock(&outBuf_mutex);
    }
    pthread_mutex_lock(&outBuf_mutex);
    ans = outBuf.front();
    outBuf.pop();
    pthread_mutex_unlock(&outBuf_mutex);
    return ans;
}

void *WavEngine::_AudioThread(void *arg)
{
    return (static_cast<WavEngine*>(arg))->AudioThread();
}

template <class T>
T limit(T val, T min, T max)
{
    return (val < min ? min : (val > max ? max : val));
}

void *WavEngine::AudioThread()
{
    short int *recordbuf_16bit = new short int [SOUND_BUFFER_SIZE*2];
    int size = SOUND_BUFFER_SIZE;


    while (enabled())
    {
        const Stereo<Sample> smps = getNext();
        for(int i = 0; i < size; i++) {
            recordbuf_16bit[i*2]   = limit((int)(smps.l()[i] * 32767.0), -32768, 32767);
            recordbuf_16bit[i*2+1] = limit((int)(smps.r()[i] * 32767.0), -32768, 32767);
        }
        file.writeStereoSamples(size, recordbuf_16bit);
    }
    pthread_exit(NULL);
}


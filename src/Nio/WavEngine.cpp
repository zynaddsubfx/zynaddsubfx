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

WavEngine::WavEngine(OutMgr *out, string _filename, int _samplerate, int _channels)
    :AudioOut(out),filename(_filename),sampleswritten(0),
    samplerate(_samplerate),channels(_channels),file(NULL)
{
    pthread_mutex_init(&write_mutex, NULL);
    pthread_cond_init(&stop_cond, NULL);
}


WavEngine::~WavEngine()
{
#warning TODO cleanup mutex
    Close();
}

bool WavEngine::openAudio()
{
    file = fopen(filename.c_str(), "w");
    if(!file)
        return false;
    this->samplerate = samplerate;
    this->channels   = channels;
    sampleswritten   = 0;
    char tmp[44];
    fwrite(tmp, 1, 44, file);
    return true;
}

bool WavEngine::Start()
{
    pthread_attr_t attr;
    enabled = true;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pThread, &attr, _AudioThread, this);

    return true;
}

void WavEngine::Stop()
{
        enabled = false;
}

void WavEngine::Close()
{
    //Stop();

    pthread_mutex_lock(&write_mutex);
    if(file) {
        unsigned int chunksize;
        rewind(file);

        fwrite("RIFF", 4, 1, file);
        chunksize = sampleswritten * 4 + 36;
        fwrite(&chunksize, 4, 1, file);

        fwrite("WAVEfmt ", 8, 1, file);
        chunksize = 16;
        fwrite(&chunksize, 4, 1, file);
        unsigned short int formattag     = 1; //uncompresed wave
        fwrite(&formattag, 2, 1, file);
        unsigned short int nchannels     = channels; //stereo
        fwrite(&nchannels, 2, 1, file);
        unsigned int samplerate_         = samplerate; //samplerate
        fwrite(&samplerate_, 4, 1, file);
        unsigned int bytespersec         = samplerate * 2 * channels; //bytes/sec
        fwrite(&bytespersec, 4, 1, file);
        unsigned short int blockalign    = 2 * channels; //2 channels * 16 bits/8
        fwrite(&blockalign, 2, 1, file);
        unsigned short int bitspersample = 16;
        fwrite(&bitspersample, 2, 1, file);

        fwrite("data", 4, 1, file);
        chunksize = sampleswritten * blockalign;
        fwrite(&chunksize, 4, 1, file);

        fclose(file);
        file = NULL;
    }
    pthread_mutex_unlock(&write_mutex);
    
}

//lazy getter
const Stereo<Sample> WavEngine::getNext()
{
    //TODO make this write remaining sample to output when stopped.
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
    outBuf.pop_front();
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
        write_stereo_samples(size, recordbuf_16bit);
    }
    return NULL;
}

void WavEngine::write_stereo_samples(int nsmps, short int *smps)
{
    pthread_mutex_lock(&write_mutex);
    if(!file)
        return;
    fwrite(smps, nsmps, 4, file);
    pthread_mutex_unlock(&write_mutex);
    sampleswritten += nsmps;
}

void WavEngine::write_mono_samples(int nsmps, short int *smps)
{
    if(!file)
        return;
    fwrite(smps, nsmps, 2, file);
    sampleswritten += nsmps;
}


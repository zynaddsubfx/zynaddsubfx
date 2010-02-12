/*
  ZynAddSubFX - a software synthesizer

  AudioOut.h - Audio Output superclass
  Copyright (C) 2009-2010 Mark McCurry
  Author: Mark McCurry

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

#include <iostream>
#include <cstring>
#include "SafeQueue.h"

using namespace std;

#include "OutMgr.h"
#include "../Misc/Master.h"
#include "AudioOut.h"

struct AudioOut::Data
{
    Data(OutMgr *out);

    int samplerate;
    int bufferSize;

    SafeQueue<Stereo<Sample> >  outBuf;
    pthread_mutex_t outBuf_mutex;
    pthread_cond_t  outBuf_cv;

    /**used for taking in samples with a different
     * samplerate/buffersize*/
    Stereo<Sample> partialIn;
    bool usePartial;
    Stereo<Sample> current;/**<used for xrun defence*/

    //The number of Samples that are used to buffer
    //Note: there is some undetermined behavior when:
    //sampleRate!=SAMPLE_RATE || bufferSize!=SOUND_BUFFER_SIZE
    unsigned int buffering;

    OutMgr *manager;
};

AudioOut::Data::Data(OutMgr *out)
    :samplerate(SAMPLE_RATE),bufferSize(SOUND_BUFFER_SIZE),
     outBuf(100),usePartial(false),
     current(Sample(SOUND_BUFFER_SIZE,0.0)),buffering(6),
     manager(out)
{}

AudioOut::AudioOut(OutMgr *out)
    :dat(new Data(out))
{
    pthread_mutex_init(&dat->outBuf_mutex, NULL);
    pthread_cond_init(&dat->outBuf_cv, NULL);
}

AudioOut::~AudioOut()
{
    pthread_mutex_destroy(&dat->outBuf_mutex);
    pthread_cond_destroy(&dat->outBuf_cv);
    delete dat;
}

void AudioOut::out(Stereo<Sample> smps)
{
    pthread_mutex_lock(&dat->outBuf_mutex);
    if(dat->samplerate != SAMPLE_RATE) { //we need to resample
        smps.l().resample(SAMPLE_RATE,dat->samplerate);
        smps.r().resample(SAMPLE_RATE,dat->samplerate);
    }

    if(dat->usePartial) { //we have a partial to use
        smps.l() = dat->partialIn.l().append(smps.l());
        smps.r() = dat->partialIn.r().append(smps.r());
    }

    if(smps.l().size() == dat->bufferSize) { //sized just right
        dat->outBuf.push(smps);
        dat->usePartial = false;
        pthread_cond_signal(&dat->outBuf_cv);
    }
    else if(smps.l().size() > dat->bufferSize) { //store overflow

        while(smps.l().size() > dat->bufferSize) {
            dat->outBuf.push(Stereo<Sample>(smps.l().subSample(0,dat->bufferSize),
                                            smps.r().subSample(0,dat->bufferSize)));
            smps = Stereo<Sample>(smps.l().subSample(dat->bufferSize,smps.l().size()),
                                  smps.r().subSample(dat->bufferSize,smps.r().size()));
        }

        if(smps.l().size() == dat->bufferSize) { //no partial
            dat->outBuf.push(smps);
            dat->usePartial = false;
        }
        else { //partial
            dat->partialIn = smps;
            dat->usePartial = true;
        }

        pthread_cond_signal(&dat->outBuf_cv);
    }
    else { //underflow
        dat->partialIn = smps;
        dat->usePartial = true;
    }
    pthread_mutex_unlock(&dat->outBuf_mutex);
}

void AudioOut::setSamplerate(int _samplerate)
{
    pthread_mutex_lock(&dat->outBuf_mutex);
    dat->usePartial = false;
    dat->samplerate = _samplerate;
    dat->outBuf.clear();
    pthread_mutex_unlock(&dat->outBuf_mutex);

}

void AudioOut::setBufferSize(int _bufferSize)
{
    pthread_mutex_lock(&dat->outBuf_mutex);
    dat->usePartial = false;
    dat->bufferSize = _bufferSize;
    dat->outBuf.clear();
    pthread_mutex_unlock(&dat->outBuf_mutex);
}

void AudioOut::bufferingSize(int nBuffering)
{
   dat->buffering = nBuffering;
}

int AudioOut::bufferingSize()
{
    return dat->buffering;
}

const Stereo<Sample> AudioOut::getNext(bool wait)
{
    const unsigned int BUFF_SIZE = dat->buffering;
    Stereo<Sample> ans;
    pthread_mutex_lock(&dat->outBuf_mutex);
    bool isEmpty = !dat->outBuf.size();
    pthread_mutex_unlock(&dat->outBuf_mutex);

    if(isEmpty)//fetch samples if possible
    {
        if((unsigned int)dat->manager->getRunning() < BUFF_SIZE)
            dat->manager->requestSamples(BUFF_SIZE-dat->manager->getRunning());
        if(true)
            cout << "-----------------Starvation------------------"<< endl;
        if(wait)
        {
            pthread_mutex_lock(&dat->outBuf_mutex);
            pthread_cond_wait(&dat->outBuf_cv,&dat->outBuf_mutex);
            pthread_mutex_unlock(&dat->outBuf_mutex);
            //now the code has a sample to fetch
        }
        else
            return dat->current;
    }

    pthread_mutex_lock(&dat->outBuf_mutex);
    dat->outBuf.pop(ans);
    if(dat->outBuf.size()+dat->manager->getRunning()<BUFF_SIZE)
        dat->manager->requestSamples(BUFF_SIZE - (dat->outBuf.size()
                    + dat->manager->getRunning()));
    if(false)
        cout << "AudioOut "<< dat->outBuf.size()<< '+' << dat->manager->getRunning() << endl;
    pthread_mutex_unlock(&dat->outBuf_mutex);

    dat->current = ans;
    return ans;
}

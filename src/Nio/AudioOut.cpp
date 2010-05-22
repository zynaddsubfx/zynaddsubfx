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

    Stereo<Sample> current;/**<used for xrun defence*/

    OutMgr *manager;
};

AudioOut::Data::Data(OutMgr *out)
    :samplerate(SAMPLE_RATE),bufferSize(SOUND_BUFFER_SIZE),
     current(Sample(SOUND_BUFFER_SIZE,0.0)),manager(out)
{}

AudioOut::AudioOut(OutMgr *out)
    :dat(new Data(out))
{}

AudioOut::~AudioOut()
{
    delete dat;
}

void AudioOut::setSamplerate(int _samplerate)
{
    dat->samplerate = _samplerate;
}

int AudioOut::getSampleRate()
{
    return dat->samplerate;
}

void AudioOut::setBufferSize(int _bufferSize)
{
    dat->bufferSize = _bufferSize;
}

//delete me
void AudioOut::bufferingSize(int nBuffering)
{
   //dat->buffering = nBuffering;
}

//delete me
int AudioOut::bufferingSize()
{
    //return dat->buffering;
}

const Stereo<Sample> AudioOut::getNext(bool wait)
{
    Stereo<REALTYPE *> tmp = sysOut->tick(dat->bufferSize);

    //stop the samples
    return Stereo<Sample>(Sample(dat->bufferSize, tmp.l()), Sample(dat->bufferSize, tmp.r()));
}

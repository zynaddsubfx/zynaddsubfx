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

AudioOut::AudioOut()
    :samplerate(SAMPLE_RATE),bufferSize(SOUND_BUFFER_SIZE)
{}

AudioOut::~AudioOut()
{
}

void AudioOut::setSamplerate(int _samplerate)
{
    samplerate = _samplerate;
}

int AudioOut::getSampleRate()
{
    return samplerate;
}

void AudioOut::setBufferSize(int _bufferSize)
{
    bufferSize = _bufferSize;
}

//delete me
void AudioOut::bufferingSize(int nBuffering)
{
   //buffering = nBuffering;
}

//delete me
int AudioOut::bufferingSize()
{
    //return buffering;
}

const Stereo<Sample> AudioOut::getNext(bool wait)
{
    Stereo<REALTYPE *> tmp = OutMgr::getInstance().tick(bufferSize);

    //stop the samples
    return Stereo<Sample>(Sample(bufferSize, tmp.l()), Sample(bufferSize, tmp.r()));
}

/*
  ZynAddSubFX - a software synthesizer

  PaEngine.cpp - Audio output for PortAudio
  Copyright (C) 2002 Nasca Octavian Paul
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

#include "PaEngine.h"
#include "../Samples/Sample.h"
#include <iostream>

using namespace std;

PaEngine::PaEngine(OutMgr *out)
    :AudioOut(out)
{
    name = "PA";
}


PaEngine::~PaEngine()
{
    Stop();
}

bool PaEngine::Start()
{
    if(enabled())
        return true;
    enabled = true;
    Pa_Initialize();
    
    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        cerr << "Error: No default output device." << endl;
        Pa_Terminate();
        return false;
    }
    outputParameters.channelCount = 2; /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = 
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;


    Pa_OpenStream(&stream,
                  NULL,
                  &outputParameters,
                  SAMPLE_RATE,
                  SOUND_BUFFER_SIZE,
                  NULL,
                  PAprocess,
                  (void *) this);
    Pa_StartStream(stream);
    return true;
}
        
int PaEngine::PAprocess(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo *outTime, PaStreamCallbackFlags flags,
        void *userData)
{
    return static_cast<PaEngine *>(userData)->process((float *) outputBuffer,
                                                      framesPerBuffer);
}

int PaEngine::process(float *out, unsigned long framesPerBuffer)
{

    const Stereo<Sample> smp = getNext();
    
    if(framesPerBuffer != smp.l().size()) 
        cerr << "Bug: PaEngine::process  SOUND_BUFFER_SIZE!=framesPerBuffer"
             << framesPerBuffer << ' ' << smp.l().size() << endl;

    for(int i = 0; i < framesPerBuffer; i++) {
        if(i >= smp.l().size())
            break;//this should never happens, except only when framesPerBuffer!>SOUND_BUFFER_SIZE
        *out++ = smp.l()[i];
        *out++ = smp.r()[i];
    }

    return 0;
}

void PaEngine::Stop()
{
    if(!enabled())
        return;
    enabled = false;
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}


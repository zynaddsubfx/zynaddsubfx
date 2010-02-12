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

#include "OssEngine.h"
#include "../Misc/Util.h"
#include "../globals.h"

#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

#include "InMgr.h"

using namespace std;

OssEngine::OssEngine(OutMgr *out)
    :AudioOut(out), engThread(NULL)
{
    name = "OSS";

    midi.handle  = -1;
    audio.handle = -1;

    audio.smps = new short[SOUND_BUFFER_SIZE * 2];
    memset(audio.smps, 0, sizeof(short) * SOUND_BUFFER_SIZE * 2);
}

OssEngine::~OssEngine()
{
    Stop();
    delete [] audio.smps;
}

bool OssEngine::openAudio()
{
    if(audio.handle != -1)
        return true; //already open

    int snd_bitsize    = 16;
    int snd_fragment   = 0x00080009; //fragment size (?);
    int snd_stereo     = 1; //stereo;
    int snd_format     = AFMT_S16_LE;
    int snd_samplerate = SAMPLE_RATE;;

    audio.handle = open(config.cfg.LinuxOSSWaveOutDev, O_WRONLY, 0);
    if(audio.handle == -1) {
        cerr << "ERROR - I can't open the "
             << config.cfg.LinuxOSSWaveOutDev << '.' << endl;
        return false;
    }
    ioctl(audio.handle, SNDCTL_DSP_RESET, NULL);
    ioctl(audio.handle, SNDCTL_DSP_SETFMT, &snd_format);
    ioctl(audio.handle, SNDCTL_DSP_STEREO, &snd_stereo);
    ioctl(audio.handle, SNDCTL_DSP_SPEED, &snd_samplerate);
    ioctl(audio.handle, SNDCTL_DSP_SAMPLESIZE, &snd_bitsize);
    ioctl(audio.handle, SNDCTL_DSP_SETFRAGMENT, &snd_fragment);
    
    if(!getMidiEn()) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        engThread = new pthread_t;
        pthread_create(engThread, &attr, _thread, this);
    }

    return true;
}

void OssEngine::stopAudio()
{
    int handle = audio.handle;
    if(handle == -1) //already closed
        return;
    audio.handle = -1;

    if(!getMidiEn())
        pthread_join(*engThread, NULL);
    delete engThread;
    engThread = NULL;
    
    close(handle);
}

bool OssEngine::Start()
{
    bool good = true;

    if(!openAudio()) {
        cerr << "Failed to open OSS audio" << endl;
        good = false;
    }

    if(!openMidi()) {
        cerr << "Failed to open OSS midi" << endl;
        good = false;
    }

    return good;
}

void OssEngine::Stop()
{
    stopAudio();
    stopMidi();
}

void OssEngine::setMidiEn(bool nval)
{
    if(nval)
        openMidi();
    else
        stopMidi();
}

bool OssEngine::getMidiEn() const
{
    return midi.handle != -1;
}

void OssEngine::setAudioEn(bool nval)
{
    if(nval)
        openAudio();
    else
        stopAudio();
}

bool OssEngine::getAudioEn() const
{
    return audio.handle != -1;
}

bool OssEngine::openMidi()
{
    int handle = midi.handle;
    if(handle != -1)
        return true;//already open

    handle = open(config.cfg.LinuxOSSSeqInDev, O_RDONLY, 0);

    if(-1 == handle) {
        return false;
    }
    midi.handle = handle;
   
    if(!getAudioEn()) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        engThread = new pthread_t;
        pthread_create(engThread, &attr, _thread, this);
    }

    return true;
}

void OssEngine::stopMidi()
{
    int handle = midi.handle;
    if(handle == -1) //already closed
        return;

    midi.handle = -1;
    
    if(!getAudioEn()) {
        pthread_join(*engThread, NULL);
        delete engThread;
        engThread = NULL;
    }
    
    close(handle);
}

void *OssEngine::_thread(void *arg)
{
    return (static_cast<OssEngine*>(arg))->thread();
}

void *OssEngine::thread()
{
    unsigned char tmp[4] = {0, 0, 0, 0};
    set_realtime();
    while (getAudioEn() || getMidiEn())
    {
        if(getAudioEn())
        {
            const Stereo<Sample> smps = getNext();

            REALTYPE l, r;
            for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
                l = smps.l()[i];
                r = smps.r()[i];

                if(l < -1.0)
                    l = -1.0;
                else
                    if(l > 1.0)
                        l = 1.0;
                if(r < -1.0)
                    r = -1.0;
                else
                    if(r > 1.0)
                        r = 1.0;

                audio.smps[i * 2]     = (short int) (l * 32767.0);
                audio.smps[i * 2 + 1] = (short int) (r * 32767.0);
            }
            int handle = audio.handle;
            if(handle != -1)
                write(handle, audio.smps, SOUND_BUFFER_SIZE * 4); // *2 because is 16 bit, again * 2 because is stereo
            else
                break;
        }

        //Collect up to 30 midi events
        for (int k = 0; k < 30 && getMidiEn(); ++k) {
            getMidi(tmp);
            unsigned char type = tmp[0];
            unsigned char header = tmp[1];
            if(header!=0xfe&&type==SEQ_MIDIPUTC&&header>=0x80)
            {
                getMidi(tmp);
                unsigned char num = tmp[1];
                getMidi(tmp);
                unsigned char value = tmp[1];

                midiProcess(header, num, value);
            }
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void OssEngine::getMidi(unsigned char *midiPtr)
{
    read(midi.handle, midiPtr, 4);
}


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
    :AudioOut(out)
{
    name = "OSS";

    midi.en  = true;
    audio.en = true;

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
    int snd_bitsize    = 16;
    int snd_fragment   = 0x00080009; //fragment size (?);
    int snd_stereo     = 1; //stereo;
    int snd_format     = AFMT_S16_LE;
    int snd_samplerate = SAMPLE_RATE;;

    audio.snd_handle = open(config.cfg.LinuxOSSWaveOutDev, O_WRONLY, 0);
    if(audio.snd_handle == -1) {
        cerr << "ERROR - I can't open the "
             << config.cfg.LinuxOSSWaveOutDev << '.' << endl;
        return false;
    }
    ioctl(audio.snd_handle, SNDCTL_DSP_RESET, NULL);
    ioctl(audio.snd_handle, SNDCTL_DSP_SETFMT, &snd_format);
    ioctl(audio.snd_handle, SNDCTL_DSP_STEREO, &snd_stereo);
    ioctl(audio.snd_handle, SNDCTL_DSP_SPEED, &snd_samplerate);
    ioctl(audio.snd_handle, SNDCTL_DSP_SAMPLESIZE, &snd_bitsize);
    ioctl(audio.snd_handle, SNDCTL_DSP_SETFRAGMENT, &snd_fragment);

    return true;
}

void OssEngine::stopAudio()
{
    int handle = audio.snd_handle;
    if(handle == -1) //already closed
        return;
    audio.snd_handle = -1;
    close(handle);
}

bool OssEngine::Start()
{
    if(enabled())
        return true;
    enabled = true;

    bool good = true;
    if(audio.en)
        if(!openAudio()) {
            cerr << "Failed to open OSS audio" << endl;
            good = false;
        }

    if(midi.en)
        if(!openMidi()) {
            cerr << "Failed to open OSS midi" << endl;
            good = false;
        }

    if(!good) {
        Stop();
        return false;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&pThread, &attr, _thread, this);

    return true;
}

void OssEngine::Stop()
{
    if(!enabled())
        return;
    enabled = false;
    stopAudio();
    stopMidi();
    pthread_join(pThread, NULL);
}

void OssEngine::setMidiEn(bool nval)
{
    midi.en = nval;
    if(enabled()) {
        if(nval)
            openMidi();
        else
            stopMidi();
    }
}

bool OssEngine::getMidiEn() const
{
    if(enabled())
        return midi.handle != -1;
    else
        return midi.en;
}

void OssEngine::setAudioEn(bool nval)
{
    audio.en = nval;
    if(enabled()) { //lets rebind the ports
        if(nval)
            openAudio();
        else
            stopAudio();
    }
}

bool OssEngine::getAudioEn() const
{
    if(enabled())
        return audio.snd_handle != -1;
    else
        return audio.en;
}

bool OssEngine::openMidi()
{
    midi.handle = open(config.cfg.LinuxOSSSeqInDev, O_RDONLY, 0);

    if(-1 == midi.handle)
       return false;

    midi.run = true;
    return true;
}

void OssEngine::stopMidi()
{
    int tmp = midi.handle;
    if(tmp == -1) //already closed
        return;

    midi.run = false;
    midi.handle = -1;
    close(tmp);
}

void *OssEngine::_thread(void *arg)
{
    return (static_cast<OssEngine*>(arg))->thread();
}


void *OssEngine::thread()
{
    MidiEvent ev;
    unsigned char tmp[4] = {0, 0, 0, 0};
    set_realtime();
    while (midi.run || audio.snd_handle != -1)
    {
        if(audio.snd_handle != -1)
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
            int handle = audio.snd_handle;
            if(handle != -1)
                write(handle, audio.smps, SOUND_BUFFER_SIZE * 4); // *2 because is 16 bit, again * 2 because is stereo
            else
                break;
        }

        //Collect up to 10 midi events
        for (int k = 0; k < 10 && midi.run; ++k) {
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

void OssEngine::midiProcess(unsigned char head, unsigned char num, unsigned char value)
{
    MidiEvent ev;
    unsigned char chan = head & 0x0f;
    switch(head & 0xf0)
    {
        case 0x80: //Note Off
            ev.type    = M_NOTE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = 0;
            sysIn->putEvent(ev);
            break;
        case 0x90: //Note On
            ev.type    = M_NOTE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = value;
            sysIn->putEvent(ev);
            break;
        case 0xb0: //Controller
            ev.type    = M_CONTROLLER;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = value;
            sysIn->putEvent(ev);
            break;
        case 0xe0: //Pitch Wheel
            ev.type    = M_CONTROLLER;
            ev.channel = chan;
            ev.num     = C_pitchwheel;
            ev.value   = (num + value * (int) 128) - 8192;
            sysIn->putEvent(ev);
            break;
    }
}


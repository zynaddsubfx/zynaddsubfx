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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

using namespace std;

OssEngine::OssEngine(OutMgr *out)
    :AudioOut(out)
{
    snd_fragment   = 0x00080009; //fragment size (?)
    snd_stereo     = 1; //stereo
    snd_format     = AFMT_S16_LE;
    snd_samplerate = SAMPLE_RATE;

    smps = new short[SOUND_BUFFER_SIZE * 2];
    memset(smps, 0, sizeof(short) * SOUND_BUFFER_SIZE * 2);

    cerr << "hello?" << endl;
}

OssEngine::~OssEngine()
{
    close(snd_handle);
    delete [] smps;
}

bool OssEngine::openAudio()
{
    int snd_bitsize = 16;
    snd_handle = open(config.cfg.LinuxOSSWaveOutDev, O_WRONLY, 0);
    cerr << config.cfg.LinuxOSSWaveOutDev << endl;
    if(snd_handle == -1) {
        cerr << "ERROR - I can't open the "
             << config.cfg.LinuxOSSWaveOutDev << '.' << endl;
        return false;
    }
    ioctl(snd_handle, SNDCTL_DSP_RESET, NULL);
    ioctl(snd_handle, SNDCTL_DSP_SETFMT, &snd_format);
    ioctl(snd_handle, SNDCTL_DSP_STEREO, &snd_stereo);
    ioctl(snd_handle, SNDCTL_DSP_SPEED, &snd_samplerate);
    ioctl(snd_handle, SNDCTL_DSP_SAMPLESIZE, &snd_bitsize);
    ioctl(snd_handle, SNDCTL_DSP_SETFRAGMENT, &snd_fragment);
    return true;
}

bool OssEngine::Start()
{
    if(!openAudio())
        return false;
    pthread_attr_t attr;
    threadStop = false;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pThread, &attr, _AudioThread, this);
    cout << "Starting Oss";
    enabled = true;

    return true;
}

void OssEngine::Stop()
{
    threadStop = true;
    enabled = false;
}

void *OssEngine::_AudioThread(void *arg)
{
    return (static_cast<OssEngine*>(arg))->AudioThread();
}


void *OssEngine::AudioThread()
{  
    //get some initial samples
    manager->requestSamples();
    manager->requestSamples();
    manager->requestSamples();
    set_realtime();
    while (!threadStop())
    {
        const Stereo<Sample> smps = getNext();
        OSSout(smps.l().c_buf(),smps.r().c_buf());
    }
    return NULL;
}

/*
 * Output the samples to the soundcard
 * The samples are bigger than -1.0 and smaller 1.0
 */
void OssEngine::OSSout(const REALTYPE *smp_left, const REALTYPE *smp_right)
{
    int i;
    REALTYPE l, r;
    for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
        l = smp_left[i];
        r = smp_right[i];

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

        smps[i * 2]     = (short int) (l * 32767.0);
        smps[i * 2 + 1] = (short int) (r * 32767.0);
    }
    write(snd_handle, smps, SOUND_BUFFER_SIZE * 4); // *2 because is 16 bit, again * 2 because is stereo
}


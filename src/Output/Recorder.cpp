/*
  ZynAddSubFX - a software synthesizer

  Recorder.C - Records sound to a file
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

#include <sys/stat.h>
#include "Recorder.h"
#ifdef NEW_IO
#include "../Nio/OutMgr.h"
#include "../Nio/WavEngine.h"
#endif

Recorder::Recorder()
    :status(0), notetrigger(0)
{
#ifndef NEW_IO
    recordbuf_16bit = new short int [SOUND_BUFFER_SIZE * 2];
    for(int i = 0; i < SOUND_BUFFER_SIZE * 2; i++)
        recordbuf_16bit[i] = 0;
#else
        wave=NULL;
#endif
}

Recorder::~Recorder()
{
    if(recording() == 1)
        stop();
#ifndef NEW_IO
    delete [] recordbuf_16bit;
#endif
}

int Recorder::preparefile(std::string filename_, int overwrite)
{
    if(!overwrite) {
        struct stat fileinfo;
        int statr;
        statr = stat(filename_.c_str(), &fileinfo);
        if(statr == 0)   //file exists
            return 1;
    }

#ifndef NEW_IO
    if(!wav.newfile(filename_, SAMPLE_RATE, 2))
        return 2;
#else
    if(!(wave=new WavEngine(sysOut, filename_, SAMPLE_RATE, 2)))
        return 2;
#endif

    status = 1; //ready

    return 0;
}

void Recorder::start()
{
    notetrigger = 0;
    status      = 2; //recording
}

void Recorder::stop()
{
#ifndef NEW_IO
    wav.close();
#else
    if(wave)
    {
        sysOut->remove(wave);
        wave->Close();
        delete wave;
        wave = NULL; //is this even needed?
    }
#endif
    status = 0;
}

void Recorder::pause()
{
    status = 0;
#ifdef NEW_IO
//        wave->Stop();
        sysOut->remove(wave);
#endif
}

int Recorder::recording()
{
    if((status == 2) && (notetrigger != 0))
        return 1;
    else
        return 0;
}

#ifndef NEW_IO
void Recorder::recordbuffer(REALTYPE *outl, REALTYPE *outr)
{
    int tmp;
    if(status != 2)
        return;
    for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        tmp = (int)(outl[i] * 32767.0);
        if(tmp < -32768)
            tmp = -32768;
        if(tmp > 32767)
            tmp = 32767;
        recordbuf_16bit[i * 2] = tmp;

        tmp = (int)(outr[i] * 32767.0);
        if(tmp < -32768)
            tmp = -32768;
        if(tmp > 32767)
            tmp = 32767;
        recordbuf_16bit[i * 2 + 1] = tmp;
    }
    wav.write_stereo_samples(SOUND_BUFFER_SIZE, recordbuf_16bit);
}
#endif

void Recorder::triggernow()
{
    if(status == 2) {
#ifdef NEW_IO
        if(notetrigger!=1) {
            wave->openAudio();
            //wave->Start();
            sysOut->add(wave);
        }
#endif
        notetrigger = 1;
    }
}


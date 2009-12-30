/*
  ZynAddSubFX - a software synthesizer

  OSSaudiooutput.h - Audio output for Open Sound System
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

#ifndef OSS_ENGINE_H
#define OSS_ENGINE_H

#include <sys/time.h>
#include "../globals.h"
#include "AudioOut.h"
#include "MidiIn.h"

class OssEngine: public AudioOut, MidiIn
{
    public:
        OssEngine(OutMgr *out);
        ~OssEngine();

        bool Start();
        void Stop();

        bool StartMidi();
        void StopMidi();

    protected:
        void *AudioThread();
        static void *_AudioThread(void *arg);
        void *MidiThread();
        static void *_MidiThread(void *arg);

    private:
        //Audio
        /**Open the audio device
         * @return true for success*/
        bool openAudio();
        
        void OSSout(const REALTYPE *smp_left, const REALTYPE *smp_right);
        int snd_handle;
        int snd_fragment;
        int snd_stereo;
        int snd_format;
        int snd_samplerate;

        short int *smps; //Samples to be sent to soundcard

        //Midi
        pthread_t pThreadMidi;
        bool openMidi();
        void stopMidi();
        unsigned char getmidibyte();
        unsigned char readbyte();

        unsigned char cmdtype; //the Message Type (noteon,noteof,sysex..)
        unsigned char cmdchan; //the channel number

        int midiHandle;
        unsigned char lastmidicmd; //last byte (>=80) received from the Midi

};

#endif


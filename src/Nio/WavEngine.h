/*

  Copyright (C) 2008 Nasca Octavian Paul
  Author: Nasca Octavian Paul
          Mark McCurry

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

#ifndef WAVENGINE_H
#define WAVENGINE_H
#include "AudioOut.h"
#include <string>

class WavEngine: public AudioOut
{
    public:
        WavEngine(OutMgr *out, std::string _filename, int _samplerate, int _channels);
        ~WavEngine();

        bool openAudio();
        bool Start();
        void Stop();
        void Close();

        const Stereo<Sample> getNext();

    protected:
        void *AudioThread();
        static void *_AudioThread(void *arg);

    private:
        void write_stereo_samples(int nsmps, short int *smps);
        void write_mono_samples(int nsmps, short int *smps);
        std::string filename;
        int   sampleswritten;
        int   samplerate;
        int   channels;
        FILE *file;
        //pthread_mutex_t run_mutex;
        pthread_mutex_t write_mutex;
        pthread_mutex_t stop_mutex;
        pthread_cond_t stop_cond;
};
#endif


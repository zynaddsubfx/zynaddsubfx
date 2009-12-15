/*
    AlsaEngine.h

    Copyright 2009, Alan Calvert

    This file is part of ZynAddSubFX, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    ZynAddSubFX is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ZynAddSubFX.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ALSA_ENGINE_H
#define ALSA_ENGINE_H

#include <pthread.h>
#include <string>
#include <alsa/asoundlib.h>
#include <queue>

#include "AudioOut.h"
//include "MidiIn.h"
#include "OutMgr.h"
#include "../Misc/Stereo.h"
#include "../Samples/Sample.h"

class AlsaEngine : public AudioOut//, MidiIn
{
    public:
        AlsaEngine(OutMgr *out);
        ~AlsaEngine() { };
        
        //bool openMidi();
        bool Start();
        void Stop();
        
        unsigned int getSamplerate() { return audio.samplerate; };
        unsigned int getBuffersize() { return audio.period_size; };
        
        std::string audioClientName();
        //std::string midiClientName();
        int audioClientId() { return audio.alsaId; };
        //int midiClientId() { return midi.alsaId; };

    protected:
        void *AudioThread();
        static void *_AudioThread(void *arg);
        //void *MidiThread(oid);
        //static void *_MidiThread();

    private:
        bool prepHwparams();
        bool prepSwparams();
        void Write(const short *InterleavedSmps, int size);
        bool Recover(int err);
        bool xrunRecover();
        bool alsaBad(int op_result, std::string err_msg);
        void closeAudio();
        //void closeMidi();

        snd_pcm_sframes_t (*pcmWrite)(snd_pcm_t *handle, const void *data,
                                      snd_pcm_uframes_t nframes);

        /**Interleave Samples. \todo move this to util*/
        const short *interleave(const Stereo<Sample> smps) const;

        struct {
            std::string        device;
            snd_pcm_t         *handle;
            unsigned int       period_time;
            unsigned int       samplerate;
            snd_pcm_uframes_t  period_size;
            snd_pcm_uframes_t  buffer_size;
            int                alsaId;
            snd_pcm_state_t    pcm_state;
            pthread_t          pThread;
        } audio;

        //struct {
        //    std::string  device;
        //    snd_seq_t   *handle;
        //    int          alsaId;
        //    pthread_t    pThread;
        //} midi;

        //from alsa example
        long loops;
        int rc;
        int size;
        snd_pcm_t *handle;
        snd_pcm_hw_params_t *params;
        unsigned int val;
        int dir;
        snd_pcm_uframes_t frames;
        const short *buffer;

        void RunStuff();
        void OpenStuff();

};

#endif

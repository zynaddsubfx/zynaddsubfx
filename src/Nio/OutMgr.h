/*
  ZynAddSubFX - a software synthesizer

  OutMgr.h - Audio Engine Interfacer
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef OUTMGR_H
#define OUTMGR_H

#include "../Misc/Stereo.h"
#include "../globals.h"
#include <list>
#if HAVE_BG_SYNTH_THREAD
#include <thread>
#include <mutex>
#include <condition_variable>
#endif
#include <string>

namespace zyn {

class AudioOut;
struct SYNTH_T;
class OutMgr
{
    public:
        enum { FRAME_SIZE_MAX = 1U << 14 };

        static OutMgr &getInstance(const SYNTH_T *synth=NULL);
        ~OutMgr();

        /**Execute a tick*/
        Stereo<float *> tick(unsigned int frameSize) REALTIME;

        /**Request a new set of samples
         * @param n number of requested samples (defaults to 1)
         * @return -1 for locking issues 0 for valid request*/
        void requestSamples(unsigned int n = 1);

        /**Gets requested driver
         * @param name case unsensitive name of driver
         * @return pointer to Audio Out or NULL
         */
        AudioOut *getOut(std::string name);

        /**Gets the name of the first running driver
         * Deprecated
         * @return if no running output, "" is returned
         */
        std::string getDriver() const;

        bool setSink(std::string name);

        std::string getSink() const;

        void setAudioCompressor(bool isEnabled);
        bool getAudioCompressor(void);

        class WavEngine * wave;     /**<The Wave Recorder*/
        friend class EngineMgr;

        void setMaster(class Master *master_);
        void applyOscEventRt(const char *msg);
#if HAVE_BG_SYNTH_THREAD
        void setBackgroundSynth(bool);
        void *refillThread();
#endif
    private:
        OutMgr(const SYNTH_T *synth);
        void addSmps(float *l, float *r);
        unsigned int curStoredSmps() const {return priBuffCurrent.l - priBuf.l; }
        void removeStaleSmps();
#if HAVE_BG_SYNTH_THREAD
        void refillLock() { bgSynthMtx.lock(); }
        void refillUnlock() { bgSynthMtx.unlock(); }
        void refillWait() { std::unique_lock<std::mutex> lock(bgSynthMtx); bgSynthCond.wait(lock); }
        void refillWakeup() { bgSynthCond.notify_one(); }
#else
        void refillLock() { }
        void refillUnlock() { }
#endif
        void refillSmps(unsigned int);

        AudioOut *currentOut; /**<The current output driver*/

        /**Buffer*/
        Stereo<float *> priBuf;         //buffer for primary drivers
        Stereo<float *> priBuffCurrent; //current array accessor

        float *outl;
        float *outr;
        class Master *master;

        /**Buffer state*/
        unsigned int stales;
        unsigned int maxStoredSmps;
        unsigned int midiFlushOffset;
        const SYNTH_T &synth;

#if HAVE_BG_SYNTH_THREAD
        /**Background synth*/
        std::mutex bgSynthMtx;
        std::condition_variable bgSynthCond;
        std::thread bgSynthThread;
        bool bgSynthEnabled;
#endif
};

}

#endif

/*
  ZynAddSubFX - a software synthesizer

  AudioOut.h - Audio Output superclass
  Copyright (C) 2009-2010 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include "../Misc/Stereo.h"
#include "../globals.h"
#include "Engine.h"

namespace zyn {

class AudioOut:public virtual Engine
{
    public:
        AudioOut(const SYNTH_T &synth);
        virtual ~AudioOut();

        /**Sets the Sample Rate of this Output
         * (used for getNext()).*/
        void setSamplerate(int _samplerate);

        /**Sets the Samples required per Out of this driver
         * not a realtime operation */
        int getSampleRate();
        void setBufferSize(int _bufferSize);

        /**Sets the Frame Size for output*/
        void bufferingSize(int nBuffering);
        int bufferingSize();

        virtual void setAudioEn(bool nval) = 0;
        virtual bool getAudioEn() const    = 0;
        
        bool isOutputCompressionEnabled = 0;

    protected:
        /**Get the next sample for output.
         * (has nsamples sampled at a rate of samplerate)*/
        const Stereo<float *> getNext();

        const SYNTH_T &synth;
        int samplerate;
        int bufferSize;
        
};

}

#endif

/*
  ZynAddSubFX - a software synthesizer

  Time.h - Frame Tracker
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <stdint.h>
#include "../globals.h"

namespace zyn {

class AbsTime
{
    public:
        AbsTime(const SYNTH_T &synth) : AbsTime(synth.dt(), synth.buffersize) {}
        AbsTime(const float dt_, const int buffersize_)
            :tempo(120),
            hostSamples(0),
            bar(0),
            beat(0),
            tick(0.0f),
            bpm(120.0f),
            ppq(1920.0f),
            frames(0),
            samplingInterval(dt_),
            buffersize(buffersize_) {};
        void operator++(){++frames;};
        void operator++(int){frames++;};
        int64_t time() const {return frames;};
        unsigned int tempo;
        int hostSamples;
        int bar;
        int beat;
        float tick;
        float beatsPerBar;
        float beatType;
        float bpm;
        float ppq;
        bool playing;
        float dt() const { return samplingInterval; }
        float framesPerSec() const { return 1.0f/samplingInterval;}
        int   samplesPerFrame() const {return buffersize;}
        int   samplerate() const {return buffersize / samplingInterval;}
    private:
        int64_t frames;
        const float samplingInterval;
        const int buffersize;

};

//Marker for an event relative to some position of the absolute timer
class RelTime
{
    public:
        RelTime(const AbsTime &t_, float sec)
            :t(t_)
        {
            //Calculate time of event
            float deltaFrames = sec*t.framesPerSec();
            int64_t tmp = (int64_t)deltaFrames;
            frame = t.time() + tmp;
            sample = (int32_t)(t.samplesPerFrame()*(deltaFrames-tmp));
        }
        bool inThisFrame() {return t.time() == frame;};
        bool inPast() {return t.time() > frame;}
        bool inFuture() {return t.time() < frame;}
    private:
        int64_t frame;
        int32_t sample;
        const AbsTime &t;
};

}

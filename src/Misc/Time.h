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
        AbsTime(const SYNTH_T &synth)
            :tempo(120),
            hostSamples(0),
            bar(0),
            beat(0),
            tick(0.0f),
            bpm(120.0f),
            ppq(1920.0f),
            frames(0),
            s(synth) {};
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
        float dt() const { return s.dt(); }
        float framesPerSec() const { return 1/s.dt();}
        int   samplesPerFrame() const {return s.buffersize;}
        int   samplerate() const {return s.buffersize / s.dt();}
    private:
        int64_t frames;
        const SYNTH_T &s;

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

/*
  ZynAddSubFX - a software synthesizer

  ModFilter.h - Modulated Filter
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include "../globals.h"
#include "../Misc/Time.h"

namespace zyn {

//Modulated instance of one of the filters in src/DSP/
//Supports stereo modes
class ModFilter
{
    public:
        ModFilter(const FilterParams &pars,
                  const SYNTH_T      &synth,
                  const AbsTime      &time,
                        Allocator    &alloc,
                        bool         stereo,
                        float        notefreq_);
        ~ModFilter(void);

        void addMod(LFO      &lfo);
        void addMod(Envelope &env);
        void addMod(SEQ      &seq);

        //normal per tick update
        void update(float relfreq, float relq);

        //updates typically seen in note-init
        void updateNoteFreq(float noteFreq_);
        void updateSense(float velocity,
                uint8_t scale, uint8_t func);

        //filter stereo/mono signal(s) in-place
        void filter(float *l, float *r);
    private:
        void paramUpdate(Filter *&f);
        void svParamUpdate(SVFilter &sv);
        void anParamUpdate(AnalogFilter &an);
        void mgParamUpdate(MoogFilter &mg);
        void cbParamUpdate(CombFilter &cb);


        const FilterParams &pars;  //Parameters to Pull Updates From
        const SYNTH_T      &synth; //Synthesizer Buffer Parameters
        const AbsTime      &time;  //Time for RT Updates
        Allocator          &alloc; //RT Memory Pool


        smooth_float baseQ;    //filter sharpness
        smooth_float baseFreq; //base filter frequency
        float noteFreq; //frequency note was initialized to
        smooth_float tracking; //shift due to note frequency
        smooth_float sense;    //shift due to note velocity


        Filter       *left; //left  channel filter
        Filter       *right;//right channel filter
        Envelope     *env;  //center freq envelope
        LFO          *lfo;  //center freq lfo
        SEQ          *seq;  //center freq lfo
};

}

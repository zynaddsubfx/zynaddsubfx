/*
  ZynAddSubFX - a software synthesizer

  FormantFilterGraph.H - OSC Formant Filter Graph View
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once

#include <FL/Fl.H>
#include "Fl_Osc_Widget.H"
#include "WidgetPDial.h"
#include "../globals.h"
#include <FL/Fl_Group.H>
#include "../Params/FilterParams.h"
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include "common.H"

class FilterParams;

class FormantFilterGraph : public Fl_Box, public Fl_Osc_Widget {
    public:
        FormantFilterGraph(int x,int y, int w, int h, const char *label=0);
        void init(int *nvowel_, int *nformant_);
        void draw_freq_line(float freq,int type);
        void draw();
        virtual ~FormantFilterGraph(void);

        void OSC_value(int x, const char *) override;
        void OSC_value(unsigned x, void *v) override;

        void rebase(std::string new_base) override;

        void updateVowel(int);
        void updateFormant(int);

        void update(void);

    private:
        int *nvowel,*nformant;
        float *graphpoints;

        struct {
            struct {
                unsigned char loc;
                unsigned char freq, amp, q; //frequency,amplitude,Q
            } formants[FF_MAX_FORMANTS];
        } Pvowels[FF_MAX_VOWELS];

        int Pnumformants;
        int Pstages;
        int Pcenterfreq;
        int Pgain;

        int Pq;
        int Poctavesfreq;


        float getfreqx(float);

        float getfreqpos(float);
        float getformantfreq(unsigned char);
        float getformantamp(unsigned char);
        float getoctavesfreq(void);
        float getcenterfreq(void);
        float getgain(void);
        float getq(void);
        float getformantq(unsigned char q);
        void formantfilterH(int, int, float*);
};

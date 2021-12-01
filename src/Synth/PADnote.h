/*
  ZynAddSubFX - a software synthesizer

  PADnote.h - The "pad" synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef PAD_NOTE_H
#define PAD_NOTE_H

#include "SynthNote.h"
#include "../globals.h"
#include "Envelope.h"
#include "LFO.h"

namespace zyn {

/**The "pad" synthesizer*/
class PADnote:public SynthNote
{
    public:
        PADnote(const PADnoteParameters *parameters, const SynthParams &pars,
                const int &interpolation, WatchManager *wm=0, const char *prefix=0);
        ~PADnote();

        SynthNote *cloneLegato(void);
        void legatonote(const LegatoParams &pars);

        int noteout(float *outl, float *outr);
        bool finished() const;
        void entomb(void);

        VecWatchPoint watch_int,watch_punch, watch_amp_int, watch_legato;

        void releasekey();
    private:
        void setup(float velocity, Portamento *portamento,
                   float note_log2_freq, bool legato = false, WatchManager *wm=0, const char *prefix=0);
        void fadein(float *smps);
        void computecurrentparameters();
        bool finished_;
        const PADnoteParameters &pars;

        int   poshi_l, poshi_r;
        float poslo;

        float note_log2_freq;
        float BendAdjust;
        float OffsetHz;
        bool  firsttime;

        int nsample;
        Portamento *portamento;

        int Compute_Linear(float *outl,
                           float *outr,
                           int freqhi,
                           float freqlo);
        int Compute_Cubic(float *outl,
                          float *outr,
                          int freqhi,
                          float freqlo);


        struct {
            /******************************************
            *     FREQUENCY GLOBAL PARAMETERS        *
            ******************************************/
            float Detune;  //cents

            Envelope *FreqEnvelope;
            LFO      *FreqLfo;

            /********************************************
             *     AMPLITUDE GLOBAL PARAMETERS          *
             ********************************************/
            float Volume;  // [ 0 .. 1 ]

            float Panning;  // [ 0 .. 1 ]

            Envelope *AmpEnvelope;
            LFO      *AmpLfo;

            float Fadein_adjustment;
            struct {
                int   Enabled;
                float initialvalue, dt, t;
            } Punch;

            /******************************************
            *        FILTER GLOBAL PARAMETERS        *
            ******************************************/
            ModFilter *GlobalFilter;
            Envelope  *FilterEnvelope;
            LFO       *FilterLfo;
        } NoteGlobalPar;


        float globaloldamplitude, globalnewamplitude, velocity, realfreq;
        const int& interpolation;
};

}

#endif

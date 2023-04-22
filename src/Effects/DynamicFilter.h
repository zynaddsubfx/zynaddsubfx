/*
  ZynAddSubFX - a software synthesizer

  DynamicFilter.h - "WahWah" effect and others
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef DYNAMICFILTER_H
#define DYNAMICFILTER_H

#include "Effect.h"
#include "EffectLFO.h"

namespace zyn {

/**DynamicFilter Effect*/
class DynamicFilter final:public Effect
{
    public:
        DynamicFilter(EffectParams pars);
        ~DynamicFilter();
        void out(const Stereo<float *> &smp);

        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset) { setpreset(npreset, false); };
        void setpreset(unsigned char npreset, bool protect);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup(void);

        static rtosc::Ports ports;
    private:
        //Parametrii DynamicFilter
        EffectLFO     lfo;          //lfo-ul DynamicFilter
        unsigned char Pvolume;      //Volume
        unsigned char Pdepth;       //the depth of the lfo
        unsigned char Pampsns;      //how the filter varies according to the input amplitude
        unsigned char Pampsnsinv;   //if the filter freq is lowered if the input amplitude rises
        unsigned char Pampsmooth;   //how smooth the input amplitude changes the filter

        //Parameter Control
        void setvolume(unsigned char _Pvolume);
        void setdepth(unsigned char _Pdepth);
        void setampsns(unsigned char _Pampsns);

        void setfilterpreset(unsigned char npreset);
        void reinitfilter(void);

        //Internal Values
        float depth, ampsns, ampsmooth;

        class Filter * filterl, *filterr;
        float ms1, ms2, ms3, ms4; //mean squares
};

}

#endif

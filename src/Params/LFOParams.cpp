/*
  ZynAddSubFX - a software synthesizer

  LFOParams.cpp - Parameters for LFO
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cstdio>
#include "../globals.h"
#include "../Misc/Util.h"
#include "../Misc/XMLwrapper.h"
#include "../Misc/Time.h"
#include "LFOParams.h"

#include <rtosc/port-sugar.h>
#include <rtosc/ports.h>
using namespace rtosc;

namespace zyn {

#define rObject LFOParams
#undef rChangeCb
#define rChangeCb if (obj->time) { obj->last_update_timestamp = obj->time->time(); }
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }
static const rtosc::Ports _ports = {
    rSelf(LFOParams),
    rPaste,
    rOption(loc, rProp(internal),
            rOptions(ad_global_amp, ad_global_freq, ad_global_filter,
                     ad_voice_amp, ad_voice_freq, ad_voice_filter, unspecified),
            "location of the filter"),
    rParamF(Pfreq, rShort("freq"), rLinear(0.0,1.0),
            rDefaultDepends(loc),
            rPreset(ad_global_amp, 0x1.42850ap-1), // 80
            rPreset(ad_global_freq, 0x1.1a3468p-1), // 70
            rPreset(ad_global_filter, 0x1.42850ap-1),
            rPreset(ad_voice_amp, 0x1.6ad5acp-1), // 90
            rPreset(ad_voice_freq, 0x1.93264cp-2), // 50
            rPreset(ad_voice_filter, 0x1.93264cp-2),
            "frequency of LFO\n"
            "lfo frequency = (2^(10*Pfreq)-1)/12 * stretch\n"
            "true frequency is [0,85.33] Hz"),
    rParamZyn(Pintensity, rShort("depth"),
              rDefaultDepends(loc),
              rDefault(0), rPreset(ad_voice_amp, 32),
              rPreset(ad_voice_freq, 40), rPreset(ad_voice_filter, 20),
              "Intensity of LFO"),
    rParamZyn(Pstartphase, rShort("start"), rSpecial(random),
              rDefaultDepends(loc), rDefault(64), rPreset(ad_voice_freq, 0),
              "Starting Phase"),
    rOption(PLFOtype, rShort("type"), rOptions(sine, triangle, square, up, down,
                exp1, exp2, ran), rDefault(sine), "Shape of LFO"),
    rParamZyn(Prandomness, rShort("a.r."), rSpecial(disable), rDefault(0),
            "Amplitude Randomness (calculated uniformly at each cycle)"),
    rParamZyn(Pfreqrand, rShort("f.r."), rSpecial(disable), rDefault(0),
            "Frequency Randomness (calculated uniformly at each cycle)"),
    rParamZyn(Pdelay, rShort("delay"), rSpecial(disable),
              rDefaultDepends(loc), rDefault(0), rPreset(ad_voice_amp, 30),
              "Delay before LFO start\n0..4 second delay"),
    rToggle(Pcontinous, rShort("c"), rDefault(false),
            "Enable for global operation"),
    rParamZyn(Pstretch, rShort("str"), rCentered, rDefault(64),
        "Note frequency stretch"),
// these are currently not yet implemented at must be hidden therefore
#ifdef DEAD_PORTS
    //Float valued aliases
    {"delay::f", rProp(parameter) rMap(units, ms) rLog(0,4000), 0,
        rBegin;

        rEnd},
#define rPseudoLog(a,b) rLog(a,b)
    {"period::f", rProp(parameter) rMap(units, ms) rPseudoLog(0.10, 1500.0), 0,
        rBegin;
        rEnd},
#endif
};
#undef rPseudoLog
#undef rBegin
#undef rEnd
#undef rChangeCb

const rtosc::Ports &LFOParams::ports = _ports;

void LFOParams::setup()
{
    switch(loc) {
        case loc_unspecified:
            fel = consumer_location_type_t::unspecified;
            break;
        case ad_global_freq:
        case ad_voice_freq:
            fel = consumer_location_type_t::freq;
            setpresettype("Plfofrequency");
            break;
        case ad_global_amp:
        case ad_voice_amp:
            fel = consumer_location_type_t::amp;
            setpresettype("Plfoamplitude");
            break;
        case ad_global_filter:
        case ad_voice_filter:
            fel = consumer_location_type_t::filter;
            setpresettype("Plfofilter");
            break;
        default:
            throw std::logic_error("Invalid envelope consumer location");
    }

    defaults();
}

// TODO: reuse
LFOParams::LFOParams(const AbsTime *time_) :
    LFOParams(64, 0, 0, 0, 0, 0, 0, loc_unspecified, time_)
{
}

LFOParams::LFOParams(char Pfreq_,
                     char Pintensity_,
                     char Pstartphase_,
                     char PLFOtype_,
                     char Prandomness_,
                     char Pdelay_,
                     char Pcontinous_,
                     consumer_location_t loc,
                     const AbsTime *time_) : loc(loc),
                                             time(time_),
                                             last_update_timestamp(0) {
    Dfreq       = Pfreq_;
    Dintensity  = Pintensity_;
    Dstartphase = Pstartphase_;
    DLFOtype    = PLFOtype_;
    Drandomness = Prandomness_;
    Ddelay      = Pdelay_;
    Dcontinous  = Pcontinous_;

    setup();
}

LFOParams::LFOParams(consumer_location_t loc,
                     const AbsTime *time_) : loc(loc),
                                             time(time_),
                                             last_update_timestamp(0) {

    auto init =
        [&](char Pfreq_, char Pintensity_, char Pstartphase_, char PLFOtype_,
            char Prandomness_, char Pdelay_, char Pcontinous_)
    {
        Dfreq       = Pfreq_;
        Dintensity  = Pintensity_;
        Dstartphase = Pstartphase_;
        DLFOtype    = PLFOtype_;
        Drandomness = Prandomness_;
        Ddelay      = Pdelay_;
        Dcontinous  = Pcontinous_;
    };

    switch(loc)
    {
        case ad_global_amp:    init(80, 0, 64, 0, 0, 0, 0); break;
        case ad_global_freq:   init(70, 0, 64, 0, 0, 0, 0); break;
        case ad_global_filter: init(80, 0, 64, 0, 0, 0, 0); break;
        case ad_voice_amp:     init(90, 32, 64, 0, 0, 30, 0); break;
        case ad_voice_freq:    init(50, 40,  0, 0, 0,  0, 0); break;
        case ad_voice_filter:  init(50, 20, 64, 0, 0,  0, 0); break;
        default: throw std::logic_error("Invalid LFO consumer location");
    }

    setup();
}

LFOParams::~LFOParams()
{}

void LFOParams::defaults()
{
    Pfreq       = Dfreq / 127.0f;
    Pintensity  = Dintensity;
    Pstartphase = Dstartphase;
    PLFOtype    = DLFOtype;
    Prandomness = Drandomness;
    Pdelay      = Ddelay;
    Pcontinous  = Dcontinous;
    Pfreqrand   = 0;
    Pstretch    = 64;
}


void LFOParams::add2XML(XMLwrapper& xml)
{
    xml.addparreal("freq", Pfreq);
    xml.addpar("intensity", Pintensity);
    xml.addpar("start_phase", Pstartphase);
    xml.addpar("lfo_type", PLFOtype);
    xml.addpar("randomness_amplitude", Prandomness);
    xml.addpar("randomness_frequency", Pfreqrand);
    xml.addpar("delay", Pdelay);
    xml.addpar("stretch", Pstretch);
    xml.addparbool("continous", Pcontinous);
}

void LFOParams::getfromXML(XMLwrapper& xml)
{
    Pfreq       = xml.getparreal("freq", Pfreq, 0.0f, 1.0f);
    Pintensity  = xml.getpar127("intensity", Pintensity);
    Pstartphase = xml.getpar127("start_phase", Pstartphase);
    PLFOtype    = xml.getpar127("lfo_type", PLFOtype);
    Prandomness = xml.getpar127("randomness_amplitude", Prandomness);
    Pfreqrand   = xml.getpar127("randomness_frequency", Pfreqrand);
    Pdelay      = xml.getpar127("delay", Pdelay);
    Pstretch    = xml.getpar127("stretch", Pstretch);
    Pcontinous  = xml.getparbool("continous", Pcontinous);
}

#define COPY(y) this->y=x.y
void LFOParams::paste(LFOParams &x)
{
    COPY(Pfreq);
    COPY(Pintensity);
    COPY(Pstartphase);
    COPY(PLFOtype);
    COPY(Prandomness);
    COPY(Pfreqrand);
    COPY(Pdelay);
    COPY(Pcontinous);
    COPY(Pstretch);

    if ( time ) {
        last_update_timestamp = time->time();
    }
}
#undef COPY

}

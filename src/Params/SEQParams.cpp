/*
  ZynAddSubFX - a software synthesizer

  SEQParams.cpp - Parameters for LFO
  Copyright (C) 2021 Michael Kirchner
  Author: Michael Kirchner

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
#include "SEQParams.h"
#include <rtosc/port-sugar.h>
#include <rtosc/ports.h>
using namespace rtosc;

namespace zyn {


#define rObject SEQParams
#undef rChangeCb
#define rChangeCb if (obj->time) { obj->last_update_timestamp = obj->time->time(); }
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

static const rtosc::Ports _ports = {
    rSelf(SEQParams),
    rPaste,
    rOption(loc, rProp(internal),
            rOptions(ad_global_amp, ad_global_freq, ad_global_filter,
                     ad_voice_amp, ad_voice_freq, ad_voice_filter, unspecified),
            "location of the filter"),
    rParamF(freq, rShort("freq"), rDefault(2.0f), rUnit(Hz), rLinear(0.1f,10.0f),
            "step frequency"),
    rParamF(intensity, rShort("depth"), rLinear(0.0, 2.0), rDefault(0.0),
              "Intensity of SEQ"),
    rParamF(cutoff, rShort("lp"), rDefault(MAX_CUTOFF), rUnit(Hz), rLinear(0.0f,MAX_CUTOFF),
            "cutoff of lp-filter for output\n 40.0=off"),
    rParamF(delay, rShort("delay"), rUnit(S),
              rLinear(0.0, 4.0), rDefault(0.0),
              "Delay before SEQ start\n0..4 second delay"),
    rToggle(continous, rShort("c"), rDefault(false),
            "Enable for global phase syncronization"),
    rParamZyn(numerator, rShort("num"), rLinear(0,99), rDefault(0),
        "Numerator of ratio to bpm\n0 = OFF"),
    rParamZyn(denominator, rShort("dem"), rLinear(0,99), rDefault(4),
        "Denominator of ratio to bpm"),
    rParamZyn(steps, rShort("steps"), rLinear(1,128), rDefault(8),
            "number of steps"),
    rArrayF(sequence, NUM_SEQ_STEPS, rLinear(-1.0f,1.0f), rDefaultDepends(loc),
              rPreset(ad_global_freq, [0.0 0.0 0.0 ...]),
              rPreset(ad_global_filter, [0.0 0.0 0.0 ...]),
              rPreset(ad_global_amp, [1.0 1.0 1.0 ...]),
              rPreset(ad_voice_freq, [0.0 0.0 0.0 ...]),
              rPreset(ad_voice_filter, [0.0 0.0 0.0 ...]),
              rPreset(ad_voice_amp, [1.0 1.0 1.0 ...]),
           "sequence values"),
};
#undef rBegin
#undef rEnd
#undef rChangeCb

const rtosc::Ports &SEQParams::ports = _ports;

void SEQParams::setup()
{
    switch(loc) {
        case loc_unspecified:
            fel = consumer_location_type_t::unspecified;
            break;
        case ad_global_freq:
        case ad_voice_freq:
            fel = consumer_location_type_t::freq;
            setpresettype("Pseqfrequency");
            break;
        case ad_global_amp:
        case ad_voice_amp:
            fel = consumer_location_type_t::amp;
            setpresettype("Pseqamplitude");
            break;
        case ad_global_filter:
        case ad_voice_filter:
            fel = consumer_location_type_t::filter;
            setpresettype("Pseqfilter");
            break;
        default:
            throw std::logic_error("Invalid envelope consumer location");
    }

    defaults();
}

// TODO: reuse
SEQParams::SEQParams(const AbsTime *time_) :
    SEQParams(2.0f, 0.0f, 20.0f, 8, 0.0f, false, loc_unspecified, time_)
{
}

SEQParams::SEQParams(float freq_,
                     float intensity_,
                     float cutoff_,
                     unsigned char steps_,
                     float delay_,
                     bool continous_,
                     consumer_location_t loc,
                     const AbsTime *time_) : loc(loc),
                                             time(time_),
                                             last_update_timestamp(0) {
    Dfreq       = freq_;
    Dintensity  = intensity_;
    Dcutoff     = cutoff_;
    Ddelay      = delay_;
    Dcontinous  = continous_;
    Dsteps      = steps_;

    setup();
}

SEQParams::SEQParams(consumer_location_t loc_,
                     const AbsTime *time_) : loc(loc_),
                                             time(time_),
                                             last_update_timestamp(0) {

    auto init =
        [&](float val_)
    {
        Dfreq       = 2.0f;
        Dintensity  = 0.0f;
        Dcutoff     = MAX_CUTOFF;
        Dsteps      = 8;
        Ddelay      = 0.0f;
        Dcontinous  = false;
        for(auto i = 0; i<NUM_SEQ_STEPS; ++i)
            Dsequence[i] = val_;

    };

    switch(loc)
    {
        case ad_global_amp:
        case ad_voice_amp:     init(1.0f); break;
        case ad_global_freq:
        case ad_global_filter:
        case ad_voice_freq:
        case ad_voice_filter:  init(0.0f); break;
        default: throw std::logic_error("Invalid SEQ consumer location");
    }
    setup();
}

SEQParams::~SEQParams()
{}

void SEQParams::defaults()
{
    freq       = Dfreq;
    intensity  = Dintensity;
    cutoff     = Dcutoff;
    delay      = Ddelay;
    continous  = Dcontinous;
    memcpy(sequence, Dsequence, NUM_SEQ_STEPS*sizeof(*sequence));
    numerator   = 0;
    denominator = 4;
}


void SEQParams::add2XML(XMLwrapper& xml)
{
    xml.addparreal("freq", freq);
    xml.addparreal("intensity", intensity);
    xml.addparreal("cutoff", cutoff);
    xml.addpar("steps", steps);
    xml.addparreal("delay", delay);
    xml.addparbool("continous", continous);
    xml.addpar("numerator", numerator);
    xml.addpar("denominator", denominator);
    xml.beginbranch("SEQUENCE");
    for(int n = 0; n < NUM_SEQ_STEPS; ++n) {
        const bool has_default_value =
            ( (sequence[n] == 1.0f) &&
              (loc == ad_global_amp || loc == ad_voice_amp) ) ||
            ( (sequence[n] == 0.0f) &&
             !(loc == ad_global_amp || loc == ad_voice_amp) );
        if(has_default_value)
            continue;
        xml.beginbranch("STEP", n + 1);
        xml.addparreal("value", sequence[n]);
        xml.endbranch();
    }
    xml.endbranch(); // SEQUENCE
}

void SEQParams::getfromXML(XMLwrapper& xml)
{
    freq       = xml.getparreal("freq", 2.0f);
    intensity  = xml.getparreal("intensity", 0.0f);
    cutoff     = xml.getparreal("cutoff", MAX_CUTOFF);
    steps      = xml.getpar("steps", 8,1,127);
    delay      = xml.getparreal("delay", 0.0f);
    continous  = xml.getparbool("continous", false);
    numerator  = xml.getpar("numerator", 0, 0, 99);
    denominator= xml.getpar("denominator", 4, 0, 99);

    if(xml.enterbranch("SEQUENCE")) {
        for(int n = 0; n < NUM_SEQ_STEPS; ++n) {
            if(xml.enterbranch("STEP", n + 1) == 0)
                continue;
            sequence[n] = xml.getparreal("value", (loc == ad_global_amp || loc == ad_voice_amp) ? 1.0f : 0.0f);
            xml.exitbranch();
        }
        xml.exitbranch(); // SEQUENCE
    }

}

#define COPY(y) this->y=x.y
void SEQParams::paste(SEQParams &x)
{
    COPY(freq);
    COPY(intensity);
    COPY(cutoff);
    COPY(steps);
    COPY(delay);
    COPY(continous);
    COPY(numerator);
    COPY(denominator);
    memcpy(sequence, x.sequence, NUM_SEQ_STEPS*sizeof(*sequence));

    if ( time ) {
        last_update_timestamp = time->time();
    }
}
#undef COPY

}

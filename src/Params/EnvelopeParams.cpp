/*
  ZynAddSubFX - a software synthesizer

  EnvelopeParams.cpp - Parameters for Envelope
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cstdlib>
#include <cassert>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

#include "zyn-version.h"
#include "EnvelopeParams.h"
#include "../Misc/Util.h"
#include "../Misc/Time.h"

using namespace rtosc;

namespace zyn {

#define rObject EnvelopeParams
#define rBegin [](const char *msg, RtData &d) { \
    EnvelopeParams *env = (rObject*) d.obj
#define rEnd }

static const rtosc::Ports localPorts = {
    rSelf(EnvelopeParams),
    rPaste,
#undef  rChangeCb
#define rChangeCb if(!obj->Pfreemode) obj->converttofree(); if (obj->time) { \
        obj->last_update_timestamp = obj->time->time(); }
    rOption(loc, rProp(internal),
            rOptions(ad_global_amp, ad_global_freq, ad_global_filter,
                     ad_voice_amp, ad_voice_freq, ad_voice_filter,
                     ad_voice_fm_amp, ad_voice_fm_freq,
                     sub_freq, sub_bandwidth),
            "location of the envelope"),
    rToggle(Pfreemode, rDefault(false), "Complex Envelope Definitions"),
#undef  rChangeCb
#define rChangeCb if(!obj->Pfreemode) obj->converttofree(); \
                  if(obj->time) { obj->last_update_timestamp = obj->time->time(); }
    rParamZyn(Penvpoints, rProp(internal), rDefaultDepends(loc),
            rPresetAtMulti(3, ad_global_freq, ad_voice_freq,
                              ad_voice_fm_freq,
                              sub_freq, sub_bandwidth),
            rDefault(4),
            "Number of points in complex definition"),
    rParamZyn(Penvsustain, rDefaultDepends(loc),
            rPresetAtMulti(1, ad_global_freq, ad_voice_freq,
                              ad_voice_fm_freq,
                              sub_freq, sub_bandwidth),
            rDefault(2),
            "Location of the sustain point"),
    rParams(Penvdt,  MAX_ENVELOPE_POINTS, "Envelope Delay Times"),
    rParams(Penvval, MAX_ENVELOPE_POINTS, "Envelope Values"),
    rParamZyn(Penvstretch,  rShort("stretch"), rDefaultDepends(loc),
            rPresetAtMulti(0, ad_global_freq, ad_global_filter,
                              ad_voice_freq, ad_voice_filter, ad_voice_fm_freq),
            rDefault(64),
            "Stretch with respect to frequency"),
    rToggle(Pforcedrelease, rShort("frcr"), rDefaultDepends(loc),
            rPresetAtMulti(true, ad_global_amp, ad_global_filter, ad_voice_amp,
                                 ad_voice_fm_amp),
            rDefault(false),
            "Force Envelope to fully evaluate"),
    rToggle(Plinearenvelope, rShort("lin/log"), rDefault(false),
            "Linear or Logarithmic Envelopes"),
    rParamZyn(PA_dt,  rShort("a.dt"), rDefaultDepends(loc),
              rPreset(ad_global_freq, 50),   rPreset(ad_global_filter, 40),
              rPreset(ad_voice_freq, 40),    rPreset(ad_voice_filter, 70),
              rPreset(ad_voice_fm_freq, 90), rPreset(ad_voice_fm_amp, 80),
              rPreset(sub_freq, 50),         rPreset(sub_bandwidth, 70),
              rDefault(0),
              "Attack Time"),
    rParamZyn(PA_val, rShort("a.val"), rDefaultDepends(loc),
              rPreset(ad_voice_freq, 30),    rPreset(ad_voice_filter, 90),
              rPreset(ad_voice_fm_freq, 20),
              rPreset(sub_freq, 30),         rPreset(sub_bandwidth, 100),
              rDefault(64),
              "Attack Value"),
    rParamZyn(PD_dt,  rShort("d.dt"),  rDefaultDepends(loc),
              rPreset(ad_global_amp, 40),    rPreset(ad_global_filter, 70),
              rPreset(ad_voice_amp, 100),    rPreset(ad_voice_filter, 70),
              rPreset(ad_voice_fm_amp, 90),
              rDefault(10),
              "Decay Time"),
    rParamZyn(PD_val, rShort("d.val"), rDefaultDepends(loc),
              rDefault(64), rPreset(ad_voice_filter, 40),
              "Decay Value"),
    rParamZyn(PS_val, rShort("s.val"), rDefaultDepends(loc),
              rDefault(64),
              rPresetAtMulti(127, ad_global_amp, ad_voice_amp, ad_voice_fm_amp),
              "Sustain Value"),
    rParamZyn(PR_dt,  rShort("r.dt"),  rDefaultDepends(loc),
              rPreset(ad_global_amp, 25),
              rPreset(ad_voice_amp, 100),    rPreset(ad_voice_filter, 10),
              rPreset(ad_voice_fm_freq, 80), rPreset(ad_voice_fm_amp, 100),
              rDefault(60),
              "Release Time"),
    rParamZyn(PR_val, rShort("r.val"), rDefaultDepends(loc),
              rPresetAtMulti(40, ad_voice_filter, ad_voice_fm_freq),
              rDefault(64),
              "Release Value"),
    {"Envmode:", rDoc("Envelope variant type"), NULL,
        rBegin;
        d.reply(d.loc, "i", env->Envmode);
        rEnd},

    {"envdt", rDoc("Envelope Delay Times"), NULL,
        rBegin;
        const int N = MAX_ENVELOPE_POINTS;
        const int M = rtosc_narguments(msg);
        if(M == 0) {
            rtosc_arg_t args[N];
            char arg_types[N+1] = {};
            for(int i=0; i<N; ++i) {
                args[i].f    = env->getdt(i);
                arg_types[i] = 'f';
            }
            d.replyArray(d.loc, arg_types, args);
        } else {
            for(int i=0; i<N && i<M; ++i)
                env->Penvdt[i] = env->inv_dt(rtosc_argument(msg, i).f);
        }
        rEnd},
    {"envval", rDoc("Envelope Delay Times"), NULL,
        rBegin;
        const int N = MAX_ENVELOPE_POINTS;
        const int M = rtosc_narguments(msg);
        if(M == 0) {
            rtosc_arg_t args[N];
            char arg_types[N+1] = {};
            for(int i=0; i<N; ++i) {
                args[i].f    = env->Penvval[i]/127.0f;
                arg_types[i] = 'f';
            }
            d.replyArray(d.loc, arg_types, args);
        } else {
            for(int i=0; i<N && i<M; ++i) {
                env->Penvval[i] = limit(roundf(rtosc_argument(msg,i).f*127.0f), 0.0f, 127.0f);
            }
        }
        rEnd},

    {"addPoint:i", rProp(internal) rDoc("Add point to envelope"), NULL,
        rBegin;
        const int curpoint = rtosc_argument(msg, 0).i;
        //int curpoint=freeedit->lastpoint;
        if (curpoint<0 || curpoint>env->Penvpoints || env->Penvpoints>=MAX_ENVELOPE_POINTS)
            return;

        for (int i=env->Penvpoints; i>=curpoint+1; i--) {
            env->Penvdt[i]=env->Penvdt[i-1];
            env->Penvval[i]=env->Penvval[i-1];
        }

        if (curpoint==0)
            env->Penvdt[1]=64;

        env->Penvpoints++;
        if (curpoint<=env->Penvsustain)
            env->Penvsustain++;
        rEnd},
    {"delPoint:i", rProp(internal) rDoc("Delete Envelope Point"), NULL,
        rBegin;
        const int curpoint=rtosc_argument(msg, 0).i;
        if(curpoint<1 || curpoint>=env->Penvpoints-1 || env->Penvpoints<=3)
            return;

        for (int i=curpoint+1;i<env->Penvpoints;i++){
            env->Penvdt[i-1]=env->Penvdt[i];
            env->Penvval[i-1]=env->Penvval[i];
        };

        env->Penvpoints--;

        if (curpoint<=env->Penvsustain)
            env->Penvsustain--;

        rEnd},
};
#undef  rChangeCb

const rtosc::Ports &EnvelopeParams::ports = localPorts;

EnvelopeParams::EnvelopeParams(unsigned char Penvstretch_,
                               unsigned char Pforcedrelease_,
                               const AbsTime *time_):
        time(time_), last_update_timestamp(0)
{
    PA_dt  = 10;
    PD_dt  = 10;
    PR_dt  = 10;
    PA_val = 64;
    PD_val = 64;
    PS_val = 64;
    PR_val = 64;

    for(int i = 0; i < MAX_ENVELOPE_POINTS; ++i) {
        Penvdt[i]  = 32;
        Penvval[i] = 64;
    }
    Penvdt[0]       = 0; //no used
    Penvsustain     = 1;
    Penvpoints      = 1;
    Envmode         = 1;
    Penvstretch     = Penvstretch_;
    Pforcedrelease  = Pforcedrelease_;
    Pfreemode       = 1;
    Plinearenvelope = 0;

    store2defaults();
}

EnvelopeParams::~EnvelopeParams()
{}

#define COPY(y) this->y = ep.y
void EnvelopeParams::paste(const EnvelopeParams &ep)
{

    COPY(Pfreemode);
    COPY(Penvpoints);
    COPY(Penvsustain);
    for(int i=0; i<MAX_ENVELOPE_POINTS; ++i) {
        this->Penvdt[i]  = ep.Penvdt[i];
        this->Penvval[i] = ep.Penvval[i];
    }
    COPY(Penvstretch);
    COPY(Pforcedrelease);
    COPY(Plinearenvelope);

    COPY(PA_dt);
    COPY(PD_dt);
    COPY(PR_dt);
    COPY(PA_val);
    COPY(PD_val);
    COPY(PS_val);
    COPY(PR_val);

    if ( time ) {
        last_update_timestamp = time->time();
    }
}
#undef COPY

void EnvelopeParams::init(zyn::consumer_location_t _loc)
{
    switch(loc = _loc)
    {
        case ad_global_amp:    ADSRinit_dB(0, 40, 127, 25); break;
        case ad_global_freq:   ASRinit(64, 50, 64, 60); break;
        case ad_global_filter:
        case sub_filter:       ADSRinit_filter(64, 40, 64, 70, 60, 64); break;
        case ad_voice_amp:     ADSRinit_dB(0, 100, 127, 100); break;
        case ad_voice_freq:    ASRinit(30, 40, 64, 60); break;
        case ad_voice_filter:  ADSRinit_filter(90, 70, 40, 70, 10, 40); break;
        case ad_voice_fm_freq: ASRinit(20, 90, 40, 80); break;
        case ad_voice_fm_amp:  ADSRinit(80, 90, 127, 100); break;
        case sub_freq:         ASRinit(30, 50, 64, 60); break;
        case sub_bandwidth:    ASRinit_bw(100, 70, 64, 60); break;
        default: throw std::logic_error("Invalid envelope consumer location");
    };
}

float EnvelopeParams::getdt(char i) const
{
    return EnvelopeParams::dt(Penvdt[(int)i]);
}

float EnvelopeParams::dt(char val)
{
    return (powf(2.0f, val / 127.0f * 12.0f) - 1.0f) * 10.0f; //miliseconds
}

char EnvelopeParams::inv_dt(float val)
{
    int ival = roundf(logf(val/10.0f + 1.0f)/logf(2.0f) * 127.0f/12.0f);
    return limit(ival, 0, 127);
}


/*
 * ADSR/ASR... initialisations
 */
void EnvelopeParams::ADSRinit(char A_dt, char D_dt, char S_val, char R_dt)
{
    setpresettype("Penvamplitude");
    Envmode   = 1;
    PA_dt     = A_dt;
    PD_dt     = D_dt;
    PS_val    = S_val;
    PR_dt     = R_dt;
    Pfreemode = 0;
    converttofree();

    store2defaults();
}

void EnvelopeParams::ADSRinit_dB(char A_dt, char D_dt, char S_val, char R_dt)
{
    setpresettype("Penvamplitude");
    Envmode   = 2;
    PA_dt     = A_dt;
    PD_dt     = D_dt;
    PS_val    = S_val;
    PR_dt     = R_dt;
    Pfreemode = 0;
    converttofree();

    store2defaults();
}

void EnvelopeParams::ASRinit(char A_val, char A_dt, char R_val, char R_dt)
{
    setpresettype("Penvfrequency");
    Envmode   = 3;
    PA_val    = A_val;
    PA_dt     = A_dt;
    PR_val    = R_val;
    PR_dt     = R_dt;
    Pfreemode = 0;
    converttofree();

    store2defaults();
}

void EnvelopeParams::ADSRinit_filter(char A_val,
                                     char A_dt,
                                     char D_val,
                                     char D_dt,
                                     char R_dt,
                                     char R_val)
{
    setpresettype("Penvfilter");
    Envmode   = 4;
    PA_val    = A_val;
    PA_dt     = A_dt;
    PD_val    = D_val;
    PD_dt     = D_dt;
    PR_dt     = R_dt;
    PR_val    = R_val;
    Pfreemode = 0;
    converttofree();
    store2defaults();
}

void EnvelopeParams::ASRinit_bw(char A_val, char A_dt, char R_val, char R_dt)
{
    setpresettype("Penvbandwidth");
    Envmode   = 5;
    PA_val    = A_val;
    PA_dt     = A_dt;
    PR_val    = R_val;
    PR_dt     = R_dt;
    Pfreemode = 0;
    converttofree();
    store2defaults();
}

/*
 * Convert the Envelope to freemode
 */
void EnvelopeParams::converttofree()
{
    switch(Envmode) {
        case 1:
        case 2:
            Penvpoints  = 4;
            Penvsustain = 2;
            Penvval[0]  = 0;
            Penvdt[1]   = PA_dt;
            Penvval[1]  = 127;
            Penvdt[2]   = PD_dt;
            Penvval[2]  = PS_val;
            Penvdt[3]   = PR_dt;
            Penvval[3]  = 0;
            break;
        case 3:
        case 5:
            Penvpoints  = 3;
            Penvsustain = 1;
            Penvval[0]  = PA_val;
            Penvdt[1]   = PA_dt;
            Penvval[1]  = 64;
            Penvdt[2]   = PR_dt;
            Penvval[2]  = PR_val;
            break;
        case 4:
            Penvpoints  = 4;
            Penvsustain = 2;
            Penvval[0]  = PA_val;
            Penvdt[1]   = PA_dt;
            Penvval[1]  = PD_val;
            Penvdt[2]   = PD_dt;
            Penvval[2]  = 64;
            Penvdt[3]   = PR_dt;
            Penvval[3]  = PR_val;
            break;
    }
}




void EnvelopeParams::add2XML(XMLwrapper& xml)
{
    xml.addparbool("free_mode", Pfreemode);
    xml.addpar("env_points", Penvpoints);
    xml.addpar("env_sustain", Penvsustain);
    xml.addpar("env_stretch", Penvstretch);
    xml.addparbool("forced_release", Pforcedrelease);
    xml.addparbool("linear_envelope", Plinearenvelope);
    xml.addpar("A_dt", PA_dt);
    xml.addpar("D_dt", PD_dt);
    xml.addpar("R_dt", PR_dt);
    xml.addpar("A_val", PA_val);
    xml.addpar("D_val", PD_val);
    xml.addpar("S_val", PS_val);
    xml.addpar("R_val", PR_val);

    if((Pfreemode != 0) || (!xml.minimal))
        for(int i = 0; i < Penvpoints; ++i) {
            xml.beginbranch("POINT", i);
            if(i != 0)
                xml.addpar("dt", Penvdt[i]);
            xml.addpar("val", Penvval[i]);
            xml.endbranch();
        }
}

float EnvelopeParams::env_dB2rap(float db) {
    return (powf(10.0f, db / 20.0f) - 0.01)/.99f;
}

float EnvelopeParams::env_rap2dB(float rap) {
    return 20.0f * log10f(rap * 0.99f + 0.01);
}

/**
    since commit 5334d94283a513ae42e472aa020db571a3589fb9, i.e. between
    versions 2.4.3 and 2.4.4, the amplitude envelope has been converted
    differently from dB to rap for AmplitudeEnvelope (mode 2)
    this converts the values read from an XML file once
*/
struct version_fixer_t
{
    const bool mismatch;
public:
    int operator()(int input) const
    {
        return (mismatch)
            // The errors occured when calling env_dB2rap. Let f be the
            // conversion function for mode 2 (see Envelope.cpp), then we
            // load values with (let "o" be the function composition symbol):
            //   f^{-1} o (env_dB2rap^{-1}) o dB2rap o f
            // from the xml file. This results in the following formula:
            ? roundf(127.0f * (0.5f *
                   log10f( 0.01f + 0.99f *
                                       powf(100, input/127.0f - 1))
                               + 1))
            : input;
    }
    version_fixer_t(const version_type& fileversion, int env_mode) :
        mismatch(fileversion < version_type(2,4,4) &&
                 (env_mode == 2)) {}
};

void EnvelopeParams::getfromXML(XMLwrapper& xml)
{
    Pfreemode       = xml.getparbool("free_mode", Pfreemode);
    Penvpoints      = xml.getpar127("env_points", Penvpoints);
    Penvsustain     = xml.getpar127("env_sustain", Penvsustain);
    Penvstretch     = xml.getpar127("env_stretch", Penvstretch);
    Pforcedrelease  = xml.getparbool("forced_release", Pforcedrelease);
    Plinearenvelope = xml.getparbool("linear_envelope", Plinearenvelope);

    version_fixer_t version_fix(xml.fileversion(), Envmode);

    PA_dt  = xml.getpar127("A_dt", PA_dt);
    PD_dt  = xml.getpar127("D_dt", PD_dt);
    PR_dt  = xml.getpar127("R_dt", PR_dt);
    PA_val = version_fix(xml.getpar127("A_val", PA_val));
    PD_val = version_fix(xml.getpar127("D_val", PD_val));
    PS_val = version_fix(xml.getpar127("S_val", PS_val));
    PR_val = version_fix(xml.getpar127("R_val", PR_val));

    for(int i = 0; i < Penvpoints; ++i) {
        if(xml.enterbranch("POINT", i) == 0)
            continue;
        if(i != 0)
            Penvdt[i] = xml.getpar127("dt", Penvdt[i]);
        Penvval[i] = version_fix(xml.getpar127("val", Penvval[i]));
        xml.exitbranch();
    }

    if(!Pfreemode)
        converttofree();
}


void EnvelopeParams::defaults()
{
    Penvstretch     = Denvstretch;
    Pforcedrelease  = Dforcedrelease;
    Plinearenvelope = Dlinearenvelope;
    PA_dt     = DA_dt;
    PD_dt     = DD_dt;
    PR_dt     = DR_dt;
    PA_val    = DA_val;
    PD_val    = DD_val;
    PS_val    = DS_val;
    PR_val    = DR_val;
    Pfreemode = 0;
    converttofree();
}

void EnvelopeParams::store2defaults()
{
    Denvstretch     = Penvstretch;
    Dforcedrelease  = Pforcedrelease;
    Dlinearenvelope = Plinearenvelope;
    DA_dt  = PA_dt;
    DD_dt  = PD_dt;
    DR_dt  = PR_dt;
    DA_val = PA_val;
    DD_val = PD_val;
    DS_val = PS_val;
    DR_val = PR_val;
}

}

/*
  ZynAddSubFX - a software synthesizer

  SUBnoteParameters.cpp - Parameters for SUBnote (SUBsynth)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "../globals.h"
#include "SUBnoteParameters.h"
#include "EnvelopeParams.h"
#include "FilterParams.h"
#include "../Misc/Util.h"
#include "../Misc/Time.h"
#include <cstdio>
#include <cmath>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
using namespace rtosc;

namespace zyn {

#define rObject SUBnoteParameters
#define rBegin [](const char *msg, RtData &d) { \
    SUBnoteParameters *obj = (SUBnoteParameters*) d.obj
#define rEnd }


#undef rChangeCb
#define rChangeCb if (obj->time) { obj->last_update_timestamp = obj->time->time(); }
static const rtosc::Ports SUBnotePorts = {
    rSelf(SUBnoteParameters),
    rPasteRt,
    rToggle(Pstereo,    rShort("stereo"), rDefault(true), "Stereo Enable"),
    rParamF(Volume,  rShort("volume"), rDefault(0), rUnit(dB), rLinear(-60.0f,20.0f), "Volume"),
    rParamZyn(PPanning, rShort("panning"), rDefault(64), "Left Right Panning"),
    rParamF(AmpVelocityScaleFunction, rShort("sense"), rDefault(70.86),
        rLinear(0.0, 100.0), "Amplitude Velocity Sensing function"),
    {"PAmpVelocityScaleFunction::i", rShort("sense") rProp(parameter) rLinear(0,127)
        rDefault(90) rDoc("Amplitude Velocity Sensing function"), 0,
        [](const char *m, rtosc::RtData &d) {
            SUBnoteParameters *obj = (SUBnoteParameters*)d.obj;
            if (rtosc_narguments(m)==0) {
                d.reply(d.loc, "i", (int) roundf(obj->AmpVelocityScaleFunction * 127.0f / 100.0f));
            } else if(rtosc_narguments(m)==1 && rtosc_type(m,0)=='i') {
                const uint8_t value = limit<char>(rtosc_argument(m, 0).i, 0, 127);
                obj->AmpVelocityScaleFunction = value * 100.0f / 127.0f;
                d.broadcast(d.loc, "i", value);
                rChangeCb
            }
    }},
    rParamI(PDetune,       rShort("detune"), rLinear(0, 16383), rDefault(8192),
        "Detune in detune type units"),
    rParamI(PCoarseDetune, rShort("cdetune"), rDefault(0), "Coarse Detune"),
    //Real values needed
    rOption(PDetuneType,               rShort("det. scl."),
            rOptions(L35 cents, L10 cents, E100 cents, E1200 cents),
            rDefaultId(L10 cents), "Detune Scale"),
    rToggle(PFreqEnvelopeEnabled,      rShort("enable"), rDefault(false),
        "Enable for Frequency Envelope"),
    rToggle(PBandWidthEnvelopeEnabled, rShort("enable"), rDefault(false),
        "Enable for Bandwidth Envelope"),
    rToggle(PGlobalFilterEnabled,                 rShort("enable"),
        rDefault(false), "Enable for Global Filter"),
    rParamZyn(PGlobalFilterVelocityScale,         rShort("scale"), rDefault(0),
        "Filter Velocity Magnitude"),
    rParamZyn(PGlobalFilterVelocityScaleFunction, rShort("sense"), rDefault(64),
        "Filter Velocity Function Shape"),
    //rRecur(FreqEnvelope, EnvelopeParams),
    //rToggle(),//continue
    rToggle(Pfixedfreq,     rShort("fixed freq"), rDefault(false),
        "Base frequency fixed frequency enable"),
    rParamZyn(PfixedfreqET, rShort("fixed ET"),   rDefault(0),
        "Equal temeperate control for fixed frequency operation"),
    rParamZyn(PBendAdjust,  rShort("bend"),       rDefault(88),
        "Pitch bend adjustment"),
    rParamZyn(POffsetHz,    rShort("+ Hz"),       rDefault(64),
        "Voice constant offset"),
#undef rChangeCb
#define rChangeCb obj->updateFrequencyMultipliers(); if (obj->time) { \
    obj->last_update_timestamp = obj->time->time(); }
    rParamI(POvertoneSpread.type, rMap(min, 0), rMap(max, 7), rShort("spread type")
            rOptions(Harmonic, ShiftU, ShiftL, PowerU, PowerL, Sine, Power, Shift),
            rDefault(Harmonic)
            "Spread of harmonic frequencies"),
    rParamI(POvertoneSpread.par1, rMap(min, 0), rMap(max, 255), rShort("p1"),
            rDefault(0), "Overtone Parameter"),
    rParamI(POvertoneSpread.par2, rMap(min, 0), rMap(max, 255), rShort("p2"),
            rDefault(0), "Overtone Parameter"),
    rParamI(POvertoneSpread.par3, rMap(min, 0), rMap(max, 255), rShort("forceH"),
            rDefault(0), "Force Overtones To Harmonics"),
#undef rChangeCb
#define rChangeCb if (obj->time) { obj->last_update_timestamp = obj->time->time(); }
    rParamI(Pnumstages, rShort("stages"), rMap(min, 1), rMap(max, 5),
            rDefault(2), "Number of filter stages"),
    rParamZyn(Pbandwidth, rShort("bandwidth"), rDefault(40),
              "Bandwidth of filters"),
    rParamZyn(Phmagtype,  rShort("mag. type"),
              rOptions(linear, -40dB, -60dB, -80dB, -100dB),
              rDefault(linear), "Magnitude scale"),
    rArray(Phmag, MAX_SUB_HARMONICS, rDefault([127 0 0 ...]),
           "Harmonic magnitudes"),
    rArray(Phrelbw, MAX_SUB_HARMONICS, rDefault([64 ...]),
           "Relative bandwidth"),
    rParamZyn(Pbwscale, rShort("stretch"), rDefault(64),
              "Bandwidth scaling with frequency"),
    rRecurp(AmpEnvelope,          "Amplitude envelope"),
    rRecurp(FreqEnvelope,         "Frequency Envelope"),
    rRecurp(BandWidthEnvelope,    "Bandwidth Envelope"),
    rRecurp(GlobalFilterEnvelope, "Post Filter Envelope"),
    rRecurp(GlobalFilter,         "Post Filter"),
    rOption(Pstart, rShort("initial"), rOptions(zero, random, ones),
            rDefault(random),
            "How harmonics are initialized"),
    {"PVolume::i", rShort("volume") rLinear(0,127)
        rDoc("Volume"), NULL,
        [](const char *msg, RtData &d)
        {
            rObject *obj = (rObject *)d.obj;
            if (!rtosc_narguments(msg))
                d.reply(d.loc, "i", (int)roundf(96.0f * (1.0f + obj->Volume/60.0f)));
            else
                obj->Volume = -60.0f * (1.0f - rtosc_argument(msg, 0).i / 96.0f);
        }},
    {"clear:", rDoc("Reset all harmonics to equal bandwidth/zero amplitude"), NULL,
        rBegin;
        (void) msg;
        for(int i=0; i<MAX_SUB_HARMONICS; ++i) {
            obj->Phmag[i]   = 0;
            obj->Phrelbw[i] = 64;
        }
        obj->Phmag[0] = 127;
        rEnd},
    {"detunevalue:", rDoc("Get note detune value"), NULL,
        rBegin;
        (void) msg;
        d.reply(d.loc, "f", getdetune(obj->PDetuneType, 0, obj->PDetune));
        rEnd},
    //weird stuff for PCoarseDetune
    {"octave::c:i", rProp(parameter) rShort("octave") rLinear(-8,7)
        rDoc("Note octave shift"), NULL,
        rBegin;
            auto get_octave = [&obj](){
                int k=obj->PCoarseDetune/1024;
                if (k>=8) k-=16;
                return k;
            };
            if(!rtosc_narguments(msg)) {
                d.reply(d.loc, "i", get_octave());
            } else {
                int k=(int) rtosc_argument(msg, 0).i;
                if (k<0) k+=16;
                obj->PCoarseDetune = k*1024 + obj->PCoarseDetune%1024;
                d.broadcast(d.loc, "i", get_octave());
            }
        rEnd},
    {"coarsedetune::c:i", rProp(parameter) rShort("coarse") rLinear(-64, 63)
        rDoc("Note coarse detune"), NULL,
        rBegin;
            auto get_coarse = [&obj](){
                int k=obj->PCoarseDetune%1024;
                if (k>=512) k-=1024;
                return k;
            };
            if(!rtosc_narguments(msg)) {
                d.reply(d.loc, "i", get_coarse());
            } else {
                int k=(int) rtosc_argument(msg, 0).i;
                if (k<0) k+=1024;
                obj->PCoarseDetune = k + (obj->PCoarseDetune/1024)*1024;
                d.broadcast(d.loc, "i", get_coarse());
            }
        rEnd},
    {"response:", rDoc("Filter response at 440Hz. with 48kHz sample rate\n\n"
            "Format: stages, filter*active_filters\n"
            "        filter = [frequency, bandwidth, amplitude]"),
    NULL,
    rBegin;
    (void) msg;

    //Identify the active harmonics
    int pos[MAX_SUB_HARMONICS];
    int harmonics;
    obj->activeHarmonics(pos, harmonics);

    float base_freq = 440.0f;

    char        types[3*MAX_SUB_HARMONICS+2];
    rtosc_arg_t args[3*MAX_SUB_HARMONICS+1];

    args[0].i = obj->Pnumstages;
    types[0]  = 'i';

    for(int n=0; n<harmonics; ++n) {
        const float freq = base_freq * obj->POvertoneFreqMult[pos[n]];
        //the bandwidth is not absolute(Hz); it is relative to frequency
        const float bw = obj->convertBandwidth(obj->Pbandwidth,
                obj->Pnumstages, freq, obj->Pbwscale, obj->Phrelbw[pos[n]]);

        //try to keep same amplitude on all freqs and bw. (empirically)
        const float hgain = obj->convertHarmonicMag(obj->Phmag[pos[n]],
                                                    obj->Phmagtype);
        const float gain  = hgain * sqrt(1500.0f / (bw * freq));

        int base = 1+3*n;
        args[base + 0].f = freq;
        args[base + 1].f = bw;
        args[base + 2].f = gain;
        types[base + 0] = 'f';
        types[base + 1] = 'f';
        types[base + 2] = 'f';
    }

    types[3*harmonics+1] = 0;
    d.replyArray(d.loc, types, args);
    rEnd},


};
#undef rChangeCb
#undef rBegin
#undef rEnd

const rtosc::Ports &SUBnoteParameters::ports = SUBnotePorts;

SUBnoteParameters::SUBnoteParameters(const AbsTime *time_)
        : Presets(), time(time_), last_update_timestamp(0)
{
    setpresettype("Psubsynth");
    AmpEnvelope = new EnvelopeParams(64, 1, time_);
    AmpEnvelope->init(ad_global_amp);
    FreqEnvelope = new EnvelopeParams(64, 0, time_);
    FreqEnvelope->init(sub_freq);
    BandWidthEnvelope = new EnvelopeParams(64, 0, time_);
    BandWidthEnvelope->init(sub_bandwidth);

    GlobalFilter = new FilterParams(sub_filter, time_);
    GlobalFilterEnvelope = new EnvelopeParams(0, 1, time_);
    GlobalFilterEnvelope->init(sub_filter);

    defaults();
}

void SUBnoteParameters::activeHarmonics(int *pos, int &harmonics) const
{
    harmonics = 0;
    for(int n = 0; n < MAX_SUB_HARMONICS; ++n) {
        if(Phmag[n] == 0)
            continue;
        pos[harmonics++] = n;
    }
}

float SUBnoteParameters::convertBandwidth(int bw_, int stages, float freq,
        int scale, int relbw)
{
    //the bandwidth is not absolute(Hz); it is relative to frequency
    float bw = powf(10, (bw_ - 127.0f) / 127.0f * 4) * stages;

    //Bandwidth Scale
    bw *= powf(1000 / freq, (scale - 64.0f) / 64.0f * 3.0f);

    //Relative BandWidth
    bw *= powf(100, (relbw - 64.0f) / 64.0f);

    if(bw > 25.0f)
        bw = 25.0f;

    return bw;
}

float SUBnoteParameters::convertHarmonicMag(int mag, int type)
{
    const float hmagnew = 1.0f - mag / 127.0f;

    switch(type) {
        case 1:
            return expf(hmagnew * logf(0.01f));
            break;
        case 2:
            return expf(hmagnew * logf(0.001f));
            break;
        case 3:
            return expf(hmagnew * logf(0.0001f));
            break;
        case 4:
            return expf(hmagnew * logf(0.00001f));
            break;
        default:
            return 1.0f - hmagnew;
    }
}


void SUBnoteParameters::defaults()
{
    Volume  = 0;
    PPanning = 64;
    AmpVelocityScaleFunction = 70.86;

    Pfixedfreq   = 0;
    PfixedfreqET = 0;
    PBendAdjust = 88; // 64 + 24
    POffsetHz = 64;
    Pnumstages   = 2;
    Pbandwidth   = 40;
    Phmagtype    = 0;
    Pbwscale     = 64;
    Pstereo      = 1;
    Pstart = 1;

    PDetune = 8192;
    PCoarseDetune = 0;
    PDetuneType   = 1;
    PFreqEnvelopeEnabled      = 0;
    PBandWidthEnvelopeEnabled = 0;

    POvertoneSpread.type = 0;
    POvertoneSpread.par1 = 0;
    POvertoneSpread.par2 = 0;
    POvertoneSpread.par3 = 0;
    updateFrequencyMultipliers();

    for(int n = 0; n < MAX_SUB_HARMONICS; ++n) {
        Phmag[n]   = 0;
        Phrelbw[n] = 64;
    }
    Phmag[0] = 127;

    PGlobalFilterEnabled = 0;
    PGlobalFilterVelocityScale = 0;
    PGlobalFilterVelocityScaleFunction = 64;

    AmpEnvelope->defaults();
    FreqEnvelope->defaults();
    BandWidthEnvelope->defaults();
    GlobalFilter->defaults();
    GlobalFilterEnvelope->defaults();
}



SUBnoteParameters::~SUBnoteParameters()
{
    delete (AmpEnvelope);
    delete (FreqEnvelope);
    delete (BandWidthEnvelope);
    delete (GlobalFilter);
    delete (GlobalFilterEnvelope);
}




void SUBnoteParameters::add2XML(XMLwrapper& xml)
{
    xml.addpar("num_stages", Pnumstages);
    xml.addpar("harmonic_mag_type", Phmagtype);
    xml.addpar("start", Pstart);

    xml.beginbranch("HARMONICS");
    for(int i = 0; i < MAX_SUB_HARMONICS; ++i) {
        if((Phmag[i] == 0) && (xml.minimal))
            continue;
        xml.beginbranch("HARMONIC", i);
        xml.addpar("mag", Phmag[i]);
        xml.addpar("relbw", Phrelbw[i]);
        xml.endbranch();
    }
    xml.endbranch();

    xml.beginbranch("AMPLITUDE_PARAMETERS");
    xml.addparbool("stereo", Pstereo);
    xml.addparreal("volume", Volume);
    xml.addpar("panning", PPanning);
    xml.addparreal("velocity_sensing", AmpVelocityScaleFunction);
    xml.beginbranch("AMPLITUDE_ENVELOPE");
    AmpEnvelope->add2XML(xml);
    xml.endbranch();
    xml.endbranch();

    xml.beginbranch("FREQUENCY_PARAMETERS");
    xml.addparbool("fixed_freq", Pfixedfreq);
    xml.addpar("fixed_freq_et", PfixedfreqET);
    xml.addpar("bend_adjust", PBendAdjust);
    xml.addpar("offset_hz", POffsetHz);

    xml.addpar("detune", PDetune);
    xml.addpar("coarse_detune", PCoarseDetune);
    xml.addpar("overtone_spread_type", POvertoneSpread.type);
    xml.addpar("overtone_spread_par1", POvertoneSpread.par1);
    xml.addpar("overtone_spread_par2", POvertoneSpread.par2);
    xml.addpar("overtone_spread_par3", POvertoneSpread.par3);
    xml.addpar("detune_type", PDetuneType);

    xml.addpar("bandwidth", Pbandwidth);
    xml.addpar("bandwidth_scale", Pbwscale);

    xml.addparbool("freq_envelope_enabled", PFreqEnvelopeEnabled);
    if((PFreqEnvelopeEnabled != 0) || (!xml.minimal)) {
        xml.beginbranch("FREQUENCY_ENVELOPE");
        FreqEnvelope->add2XML(xml);
        xml.endbranch();
    }

    xml.addparbool("band_width_envelope_enabled", PBandWidthEnvelopeEnabled);
    if((PBandWidthEnvelopeEnabled != 0) || (!xml.minimal)) {
        xml.beginbranch("BANDWIDTH_ENVELOPE");
        BandWidthEnvelope->add2XML(xml);
        xml.endbranch();
    }
    xml.endbranch();

    xml.beginbranch("FILTER_PARAMETERS");
    xml.addparbool("enabled", PGlobalFilterEnabled);
    if((PGlobalFilterEnabled != 0) || (!xml.minimal)) {
        xml.beginbranch("FILTER");
        GlobalFilter->add2XML(xml);
        xml.endbranch();

        xml.addpar("filter_velocity_sensing",
                    PGlobalFilterVelocityScaleFunction);
        xml.addpar("filter_velocity_sensing_amplitude",
                    PGlobalFilterVelocityScale);

        xml.beginbranch("FILTER_ENVELOPE");
        GlobalFilterEnvelope->add2XML(xml);
        xml.endbranch();
    }
    xml.endbranch();
}



void SUBnoteParameters::updateFrequencyMultipliers(void) {
    float par1 = POvertoneSpread.par1 / 255.0f;
    float par1pow = powf(10.0f,
            -(1.0f - POvertoneSpread.par1 / 255.0f) * 3.0f);
    float par2 = POvertoneSpread.par2 / 255.0f;
    float par3 = 1.0f - POvertoneSpread.par3 / 255.0f;
    float result;
    float tmp = 0.0f;
    int   thresh = 0;

    for(int n = 0; n < MAX_SUB_HARMONICS; ++n) {
        float n1     = n + 1.0f;
        switch(POvertoneSpread.type) {
            case 1:
                thresh = (int)(100.0f * par2 * par2) + 1;
                if (n1 < thresh)
                    result = n1;
                else
                    result = n1 + 8.0f * (n1 - thresh) * par1pow;
                break;
            case 2:
                thresh = (int)(100.0f * par2 * par2) + 1;
                if (n1 < thresh)
                    result = n1;
                else
                    result = n1 + 0.9f * (thresh - n1) * par1pow;
                break;
            case 3:
                tmp = par1pow * 100.0f + 1.0f;
                result = powf(n / tmp, 1.0f - 0.8f * par2) * tmp + 1.0f;
                break;
            case 4:
                result = n * (1.0f - par1pow) +
                    powf(0.1f * n, 3.0f * par2 + 1.0f) *
                    10.0f * par1pow + 1.0f;
                break;

            case 5:
                result = n1 + 2.0f * sinf(n * par2 * par2 * PI * 0.999f) *
                    sqrt(par1pow);
                break;
            case 6:
                tmp    = powf(2.0f * par2, 2.0f) + 0.1f;
                result = n * powf(par1 * powf(0.8f * n, tmp) + 1.0f, tmp) +
                    1.0f;
                break;

            case 7:
                result = (n1 + par1) / (par1 + 1);
                break;
            default:
                result = n1;
        }
        float iresult = floor(result + 0.5f);
        POvertoneFreqMult[n] = iresult + par3 * (result - iresult);
    }
}

#define doPaste(x) this->x = sub.x;
#define doPPaste(x) this->x->paste(*sub.x);
void SUBnoteParameters::paste(SUBnoteParameters &sub)
{
    doPaste(Pstereo);
    doPaste(Volume);
    doPaste(PPanning);
    doPaste(AmpVelocityScaleFunction);
    doPPaste(AmpEnvelope);

    //Frequency Parameters
    doPaste(PDetune);
    doPaste(PCoarseDetune);
    doPaste(PDetuneType);
    doPaste(PBendAdjust);
    doPaste(POffsetHz);
    doPaste(PFreqEnvelopeEnabled);
    doPPaste(FreqEnvelope);
    doPaste(PBandWidthEnvelopeEnabled);
    doPPaste(BandWidthEnvelope);

    //Filter Parameters (Global)
    doPaste(PGlobalFilterEnabled);
    doPPaste(GlobalFilter);
    doPaste(PGlobalFilterVelocityScale);
    doPaste(PGlobalFilterVelocityScaleFunction);
    doPPaste(GlobalFilterEnvelope);


    //Other Parameters
    doPaste(Pfixedfreq);
    doPaste(PfixedfreqET);
    doPaste(POvertoneSpread.type);
    doPaste(POvertoneSpread.par1);
    doPaste(POvertoneSpread.par2);
    doPaste(POvertoneSpread.par3);

    for(int i=0; i<MAX_SUB_HARMONICS; ++i)
        doPaste(POvertoneFreqMult[i]);

    doPaste(Pnumstages);
    doPaste(Pbandwidth);
    doPaste(Phmagtype);

    for(int i=0; i<MAX_SUB_HARMONICS; ++i) {
        doPaste(Phmag[i]);
        doPaste(Phrelbw[i]);
    }

    doPaste(Pbwscale);
    doPaste(Pstart);

    if ( time ) {
        last_update_timestamp = time->time();
    }
}

void SUBnoteParameters::getfromXML(XMLwrapper& xml)
{
    Pnumstages = xml.getpar127("num_stages", Pnumstages);
    Phmagtype  = xml.getpar127("harmonic_mag_type", Phmagtype);
    Pstart     = xml.getpar127("start", Pstart);

    if(xml.enterbranch("HARMONICS")) {
        Phmag[0] = 0;
        for(int i = 0; i < MAX_SUB_HARMONICS; ++i) {
            if(xml.enterbranch("HARMONIC", i) == 0)
                continue;
            Phmag[i]   = xml.getpar127("mag", Phmag[i]);
            Phrelbw[i] = xml.getpar127("relbw", Phrelbw[i]);
            xml.exitbranch();
        }
        xml.exitbranch();
    }

    if(xml.enterbranch("AMPLITUDE_PARAMETERS")) {
        Pstereo  = xml.getparbool("stereo", Pstereo);
        bool upgrade_3_0_3 = (xml.fileversion() < version_type(3,0,3)) ||
            (!xml.hasparreal("volume"));
        if (upgrade_3_0_3) {
            int vol = xml.getpar127("volume", 0);
            Volume    = -60.0f * ( 1.0f - vol / 96.0f);
        } else {
            Volume    = xml.getparreal("volume", Volume);
        }
        PPanning = xml.getpar127("panning", PPanning);
        upgrade_3_0_3 = (xml.fileversion() < version_type(3,0,3)) ||
            (xml.getparreal("velocity_sensing", -1) < 0);

        if (upgrade_3_0_3) {
            int tmp_val = xml.getpar127("velocity_sensing", 0);
            AmpVelocityScaleFunction    = 100.0f * tmp_val / 127.0f;
        } else {
            AmpVelocityScaleFunction    = xml.getparreal("velocity_sensing", AmpVelocityScaleFunction);
        }


        if(xml.enterbranch("AMPLITUDE_ENVELOPE")) {
            AmpEnvelope->getfromXML(xml);
            xml.exitbranch();
        }
        xml.exitbranch();
    }

    if(xml.enterbranch("FREQUENCY_PARAMETERS")) {
        Pfixedfreq   = xml.getparbool("fixed_freq", Pfixedfreq);
        PfixedfreqET = xml.getpar127("fixed_freq_et", PfixedfreqET);
        PBendAdjust  = xml.getpar127("bend_adjust", PBendAdjust);
        POffsetHz  = xml.getpar127("offset_hz", POffsetHz);

        PDetune = xml.getpar("detune", PDetune, 0, 16383);
        PCoarseDetune = xml.getpar("coarse_detune", PCoarseDetune, 0, 16383);
        POvertoneSpread.type =
            xml.getpar127("overtone_spread_type", POvertoneSpread.type);
        POvertoneSpread.par1 =
            xml.getpar("overtone_spread_par1", POvertoneSpread.par1, 0, 255);
        POvertoneSpread.par2 =
            xml.getpar("overtone_spread_par2", POvertoneSpread.par2, 0, 255);
        POvertoneSpread.par3 =
            xml.getpar("overtone_spread_par3", POvertoneSpread.par3, 0, 255);
        updateFrequencyMultipliers();
        PDetuneType   = xml.getpar127("detune_type", PDetuneType);

        Pbandwidth = xml.getpar127("bandwidth", Pbandwidth);
        Pbwscale   = xml.getpar127("bandwidth_scale", Pbwscale);

        PFreqEnvelopeEnabled = xml.getparbool("freq_envelope_enabled",
                PFreqEnvelopeEnabled);
        if(xml.enterbranch("FREQUENCY_ENVELOPE")) {
            FreqEnvelope->getfromXML(xml);
            xml.exitbranch();
        }

        PBandWidthEnvelopeEnabled = xml.getparbool(
                "band_width_envelope_enabled",
                PBandWidthEnvelopeEnabled);
        if(xml.enterbranch("BANDWIDTH_ENVELOPE")) {
            BandWidthEnvelope->getfromXML(xml);
            xml.exitbranch();
        }

        xml.exitbranch();
    }

    if(xml.enterbranch("FILTER_PARAMETERS")) {
        PGlobalFilterEnabled = xml.getparbool("enabled", PGlobalFilterEnabled);
        if(xml.enterbranch("FILTER")) {
            GlobalFilter->getfromXML(xml);
            xml.exitbranch();
        }

        PGlobalFilterVelocityScaleFunction = xml.getpar127(
                "filter_velocity_sensing",
                PGlobalFilterVelocityScaleFunction);
        PGlobalFilterVelocityScale = xml.getpar127(
                "filter_velocity_sensing_amplitude",
                PGlobalFilterVelocityScale);

        if(xml.enterbranch("FILTER_ENVELOPE")) {
            GlobalFilterEnvelope->getfromXML(xml);
            xml.exitbranch();
        }

        xml.exitbranch();
    }
}

}

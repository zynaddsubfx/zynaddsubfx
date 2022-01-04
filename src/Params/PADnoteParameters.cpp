/*
  ZynAddSubFX - a software synthesizer

  PADnoteParameters.cpp - Parameters for PADnote (PADsynth)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include <limits>
#include <cmath>
#include "PADnoteParameters.h"
#include "FilterParams.h"
#include "EnvelopeParams.h"
#include "LFOParams.h"
#include "../Synth/Resonance.h"
#include "../Synth/OscilGen.h"
#include "../Misc/WavFile.h"
#include "../Misc/Time.h"
#include <cstdio>
#include <thread>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
using namespace rtosc;

namespace zyn {

#define rObject PADnoteParameters
#undef rChangeCb
#define rChangeCb if (obj->time) { obj->last_update_timestamp = obj->time->time(); }
static const rtosc::Ports realtime_ports =
{
    rRecurp(FreqLfo, "Frequency LFO"),
    rRecurp(AmpLfo,   "Amplitude LFO"),
    rRecurp(FilterLfo, "Filter LFO"),
    rRecurp(FreqEnvelope, "Frequency Envelope"),
    rRecurp(AmpEnvelope, "Amplitude Envelope"),
    rRecurp(FilterEnvelope, "Filter Envelope"),
    rRecurp(GlobalFilter, "Post Filter"),

    //Volume
    rToggle(PStereo,    rShort("stereo"), rDefault(true), "Stereo/Mono Mode"),
    rParamZyn(PPanning, rShort("panning"), rDefault(64), "Left Right Panning"),
    rParamZyn(PVolume,  rShort("vol"), rDefault(90), "Synth Volume"),
    rParamZyn(PAmpVelocityScaleFunction, rShort("sense"), rDefault(64),
        "Amplitude Velocity Sensing function"),

    rParamZyn(Fadein_adjustment, rShort("a.pop."),
        rDefault(FADEIN_ADJUSTMENT_SCALE), "Adjustment for anti-pop strategy."),

    //Punch
    rParamZyn(PPunchStrength, rShort("strength"), rDefault(0),
        "Punch Strength"),
    rParamZyn(PPunchTime,     rShort("time"),     rDefault(60),
        "Length of punch"),
    rParamZyn(PPunchStretch,  rShort("stretch"), rDefault(64),
        "How Punch changes with note frequency"),
    rParamZyn(PPunchVelocitySensing, rShort("sense"), rDefault(72),
        "Punch Velocity control"),

    //Filter
    rParamZyn(PFilterVelocityScale,         rShort("scale"), rDefault(0),
        "Filter Velocity Magnitude"),
    rParamZyn(PFilterVelocityScaleFunction, rShort("sense"), rDefault(64),
        "Filter Velocity Function Shape"),

    //Freq
    rToggle(Pfixedfreq,     rShort("fixed"),  rDefault(false),
        "Base frequency fixed frequency enable"),
    rParamZyn(PfixedfreqET, rShort("f.ET"),   rDefault(0),
        "Equal temeperate control for fixed frequency operation"),
    rParamZyn(PBendAdjust,          rDefault(88),
        "Pitch bend adjustment"),
    rParamZyn(POffsetHz,    rShort("offset"), rDefault(64),
        "Voice constant offset"),
    rParamI(PDetune,        rShort("fine"), rLinear(0, 16383), rDefault(8192),
        "Fine Detune"),
    rParamI(PCoarseDetune,  rShort("coarse"), rDefault(0), "Coarse Detune"),
    rParamZyn(PDetuneType,  rShort("type"),
            rOptions(L35cents, L10cents, E100cents, E1200cents),
            rDefault(L10cents), "Magnitude of Detune"),

    {"sample#64:ifb", rProp(internal) rDoc("Nothing to see here"), 0,
        [](const char *m, rtosc::RtData &d)
        {
            // MiddleWare calls this to send the generated sample buffers to us
            assert(rtosc_argument(m,2).b.len == sizeof(void*));
            PADnoteParameters *p = (PADnoteParameters*)d.obj;
            const char *mm = m;
            while(!isdigit(*mm))++mm;
            int n = atoi(mm);
            float *oldsmp = p->sample[n].smp;
            p->sample[n].size     = rtosc_argument(m,0).i;
            p->sample[n].basefreq = rtosc_argument(m,1).f;
            p->sample[n].smp      = *(float**)rtosc_argument(m,2).b.data;
            if (oldsmp)
                d.reply("/free", "sb", "PADsample", sizeof(void*), &oldsmp);
        }},
    //weird stuff for PCoarseDetune
    {"detunevalue:", rMap(unit,cents) rDoc("Get detune value"), NULL,
        [](const char *, RtData &d)
        {
            PADnoteParameters *obj = (PADnoteParameters *)d.obj;
            d.reply(d.loc, "f", getdetune(obj->PDetuneType, 0, obj->PDetune));
        }},
    {"octave::c:i", rProp(parameter) rShort("octave") rLinear(-8,7)
        rDoc("Octave note offset"), NULL,
        [](const char *msg, RtData &d)
        {
            PADnoteParameters *obj = (PADnoteParameters *)d.obj;
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
        }},
    {"coarsedetune::c:i", rProp(parameter) rShort("coarse") rLinear(-64, 63)
        rDoc("Coarse note detune"), NULL,
        [](const char *msg, RtData &d)
        {
            PADnoteParameters *obj = (PADnoteParameters *)d.obj;
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
        }},
    {"paste:b", rProp(internal) rDoc("paste port"), 0,
    [](const char *m, rtosc::RtData &d){
        rObject &paste = **(rObject **)rtosc_argument(m,0).b.data;
        rObject &o = *(rObject*)d.obj;
        o.pasteRT(paste);}}

};
static const rtosc::Ports non_realtime_ports =
{
    rSelf(PADnoteParameters),
    rPresetType,
    {"paste:b", rProp(internal) rDoc("paste port"), 0,
    [](const char *m, rtosc::RtData &d){
        rObject &paste = **(rObject **)rtosc_argument(m,0).b.data;
        rObject &o = *(rObject*)d.obj;
        o.paste(paste);
        //avoid the match to forward the request along
        d.matches--;}},
    //Harmonic Source Distribution
    rRecurp(oscilgen, "Oscillator"),
    rRecurp(resonance, "Resonance"),

    //Harmonic Shape
    rOption(Pmode, rMap(min, 0), rMap(max, 2), rShort("distribution"),
            rOptions(bandwidth,discrete,continious),
            rDefault(bandwidth),
            "Harmonic Distribution Model"),
    rOption(Php.base.type, rOptions(Gaussian, Rectangular, Double Exponential),
            rShort("shape"), rDefault(Gaussian),
            "Harmonic profile shape"),
    rParamZyn(Php.base.par1, rShort("warp"),      rDefault(80),
              "Harmonic shape distribution parameter"),
    rParamZyn(Php.freqmult,  rShort("clone"),     rDefault(0),
              "Frequency multiplier on distribution"),
    rParamZyn(Php.modulator.par1, rShort("p1"),   rDefault(0),
              "Distribution modulator parameter"),
    rParamZyn(Php.modulator.freq, rShort("freq"), rDefault(30),
              "Frequency of modulator parameter"),
    rParamZyn(Php.width,     rShort("bandwidth"), rDefault(127),
              "Width of base harmonic"),
    rOption(Php.amp.mode,    rShort("mode"),
            rOptions(Sum, Mult, Div1, Div2), rDefault(Sum),
            "Amplitude harmonic multiplier type"),

    //Harmonic Modulation
    rOption(Php.amp.type, rShort("mult"), rOptions(Off, Gauss, Sine, Flat),
            rDefault(Off), "Type of amplitude multiplier"),
    rParamZyn(Php.amp.par1, rShort("p1"),   rDefault(80),
        "Amplitude multiplier parameter"),
    rParamZyn(Php.amp.par2, rShort("p2"),   rDefault(60),
        "Amplitude multiplier parameter"),
    rToggle(Php.autoscale,  rShort("auto"), rDefault(true),
        "Autoscaling Harmonics"),
    rOption(Php.onehalf, rShort("side"),
            rOptions(Full, Upper Half, Lower Half), rDefault(Full)
            "Harmonic cutoff model"),

    //Harmonic Bandwidth
    rOption(Pbwscale, rShort("bw scale"),
            rOptions(Normal,
              EqualHz, Quarter,
              Half, 75%, 150%,
              Double, Inv. Half),
            rDefault(Normal),
            "Bandwidth scaling"),

    //Harmonic Position Modulation
    rOption(Phrpos.type,
            rOptions(Harmonic, ShiftU, ShiftL, PowerU, PowerL, Sine,
                Power, Shift),
            rDefault(Harmonic)
            "Harmonic Overtone shifting mode"),
    rParamI(Phrpos.par1, rShort("p1"), rLinear(0,255), rDefault(0),
        "Harmonic position parameter"),
    rParamI(Phrpos.par2, rShort("p2"), rLinear(0,255), rDefault(0),
        "Harmonic position parameter"),
    rParamI(Phrpos.par3, rShort("force h."), rLinear(0,255), rDefault(0),
        "Harmonic position parameter"),

    //Quality
    rOption(Pquality.samplesize, rShort("quality"),
            rOptions(16k (Tiny), 32k, 64k (Small), 128k,
              256k (Normal), 512k, 1M (Big)),
            rDefaultId(128k),
            "Size of each wavetable element"),
    rOption(Pquality.basenote, rShort("basenote"),
            rOptions(C-2, G-2, C-3, G-3, C-4,
                G-4, C-5, G-5, G-6,),
            rDefaultId(C-4),
            "Base note for wavetable"),
    rOption(Pquality.smpoct, rShort("smp/oct"),
            rOptions(0.5, 1, 2, 3, 4, 6, 12),
            rDefault(2),
            "Samples per octave"),
    rParamI(Pquality.oct, rShort("octaves"), rLinear(0,7), rDefault(3),
            "Number of octaves to sample (above the first sample"),

    {"Pbandwidth::i", rShort("bandwidth") rProp(parameter) rLinear(0,1000)
        rDefault(500) rDoc("Bandwidth Of Harmonics"), NULL,
        [](const char *msg, rtosc::RtData &d) {
            PADnoteParameters *p = ((PADnoteParameters*)d.obj);
            if(rtosc_narguments(msg)) {
                p->setPbandwidth(rtosc_argument(msg, 0).i);
                d.broadcast(d.loc, "i", p->Pbandwidth);
            } else {
                d.reply(d.loc, "i", p->Pbandwidth);
            }}},

    {"bandwidthvalue:", rMap(unit, cents) rDoc("Get Bandwidth"), NULL,
        [](const char *, rtosc::RtData &d) {
            PADnoteParameters *p = ((PADnoteParameters*)d.obj);
            d.reply(d.loc, "f", p->setPbandwidth(p->Pbandwidth));
        }},


    {"nhr:", rProp(non-realtime) rDoc("Returns the harmonic shifts"),
        NULL, [](const char *, rtosc::RtData &d) {
            PADnoteParameters *p = ((PADnoteParameters*)d.obj);
            const unsigned n = p->synth.oscilsize / 2;
            float *tmp = new float[n];
            *tmp = 0;
            for(unsigned i=1; i<n; ++i)
                tmp[i] = p->getNhr(i);
            d.reply(d.loc, "b", n*sizeof(float), tmp);
            delete[] tmp;}},
    {"profile:i", rProp(non-realtime) rDoc("UI display of the harmonic profile"),
        NULL, [](const char *m, rtosc::RtData &d) {
            PADnoteParameters *p = ((PADnoteParameters*)d.obj);
            const int n = rtosc_argument(m, 0).i;
            if(n<=0)
                return;
            float *tmp = new float[n];
            float realbw = p->getprofile(tmp, n);
            d.reply(d.loc, "b", n*sizeof(float), tmp);
            d.reply(d.loc, "i", (int)realbw);
            delete[] tmp;}},
    {"harmonic_profile:", rProp(non-realtime) rDoc("UI display of the harmonic profile"),
        NULL, [](const char *, rtosc::RtData &d) {
            PADnoteParameters *p = ((PADnoteParameters*)d.obj);
#define RES 512
            char        types[RES+2] = {};
            rtosc_arg_t args[RES+1];
            float tmp[RES];
            types[0]  = 'f';
            args[0].f = p->getprofile(tmp, RES);
            for(int i=0; i<RES; ++i) {
                types[i+1]  = 'f';
                args[i+1].f = tmp[i];
            }
            d.replyArray(d.loc, types, args);
#undef RES
        }},
    {"export2wav:s", rDoc("Export padsynth waveforms to .wav files"),
        NULL, [](const char *m, rtosc::RtData&d) {
            PADnoteParameters *p = ((PADnoteParameters*)d.obj);
            p->export2wav(rtosc_argument(m, 0).s);
        }},
    {"needPrepare:", rDoc("Unimplemented Stub"),
        NULL, [](const char *, rtosc::RtData&) {}},
};
#undef rChangeCb

const rtosc::Ports &PADnoteParameters::non_realtime_ports = zyn::non_realtime_ports;
const rtosc::Ports &PADnoteParameters::realtime_ports     = zyn::realtime_ports;


const rtosc::MergePorts PADnoteParameters::ports =
{
    &realtime_ports,
    &non_realtime_ports
};


PADnoteParameters::PADnoteParameters(const SYNTH_T &synth_, FFTwrapper *fft_,
                                     const AbsTime *time_)
        : Presets(), time(time_), last_update_timestamp(0), synth(synth_)
{
    setpresettype("Ppadsynth");

    resonance = new Resonance();
    oscilgen  = new OscilGen(synth, fft_, resonance);
    oscilgen->ADvsPAD = true;

    FreqEnvelope = new EnvelopeParams(0, 0, time_);
    FreqEnvelope->init(ad_global_freq);
    FreqLfo = new LFOParams(ad_global_freq, time_);

    AmpEnvelope = new EnvelopeParams(64, 1, time_);
    AmpEnvelope->init(ad_global_amp);
    AmpLfo = new LFOParams(ad_global_amp, time_);

    GlobalFilter   = new FilterParams(ad_global_filter, time_);
    FilterEnvelope = new EnvelopeParams(0, 1, time_);
    FilterEnvelope->init(ad_global_filter);
    FilterLfo = new LFOParams(ad_global_filter, time_);

    for(int i = 0; i < PAD_MAX_SAMPLES; ++i)
        sample[i].smp = NULL;

    defaults();
}

PADnoteParameters::~PADnoteParameters()
{
    deletesamples();
    delete (oscilgen);
    delete (resonance);

    delete (FreqEnvelope);
    delete (FreqLfo);
    delete (AmpEnvelope);
    delete (AmpLfo);
    delete (GlobalFilter);
    delete (FilterEnvelope);
    delete (FilterLfo);
}

void PADnoteParameters::defaults()
{
    Pmode = pad_mode::bandwidth;
    Php.base.type      = 0;
    Php.base.par1      = 80;
    Php.freqmult       = 0;
    Php.modulator.par1 = 0;
    Php.modulator.freq = 30;
    Php.width     = 127;
    Php.amp.type  = 0;
    Php.amp.mode  = 0;
    Php.amp.par1  = 80;
    Php.amp.par2  = 64;
    Php.autoscale = true;
    Php.onehalf   = 0;

    setPbandwidth(500);
    Pbwscale = 0;

    resonance->defaults();
    oscilgen->defaults();

    Phrpos.type = 0;
    Phrpos.par1 = 0;
    Phrpos.par2 = 0;
    Phrpos.par3 = 0;

    Pquality.samplesize = 3;
    Pquality.basenote   = 4;
    Pquality.oct    = 3;
    Pquality.smpoct = 2;

    PStereo = 1; //stereo
    /* Frequency Global Parameters */
    Pfixedfreq    = 0;
    PfixedfreqET  = 0;
    PBendAdjust = 88; // 64 + 24
    POffsetHz = 64;
    PDetune       = 8192; //zero
    PCoarseDetune = 0;
    PDetuneType   = 1;
    FreqEnvelope->defaults();
    FreqLfo->defaults();

    /* Amplitude Global Parameters */
    PVolume  = 90;
    PPanning = 64; //center
    PAmpVelocityScaleFunction = 64;
    AmpEnvelope->defaults();
    AmpLfo->defaults();
    Fadein_adjustment = FADEIN_ADJUSTMENT_SCALE;
    PPunchStrength = 0;
    PPunchTime     = 60;
    PPunchStretch  = 64;
    PPunchVelocitySensing = 72;

    /* Filter Global Parameters*/
    PFilterVelocityScale = 0;
    PFilterVelocityScaleFunction = 64;
    GlobalFilter->defaults();
    FilterEnvelope->defaults();
    FilterLfo->defaults();

    deletesamples();
}

void PADnoteParameters::deletesample(int n)
{
    if((n < 0) || (n >= PAD_MAX_SAMPLES))
        return;

    delete[] sample[n].smp;
    sample[n].smp = NULL;
    sample[n].size     = 0;
    sample[n].basefreq = 440.0f;
}

void PADnoteParameters::deletesamples()
{
    for(int i = 0; i < PAD_MAX_SAMPLES; ++i)
        deletesample(i);
}

/*
 * Get the harmonic profile (i.e. the frequency distributio of a single harmonic)
 */
float PADnoteParameters::getprofile(float *smp, int size)
{
    for(int i = 0; i < size; ++i)
        smp[i] = 0.0f;
    const int supersample = 16;
    float     basepar     = powf(2.0f, (1.0f - Php.base.par1 / 127.0f) * 12.0f);
    float     freqmult    = floorf(powf(2.0f,
                                       Php.freqmult / 127.0f
                                       * 5.0f) + 0.000001f);

    float modfreq = floorf(powf(2.0f,
                               Php.modulator.freq / 127.0f
                               * 5.0f) + 0.000001f);
    float modpar1 = powf(Php.modulator.par1 / 127.0f, 4.0f) * 5.0f / sqrtf(
        modfreq);
    float amppar1 =
        powf(2.0f, powf(Php.amp.par1 / 127.0f, 2.0f) * 10.0f) - 0.999f;
    float amppar2 = (1.0f - Php.amp.par2 / 127.0f) * 0.998f + 0.001f;
    float width   = powf(150.0f / (Php.width + 22.0f), 2.0f);

    for(int i = 0; i < size * supersample; ++i) {
        bool  makezero = false;
        float x = i * 1.0f / (size * (float) supersample);

        float origx = x;

        //do the sizing (width)
        x = (x - 0.5f) * width + 0.5f;
        if(x < 0.0f) {
            x = 0.0f;
            makezero = true;
        }
        else
        if(x > 1.0f) {
            x = 1.0f;
            makezero = true;
        }

        //compute the full profile or one half
        switch(Php.onehalf) {
            case 1:
                x = x * 0.5f + 0.5f;
                break;
            case 2:
                x = x * 0.5f;
                break;
        }

        float x_before_freq_mult = x;

        //do the frequency multiplier
        x *= freqmult;

        //do the modulation of the profile
        x += sinf(x_before_freq_mult * 3.1415926f * modfreq) * modpar1;
        x  = fmodf(x + 1000.0f, 1.0f) * 2.0f - 1.0f;


        //this is the base function of the profile
        float f;
        switch(Php.base.type) {
            case 1:
                f = expf(-(x * x) * basepar);
                if(f < 0.4f)
                    f = 0.0f;
                else
                    f = 1.0f;
                break;
            case 2:
                f = expf(-(fabsf(x)) * sqrtf(basepar));
                break;
            default:
                f = expf(-(x * x) * basepar);
                break;
        }
        if(makezero)
            f = 0.0f;

        float amp = 1.0f;
        origx = origx * 2.0f - 1.0f;

        //compute the amplitude multiplier
        switch(Php.amp.type) {
            case 1:
                amp = expf(-(origx * origx) * 10.0f * amppar1);
                break;
            case 2:
                amp = 0.5f
                      * (1.0f
                         + cosf(3.1415926f * origx * sqrtf(amppar1 * 4.0f + 1.0f)));
                break;
            case 3:
                amp = 1.0f
                      / (powf(origx * (amppar1 * 2.0f + 0.8f), 14.0f) + 1.0f);
                break;
        }

        //apply the amplitude multiplier
        float finalsmp = f;
        if(Php.amp.type != 0) {
            switch(Php.amp.mode) {
                case 0:
                    finalsmp = amp * (1.0f - amppar2) + finalsmp * amppar2;
                    break;
                case 1:
                    finalsmp *= amp * (1.0f - amppar2) + amppar2;
                    break;
                case 2:
                    finalsmp = finalsmp
                               / (amp + powf(amppar2, 4.0f) * 20.0f + 0.0001f);
                    break;
                case 3:
                    finalsmp = amp
                               / (finalsmp
                                  + powf(amppar2, 4.0f) * 20.0f + 0.0001f);
                    break;
            }
        }

        smp[i / supersample] += finalsmp / supersample;
    }

    //normalize the profile (make the max. to be equal to 1.0f)
    float max = 0.0f;
    for(int i = 0; i < size; ++i) {
        if(smp[i] < 0.0f)
            smp[i] = 0.0f;
        if(smp[i] > max)
            max = smp[i];
    }
    if(max < 0.00001f)
        max = 1.0f;
    for(int i = 0; i < size; ++i)
        smp[i] /= max;

    if(!Php.autoscale)
        return 0.5f;

    //compute the estimated perceived bandwidth
    float sum = 0.0f;
    int   i;
    for(i = 0; i < size / 2 - 2; ++i) {
        sum += smp[i] * smp[i] + smp[size - i - 1] * smp[size - i - 1];
        if(sum >= 4.0f)
            break;
    }

    float result = 1.0f - 2.0f * i / (float) size;
    return result;
}

/*
 * Compute the real bandwidth in cents and returns it
 * Also, sets the bandwidth parameter
 */
float PADnoteParameters::setPbandwidth(int Pbandwidth)
{
    this->Pbandwidth = Pbandwidth;
    float result = powf(Pbandwidth / 1000.0f, 1.1f);
    result = powf(10.0f, result * 4.0f) * 0.25f;
    return result;
}

/*
 * Get the harmonic(overtone) position
 */
float PADnoteParameters::getNhr(int n) const
{
    float result = 1.0f;
    const float par1   = powf(10.0f, -(1.0f - Phrpos.par1 / 255.0f) * 3.0f);
    const float par2   = Phrpos.par2 / 255.0f;

    const float n0     = n - 1.0f;
    float tmp    = 0.0f;
    int   thresh = 0;
    switch(Phrpos.type) {
        case 1:
            thresh = (int)(par2 * par2 * 100.0f) + 1;
            if(n < thresh)
                result = n;
            else
                result = 1.0f + n0 + (n0 - thresh + 1.0f) * par1 * 8.0f;
            break;
        case 2:
            thresh = (int)(par2 * par2 * 100.0f) + 1;
            if(n < thresh)
                result = n;
            else
                result = 1.0f + n0 - (n0 - thresh + 1.0f) * par1 * 0.90f;
            break;
        case 3:
            tmp    = par1 * 100.0f + 1.0f;
            result = powf(n0 / tmp, 1.0f - par2 * 0.8f) * tmp + 1.0f;
            break;
        case 4:
            result = n0
                     * (1.0f
                        - par1)
                     + powf(n0 * 0.1f, par2 * 3.0f
                            + 1.0f) * par1 * 10.0f + 1.0f;
            break;
        case 5:
            result = n0
                     + sinf(n0 * par2 * par2 * PI
                            * 0.999f) * sqrtf(par1) * 2.0f + 1.0f;
            break;
        case 6:
            tmp    = powf(par2 * 2.0f, 2.0f) + 0.1f;
            result = n0 * powf(1.0f + par1 * powf(n0 * 0.8f, tmp), tmp) + 1.0f;
            break;
        case 7:
            result = (n + Phrpos.par1 / 255.0f) / (Phrpos.par1 / 255.0f + 1);
            break;
        default:
            result = n;
            break;
    }

    const float par3 = Phrpos.par3 / 255.0f;

    const float iresult = floorf(result + 0.5f);
    const float dresult = result - iresult;

    return iresult + (1.0f - par3) * dresult;
}

//Transform non zero positive signals into ones with a max of one
static void normalize_max(float *f, size_t len)
{
    float max = 0.0f;
    for(unsigned i = 0; i < len; ++i)
        if(f[i] > i)
            max = f[i];
    if(max > 0.000001f)
        for(unsigned i = 0; i < len; ++i)
            f[i] /= max;
}

//Translate Bandwidth scale integer into floating point value
static float Pbwscale_translate(char Pbwscale)
{
        switch(Pbwscale) {
            case 0: return 1.0f;
            case 1: return 0.0f;
            case 2: return 0.25f;
            case 3: return 0.5f;
            case 4: return 0.75f;
            case 5: return 1.5f;
            case 6: return 2.0f;
            case 7: return -0.5f;
            default: return 1.0;
        }
}

/*
 * Generates the long spectrum for Bandwidth mode (only amplitudes are generated; phases will be random)
 */

//Requires
// - bandwidth scaling power
// - bandwidth
// - oscillator harmonics at various frequencies (oodles of data)
// - sampled resonance
void PADnoteParameters::generatespectrum_bandwidthMode(float *spectrum,
                                                       int size,
                                                       float basefreq,
                                                       const float *profile,
                                                       int profilesize,
                                                       float bwadjust) const
{
    float harmonics[synth.oscilsize];
    memset(spectrum, 0, sizeof(float) * size);
    memset(harmonics, 0, sizeof(float) * synth.oscilsize);

    //get the harmonic structure from the oscillator (I am using the frequency amplitudes, only)
    oscilgen->get(harmonics, basefreq, false);

    //normalize
    normalize_max(harmonics, synth.oscilsize / 2);

    //Constants across harmonics
    const float power = Pbwscale_translate(Pbwscale);
    const float bandwidthcents = const_cast<PADnoteParameters*>(this)->setPbandwidth(Pbandwidth);

    for(int nh = 1; nh < synth.oscilsize / 2; ++nh) { //for each harmonic
        const float realfreq = getNhr(nh) * basefreq;
        if(realfreq > synth.samplerate_f * 0.49999f)
            break;
        if(realfreq < 20.0f)
            break;
        if(harmonics[nh - 1] < 1e-4)
            continue;

        //compute the bandwidth of each harmonic
        const float bw =
            ((powf(2.0f, bandwidthcents / 1200.0f) - 1.0f) * basefreq / bwadjust)
            * powf(realfreq / basefreq, power);
        const int ibw = (int)((bw / (synth.samplerate_f * 0.5f) * size)) + 1;

        float amp = harmonics[nh - 1];
        if(resonance->Penabled)
            amp *= resonance->getfreqresponse(realfreq);

        if(ibw > profilesize) { //if the bandwidth is larger than the profilesize
            const float rap   = sqrtf((float)profilesize / (float)ibw);
            const int   cfreq =
                (int) (realfreq
                       / (synth.samplerate_f * 0.5f) * size) - ibw / 2;
            for(int i = 0; i < ibw; ++i) {
                const int src    = (int)(i * rap * rap);
                const int spfreq = i + cfreq;
                if(spfreq < 0)
                    continue;
                if(spfreq >= size)
                    break;
                spectrum[spfreq] += amp * profile[src] * rap;
            }
        }
        else {  //if the bandwidth is smaller than the profilesize
            const float rap = sqrtf((float)ibw / (float)profilesize);
            const float ibasefreq = realfreq / (synth.samplerate_f * 0.5f) * size;
            for(int i = 0; i < profilesize; ++i) {
                const float idfreq = (i / (float)profilesize - 0.5f) * ibw;
                const float freqsum = idfreq + ibasefreq;
                const int   spfreq  = (int)freqsum;
                const float fspfreq = freqsum - spfreq;
                if(spfreq <= 0)
                    continue;
                if(spfreq >= size - 1)
                    break;
                spectrum[spfreq] += amp * profile[i] * rap
                                    * (1.0f - fspfreq);
                spectrum[spfreq + 1] += amp * profile[i] * rap * fspfreq;
            }
        }
    }
}

/*
 * Generates the long spectrum for non-Bandwidth modes (only amplitudes are generated; phases will be random)
 */
void PADnoteParameters::generatespectrum_otherModes(float *spectrum,
                                                    int size,
                                                    float basefreq) const
{
    float harmonics[synth.oscilsize];
    memset(spectrum,  0, sizeof(float) * size);
    memset(harmonics, 0, sizeof(float) * synth.oscilsize);

    //get the harmonic structure from the oscillator (I am using the frequency amplitudes, only)
    oscilgen->get(harmonics, basefreq, false);

    //normalize
    normalize_max(harmonics, synth.oscilsize / 2);

    for(int nh = 1; nh < synth.oscilsize / 2; ++nh) { //for each harmonic
        const float realfreq = getNhr(nh) * basefreq;

        //take care of interpolation if frequency decreases
        if(realfreq > synth.samplerate_f * 0.49999f)
            break;
        if(realfreq < 20.0f)
            break;


        float amp = harmonics[nh - 1];
        if(resonance->Penabled)
            amp *= resonance->getfreqresponse(realfreq);
        const int cfreq = (int)(realfreq / (synth.samplerate_f * 0.5f) * size);

        spectrum[cfreq] = amp + (float)1e-9;
    }

    //In continous mode the spectrum gets additional interpolation between the
    //spectral peaks
    if(Pmode == pad_mode::continous) { //continous mode
        int old = 0;
        for(int k = 1; k < size; ++k)
            if((spectrum[k] > 1e-10) || (k == (size - 1))) {
                const int   delta  = k - old;
                const float val1   = spectrum[old];
                const float val2   = spectrum[k];
                const float idelta = 1.0f / delta;
                for(int i = 0; i < delta; ++i) {
                    const float x = idelta * i;
                    spectrum[old + i] = val1 * (1.0f - x) + val2 * x;
                }
                old = k;
            }
    }
}

/*
 * Applies the parameters (i.e. computes all the samples, based on parameters);
 */
void PADnoteParameters::applyparameters()
{
    applyparameters([]{return false;});
}

void PADnoteParameters::applyparameters(std::function<bool()> do_abort,
                                        unsigned max_threads)
{
    if(do_abort())
        return;
    unsigned num = sampleGenerator([this]
                       (unsigned N, PADnoteParameters::Sample&& smp) {
                           delete[] sample[N].smp;
                           sample[N] = std::move(smp);
                       },
                       do_abort, max_threads);

    //Delete remaining unused samples
    for(unsigned i = num; i < PAD_MAX_SAMPLES; ++i)
        deletesample(i);
}

//Requires
// - Pquality.samplesize
// - Pquality.basenote
// - Pquality.oct
// - Pquality.smpoct
// - spectrum at various frequencies (oodles of data)
int PADnoteParameters::sampleGenerator(PADnoteParameters::callback cb,
        std::function<bool()> do_abort,
        unsigned max_threads)
{
    if(!max_threads)
        max_threads = std::numeric_limits<unsigned>::max();

    const int samplesize   = (((int) 1) << (Pquality.samplesize + 14));
    const int spectrumsize = samplesize / 2;
    const int profilesize = 512;

    float     profile[profilesize];


    const float bwadjust = getprofile(profile, profilesize);
    float basefreq = 65.406f * powf(2.0f, Pquality.basenote / 2);
    if(Pquality.basenote % 2 == 1)
        basefreq *= 1.5f;

    int samplemax = Pquality.oct + 1;
    int smpoct    = Pquality.smpoct;
    if(Pquality.smpoct == 5)
        smpoct = 6;
    if(Pquality.smpoct == 6)
        smpoct = 12;
    if(smpoct != 0)
        samplemax *= smpoct;
    else
        samplemax = samplemax / 2 + 1;
    if(samplemax == 0)
        samplemax = 1;

    if(samplemax > PAD_MAX_SAMPLES)
        samplemax = PAD_MAX_SAMPLES;

    //this is used to compute frequency relation to the base frequency
    float adj[samplemax];
    for(int nsample = 0; nsample < samplemax; ++nsample)
        adj[nsample] = (Pquality.oct + 1.0f) * (float)nsample / samplemax;
    // QtCreator can't capture VLAs (QTCREATORBUG-23722), so this is
    // a workaround to allow using the IDE
    float * const adj_ptr = adj;

    const PADnoteParameters* this_c = this;

    auto thread_cb = [basefreq, bwadjust, &cb, do_abort,
                      samplesize, samplemax, spectrumsize,
                      adj_ptr, &profile, this_c](
                      unsigned nthreads, unsigned threadno)
    {
        //prepare a BIG IFFT
        FFTwrapper    *fft      = new FFTwrapper(samplesize);
        FFTfreqBuffer  fftfreqs = fft->allocFreqBuf();
        float         *spectrum = new float[spectrumsize];

        for(int nsample = 0; nsample < samplemax; ++nsample)
        if(nsample % nthreads == threadno)
        {
            if(do_abort())
                break;
            const float basefreqadjust =
                powf(2.0f, adj_ptr[nsample] - adj_ptr[samplemax - 1] * 0.5f);

            if(this_c->Pmode == pad_mode::bandwidth)
                this_c->generatespectrum_bandwidthMode(spectrum,
                                                       spectrumsize,
                                                       basefreq*basefreqadjust,
                                                       profile,
                                                       profilesize,
                                                       bwadjust);
            else
                this_c->generatespectrum_otherModes(spectrum, spectrumsize,
                                                    basefreq * basefreqadjust);

            //the last samples contain the first samples
            //(used for linear/cubic interpolation)
            const int extra_samples = 5;
            PADnoteParameters::Sample newsample;
            newsample.smp = new float[samplesize + extra_samples];

            newsample.smp[0] = 0.0f;
            fftfreqs[0] = fft_t(0, 0);
            for(int i = 1; i < spectrumsize; ++i) //randomize the phases
                fftfreqs[i] = FFTpolar(spectrum[i], (float)RND * 2 * PI);
            //that's all; here is the only ifft for the whole sample;
            //no windows are used ;-)
            fft->freqs2smps_noconst_input(fftfreqs, fft->allocSampleBuf(newsample.smp));

            //normalize(rms)
            float rms = 0.0f;
            for(int i = 0; i < samplesize; ++i)
                rms += newsample.smp[i] * newsample.smp[i];
            rms = sqrtf(rms);
            if(rms < 0.000001f)
                rms = 1.0f;
            rms *= sqrtf(262144.0f / samplesize);//262144=2^18
            for(int i = 0; i < samplesize; ++i)
                newsample.smp[i] *= 1.0f / rms * 50.0f;

            //prepare extra samples used by the linear or cubic interpolation
            for(int i = 0; i < extra_samples; ++i)
                newsample.smp[i + samplesize] = newsample.smp[i];

            //yield new sample
            newsample.size     = samplesize;
            newsample.basefreq = basefreq * basefreqadjust;
            cb(nsample, std::move(newsample));
        }

        //Cleanup
        delete (fft);
        delete[] fftfreqs.data;
        delete[] spectrum;
    };

    if(oscilgen->needPrepare())
        oscilgen->prepare();

#ifdef WIN32
    //Temporarily disable multi-threading here as C++11 threads are broken on
    //mingw cross compilation
    thread_cb(1,0);
#else
    unsigned nthreads = std::min(max_threads,
                                 std::thread::hardware_concurrency());
    std::vector<std::thread> threads(nthreads);
    for(unsigned i = 0; i < nthreads; ++i)
        threads[i] = std::thread(thread_cb, nthreads, i);
    for(unsigned i = 0; i < nthreads; ++i)
        threads[i].join();
#endif

    return samplemax;
}

void PADnoteParameters::export2wav(std::string basefilename)
{
    applyparameters();
    basefilename += "_PADsynth_";
    for(int k = 0; k < PAD_MAX_SAMPLES; ++k) {
        if(sample[k].smp == NULL)
            continue;
        char tmpstr[20];
        snprintf(tmpstr, 20, "_%02d", k + 1);
        std::string filename = basefilename + std::string(tmpstr) + ".wav";
        WavFile     wav(filename, synth.samplerate, 1);
        if(wav.good()) {
            int nsmps = sample[k].size;
            short int *smps = new short int[nsmps];
            for(int i = 0; i < nsmps; ++i)
                smps[i] = (short int)(sample[k].smp[i] * 32767.0f);
            wav.writeMonoSamples(nsmps, smps);
        }
    }
}

void PADnoteParameters::add2XML(XMLwrapper& xml)
{
    xml.setPadSynth(true);

    xml.addparbool("stereo", PStereo);
    xml.addpar("mode", (int)Pmode);
    xml.addpar("bandwidth", Pbandwidth);
    xml.addpar("bandwidth_scale", Pbwscale);

    xml.beginbranch("HARMONIC_PROFILE");
    xml.addpar("base_type", Php.base.type);
    xml.addpar("base_par1", Php.base.par1);
    xml.addpar("frequency_multiplier", Php.freqmult);
    xml.addpar("modulator_par1", Php.modulator.par1);
    xml.addpar("modulator_frequency", Php.modulator.freq);
    xml.addpar("width", Php.width);
    xml.addpar("amplitude_multiplier_type", Php.amp.type);
    xml.addpar("amplitude_multiplier_mode", Php.amp.mode);
    xml.addpar("amplitude_multiplier_par1", Php.amp.par1);
    xml.addpar("amplitude_multiplier_par2", Php.amp.par2);
    xml.addparbool("autoscale", Php.autoscale);
    xml.addpar("one_half", Php.onehalf);
    xml.endbranch();

    xml.beginbranch("OSCIL");
    oscilgen->add2XML(xml);
    xml.endbranch();

    xml.beginbranch("RESONANCE");
    resonance->add2XML(xml);
    xml.endbranch();

    xml.beginbranch("HARMONIC_POSITION");
    xml.addpar("type", Phrpos.type);
    xml.addpar("parameter1", Phrpos.par1);
    xml.addpar("parameter2", Phrpos.par2);
    xml.addpar("parameter3", Phrpos.par3);
    xml.endbranch();

    xml.beginbranch("SAMPLE_QUALITY");
    xml.addpar("samplesize", Pquality.samplesize);
    xml.addpar("basenote", Pquality.basenote);
    xml.addpar("octaves", Pquality.oct);
    xml.addpar("samples_per_octave", Pquality.smpoct);
    xml.endbranch();

    xml.beginbranch("AMPLITUDE_PARAMETERS");
    xml.addpar("volume", PVolume);
    xml.addpar("panning", PPanning);
    xml.addpar("velocity_sensing", PAmpVelocityScaleFunction);
    xml.addpar("fadein_adjustment", Fadein_adjustment);
    xml.addpar("punch_strength", PPunchStrength);
    xml.addpar("punch_time", PPunchTime);
    xml.addpar("punch_stretch", PPunchStretch);
    xml.addpar("punch_velocity_sensing", PPunchVelocitySensing);

    xml.beginbranch("AMPLITUDE_ENVELOPE");
    AmpEnvelope->add2XML(xml);
    xml.endbranch();

    xml.beginbranch("AMPLITUDE_LFO");
    AmpLfo->add2XML(xml);
    xml.endbranch();

    xml.endbranch();

    xml.beginbranch("FREQUENCY_PARAMETERS");
    xml.addpar("fixed_freq", Pfixedfreq);
    xml.addpar("fixed_freq_et", PfixedfreqET);
    xml.addpar("bend_adjust", PBendAdjust);
    xml.addpar("offset_hz", POffsetHz);
    xml.addpar("detune", PDetune);
    xml.addpar("coarse_detune", PCoarseDetune);
    xml.addpar("detune_type", PDetuneType);

    xml.beginbranch("FREQUENCY_ENVELOPE");
    FreqEnvelope->add2XML(xml);
    xml.endbranch();

    xml.beginbranch("FREQUENCY_LFO");
    FreqLfo->add2XML(xml);
    xml.endbranch();
    xml.endbranch();

    xml.beginbranch("FILTER_PARAMETERS");
    xml.addpar("velocity_sensing_amplitude", PFilterVelocityScale);
    xml.addpar("velocity_sensing", PFilterVelocityScaleFunction);

    xml.beginbranch("FILTER");
    GlobalFilter->add2XML(xml);
    xml.endbranch();

    xml.beginbranch("FILTER_ENVELOPE");
    FilterEnvelope->add2XML(xml);
    xml.endbranch();

    xml.beginbranch("FILTER_LFO");
    FilterLfo->add2XML(xml);
    xml.endbranch();
    xml.endbranch();
}

void PADnoteParameters::getfromXML(XMLwrapper& xml)
{
    PStereo    = xml.getparbool("stereo", PStereo);
    Pmode      = (pad_mode)xml.getpar127("mode", 0);
    Pbandwidth = xml.getpar("bandwidth", Pbandwidth, 0, 1000);
    Pbwscale   = xml.getpar127("bandwidth_scale", Pbwscale);

    if(xml.enterbranch("HARMONIC_PROFILE")) {
        Php.base.type = xml.getpar127("base_type", Php.base.type);
        Php.base.par1 = xml.getpar127("base_par1", Php.base.par1);
        Php.freqmult  = xml.getpar127("frequency_multiplier",
                                       Php.freqmult);
        Php.modulator.par1 = xml.getpar127("modulator_par1",
                                            Php.modulator.par1);
        Php.modulator.freq = xml.getpar127("modulator_frequency",
                                            Php.modulator.freq);
        Php.width    = xml.getpar127("width", Php.width);
        Php.amp.type = xml.getpar127("amplitude_multiplier_type",
                                      Php.amp.type);
        Php.amp.mode = xml.getpar127("amplitude_multiplier_mode",
                                      Php.amp.mode);
        Php.amp.par1 = xml.getpar127("amplitude_multiplier_par1",
                                      Php.amp.par1);
        Php.amp.par2 = xml.getpar127("amplitude_multiplier_par2",
                                      Php.amp.par2);
        Php.autoscale = xml.getparbool("autoscale", Php.autoscale);
        Php.onehalf   = xml.getpar127("one_half", Php.onehalf);
        xml.exitbranch();
    }

    if(xml.enterbranch("OSCIL")) {
        oscilgen->getfromXML(xml);
        xml.exitbranch();
    }

    if(xml.enterbranch("RESONANCE")) {
        resonance->getfromXML(xml);
        xml.exitbranch();
    }

    if(xml.enterbranch("HARMONIC_POSITION")) {
        Phrpos.type = xml.getpar127("type", Phrpos.type);
        Phrpos.par1 = xml.getpar("parameter1", Phrpos.par1, 0, 255);
        Phrpos.par2 = xml.getpar("parameter2", Phrpos.par2, 0, 255);
        Phrpos.par3 = xml.getpar("parameter3", Phrpos.par3, 0, 255);
        xml.exitbranch();
    }

    if(xml.enterbranch("SAMPLE_QUALITY")) {
        Pquality.samplesize = xml.getpar127("samplesize", Pquality.samplesize);
        Pquality.basenote   = xml.getpar127("basenote", Pquality.basenote);
        Pquality.oct    = xml.getpar127("octaves", Pquality.oct);
        Pquality.smpoct = xml.getpar127("samples_per_octave",
                                         Pquality.smpoct);
        xml.exitbranch();
    }

    if(xml.enterbranch("AMPLITUDE_PARAMETERS")) {
        PVolume  = xml.getpar127("volume", PVolume);
        PPanning = xml.getpar127("panning", PPanning);
        PAmpVelocityScaleFunction = xml.getpar127("velocity_sensing",
                                                   PAmpVelocityScaleFunction);
        Fadein_adjustment = xml.getpar127("fadein_adjustment", Fadein_adjustment);
        PPunchStrength = xml.getpar127("punch_strength", PPunchStrength);
        PPunchTime     = xml.getpar127("punch_time", PPunchTime);
        PPunchStretch  = xml.getpar127("punch_stretch", PPunchStretch);
        PPunchVelocitySensing = xml.getpar127("punch_velocity_sensing",
                                               PPunchVelocitySensing);

        xml.enterbranch("AMPLITUDE_ENVELOPE");
        AmpEnvelope->getfromXML(xml);
        xml.exitbranch();

        xml.enterbranch("AMPLITUDE_LFO");
        AmpLfo->getfromXML(xml);
        xml.exitbranch();

        xml.exitbranch();
    }

    if(xml.enterbranch("FREQUENCY_PARAMETERS")) {
        Pfixedfreq    = xml.getpar127("fixed_freq", Pfixedfreq);
        PfixedfreqET  = xml.getpar127("fixed_freq_et", PfixedfreqET);
        PBendAdjust  = xml.getpar127("bend_adjust", PBendAdjust);
        POffsetHz  = xml.getpar127("offset_hz", POffsetHz);
        PDetune       = xml.getpar("detune", PDetune, 0, 16383);
        PCoarseDetune = xml.getpar("coarse_detune", PCoarseDetune, 0, 16383);
        PDetuneType   = xml.getpar127("detune_type", PDetuneType);

        xml.enterbranch("FREQUENCY_ENVELOPE");
        FreqEnvelope->getfromXML(xml);
        xml.exitbranch();

        xml.enterbranch("FREQUENCY_LFO");
        FreqLfo->getfromXML(xml);
        xml.exitbranch();
        xml.exitbranch();
    }

    if(xml.enterbranch("FILTER_PARAMETERS")) {
        PFilterVelocityScale = xml.getpar127("velocity_sensing_amplitude",
                                              PFilterVelocityScale);
        PFilterVelocityScaleFunction = xml.getpar127(
            "velocity_sensing",
            PFilterVelocityScaleFunction);

        xml.enterbranch("FILTER");
        GlobalFilter->getfromXML(xml);
        xml.exitbranch();

        xml.enterbranch("FILTER_ENVELOPE");
        FilterEnvelope->getfromXML(xml);
        xml.exitbranch();

        xml.enterbranch("FILTER_LFO");
        FilterLfo->getfromXML(xml);
        xml.exitbranch();
        xml.exitbranch();
    }
}

#define COPY(y) this->y = x.y
void PADnoteParameters::paste(PADnoteParameters &x)
{
    COPY(Pmode);

    COPY(Php.base.type);
    COPY(Php.base.par1);
    COPY(Php.freqmult);
    COPY(Php.modulator.par1);
    COPY(Php.modulator.freq);
    COPY(Php.width);
    COPY(Php.amp.mode);
    COPY(Php.amp.type);
    COPY(Php.amp.par1);
    COPY(Php.amp.par2);
    COPY(Php.autoscale);
    COPY(Php.onehalf);

    COPY(Pbandwidth);
    COPY(Pbwscale);

    COPY(Phrpos.type);
    COPY(Phrpos.par1);
    COPY(Phrpos.par2);
    COPY(Phrpos.par3);

    COPY(Pquality.samplesize);
    COPY(Pquality.basenote);
    COPY(Pquality.oct);
    COPY(Pquality.smpoct);

    oscilgen->paste(*x.oscilgen);
    resonance->paste(*x.resonance);

    if ( time ) {
        last_update_timestamp = time->time();
    }
}

void PADnoteParameters::pasteRT(PADnoteParameters &x)
{
    //Realtime stuff

    COPY(Pfixedfreq);

    COPY(PfixedfreqET);
    COPY(PBendAdjust);
    COPY(POffsetHz);
    COPY(PDetune);
    COPY(PCoarseDetune);
    COPY(PDetuneType);

    FreqEnvelope->paste(*x.FreqEnvelope);
    FreqLfo->paste(*x.FreqLfo);

    COPY(PStereo);
    COPY(PPanning);
    COPY(PVolume);
    COPY(PAmpVelocityScaleFunction);

    AmpEnvelope->paste(*x.AmpEnvelope);
    AmpLfo->paste(*x.AmpLfo);

    COPY(Fadein_adjustment);
    COPY(PPunchStrength);
    COPY(PPunchTime);
    COPY(PPunchStretch);
    COPY(PPunchVelocitySensing);

    GlobalFilter->paste(*x.GlobalFilter);

    COPY(PFilterVelocityScale);
    COPY(PFilterVelocityScaleFunction);

    FilterEnvelope->paste(*x.FilterEnvelope);
    FilterLfo->paste(*x.FilterLfo);

    if ( time ) {
        last_update_timestamp = time->time();
    }
}
#undef COPY

}

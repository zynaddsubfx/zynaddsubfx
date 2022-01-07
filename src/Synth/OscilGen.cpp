/*
  ZynAddSubFX - a software synthesizer

  OscilGen.cpp - Waveform generator for ADnote
  Copyright (C) 2021 Hans Petter Selasky
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "OscilGen.h"
#include "../DSP/FFTwrapper.h"
#include "../Synth/Resonance.h"
#include "../Misc/WaveShapeSmps.h"

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstddef>
#include <complex>

#include <unistd.h>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

namespace zyn {


#define rObject OscilGen
const rtosc::Ports OscilGen::non_realtime_ports = {
    rSelf(OscilGen),
    rPaste,
    //TODO ensure min/max
    rOption(Phmagtype, rShort("scale"),
    rOptions(linear,dB scale (-40),
    dB scale (-60), dB scale (-80),
    dB scale (-100)),
            rDefault(linear),
            "Type of magnitude for harmonics"),
    rOption(Pcurrentbasefunc, rShort("base"),
            rOptions(sine, triangle, pulse, saw, power, gauss,
                diode, abssine, pulsesine, stretchsine,
                chirp, absstretchsine, chebyshev, sqr,
                spike, circle, powersinus), rOpt(127,use-as-base waveform),
            rDefault(sine),
            "Base Waveform for harmonics"),
    rParamZyn(Pbasefuncpar, rShort("shape"), rDefault(64),
            "Morph between possible base function shapes "
            "(e.g. rising sawtooth vs a falling sawtooth)"),
    rOption(Pbasefuncmodulation, rShort("mod"),
            rOptions(None, Rev, Sine, Power, Chop), rDefault(None),
            "Modulation applied to Base function spectra"),
    rParamZyn(Pbasefuncmodulationpar1, rShort("p1"), rDefault(64),
            "Base function modulation parameter"),
    rParamZyn(Pbasefuncmodulationpar2, rShort("p2"), rDefault(64),
            "Base function modulation parameter"),
    rParamZyn(Pbasefuncmodulationpar3, rShort("p3"), rDefault(32),
            "Base function modulation parameter"),
    rParamZyn(Pwaveshaping, rShort("amount"), rDefault(64),
            "Degree Of Waveshaping"),
    rOption(Pwaveshapingfunction, rShort("distort"), rDefault(Undistorted),
            rOptions(Undistorted,
                Arctangent, Asymmetric, Pow, Sine, Quantisize,
                Zigzag, Limiter, Upper Limiter, Lower Limiter,
                Inverse Limiter, Clip, Asym2, Pow2, sigmoid, Tanh, Cubic, Square),
            "Shape of distortion to be applied"),
    rOption(Pfiltertype, rShort("filter"), rOptions(No Filter,
            lp, hp1, hp1b, bp1, bs1, lp2, hp2, bp2, bs2,
            cos, sin, low_shelf, s, lpsk), rDefaultId(No Filter), "Harmonic Filter"),
    rParamZyn(Pfilterpar1, rShort("p1"), rDefault(64), "Filter parameter"),
    rParamZyn(Pfilterpar2, rShort("p2"), rDefault(64), "Filter parameter"),
    rToggle(Pfilterbeforews, rShort("pre/post"), rDefault(false),
            "Filter before waveshaping spectra;"
            "When enabled oscilfilter(freqs); then waveshape(freqs);, "
            "otherwise waveshape(freqs); then oscilfilter(freqs);"),
    rOption(Psatype, rShort("spec. adj."), rOptions(None, Pow, ThrsD, ThrsU),
            rDefault(None), "Spectral Adjustment Type"),
    rParamZyn(Psapar, rShort("p1"), rDefault(64),
              "Spectral Adjustment Parameter"),
    rParamI(Pharmonicshift, rLinear(-64,64), rShort("shift"), rDefault(0),
            "Amount of shift on harmonics"),
    rToggle(Pharmonicshiftfirst, rShort("pre/post"), rDefault(false),
            "If harmonics are shifted before waveshaping/filtering"),
    rOption(Pmodulation, rShort("FM"), rOptions(None, Rev, Sine, Power),
            rDefault(None), "Frequency Modulation To Combined Spectra"),
    rParamZyn(Pmodulationpar1, rShort("p1"), rDefault(64),
              "modulation parameter"),
    rParamZyn(Pmodulationpar2, rShort("p2"), rDefault(64),
              "modulation parameter"),
    rParamZyn(Pmodulationpar3, rShort("p3"), rDefault(32),
              "modulation parameter"),
    rToggle(ADvsPAD, rShort("If it is used by PADSynth"),
            "If it is used by PADSynth (and not ADSynth)"),


    //TODO update to rArray and test
    {"phase#128::c:i", rProp(parameter) rLinear(0,127)
        rDefault([64 ...])
        rDoc("Sets harmonic phase"),
        NULL, [](const char *m, rtosc::RtData &d) {
            const char *mm = m;
            while(*mm && !isdigit(*mm)) ++mm;
            unsigned char &phase = ((OscilGen*)d.obj)->Phphase[atoi(mm)];
            if(!rtosc_narguments(m))
                d.reply(d.loc, "i", phase);
            else {
                phase = rtosc_argument(m,0).i;
                //XXX hack hack
                char  repath[128];
                strcpy(repath, d.loc);
                char *edit   = strrchr(repath, '/')+1;
                strcpy(edit, "prepare");
                OscilGen &o = *((OscilGen*)d.obj);
                FFTfreqBuffer freqs = o.fft->allocFreqBuf();
                OscilGenBuffers& bfrs = o.myBuffers();
                o.prepare(bfrs, freqs);
                // fprintf(stderr, "sending '%p' of fft data\n", data);
                d.chain(repath, "b", sizeof(fft_t*), &freqs.data);
                bfrs.pendingfreqs = freqs.data;
                d.broadcast(d.loc, "i", phase);
            }
        }},
    //TODO update to rArray and test
    {"magnitude#128::c:i", rProp(parameter) rLinear(0,127)
        rDefault([127 64 64 ...]) rDoc("Sets harmonic magnitude"),
        NULL, [](const char *m, rtosc::RtData &d) {
            //printf("I'm at '%s'\n", d.loc);
            const char *mm = m;
            while(*mm && !isdigit(*mm)) ++mm;
            unsigned char &mag = ((OscilGen*)d.obj)->Phmag[atoi(mm)];
            if(!rtosc_narguments(m))
                d.reply(d.loc, "i", mag);
            else {
                mag = rtosc_argument(m,0).i;
                //printf("setting magnitude\n\n");
                //XXX hack hack
                char  repath[128];
                strcpy(repath, d.loc);
                char *edit   = strrchr(repath, '/')+1;
                strcpy(edit, "prepare");
                OscilGen &o = *((OscilGen*)d.obj);
                FFTfreqBuffer freqs = o.fft->allocFreqBuf();
                OscilGenBuffers& bfrs = o.myBuffers();
                o.prepare(bfrs, freqs);
                // fprintf(stderr, "sending '%p' of fft data\n", data);
                d.chain(repath, "b", sizeof(fft_t*), &freqs.data);
                bfrs.pendingfreqs = freqs.data;
                d.broadcast(d.loc, "i", mag);
            }
        }},
    {"base-spectrum:", rProp(non-realtime) rDoc("Returns spectrum of base waveshape"),
        NULL, [](const char *, rtosc::RtData &d) {
            OscilGen &o = *((OscilGen*)d.obj);
            const unsigned n = o.synth.oscilsize / 2;
            float *spc = new float[n];
            memset(spc, 0, 4*n);
            ((OscilGen*)d.obj)->getspectrum(n,spc,1);
            d.reply(d.loc, "b", n*sizeof(float), spc);
            delete[] spc;
        }},
    {"base-waveform:", rProp(non-realtime) rDoc("Returns base waveshape points"),
        NULL, [](const char *, rtosc::RtData &d) {
            OscilGen &o = *((OscilGen*)d.obj);
            FFTsampleBuffer smps = o.fft->allocSampleBuf();
            const unsigned n = smps.allocSize() * sizeof(float);
            memset(smps.data, 0, n);
            ((OscilGen*)d.obj)->getcurrentbasefunction(smps);
            d.reply(d.loc, "b", n, smps.data);
            delete[] smps.data;
        }},
    {"prepare:", rProp(non-realtime) rDoc("Performs setup operation to oscillator"),
        NULL, [](const char *, rtosc::RtData &d) {
            //fprintf(stderr, "prepare: got a message from '%s'\n", m);
            OscilGen &o = *(OscilGen*)d.obj;
            FFTfreqBuffer freqs = o.fft->allocFreqBuf();
            OscilGenBuffers& bfrs = o.myBuffers();
            o.prepare(bfrs, freqs);
            // fprintf(stderr, "sending '%p' of fft data\n", data);
            d.chain(d.loc, "b", sizeof(fft_t*), &freqs.data);
            bfrs.pendingfreqs = freqs.data;
        }},
    {"convert2sine:", rProp(non-realtime) rDoc("Translates waveform into FS"),
        NULL, [](const char *, rtosc::RtData &d) {
            ((OscilGen*)d.obj)->convert2sine();
            //XXX hack hack
            char  repath[128];
            strcpy(repath, d.loc);
            char *edit   = strrchr(repath, '/')+1;
            *edit = 0;
            d.broadcast("/damage", "s", repath);
        }},
    {"use-as-base:", rProp(non-realtime) rDoc("Translates current waveform into base"),
        NULL, [](const char *, rtosc::RtData &d) {
            ((OscilGen*)d.obj)->useasbase();
            //XXX hack hack
            char  repath[128];
            strcpy(repath, d.loc);
            char *edit   = strrchr(repath, '/')+1;
            *edit = 0;
            d.broadcast("/damage", "s", repath);
        }}};

const rtosc::Ports OscilGen::realtime_ports{
    rSelf(OscilGen),
    rPresetType,
    rParamZyn(Prand, rLinear(-64, 63), rShort("phase rnd"),
            rDefaultDepends(ADvsPAD), rPreset(true, 127), rPreset(false, 64),
            "Oscillator Phase Randomness: smaller than 0 is \""
            "group\", larger than 0 is for each harmonic"),
    rParamZyn(Pamprandpower, rShort("variance"), rDefault(64),
            "Variance of harmonic randomness"),
    rOption(Pamprandtype, rShort("distribution"), rOptions(None, Pow, Sin),
            rDefault(None),
            "Harmonic random distribution to select from"),
    rOption(Padaptiveharmonics, rShort("adapt")
            rOptions(OFF, ON, Square, 2xSub, 2xAdd, 3xSub, 3xAdd, 4xSub, 4xAdd),
            rDefault(OFF),
            "Adaptive Harmonics Mode"),
    rParamI(Padaptiveharmonicsbasefreq, rShort("c. freq"), rLinear(0,255),
            rDefault(128), "Base frequency of adaptive harmonic (30..3000Hz)"),
    rParamI(Padaptiveharmonicspower, rShort("amount"), rLinear(0,200),
            rDefault(100), "Adaptive Harmonic Strength"),
    rParamI(Padaptiveharmonicspar, rShort("power"), rLinear(0,100),
            rDefault(50), "Adaptive Harmonics Postprocessing Power"),
    {"waveform:", rDoc("Returns waveform points"),
        NULL, [](const char *, rtosc::RtData &d) {
            OscilGen &o = *((OscilGen*)d.obj);
            const unsigned n = o.synth.oscilsize;
            float *smps = new float[n]; // XXXRT
            memset(smps, 0, 4*n);
            //printf("%d\n", o->needPrepare());
            o.get(o.myBuffers(),smps,-1.0);
            //printf("wave: %f %f %f %f\n", smps[0], smps[1], smps[2], smps[3]);
            d.reply(d.loc, "b", n*sizeof(float), smps);
            delete[] smps;
        }},
    {"spectrum:", rDoc("Returns spectrum of waveform"),
        NULL, [](const char *, rtosc::RtData &d) {
            OscilGen &o = *((OscilGen*)d.obj);
            const unsigned n = o.synth.oscilsize / 2;
            float *spc = new float[n]; // XXXRT
            memset(spc, 0, 4*n);
            ((OscilGen*)d.obj)->getspectrum(n,spc,0);
            d.reply(d.loc, "b", n*sizeof(float), spc);
            delete[] spc;
        }},
    {"prepare:b", rProp(internal) rProp(realtime) rProp(pointer) rDoc("Sets prepared fft data"),
        NULL, [](const char *m, rtosc::RtData &d) {
            // fprintf(stderr, "prepare:b got a message from '%s'\n", m);
            OscilGen &o = *(OscilGen*)d.obj;
            OscilGenBuffers& bfrs = o.myBuffers();
            assert(rtosc_argument(m,0).b.len == sizeof(void*));
            d.reply("/free", "sb", "fft_t", sizeof(void*), &bfrs.oscilFFTfreqs.data);
            assert(bfrs.oscilFFTfreqs.data !=*(fft_t**)rtosc_argument(m,0).b.data);
            bfrs.oscilFFTfreqs.data = *(fft_t**)rtosc_argument(m,0).b.data;
        }},

};

const rtosc::MergePorts OscilGen::ports{
    &OscilGen::realtime_ports,
    &OscilGen::non_realtime_ports
};

#ifndef M_PI_2
# define M_PI_2		1.57079632679489661923	/* pi/2 */
#endif


//operations on FFTfreqs
inline void clearAll(fft_t *freqs, int oscilsize)
{
    fft_t zero = 0;
    std::fill_n(freqs, oscilsize / 2, zero);
}

inline void clearDC(fft_t *freqs)
{
    freqs[0] = fft_t(0.0f, 0.0f);
}

//return magnitude squared
inline float normal(const fft_t *freqs, off_t x)
{
    return (float)norm(freqs[x]);
}

//return magnitude
inline float abs(const fft_t *freqs, off_t x)
{
    return (float)abs(freqs[x]);
}

//return angle aka phase from a sine (not cosine wave)
inline float arg(const fft_t *freqs, off_t x)
{
    const fft_t tmp(freqs[x].imag(), freqs[x].real());
    return (float)arg(tmp);
}

/**
 * Take frequency spectrum and ensure values are normalized based upon
 * magnitude to 0<=x<=1
 */
void normalize(fft_t *freqs, int oscilsize)
{
    float normMax = 0.0f;
    for(int i = 0; i < oscilsize / 2; ++i) {
        //magnitude squared
        const float norm = normal(freqs, i);
        if(normMax < norm)
            normMax = norm;
    }

    const float max = sqrtf(normMax);
    if(max < 1e-8) //data is all ~zero, do not amplify noise
        return;

    for(int i = 0; i < oscilsize / 2; ++i)
        freqs[i] /= max;
}

//Full RMS normalize
void rmsNormalize(fft_t *freqs, int oscilsize)
{
    float sum = 0.0f;
    for(int i = 1; i < oscilsize / 2; ++i)
        sum += normal(freqs, i);

    if(sum < 0.000001f)
        return;  //data is all ~zero, do not amplify noise

    const float gain = 1.0f / sqrtf(sum);

    for(int i = 1; i < oscilsize / 2; ++i)
        freqs[i] *= gain;
}

#define DIFF(par) (bfrs.old ## par != P ## par)

FFTfreqBuffer ctorAllocFreqs(FFTwrapper* fft, int oscilsize) {
    return fft ? fft->allocFreqBuf() : FFTwrapper::riskAllocFreqBufWithSize(oscilsize);
}

FFTsampleBuffer ctorAllocSamples(FFTwrapper* fft, int oscilsize) {
    return fft ? fft->allocSampleBuf() : FFTwrapper::riskAllocSampleBufWithSize(oscilsize);
}

OscilGenBuffers::OscilGenBuffers(zyn::OscilGenBuffersCreator c) :
    oscilsize(c.oscilsize),
    // fft_ can be nullptr in case of pasting
    oscilFFTfreqs(ctorAllocFreqs(c.fft, c.oscilsize)),
    pendingfreqs(oscilFFTfreqs.data),
    tmpsmps(ctorAllocSamples(c.fft, c.oscilsize)),
    outoscilFFTfreqs(ctorAllocFreqs(c.fft, c.oscilsize)),
    cachedbasefunc(ctorAllocSamples(c.fft, c.oscilsize)),
    cachedbasevalid(false),
    basefuncFFTfreqs(ctorAllocFreqs(c.fft, c.oscilsize)),
    scratchFreqs(ctorAllocFreqs(c.fft, c.oscilsize))
{
    defaults();
}

OscilGenBuffers::~OscilGenBuffers()
{
    delete[] tmpsmps.data;
    delete[] outoscilFFTfreqs.data;
    delete[] basefuncFFTfreqs.data;
    delete[] oscilFFTfreqs.data;
    delete[] cachedbasefunc.data;
    delete[] scratchFreqs.data;
}

zyn::OscilGenBuffersCreator OscilGen::createOscilGenBuffers() const
{
    return OscilGenBuffersCreator(fft, synth.oscilsize);
}

void OscilGenBuffers::defaults()
{
    oldbasefunc = 0;
    oldbasepar  = 64;
    oldhmagtype = 0;
    oldwaveshapingfunction = 0;
    oldwaveshaping = 64;
    oldbasefuncmodulation     = 0;
    oldharmonicshift          = 0;
    oldbasefuncmodulationpar1 = 0;
    oldbasefuncmodulationpar2 = 0;
    oldbasefuncmodulationpar3 = 0;
    oldmodulation     = 0;
    oldmodulationpar1 = 0;
    oldmodulationpar2 = 0;
    oldmodulationpar3 = 0;

    for(int i = 0; i < MAX_AD_HARMONICS; ++i) {
        hmag[i]    = 0.0f;
        hphase[i]  = 0.0f;
    }

    clearAll(oscilFFTfreqs.data, oscilsize);
    clearAll(basefuncFFTfreqs.data, oscilsize);
    oscilprepared = 0;
    oldfilterpars = 0;
    oldsapars     = 0;
}

OscilGen::OscilGen(const SYNTH_T &synth_, FFTwrapper *fft_, Resonance *res_)
    :Presets(),
      m_myBuffers(OscilGenBuffersCreator(fft_, synth_.oscilsize)),
      fft(fft_),
      res(res_),
      synth(synth_)
{
    if(fft_) {
        // FFTwrapper should operate exactly on all "oscilsize" bytes
        assert(fft_->fftsize() == synth_.oscilsize);
    } else {
        // this is possible if *this is a temporary paste object
    }

    setpresettype("Poscilgen");

    randseed = 1; // TODO: take care of random?
    ADvsPAD  = false;

    defaults();
}

void OscilGen::defaults()
{
    for(int i = 0; i < MAX_AD_HARMONICS; ++i) {
        Phmag[i]   = 64;
        Phphase[i] = 64;
    }
    Phmag[0]  = 127;
    Phmagtype = 0;
    if(ADvsPAD)
        Prand = 127;       //max phase randomness (useful if the oscil will be imported to a ADsynth from a PADsynth
    else
        Prand = 64;  //no randomness

    Pcurrentbasefunc = 0;
    Pbasefuncpar     = 64;

    Pbasefuncmodulation     = 0;
    Pbasefuncmodulationpar1 = 64;
    Pbasefuncmodulationpar2 = 64;
    Pbasefuncmodulationpar3 = 32;

    Pmodulation     = 0;
    Pmodulationpar1 = 64;
    Pmodulationpar2 = 64;
    Pmodulationpar3 = 32;

    Pwaveshapingfunction = 0;
    Pwaveshaping    = 64;
    Pfiltertype     = 0;
    Pfilterpar1     = 64;
    Pfilterpar2     = 64;
    Pfilterbeforews = 0;
    Psatype = 0;
    Psapar  = 64;

    Pamprandpower = 64;
    Pamprandtype  = 0;

    Pharmonicshift      = 0;
    Pharmonicshiftfirst = 0;

    Padaptiveharmonics         = 0;
    Padaptiveharmonicspower    = 100;
    Padaptiveharmonicsbasefreq = 128;
    Padaptiveharmonicspar      = 50;

    prepare(myBuffers());
}

void OscilGen::convert2sine()
{
    OscilGenBuffers& bfrs = myBuffers();
    float  mag[MAX_AD_HARMONICS], phase[MAX_AD_HARMONICS];

    {
        FFTwrapper *fft = new FFTwrapper(synth.oscilsize);
        FFTsampleBuffer oscil = fft->allocSampleBuf();
        get(oscil.data, -1.0f);
        fft->smps2freqs_noconst_input(oscil, bfrs.scratchFreqs);
        delete (fft);
    }

    normalize(bfrs.scratchFreqs.data, synth.oscilsize);

    mag[0]   = 0;
    phase[0] = 0;
    for(int i = 0; i < MAX_AD_HARMONICS; ++i) {
        mag[i]   = abs(bfrs.scratchFreqs.data, i + 1);
        phase[i] = arg(bfrs.scratchFreqs.data, i + 1);
    }

    defaults();

    for(int i = 0; i < MAX_AD_HARMONICS - 1; ++i) {
        float newmag   = mag[i];
        float newphase = phase[i];

        Phmag[i] = (int) ((newmag) * 63.0f) + 64;

        Phphase[i] = 64 - (int) (64.0f * newphase / PI);
        if(Phphase[i] > 127)
            Phphase[i] = 127;

        if(Phmag[i] == 64)
            Phphase[i] = 64;
    }
    prepare();
}

float OscilGen::userfunc(OscilGenBuffers& bfrs, float x) const
{
    if (!fft)
        return 0;
    if (!bfrs.cachedbasevalid) {
        fft->freqs2smps(bfrs.basefuncFFTfreqs, bfrs.cachedbasefunc, bfrs.scratchFreqs);
        bfrs.cachedbasevalid = true;
    }
    return cinterpolate(bfrs.cachedbasefunc.data,
                        synth.oscilsize,
                        synth.oscilsize * (x + 1) - 1);
}

/*
 * Get the base function
 */
void OscilGen::getbasefunction(OscilGenBuffers& bfrs, FFTsampleBuffer smps) const
{
    float par = (Pbasefuncpar + 0.5f) / 128.0f;
    if(Pbasefuncpar == 64)
        par = 0.5f;

    float p1 = Pbasefuncmodulationpar1 / 127.0f,
          p2 = Pbasefuncmodulationpar2 / 127.0f,
          p3 = Pbasefuncmodulationpar3 / 127.0f;

    switch(Pbasefuncmodulation) {
        case 1:
            p1 = (powf(2, p1 * 5.0f) - 1.0f) / 10.0f;
            p3 = floorf(powf(2, p3 * 5.0f) - 1.0f);
            if(p3 < 0.9999f)
                p3 = -1.0f;
            break;
        case 2:
            p1 = (powf(2, p1 * 5.0f) - 1.0f) / 10.0f;
            p3 = 1.0f + floorf(powf(2, p3 * 5.0f) - 1.0f);
            break;
        case 3:
            p1 = (powf(2, p1 * 7.0f) - 1.0f) / 10.0f;
            p3 = 0.01f + (powf(2, p3 * 16.0f) - 1.0f) / 10.0f;
            break;
    }

    base_func_t *func = getBaseFunction(Pcurrentbasefunc);

    for(int i = 0; i < synth.oscilsize; ++i) {
        float t = i * 1.0f / synth.oscilsize;

        switch(Pbasefuncmodulation) {
            case 1: //rev
                t = t * p3 + sinf((t + p2) * 2.0f * PI) * p1;
                break;
            case 2: //sine
                t += sinf( (t * p3 + p2) * 2.0f * PI) * p1;
                break;
            case 3: //power
                t += powf((1.0f - cosf((t + p2) * 2.0f * PI)) * 0.5f, p3) * p1;
                break;
            case 4: //chop
                t = t * (powf(2.0, Pbasefuncmodulationpar1/32.f +
                              Pbasefuncmodulationpar2/2048.f)) + p3;
        }

        t = t - floorf(t);

        if(func)
            smps[i] = func(t, par);
        else if (Pcurrentbasefunc == 0)
            smps[i] = -sinf(2.0f * PI * i / synth.oscilsize);
        else
            smps[i] = userfunc(bfrs, t);
    }
}


/*
 * Filter the oscillator
 */
void OscilGen::oscilfilter(fft_t *freqs) const
{
    if(Pfiltertype == 0)
        return;

    const float par    = 1.0f - Pfilterpar1 / 128.0f;
    const float par2   = Pfilterpar2 / 127.0f;
    filter_func_t *filter = getFilter(Pfiltertype);

    for(int i = 1; i < synth.oscilsize / 2; ++i)
        freqs[i] *= filter(i, par, par2);

    normalize(freqs, synth.oscilsize);
}


/*
 * Change the base function
 */
void OscilGen::changebasefunction(OscilGenBuffers& bfrs) const
{
    if(Pcurrentbasefunc != 0) {
        getbasefunction(bfrs, bfrs.tmpsmps);
        if(fft)
            fft->smps2freqs_noconst_input(bfrs.tmpsmps, bfrs.basefuncFFTfreqs);
        clearDC(bfrs.basefuncFFTfreqs.data);
    }
    else //in this case bfrs.basefuncFFTfreqs are not used
        clearAll(bfrs.basefuncFFTfreqs.data, synth.oscilsize);
    bfrs.oscilprepared = 0;
    bfrs.oldbasefunc   = Pcurrentbasefunc;
    bfrs.oldbasepar    = Pbasefuncpar;
    bfrs.oldbasefuncmodulation     = Pbasefuncmodulation;
    bfrs.oldbasefuncmodulationpar1 = Pbasefuncmodulationpar1;
    bfrs.oldbasefuncmodulationpar2 = Pbasefuncmodulationpar2;
    bfrs.oldbasefuncmodulationpar3 = Pbasefuncmodulationpar3;
}

inline void normalize(float *smps, size_t N)
{
    //Find max
    float max = 0.0f;
    for(size_t i = 0; i < N; ++i)
        if(max < fabsf(smps[i]))
            max = fabsf(smps[i]);
    if(max < 0.00001f)
        max = 1.0f;

    //Normalize to +-1
    for(size_t i = 0; i < N; ++i)
        smps[i] /= max;
}

/*
 * Waveshape
 */
void OscilGen::waveshape(OscilGenBuffers& bfrs, FFTfreqBuffer freqs) const
{
    bfrs.oldwaveshapingfunction = Pwaveshapingfunction;
    bfrs.oldwaveshaping = Pwaveshaping;
    if(Pwaveshapingfunction == 0)
        return;

    clearDC(freqs.data);
    //reduce the amplitude of the freqs near the nyquist
    for(int i = 1; i < synth.oscilsize / 8; ++i) {
        float gain = i / (synth.oscilsize / 8.0f);
        freqs[synth.oscilsize / 2 - i] *= gain;
    }
    fft->freqs2smps_noconst_input(freqs, bfrs.tmpsmps);

    //Normalize
    normalize(bfrs.tmpsmps.data, synth.oscilsize);

    //Do the waveshaping
    waveShapeSmps(synth.oscilsize, bfrs.tmpsmps.data, Pwaveshapingfunction, Pwaveshaping);

    fft->smps2freqs_noconst_input(bfrs.tmpsmps, freqs); //perform FFT
}


/*
 * Do the Frequency Modulation of the Oscil
 */
void OscilGen::modulation(OscilGenBuffers& bfrs, FFTfreqBuffer freqs) const
{
    bfrs.oldmodulation     = Pmodulation;
    bfrs.oldmodulationpar1 = Pmodulationpar1;
    bfrs.oldmodulationpar2 = Pmodulationpar2;
    bfrs.oldmodulationpar3 = Pmodulationpar3;
    if(Pmodulation == 0)
        return;


    float modulationpar1 = Pmodulationpar1 / 127.0f,
          modulationpar2 = 0.5f - Pmodulationpar2 / 127.0f,
          modulationpar3 = Pmodulationpar3 / 127.0f;

    switch(Pmodulation) {
        case 1:
            modulationpar1 = (powf(2, modulationpar1 * 7.0f) - 1.0f) / 100.0f;
            modulationpar3 = floorf((powf(2, modulationpar3 * 5.0f) - 1.0f));
            if(modulationpar3 < 0.9999f)
                modulationpar3 = -1.0f;
            break;
        case 2:
            modulationpar1 = (powf(2, modulationpar1 * 7.0f) - 1.0f) / 100.0f;
            modulationpar3 = 1.0f
                             + floorf((powf(2, modulationpar3 * 5.0f) - 1.0f));
            break;
        case 3:
            modulationpar1 = (powf(2, modulationpar1 * 9.0f) - 1.0f) / 100.0f;
            modulationpar3 = 0.01f
                             + (powf(2, modulationpar3 * 16.0f) - 1.0f) / 10.0f;
            break;
    }

    clearDC(freqs.data); //remove the DC
    //reduce the amplitude of the freqs near the nyquist
    for(int i = 1; i < synth.oscilsize / 8; ++i) {
        const float tmp = i / (synth.oscilsize / 8.0f);
        freqs[synth.oscilsize / 2 - i] *= tmp;
    }
    fft->freqs2smps_noconst_input(freqs, bfrs.tmpsmps);
    const int    extra_points = 2;
    float *in = new float[synth.oscilsize + extra_points];

    //Normalize
    normalize(bfrs.tmpsmps.data, synth.oscilsize);

    for(int i = 0; i < synth.oscilsize; ++i)
        in[i] = bfrs.tmpsmps[i];
    for(int i = 0; i < extra_points; ++i)
        in[i + synth.oscilsize] = bfrs.tmpsmps[i];

    //Do the modulation
    for(int i = 0; i < synth.oscilsize; ++i) {
        float t = i * 1.0f / synth.oscilsize;

        switch(Pmodulation) {
            case 1:
                t = t * modulationpar3
                    + sinf((t + modulationpar2) * 2.0f * PI) * modulationpar1; //rev
                break;
            case 2:
                t = t
                    + sinf((t * modulationpar3
                            + modulationpar2) * 2.0f * PI) * modulationpar1; //sine
                break;
            case 3:
                t = t + powf((1.0f - cosf(
                                  (t + modulationpar2) * 2.0f * PI)) * 0.5f,
                             modulationpar3) * modulationpar1; //power
                break;
        }

        t = (t - floorf(t)) * synth.oscilsize;

        const int   poshi = (int) t;
        const float poslo = t - floorf(t);

        bfrs.tmpsmps[i] = in[poshi] * (1.0f - poslo) + in[poshi + 1] * poslo;
    }

    delete [] in;
    fft->smps2freqs_noconst_input(bfrs.tmpsmps, freqs); //perform FFT
}


/*
 * Adjust the spectrum
 */
void OscilGen::spectrumadjust(fft_t *freqs) const
{
    if(Psatype == 0)
        return;
    float par = Psapar / 127.0f;
    switch(Psatype) {
        case 1:
            par = 1.0f - par * 2.0f;
            if(par >= 0.0f)
                par = powf(5.0f, par);
            else
                par = powf(8.0f, par);
            break;
        case 2:
            par = powf(10.0f, (1.0f - par) * 3.0f) * 0.001f;
            break;
        case 3:
            par = powf(10.0f, (1.0f - par) * 3.0f) * 0.001f;
            break;
    }

    normalize(freqs, synth.oscilsize);

    for(int i = 0; i < synth.oscilsize / 2; ++i) {
        float mag   = abs(freqs, i);
        float phase = ((float)M_PI_2) - arg(freqs, i);

        switch(Psatype) {
            case 1:
                mag = powf(mag, par);
                break;
            case 2:
                if(mag < par)
                    mag = 0.0f;
                break;
            case 3:
                mag /= par;
                if(mag > 1.0f)
                    mag = 1.0f;
                break;
        }
        freqs[i] = FFTpolar<fftwf_real>(mag, phase);
    }
}

void OscilGen::shiftharmonics(fft_t *freqs) const
{
    if(Pharmonicshift == 0)
        return;

    int   harmonicshift = -Pharmonicshift;
    fft_t h;

    if(harmonicshift > 0)
        for(int i = synth.oscilsize / 2 - 2; i >= 0; i--) {
            int oldh = i - harmonicshift;
            if(oldh < 0)
                h = 0.0f;
            else
                h = freqs[oldh + 1];
            freqs[i + 1] = h;
        }
    else
        for(int i = 0; i < synth.oscilsize / 2 - 1; ++i) {
        int oldh = i + ::abs(harmonicshift);
            if(oldh >= (synth.oscilsize / 2 - 1))
                h = 0.0f;
            else {
                h = freqs[oldh + 1];
                if(abs(h) < 0.000001f)
                    h = 0.0f;
            }

            freqs[i + 1] = h;
        }

    clearDC(freqs);
}

/*
 * Prepare the Oscillator
 */
void OscilGen::prepare(OscilGenBuffers& bfrs) const
{
    prepare(bfrs, bfrs.oscilFFTfreqs);
}

void OscilGen::prepare(OscilGenBuffers& bfrs, FFTfreqBuffer freqs) const
{
    if((bfrs.oldbasepar != Pbasefuncpar) || (bfrs.oldbasefunc != Pcurrentbasefunc)
       || DIFF(basefuncmodulation) || DIFF(basefuncmodulationpar1)
       || DIFF(basefuncmodulationpar2) || DIFF(basefuncmodulationpar3))
        changebasefunction(bfrs);

    for(int i = 0; i < MAX_AD_HARMONICS; ++i)
        bfrs.hphase[i] = (Phphase[i] - 64.0f) / 64.0f * PI / (i + 1);

    for(int i = 0; i < MAX_AD_HARMONICS; ++i) {
        const float hmagnew = 1.0f - fabsf(Phmag[i] / 64.0f - 1.0f);
        switch(Phmagtype) {
            case 1:
                bfrs.hmag[i] = expf(hmagnew * logf(0.01f));
                break;
            case 2:
                bfrs.hmag[i] = expf(hmagnew * logf(0.001f));
                break;
            case 3:
                bfrs.hmag[i] = expf(hmagnew * logf(0.0001f));
                break;
            case 4:
                bfrs.hmag[i] = expf(hmagnew * logf(0.00001f));
                break;
            default:
                bfrs.hmag[i] = 1.0f - hmagnew;
                break;
        }

        if(Phmag[i] < 64)
            bfrs.hmag[i] = -bfrs.hmag[i];
    }

    //remove the harmonics where Phmag[i]==64
    for(int i = 0; i < MAX_AD_HARMONICS; ++i)
        if(Phmag[i] == 64)
            bfrs.hmag[i] = 0.0f;


    clearAll(freqs.data, synth.oscilsize);
    if(Pcurrentbasefunc == 0)   //the sine case
        for(int i = 0; i < MAX_AD_HARMONICS - 1; ++i) {
            freqs[i + 1] =
                std::complex<float>(-bfrs.hmag[i] * sinf(bfrs.hphase[i] * (i + 1)) / 2.0f,
                        bfrs.hmag[i] * cosf(bfrs.hphase[i] * (i + 1)) / 2.0f);
        }
    else
        for(int j = 0; j < MAX_AD_HARMONICS; ++j) {
            if(Phmag[j] == 64)
                continue;
            for(int i = 1; i < synth.oscilsize / 2; ++i) {
                int k = i * (j + 1);
                if(k >= synth.oscilsize / 2)
                    break;
                freqs[k] += bfrs.basefuncFFTfreqs[i] * FFTpolar<fftwf_real>(
                    bfrs.hmag[j],
                    bfrs.hphase[j] * k);
            }
        }

    if(Pharmonicshiftfirst != 0)
        shiftharmonics(freqs.data);

    if(Pfilterbeforews) {
        oscilfilter(freqs.data);
        waveshape(bfrs, freqs);
    } else {
        waveshape(bfrs, freqs);
        oscilfilter(freqs.data);
    }

    modulation(bfrs, freqs);
    spectrumadjust(freqs.data);
    if(Pharmonicshiftfirst == 0)
        shiftharmonics(freqs.data);

    clearDC(freqs.data);

    bfrs.oldhmagtype      = Phmagtype;
    bfrs.oldharmonicshift = Pharmonicshift + Pharmonicshiftfirst * 256;

    bfrs.oscilprepared = 1;
}

fft_t operator*(float a, fft_t b)
{
    return std::complex<float>((float)(a*b.real()), (float)(a*b.imag()));
}

void OscilGen::adaptiveharmonic(fft_t *f, float freq) const
{
    if(Padaptiveharmonics == 0 /*||(freq<1.0f)*/)
        return;
    if(freq < 1.0f)
        freq = 440.0f;

    fft_t *inf = new fft_t[synth.oscilsize / 2]; // XXXRT
    for(int i = 0; i < synth.oscilsize / 2; ++i)
        inf[i] = f[i];
    clearAll(f, synth.oscilsize);
    clearDC(inf);

    float basefreq = 30.0f * powf(10.0f, Padaptiveharmonicsbasefreq / 128.0f);
    float power    = (Padaptiveharmonicspower + 1.0f) / 101.0f;

    float rap = freq / basefreq;

    rap = powf(rap, power);

    bool down = false;
    if(rap > 1.0f) {
        rap  = 1.0f / rap;
        down = true;
    }

    for(int i = 0; i < synth.oscilsize / 2 - 2; ++i) {
        const int   high = (int)(i * rap);
        const float low  = fmodf(i * rap, 1.0f);

        if(high >= (synth.oscilsize / 2 - 2))
            break;

        if(down) {
            f[high] += (1.0f - low) * inf[i];
            f[high + 1] += low * inf[i];
        }
        else {
            f[i] = (1.0f - low) * inf[high] + low * inf[high + 1];
        }
    }
    if(!down)//correct the amplitude of the first harmonic
        f[0] *= rap;

    f[1] += f[0];
    clearDC(f);
    delete[] inf;
}

void OscilGen::adaptiveharmonicpostprocess(fft_t *f, int size) const
{
    if(Padaptiveharmonics <= 1)
        return;
    fft_t *inf = new fft_t[size]; // XXXRT
    float  par = Padaptiveharmonicspar * 0.01f;
    par = 1.0f - powf((1.0f - par), 1.5f);

    for(int i = 0; i < size; ++i) {
        inf[i] = f[i] * par;
        f[i]  *= (1.0f - par);
    }


    if(Padaptiveharmonics == 2) { //2n+1
        for(int i = 0; i < size; ++i)
            if((i % 2) == 0)
                f[i] += inf[i];  //i=0 first harmonic,etc.
    }
    else {  //other ways
        int nh = (Padaptiveharmonics - 3) / 2 + 2;
        int sub_vs_add = (Padaptiveharmonics - 3) % 2;
        if(sub_vs_add == 0) {
            for(int i = 0; i < size; ++i)
                if(((i + 1) % nh) == 0)
                    f[i] += inf[i];
        }
        else
            for(int i = 0; i < size / nh - 1; ++i)
                f[(i + 1) * nh - 1] += inf[i];
    }

    delete [] inf;
}

void OscilGen::newrandseed(unsigned int randseed)
{
    this->randseed = randseed;
}

bool OscilGen::needPrepare(OscilGenBuffers& bfrs) const
{
    bool outdated = false;

    //Check function parameters
    if((bfrs.oldbasepar != Pbasefuncpar) || (bfrs.oldbasefunc != Pcurrentbasefunc)
       || DIFF(hmagtype) || DIFF(waveshaping) || DIFF(waveshapingfunction))
        outdated = true;

    //Check filter parameters
    if(bfrs.oldfilterpars != Pfiltertype * 256 + Pfilterpar1 + Pfilterpar2 * 65536
       + Pfilterbeforews * 16777216) {
        outdated      = true;
        bfrs.oldfilterpars = Pfiltertype * 256 + Pfilterpar1 + Pfilterpar2 * 65536
                        + Pfilterbeforews * 16777216;
    }

    //Check spectrum adjustments
    if(bfrs.oldsapars != Psatype * 256 + Psapar) {
        outdated  = true;
        bfrs.oldsapars = Psatype * 256 + Psapar;
    }

    //Check function modulation
    if(DIFF(basefuncmodulation) || DIFF(basefuncmodulationpar1)
       || DIFF(basefuncmodulationpar2) || DIFF(basefuncmodulationpar3))
        outdated = true;

    //Check overall modulation
    if(DIFF(modulation) || DIFF(modulationpar1)
       || DIFF(modulationpar2) || DIFF(modulationpar3))
        outdated = true;

    //Check harmonic shifts
    if(bfrs.oldharmonicshift != Pharmonicshift + Pharmonicshiftfirst * 256)
        outdated = true;

    return outdated == true || bfrs.oscilprepared == false;
}

/*
 * Get the oscillator function
 */
short int OscilGen::get(OscilGenBuffers& bfrs, float* smps, float freqHz, int resonance) const
{
    if(needPrepare(bfrs))
        prepare(bfrs);

    fft_t *input = freqHz > 0.0f ? bfrs.oscilFFTfreqs.data : bfrs.pendingfreqs;

    unsigned int realrnd = prng();
    sprng(randseed);

    int outpos =
        (int)((RND * 2.0f
               - 1.0f) * synth.oscilsize_f * (Prand - 64.0f) / 64.0f);
    outpos = (outpos + 2 * synth.oscilsize) % synth.oscilsize;


    clearAll(bfrs.outoscilFFTfreqs.data, synth.oscilsize);

    int nyquist = (int)(0.5f * synth.samplerate_f / fabsf(freqHz)) + 2;
    if(ADvsPAD)
        nyquist = (int)(synth.oscilsize / 2);
    if(nyquist > synth.oscilsize / 2)
        nyquist = synth.oscilsize / 2;

    //Process harmonics
    {
        int realnyquist = nyquist;

        if(Padaptiveharmonics != 0)
            nyquist = synth.oscilsize / 2;
        for(int i = 1; i < nyquist - 1; ++i)
            bfrs.outoscilFFTfreqs[i] = input[i];

        adaptiveharmonic(bfrs.outoscilFFTfreqs.data, freqHz);
        adaptiveharmonicpostprocess(&bfrs.outoscilFFTfreqs[1],
                                    synth.oscilsize / 2 - 1);

        nyquist = realnyquist;
    }

    if(Padaptiveharmonics)   //do the antialiasing in the case of adaptive harmonics
        for(int i = nyquist; i < synth.oscilsize / 2; ++i)
            bfrs.outoscilFFTfreqs[i] = fft_t(0.0f, 0.0f);

    // Randomness (each harmonic), the block type is computed
    // in ADnote by setting start position according to this setting
    if((Prand > 64) && (freqHz >= 0.0f) && (!ADvsPAD)) {
        const float rnd = PI * powf((Prand - 64.0f) / 64.0f, 2.0f);
        for(int i = 1; i < nyquist - 1; ++i) //to Nyquist only for AntiAliasing
            bfrs.outoscilFFTfreqs[i] *=
                FFTpolar<fftwf_real>(1.0f, (float)(rnd * i * RND));
    }

    //Harmonic Amplitude Randomness
    if((freqHz > 0.1f) && (!ADvsPAD)) {

        float power     = Pamprandpower / 127.0f;
        float normalize = 1.0f / (1.2f - power);
        switch(Pamprandtype) {
            case 1:
                power = power * 2.0f - 0.5f;
                power = powf(15.0f, power);
                for(int i = 1; i < nyquist - 1; ++i)
                    bfrs.outoscilFFTfreqs[i] *= powf(RND, power) * normalize;
                break;
            case 2:
                power = power * 2.0f - 0.5f;
                power = powf(15.0f, power) * 2.0f;
                float rndfreq = 2 * PI * RND;
                for(int i = 1; i < nyquist - 1; ++i)
                    bfrs.outoscilFFTfreqs[i] *= powf(fabsf(sinf(i * rndfreq)), power)
                                           * normalize;
                break;
        }
    }

    if((freqHz > 0.1f) && (resonance != 0))
        res->applyres(nyquist - 1, bfrs.outoscilFFTfreqs.data, freqHz);

    rmsNormalize(bfrs.outoscilFFTfreqs.data, synth.oscilsize);

    if((ADvsPAD) && (freqHz > 0.1f)) //in this case the smps will contain the freqs
        for(int i = 1; i < synth.oscilsize / 2; ++i)
            smps[i - 1] = abs(bfrs.outoscilFFTfreqs.data, i);
    else {
        fft->freqs2smps(bfrs.outoscilFFTfreqs, bfrs.tmpsmps, bfrs.scratchFreqs);
        for(int i = 0; i < synth.oscilsize; ++i)
            smps[i] = bfrs.tmpsmps[i] * 0.25f;            //correct the amplitude
    }

    sprng(realrnd + 1);

    if(Prand < 64)
        return outpos;
    else
        return 0;
}

///*
// * Get the oscillator function's harmonics
// */
//void OscilGen::getPad(float *smps, float freqHz)
//{
//    if(needPrepare())
//        prepare();
//
//    clearAll(bfrs.outoscilFFTfreqs);
//
//    const int nyquist = (synth.oscilsize / 2);
//
//    //Process harmonics
//    for(int i = 1; i < nyquist - 1; ++i)
//        bfrs.outoscilFFTfreqs[i] = bfrs.oscilFFTfreqs[i];
//
//    adaptiveharmonic(bfrs.outoscilFFTfreqs, freqHz);
//    adaptiveharmonicpostprocess(&bfrs.outoscilFFTfreqs[1], nyquist - 1);
//
//    rmsNormalize(bfrs.outoscilFFTfreqs);
//
//    for(int i = 1; i < nyquist; ++i)
//        smps[i - 1] = abs(bfrs.outoscilFFTfreqs, i);
//}
//

/*
 * Get the spectrum of the oscillator for the UI
 */
void OscilGen::getspectrum(int n, float *spc, int what)
{
    OscilGenBuffers& bfrs = myBuffers();

    if(n > synth.oscilsize / 2)
        n = synth.oscilsize / 2;

    for(int i = 1; i < n; ++i) {
        if(what == 0)
            spc[i] = abs(bfrs.pendingfreqs, i);
        else {
            if(Pcurrentbasefunc == 0)
                spc[i] = ((i == 1) ? (1.0f) : (0.0f));
            else
                spc[i] = abs(bfrs.basefuncFFTfreqs.data, i);
        }
    }
    spc[0]=0;

    if(what == 0) {
        for(int i = 0; i < n; ++i)
            bfrs.outoscilFFTfreqs[i] = fft_t(spc[i], spc[i]);
        fft_t zero = 0;
        std::fill_n(bfrs.outoscilFFTfreqs.data + n, synth.oscilsize / 2 - n, zero);
        adaptiveharmonic(bfrs.outoscilFFTfreqs.data, 0.0f);
        adaptiveharmonicpostprocess(bfrs.outoscilFFTfreqs.data, n - 1);
        for(int i = 0; i < n; ++i)
            spc[i] = (float)bfrs.outoscilFFTfreqs[i].imag();
    }
}


/*
 * Convert the oscillator as base function
 */
void OscilGen::useasbase()
{
    OscilGenBuffers& bfrs = myBuffers();

    for(int i = 0; i < synth.oscilsize / 2; ++i)
        bfrs.basefuncFFTfreqs[i] = bfrs.oscilFFTfreqs[i];

    bfrs.oldbasefunc = Pcurrentbasefunc = 127;
    prepare(bfrs);
    bfrs.cachedbasevalid = false;
}


/*
 * Get the base function for UI
 */
void OscilGen::getcurrentbasefunction(FFTsampleBuffer smps)
{
    OscilGenBuffers& bfrs = myBuffers();
    if(Pcurrentbasefunc != 0)
        fft->freqs2smps(bfrs.basefuncFFTfreqs, smps, bfrs.scratchFreqs);
    else
        getbasefunction(bfrs, smps);   //the sine case
}

#define COPY(y) this->y = o.y
void OscilGen::paste(OscilGen &o)
{
    OscilGenBuffers& bfrs = myBuffers();

    //XXX Figure out a better implementation of this sensitive to RT issues...
    for(int i=0; i<MAX_AD_HARMONICS; ++i) {
        COPY(Phmag[i]);
        COPY(Phphase[i]);
    }

    COPY(Phmagtype);
    COPY(Pcurrentbasefunc);
    COPY(Pbasefuncpar);

    COPY(Pbasefuncmodulation);
    COPY(Pbasefuncmodulationpar1);
    COPY(Pbasefuncmodulationpar2);
    COPY(Pbasefuncmodulationpar3);

    COPY(Pwaveshaping);
    COPY(Pwaveshapingfunction);
    COPY(Pfiltertype);
    COPY(Pfilterpar1);
    COPY(Pfilterpar2);
    COPY(Pfilterbeforews);
    COPY(Psatype);
    COPY(Psapar);

    COPY(Pharmonicshift);
    COPY(Pharmonicshiftfirst);

    COPY(Pmodulation);
    COPY(Pmodulationpar1);
    COPY(Pmodulationpar2);
    COPY(Pmodulationpar3);

    COPY(Prand);
    COPY(Pamprandpower);
    COPY(Pamprandtype);
    COPY(Padaptiveharmonics);
    COPY(Padaptiveharmonicsbasefreq);
    COPY(Padaptiveharmonicspower);
    COPY(Padaptiveharmonicspar);


    if(this->Pcurrentbasefunc)
        changebasefunction(bfrs);
    this->prepare(bfrs);
}
#undef COPY

void OscilGen::add2XML(XMLwrapper& xml)
{
    const OscilGenBuffers& bfrs = myBuffers();

    xml.addpar("harmonic_mag_type", Phmagtype);

    xml.addpar("base_function", Pcurrentbasefunc);
    xml.addpar("base_function_par", Pbasefuncpar);
    xml.addpar("base_function_modulation", Pbasefuncmodulation);
    xml.addpar("base_function_modulation_par1", Pbasefuncmodulationpar1);
    xml.addpar("base_function_modulation_par2", Pbasefuncmodulationpar2);
    xml.addpar("base_function_modulation_par3", Pbasefuncmodulationpar3);

    xml.addpar("modulation", Pmodulation);
    xml.addpar("modulation_par1", Pmodulationpar1);
    xml.addpar("modulation_par2", Pmodulationpar2);
    xml.addpar("modulation_par3", Pmodulationpar3);

    xml.addpar("wave_shaping", Pwaveshaping);
    xml.addpar("wave_shaping_function", Pwaveshapingfunction);

    xml.addpar("filter_type", Pfiltertype);
    xml.addpar("filter_par1", Pfilterpar1);
    xml.addpar("filter_par2", Pfilterpar2);
    xml.addpar("filter_before_wave_shaping", Pfilterbeforews);

    xml.addpar("spectrum_adjust_type", Psatype);
    xml.addpar("spectrum_adjust_par", Psapar);

    xml.addpar("rand", Prand);
    xml.addpar("amp_rand_type", Pamprandtype);
    xml.addpar("amp_rand_power", Pamprandpower);

    xml.addpar("harmonic_shift", Pharmonicshift);
    xml.addparbool("harmonic_shift_first", Pharmonicshiftfirst);

    xml.addpar("adaptive_harmonics", Padaptiveharmonics);
    xml.addpar("adaptive_harmonics_base_frequency", Padaptiveharmonicsbasefreq);
    xml.addpar("adaptive_harmonics_power", Padaptiveharmonicspower);
    xml.addpar("adaptive_harmonics_par", Padaptiveharmonicspar);

    xml.beginbranch("HARMONICS");
    for(int n = 0; n < MAX_AD_HARMONICS; ++n) {
        if((Phmag[n] == 64) && (Phphase[n] == 64))
            continue;
        xml.beginbranch("HARMONIC", n + 1);
        xml.addpar("mag", Phmag[n]);
        xml.addpar("phase", Phphase[n]);
        xml.endbranch();
    }
    xml.endbranch();

    if(Pcurrentbasefunc == 127) {
        normalize(bfrs.basefuncFFTfreqs.data, synth.oscilsize);

        xml.beginbranch("BASE_FUNCTION");
        for(int i = 1; i < synth.oscilsize / 2; ++i) {
            float xc = (float)bfrs.basefuncFFTfreqs[i].real();
            float xs = (float)bfrs.basefuncFFTfreqs[i].imag();
            if((fabsf(xs) > 1e-6f) || (fabsf(xc) > 1e-6f)) {
                xml.beginbranch("BF_HARMONIC", i);
                xml.addparreal("cos", xc);
                xml.addparreal("sin", xs);
                xml.endbranch();
            }
        }
        xml.endbranch();
    }
}

void OscilGen::getfromXML(XMLwrapper& xml)
{
    OscilGenBuffers& bfrs = myBuffers();

    Phmagtype = xml.getpar127("harmonic_mag_type", Phmagtype);

    Pcurrentbasefunc = xml.getpar127("base_function", Pcurrentbasefunc);
    Pbasefuncpar     = xml.getpar127("base_function_par", Pbasefuncpar);

    Pbasefuncmodulation = xml.getpar127("base_function_modulation",
                                         Pbasefuncmodulation);
    Pbasefuncmodulationpar1 = xml.getpar127("base_function_modulation_par1",
                                             Pbasefuncmodulationpar1);
    Pbasefuncmodulationpar2 = xml.getpar127("base_function_modulation_par2",
                                             Pbasefuncmodulationpar2);
    Pbasefuncmodulationpar3 = xml.getpar127("base_function_modulation_par3",
                                             Pbasefuncmodulationpar3);

    Pmodulation     = xml.getpar127("modulation", Pmodulation);
    Pmodulationpar1 = xml.getpar127("modulation_par1",
                                     Pmodulationpar1);
    Pmodulationpar2 = xml.getpar127("modulation_par2",
                                     Pmodulationpar2);
    Pmodulationpar3 = xml.getpar127("modulation_par3",
                                     Pmodulationpar3);

    Pwaveshaping = xml.getpar127("wave_shaping", Pwaveshaping);
    Pwaveshapingfunction = xml.getpar127("wave_shaping_function",
                                          Pwaveshapingfunction);

    Pfiltertype     = xml.getpar127("filter_type", Pfiltertype);
    Pfilterpar1     = xml.getpar127("filter_par1", Pfilterpar1);
    Pfilterpar2     = xml.getpar127("filter_par2", Pfilterpar2);
    Pfilterbeforews = xml.getpar127("filter_before_wave_shaping",
                                     Pfilterbeforews);

    Psatype = xml.getpar127("spectrum_adjust_type", Psatype);
    Psapar  = xml.getpar127("spectrum_adjust_par", Psapar);

    Prand = xml.getpar127("rand", Prand);
    Pamprandtype  = xml.getpar127("amp_rand_type", Pamprandtype);
    Pamprandpower = xml.getpar127("amp_rand_power", Pamprandpower);

    Pharmonicshift = xml.getpar("harmonic_shift",
                                 Pharmonicshift,
                                 -64,
                                 64);
    Pharmonicshiftfirst = xml.getparbool("harmonic_shift_first",
                                          Pharmonicshiftfirst);

    Padaptiveharmonics = xml.getpar("adaptive_harmonics",
                                     Padaptiveharmonics,
                                     0,
                                     127);
    Padaptiveharmonicsbasefreq = xml.getpar(
        "adaptive_harmonics_base_frequency",
        Padaptiveharmonicsbasefreq,
        0,
        255);
    Padaptiveharmonicspower = xml.getpar("adaptive_harmonics_power",
                                          Padaptiveharmonicspower,
                                          0,
                                          200);
    Padaptiveharmonicspar = xml.getpar("adaptive_harmonics_par",
                                       Padaptiveharmonicspar,
                                       0,
                                       100);


    if(xml.enterbranch("HARMONICS")) {
        Phmag[0]   = 64;
        Phphase[0] = 64;
        for(int n = 0; n < MAX_AD_HARMONICS; ++n) {
            if(xml.enterbranch("HARMONIC", n + 1) == 0)
                continue;
            Phmag[n]   = xml.getpar127("mag", 64);
            Phphase[n] = xml.getpar127("phase", 64);
            xml.exitbranch();
        }
        xml.exitbranch();
    }

    if(Pcurrentbasefunc != 0)
        changebasefunction(bfrs);

    if(xml.enterbranch("BASE_FUNCTION")) {
        for(int i = 1; i < synth.oscilsize / 2; ++i)
            if(xml.enterbranch("BF_HARMONIC", i)) {
                bfrs.basefuncFFTfreqs[i] =
                    std::complex<float>(xml.getparreal("cos", 0.0f),
                            xml.getparreal("sin", 0.0f));
                xml.exitbranch();
            }
        xml.exitbranch();

        clearDC(bfrs.basefuncFFTfreqs.data);
        normalize(bfrs.basefuncFFTfreqs.data, synth.oscilsize);
        bfrs.cachedbasevalid = false;
    }}


//Define basic functions
#define FUNC(b) float basefunc_ ## b(float x, float a)

FUNC(pulse)
{
    return (fmodf(x, 1.0f) < a) ? -1.0f : 1.0f;
}

FUNC(saw)
{
    if(a < 0.00001f)
        a = 0.00001f;
    else
    if(a > 0.99999f)
        a = 0.99999f;
    x = fmodf(x, 1);
    if(x < a)
        return x / a * 2.0f - 1.0f;
    else
        return (1.0f - x) / (1.0f - a) * 2.0f - 1.0f;
}

FUNC(triangle)
{
    x = fmodf(x + 0.25f, 1);
    a = 1 - a;
    if(a < 0.00001f)
        a = 0.00001f;
    if(x < 0.5f)
        x = x * 4 - 1.0f;
    else
        x = (1.0f - x) * 4 - 1.0f;
    x /= -a;
    if(x < -1.0f)
        x = -1.0f;
    if(x > 1.0f)
        x = 1.0f;
    return x;
}

FUNC(power)
{
    x = fmodf(x, 1);
    if(a < 0.00001f)
        a = 0.00001f;
    else
    if(a > 0.99999f)
        a = 0.99999f;
    return powf(x, expf((a - 0.5f) * 10.0f)) * 2.0f - 1.0f;
}

FUNC(gauss)
{
    x = fmodf(x, 1) * 2.0f - 1.0f;
    if(a < 0.00001f)
        a = 0.00001f;
    return expf(-x * x * (expf(a * 8) + 5.0f)) * 2.0f - 1.0f;
}

FUNC(diode)
{
    if(a < 0.00001f)
        a = 0.00001f;
    else
    if(a > 0.99999f)
        a = 0.99999f;
    a = a * 2.0f - 1.0f;
    x = cosf((x + 0.5f) * 2.0f * PI) - a;
    if(x < 0.0f)
        x = 0.0f;
    return x / (1.0f - a) * 2 - 1.0f;
}

FUNC(abssine)
{
    x = fmodf(x, 1);
    if(a < 0.00001f)
        a = 0.00001f;
    else
    if(a > 0.99999f)
        a = 0.99999f;
    return sinf(powf(x, expf((a - 0.5f) * 5.0f)) * PI) * 2.0f - 1.0f;
}

FUNC(pulsesine)
{
    if(a < 0.00001f)
        a = 0.00001f;
    x = (fmodf(x, 1) - 0.5f) * expf((a - 0.5f) * logf(128));
    if(x < -0.5f)
        x = -0.5f;
    else
    if(x > 0.5f)
        x = 0.5f;
    x = sinf(x * PI * 2.0f);
    return x;
}

FUNC(stretchsine)
{
    x = fmodf(x + 0.5f, 1) * 2.0f - 1.0f;
    a = (a - 0.5f) * 4;
    if(a > 0.0f)
        a *= 2;
    a = powf(3.0f, a);
    float b = powf(fabsf(x), a);
    if(x < 0)
        b = -b;
    return -sinf(b * PI);
}

FUNC(chirp)
{
    x = fmodf(x, 1.0f) * 2.0f * PI;
    a = (a - 0.5f) * 4;
    if(a < 0.0f)
        a *= 2.0f;
    a = powf(3.0f, a);
    return sinf(x / 2.0f) * sinf(a * x * x);
}

FUNC(absstretchsine)
{
    x = fmodf(x + 0.5f, 1) * 2.0f - 1.0f;
    a = (a - 0.5f) * 9;
    a = powf(3.0f, a);
    float b = powf(fabsf(x), a);
    if(x < 0)
        b = -b;
    return -powf(sinf(b * PI), 2);
}

FUNC(chebyshev)
{
    a = a * a * a * 30.0f + 1.0f;
    return cosf(acosf(x * 2.0f - 1.0f) * a);
}

FUNC(sqr)
{
    a = a * a * a * a * 160.0f + 0.001f;
    return -atanf(sinf(x * 2.0f * PI) * a);
}

FUNC(spike)
{
    float b = a * 0.66666f; // the width of the range: if a == 0.5, b == 0.33333

    if(x < 0.5) {
        if(x < (0.5 - (b / 2.0)))
            return 0.0;
        else {
            x = (x + (b / 2) - 0.5f) * (2.f / b); // shift to zero, and expand to range from 0 to 1
            return x * (2.f / b); // this is the slope: 1 / (b / 2)
        }
    }
    else {
        if(x > (0.5 + (b / 2.0)))
            return 0.0;
        else {
            x = (x - .5f) * (2.f / b);
            return (1 - x) * (2.f / b);
        }
    }
}

FUNC(circle)
{
    // a is parameter: 0 -> 0.5 -> 1 // O.5 = circle
    float b, y;

    b = 2 - (a * 2); // b goes from 2 to 0
    x = x * 4;

    if(x < 2) {
        x = x - 1; // x goes from -1 to 1
        if((x < -b) || (x > b))
            y = 0;
        else
            y = sqrtf(1 - (powf(x, 2) / powf(b, 2)));  // normally * a^2, but a stays 1
    }
    else {
        x = x - 3; // x goes from -1 to 1 as well
        if((x < -b) || (x > b))
            y = 0;
        else
            y = -sqrtf(1 - (powf(x, 2) / powf(b, 2)));
    }
    return y;
}

static float
power_cosinus_32(float _x, double _power)
{
    uint32_t x = (_x - floorf(_x)) * (1ULL << 32);
    double retval;
    uint8_t num;

    /* Handle special cases, if any */
    switch (x) {
    case 0xFFFFFFFFU:
    case 0x00000000U:
        return (1.0f);
    case 0x3FFFFFFFU:
    case 0x40000000U:
    case 0xBFFFFFFFU:
    case 0xC0000000U:
        return (0.0f);
    case 0x7FFFFFFFU:
    case 0x80000000U:
        return (-1.0f);
    }

    /* Apply "grey" encoding */
    for (uint32_t mask = 1U << 31; mask != 1; mask /= 2) {
        if (x & mask)
            x ^= (mask - 1);
    }

    /* Find first set bit */
    for (num = 0; num != 30; num++) {
        if (x & (1U << num)) {
            num++;
            break;
        }
    }

    /* Initialize return value */
    retval = 0.0;

    /* Compute the rest of the power series */
    for (; num != 30; num++) {
        if (x & (1U << num))
            retval = pow((1.0 - retval) / 2.0, _power);
        else
            retval = pow((1.0 + retval) / 2.0, _power);
    }

    /* Check if halfway */
    if (x & (1ULL << 30))
        retval = -retval;

    return (retval);
}

//
// power argument magic values:
//     0.0: Converges to a square wave
//     0.5: Sinus wave
//     1.0: Triangle wave
// x: phase value [0..1>
//
static float
power_sinus_32(float _x, double _power)
{
    return (power_cosinus_32(_x + 0.75f, _power));
}

FUNC(powersinus)
{
    return (power_sinus_32(x, 2.0 * a));
}

base_func_t *getBaseFunction(unsigned char func)
{
    static base_func_t * const functions[] = {
        basefunc_triangle,
        basefunc_pulse,
        basefunc_saw,
        basefunc_power,
        basefunc_gauss,
        basefunc_diode,
        basefunc_abssine,
        basefunc_pulsesine,
        basefunc_stretchsine,
        basefunc_chirp,
        basefunc_absstretchsine,
        basefunc_chebyshev,
        basefunc_sqr,
        basefunc_spike,
        basefunc_circle,
        basefunc_powersinus,
    };

    if(!func)
        return NULL;

    if(func == 127) //should be the custom wave
        return NULL;

    func--;
    assert(func < (sizeof(functions)/sizeof(functions[0])));
    return functions[func];
}

//And filters

#define FILTER(x) float osc_ ## x(unsigned int i, float par, float par2)
FILTER(lp)
{
    float gain = powf(1.0f - par * par * par * 0.99f, i);
    float tmp  = par2 * par2 * par2 * par2 * 0.5f + 0.0001f;
    if(gain < tmp)
        gain = powf(gain, 10.0f) / powf(tmp, 9.0f);
    return gain;
}

FILTER(hp1)
{
    float gain = 1.0f - powf(1.0f - par * par, i + 1);
    return powf(gain, par2 * 2.0f + 0.1f);
}

FILTER(hp1b)
{
    if(par < 0.2f)
        par = par * 0.25f + 0.15f;
    float gain = 1.0f - powf(1.0f - par * par * 0.999f + 0.001f,
                             i * 0.05f * i + 1.0f);
    float tmp = powf(5.0f, par2 * 2.0f);
    return powf(gain, tmp);
}

FILTER(bp1)
{
    float gain = i + 1 - powf(2, (1.0f - par) * 7.5f);
    gain = 1.0f / (1.0f + gain * gain / (i + 1.0f));
    float tmp = powf(5.0f, par2 * 2.0f);
    gain = powf(gain, tmp);
    if(gain < 1e-5)
        gain = (float)1e-5;
    return gain;
}

FILTER(bs1)
{
    float gain = i + 1 - powf(2, (1.0f - par) * 7.5f);
    gain = powf(atanf(gain / (i / 10.0f + 1)) / 1.57f, 6);
    return powf(gain, par2 * par2 * 3.9f + 0.1f);
}

FILTER(lp2)
{
    return (i + 1 >
            powf(2, (1.0f - par) * 10) ? 0.0f : 1.0f) * par2 + (1.0f - par2);
}

FILTER(hp2)
{
    if(par == 1)
        return 1.0f;
    return (i + 1 >
            powf(2, (1.0f - par) * 7) ? 1.0f : 0.0f) * par2 + (1.0f - par2);
}

FILTER(bp2)
{
    return (fabsf(powf(2,
                      (1.0f
                       - par)
                      * 7)
                 - i) > i / 2 + 1 ? 0.0f : 1.0f) * par2 + (1.0f - par2);
}

FILTER(bs2)
{
    return (fabsf(powf(2,
                      (1.0f
                       - par)
                      * 7)
                 - i) < i / 2 + 1 ? 0.0f : 1.0f) * par2 + (1.0f - par2);
}

bool floatEq(float a, float b)
{
    const float fudge = .01f;
    return a + fudge > b && a - fudge < b;
}

FILTER(cos)
{
    float tmp = powf(5.0f, par2 * 2.0f - 1.0f);
    tmp = powf(i / 32.0f, tmp) * 32.0f;
    if(floatEq(par2 * 127.0f, 64.0f))
        tmp = i;
    float gain = cosf(par * par * PI / 2.0f * tmp);
    gain *= gain;
    return gain;
}

FILTER(sin)
{
    float tmp = powf(5.0f, par2 * 2.0f - 1.0f);
    tmp = powf(i / 32.0f, tmp) * 32.0f;
    if(floatEq(par2 * 127.0f, 64.0f))
        tmp = i;
    float gain = sinf(par * par * PI / 2.0f * tmp);
    gain *= gain;
    return gain;
}

FILTER(low_shelf)
{
    float p2 = 1.0f - par + 0.2f;
    float x  = i / (64.0f * p2 * p2);
    if(x < 0.0f)
        x = 0.0f;
    else
    if(x > 1.0f)
        x = 1.0f;
    float tmp = powf(1.0f - par2, 2.0f);
    return cosf(x * PI) * (1.0f - tmp) + 1.01f + tmp;
}

FILTER(s)
{
    unsigned int tmp = (int) (powf(2.0f, (1.0f - par) * 7.2f));
    float gain = 1.0f;
    if(i == tmp)
        gain = powf(2.0f, par2 * par2 * 8.0f);
    return gain;
}

FILTER(lpsk)
{
    float tmp2PIf = 2.0f * PI * (1.05f-par)*64.0f;
    std::complex<float> s  (0.0f,2.0f*PI*i);
    float vOut = tmp2PIf * tmp2PIf;
    std::complex<float> vIn = s*s + tmp2PIf*s/((par2)+(2.0f*par*par2)+0.5f) + tmp2PIf*tmp2PIf;
    return std::abs((vOut*vOut*vOut) / (vIn*vIn*vIn));
    
}
#undef FILTER

filter_func_t *getFilter(unsigned char func)
{
    static filter_func_t * const functions[] = {
        osc_lp,
        osc_hp1,
        osc_hp1b,
        osc_bp1,
        osc_bs1,
        osc_lp2,
        osc_hp2,
        osc_bp2,
        osc_bs2,
        osc_cos,
        osc_sin,
        osc_low_shelf,
        osc_s,
        osc_lpsk
    };

    if(!func)
        return NULL;

    func--;
    assert(func < (sizeof(functions)/sizeof(functions[0])));
    return functions[func];
}

}

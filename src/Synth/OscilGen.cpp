/*
  ZynAddSubFX - a software synthesizer

  OscilGen.cpp - Waveform generator for ADnote
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "OscilGen.h"
#include "../Misc/WaveShapeSmps.h"

#include <cassert>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>


//operations on FFTfreqs
inline void clearAll(fft_t *freqs)
{
    memset(freqs, 0, OSCIL_SIZE / 2 * sizeof(fft_t));
}

inline void clearDC(fft_t *freqs)
{
    freqs[0] = fft_t(0.0f, 0.0f);
}

//return magnitude squared
inline float normal(const fft_t *freqs, off_t x)
{
    return norm(freqs[x]);
}

//return magnitude
inline float abs(const fft_t *freqs, off_t x)
{
    return abs(freqs[x]);
}

//return angle aka phase
inline float arg(const fft_t *freqs, off_t x)
{
    return arg(freqs[x]);
}

/**
 * Take frequency spectrum and ensure values are normalized based upon
 * magnitude to 0<=x<=1
 */
void normalize(fft_t *freqs)
{
    float normMax = 0.0;
    for(int i = 0; i < OSCIL_SIZE / 2; ++i) {
        //magnitude squared
        const float norm = normal(freqs, i);
        if(normMax < norm)
            normMax = norm;
    }

    const float max = sqrt(normMax);
    if(max < 1e-8) //data is all ~zero, do not amplify noise
        return;

    for(int i=0; i < OSCIL_SIZE / 2; ++i)
        freqs[i] /= max;
}

//Full RMS normalize
void rmsNormalize(fft_t *freqs)
{
    float sum = 0.0f;
    for(int i = 1; i < OSCIL_SIZE / 2; ++i)
        sum += normal(freqs, i);

    if(sum < 0.000001)
        return; //data is all ~zero, do not amplify noise

    const float gain = 1.0 / sqrt(sum);

    for(int i = 1; i < OSCIL_SIZE / 2; i++)
        freqs[i] *= gain;
}

#define DIFF(par) (old##par != P##par)

OscilGen::OscilGen(FFTwrapper *fft_, Resonance *res_):Presets()
{
    assert(fft_);

    setpresettype("Poscilgen");
    fft     = fft_;
    res     = res_;


    tmpsmps = new float[OSCIL_SIZE];
    outoscilFFTfreqs = new fft_t[OSCIL_SIZE / 2];
    oscilFFTfreqs    = new fft_t[OSCIL_SIZE / 2];
    basefuncFFTfreqs = new fft_t[OSCIL_SIZE / 2];

    randseed = 1;
    ADvsPAD  = false;

    defaults();
}

OscilGen::~OscilGen()
{
    delete[] tmpsmps;
    delete[] outoscilFFTfreqs;
    delete[] basefuncFFTfreqs;
    delete[] oscilFFTfreqs;
}


void OscilGen::defaults()
{
    oldbasefunc = 0;
    oldbasepar  = 64;
    oldhmagtype = 0;
    oldwaveshapingfunction    = 0;
    oldwaveshaping            = 64;
    oldbasefuncmodulation     = 0;
    oldharmonicshift          = 0;
    oldbasefuncmodulationpar1 = 0;
    oldbasefuncmodulationpar2 = 0;
    oldbasefuncmodulationpar3 = 0;
    oldmodulation             = 0;
    oldmodulationpar1         = 0;
    oldmodulationpar2         = 0;
    oldmodulationpar3         = 0;

    for(int i = 0; i < MAX_AD_HARMONICS; i++) {
        hmag[i]    = 0.0;
        hphase[i]  = 0.0;
        Phmag[i]   = 64;
        Phphase[i] = 64;
    }
    Phmag[0]  = 127;
    Phmagtype = 0;
    if(ADvsPAD)
        Prand = 127;       //max phase randomness (usefull if the oscil will be imported to a ADsynth from a PADsynth
    else
        Prand = 64; //no randomness

    Pcurrentbasefunc = 0;
    Pbasefuncpar     = 64;

    Pbasefuncmodulation     = 0;
    Pbasefuncmodulationpar1 = 64;
    Pbasefuncmodulationpar2 = 64;
    Pbasefuncmodulationpar3 = 32;

    Pmodulation          = 0;
    Pmodulationpar1      = 64;
    Pmodulationpar2      = 64;
    Pmodulationpar3      = 32;

    Pwaveshapingfunction = 0;
    Pwaveshaping         = 64;
    Pfiltertype          = 0;
    Pfilterpar1          = 64;
    Pfilterpar2          = 64;
    Pfilterbeforews      = 0;
    Psatype = 0;
    Psapar  = 64;

    Pamprandpower = 64;
    Pamprandtype = 0;

    Pharmonicshift      = 0;
    Pharmonicshiftfirst = 0;

    Padaptiveharmonics  = 0;
    Padaptiveharmonicspower    = 100;
    Padaptiveharmonicsbasefreq = 128;
    Padaptiveharmonicspar      = 50;

    clearAll(oscilFFTfreqs);
    clearAll(basefuncFFTfreqs);
    oscilprepared = 0;
    oldfilterpars = 0;
    oldsapars     = 0;
    prepare();
}

void OscilGen::convert2sine()
{
    float mag[MAX_AD_HARMONICS], phase[MAX_AD_HARMONICS];
    float oscil[OSCIL_SIZE];
    fft_t *freqs = new fft_t[OSCIL_SIZE / 2];

    get(oscil, -1.0);
    FFTwrapper *fft = new FFTwrapper(OSCIL_SIZE);
    fft->smps2freqs(oscil, freqs);
    delete (fft);

    normalize(freqs);

    mag[0]   = 0;
    phase[0] = 0;
    for(int i = 0; i < MAX_AD_HARMONICS; i++) {
        mag[i]   = abs(freqs, i + 1);
        phase[i] = arg(freqs, i + 1);
    }

    defaults();

    for(int i = 0; i < MAX_AD_HARMONICS - 1; i++) {
        float newmag   = mag[i];
        float newphase = phase[i];

        Phmag[i]   = (int) ((newmag) * 64.0) + 64;

        Phphase[i] = 64 - (int) (64.0 * newphase / PI);
        if(Phphase[i] > 127)
            Phphase[i] = 127;

        if(Phmag[i] == 64)
            Phphase[i] = 64;
    }
    delete[] freqs;
    prepare();
}

/*
 * Get the base function
 */
void OscilGen::getbasefunction(float *smps)
{
    int      i;
    float par = (Pbasefuncpar + 0.5) / 128.0;
    if(Pbasefuncpar == 64)
        par = 0.5;

    float basefuncmodulationpar1 = Pbasefuncmodulationpar1 / 127.0,
             basefuncmodulationpar2 = Pbasefuncmodulationpar2 / 127.0,
             basefuncmodulationpar3 = Pbasefuncmodulationpar3 / 127.0;

    switch(Pbasefuncmodulation) {
    case 1:
        basefuncmodulationpar1 =
            (pow(2, basefuncmodulationpar1 * 5.0) - 1.0) / 10.0;
        basefuncmodulationpar3 =
            floor((pow(2, basefuncmodulationpar3 * 5.0) - 1.0));
        if(basefuncmodulationpar3 < 0.9999)
            basefuncmodulationpar3 = -1.0;
        break;
    case 2:
        basefuncmodulationpar1 =
            (pow(2, basefuncmodulationpar1 * 5.0) - 1.0) / 10.0;
        basefuncmodulationpar3 = 1.0
                                 + floor((pow(2, basefuncmodulationpar3
                                              * 5.0) - 1.0));
        break;
    case 3:
        basefuncmodulationpar1 =
            (pow(2, basefuncmodulationpar1 * 7.0) - 1.0) / 10.0;
        basefuncmodulationpar3 = 0.01
                                 + (pow(2, basefuncmodulationpar3
                                        * 16.0) - 1.0) / 10.0;
        break;
    }

    base_func func = getBaseFunction(Pcurrentbasefunc);

    for(i = 0; i < OSCIL_SIZE; i++) {
        float t = i * 1.0 / OSCIL_SIZE;

        switch(Pbasefuncmodulation) {
        case 1:
            t = t * basefuncmodulationpar3 + sin(
                (t
                 + basefuncmodulationpar2) * 2.0 * PI) * basefuncmodulationpar1;//rev
            break;
        case 2:
            t = t + sin(
                (t * basefuncmodulationpar3
                 + basefuncmodulationpar2) * 2.0 * PI) * basefuncmodulationpar1;//sine
            break;
        case 3:
            t = t + pow((1.0 - cos(
                             (t + basefuncmodulationpar2) * 2.0 * PI)) * 0.5,
                        basefuncmodulationpar3) * basefuncmodulationpar1;//power
            break;
        }

        t = t - floor(t);

        if(func)
            smps[i] = func(t, par);
        else
            smps[i] = -sin(2.0 * PI * i / OSCIL_SIZE);
    }
}


/*
 * Filter the oscillator
 */
void OscilGen::oscilfilter()
{
    if(Pfiltertype == 0)
        return;

    const float par  = 1.0 - Pfilterpar1 / 128.0;
    const float par2 = Pfilterpar2 / 127.0;
    filter_func filter = getFilter(Pfiltertype);

    for(int i = 1; i < OSCIL_SIZE / 2; i++)
        oscilFFTfreqs[i] *= filter(i,par,par2);

    normalize(oscilFFTfreqs);
}


/*
 * Change the base function
 */
void OscilGen::changebasefunction()
{
    if(Pcurrentbasefunc != 0) {
        getbasefunction(tmpsmps);
        fft->smps2freqs(tmpsmps, basefuncFFTfreqs);
        clearDC(basefuncFFTfreqs);
    }
    else //in this case basefuncFFTfreqs are not used
        clearAll(basefuncFFTfreqs);
    oscilprepared = 0;
    oldbasefunc   = Pcurrentbasefunc;
    oldbasepar    = Pbasefuncpar;
    oldbasefuncmodulation     = Pbasefuncmodulation;
    oldbasefuncmodulationpar1 = Pbasefuncmodulationpar1;
    oldbasefuncmodulationpar2 = Pbasefuncmodulationpar2;
    oldbasefuncmodulationpar3 = Pbasefuncmodulationpar3;
}

inline void normalize(float *smps, size_t N)
{
    //Find max
    float max = 0.0;
    for(size_t i = 0; i < N; i++)
        if(max < fabs(smps[i]))
            max = fabs(smps[i]);
    if(max < 0.00001)
        max = 1.0;

    //Normalize to +-1
    for(size_t i = 0; i < N; i++)
        smps[i] /= max;
}

/*
 * Waveshape
 */
void OscilGen::waveshape()
{
    oldwaveshapingfunction = Pwaveshapingfunction;
    oldwaveshaping = Pwaveshaping;
    if(Pwaveshapingfunction == 0)
        return;

    clearDC(oscilFFTfreqs);
    //reduce the amplitude of the freqs near the nyquist
    for(int i = 1; i < OSCIL_SIZE / 8; i++) {
        float gain = i / (OSCIL_SIZE / 8.0);
        oscilFFTfreqs[OSCIL_SIZE / 2 - i] *= gain;
    }
    fft->freqs2smps(oscilFFTfreqs, tmpsmps);

    //Normalize
    normalize(tmpsmps, OSCIL_SIZE);

    //Do the waveshaping
    waveShapeSmps(OSCIL_SIZE, tmpsmps, Pwaveshapingfunction, Pwaveshaping);

    fft->smps2freqs(tmpsmps, oscilFFTfreqs); //perform FFT
}


/*
 * Do the Frequency Modulation of the Oscil
 */
void OscilGen::modulation()
{
    int i;

    oldmodulation     = Pmodulation;
    oldmodulationpar1 = Pmodulationpar1;
    oldmodulationpar2 = Pmodulationpar2;
    oldmodulationpar3 = Pmodulationpar3;
    if(Pmodulation == 0)
        return;


    float modulationpar1 = Pmodulationpar1 / 127.0,
             modulationpar2 = 0.5 - Pmodulationpar2 / 127.0,
             modulationpar3 = Pmodulationpar3 / 127.0;

    switch(Pmodulation) {
    case 1:
        modulationpar1 = (pow(2, modulationpar1 * 7.0) - 1.0) / 100.0;
        modulationpar3 = floor((pow(2, modulationpar3 * 5.0) - 1.0));
        if(modulationpar3 < 0.9999)
            modulationpar3 = -1.0;
        break;
    case 2:
        modulationpar1 = (pow(2, modulationpar1 * 7.0) - 1.0) / 100.0;
        modulationpar3 = 1.0 + floor((pow(2, modulationpar3 * 5.0) - 1.0));
        break;
    case 3:
        modulationpar1 = (pow(2, modulationpar1 * 9.0) - 1.0) / 100.0;
        modulationpar3 = 0.01 + (pow(2, modulationpar3 * 16.0) - 1.0) / 10.0;
        break;
    }

    clearDC(oscilFFTfreqs); //remove the DC
    //reduce the amplitude of the freqs near the nyquist
    for(i = 1; i < OSCIL_SIZE / 8; i++) {
        float tmp = i / (OSCIL_SIZE / 8.0);
        oscilFFTfreqs[OSCIL_SIZE / 2 - i] *= tmp;
    }
    fft->freqs2smps(oscilFFTfreqs, tmpsmps);
    int extra_points = 2;
    float *in     = new float[OSCIL_SIZE + extra_points];

    //Normalize
    normalize(tmpsmps, OSCIL_SIZE);

    for(i = 0; i < OSCIL_SIZE; i++)
        in[i] = tmpsmps[i];
    for(i = 0; i < extra_points; i++)
        in[i + OSCIL_SIZE] = tmpsmps[i];

    //Do the modulation
    for(i = 0; i < OSCIL_SIZE; i++) {
        float t = i * 1.0 / OSCIL_SIZE;

        switch(Pmodulation) {
        case 1:
            t = t * modulationpar3
                + sin((t + modulationpar2) * 2.0 * PI) * modulationpar1; //rev
            break;
        case 2:
            t = t
                + sin((t * modulationpar3
                       + modulationpar2) * 2.0 * PI) * modulationpar1; //sine
            break;
        case 3:
            t = t + pow((1.0 - cos(
                             (t + modulationpar2) * 2.0 * PI)) * 0.5,
                        modulationpar3) * modulationpar1; //power
            break;
        }

        t = (t - floor(t)) * OSCIL_SIZE;

        int      poshi = (int) t;
        float poslo = t - floor(t);

        tmpsmps[i] = in[poshi] * (1.0 - poslo) + in[poshi + 1] * poslo;
    }

    delete [] in;
    fft->smps2freqs(tmpsmps, oscilFFTfreqs); //perform FFT
}


/*
 * Adjust the spectrum
 */
void OscilGen::spectrumadjust()
{
    if(Psatype == 0)
        return;
    float par = Psapar / 127.0;
    switch(Psatype) {
    case 1:
        par = 1.0 - par * 2.0;
        if(par >= 0.0)
            par = pow(5.0, par);
        else
            par = pow(8.0, par);
        break;
    case 2:
        par = pow(10.0, (1.0 - par) * 3.0) * 0.25;
        break;
    case 3:
        par = pow(10.0, (1.0 - par) * 3.0) * 0.25;
        break;
    }


    normalize(oscilFFTfreqs);

    for(int i = 0; i < OSCIL_SIZE / 2; i++) {
        float mag   = abs(oscilFFTfreqs, i);
        float phase = arg(oscilFFTfreqs, i);

        switch(Psatype) {
        case 1:
            mag = pow(mag, par);
            break;
        case 2:
            if(mag < par)
                mag = 0.0;
            break;
        case 3:
            mag /= par;
            if(mag > 1.0)
                mag = 1.0;
            break;
        }
        oscilFFTfreqs[i] = std::polar(mag, phase);
    }
}

void OscilGen::shiftharmonics()
{
    if(Pharmonicshift == 0)
        return;

    int   harmonicshift = -Pharmonicshift;
    fft_t h;

    if(harmonicshift > 0) {
        for(int i = OSCIL_SIZE / 2 - 2; i >= 0; i--) {
            int oldh = i - harmonicshift;
            if(oldh < 0)
                h = 0.0f;
            else
                h = oscilFFTfreqs[oldh + 1];
            oscilFFTfreqs[i + 1] = h;
        }
    }
    else {
        for(int i = 0; i < OSCIL_SIZE / 2 - 1; i++) {
            int oldh = i + abs(harmonicshift);
            if(oldh >= (OSCIL_SIZE / 2 - 1))
                h = 0.0;
            else {
                h = oscilFFTfreqs[oldh + 1];
                if(abs(h) < 0.000001)
                    h = 0.0;
            }

            oscilFFTfreqs[i + 1] = h;
        }
    }

    clearDC(oscilFFTfreqs);
}

/*
 * Prepare the Oscillator
 */
void OscilGen::prepare()
{
    if((oldbasepar != Pbasefuncpar) || (oldbasefunc != Pcurrentbasefunc)
       || DIFF(basefuncmodulation) || DIFF(basefuncmodulationpar1)
       || DIFF(basefuncmodulationpar2) || DIFF(basefuncmodulationpar3))
        changebasefunction();

    for(int i = 0; i < MAX_AD_HARMONICS; i++)
        hphase[i] = (Phphase[i] - 64.0) / 64.0 * PI / (i + 1);

    for(int i = 0; i < MAX_AD_HARMONICS; i++) {
        const float hmagnew = 1.0 - fabs(Phmag[i] / 64.0 - 1.0);
        switch(Phmagtype) {
        case 1:
            hmag[i] = exp(hmagnew * log(0.01));
            break;
        case 2:
            hmag[i] = exp(hmagnew * log(0.001));
            break;
        case 3:
            hmag[i] = exp(hmagnew * log(0.0001));
            break;
        case 4:
            hmag[i] = exp(hmagnew * log(0.00001));
            break;
        default:
            hmag[i] = 1.0 - hmagnew;
            break;
        }

        if(Phmag[i] < 64)
            hmag[i] = -hmag[i];
    }

    //remove the harmonics where Phmag[i]==64
    for(int i = 0; i < MAX_AD_HARMONICS; i++)
        if(Phmag[i] == 64)
            hmag[i] = 0.0;


    clearAll(oscilFFTfreqs);
    if(Pcurrentbasefunc == 0) { //the sine case
        for(int i = 0; i < MAX_AD_HARMONICS; i++) {
            oscilFFTfreqs[i + 1].real() = -hmag[i] * sin(hphase[i] * (i + 1)) / 2.0;
            oscilFFTfreqs[i + 1].imag() = hmag[i] * cos(hphase[i] * (i + 1)) / 2.0;
        }
    }
    else {
        for(int j = 0; j < MAX_AD_HARMONICS; j++) {
            if(Phmag[j] == 64)
                continue;
            for(int i = 1; i < OSCIL_SIZE / 2; i++) {
                int k = i * (j + 1);
                if(k >= OSCIL_SIZE / 2)
                    break;
                oscilFFTfreqs[k] += basefuncFFTfreqs[i] * std::polar(hmag[j], hphase[j] * k);
            }
        }
    }

    if(Pharmonicshiftfirst != 0)
        shiftharmonics();

    if(Pfilterbeforews == 0) {
        waveshape();
        oscilfilter();
    }
    else {
        oscilfilter();
        waveshape();
    }

    modulation();
    spectrumadjust();
    if(Pharmonicshiftfirst == 0)
        shiftharmonics();

    clearDC(oscilFFTfreqs);

    oldhmagtype      = Phmagtype;
    oldharmonicshift = Pharmonicshift + Pharmonicshiftfirst * 256;

    oscilprepared    = 1;
}

void OscilGen::adaptiveharmonic(fft_t *f, float freq)
{
    if(Padaptiveharmonics == 0 /*||(freq<1.0)*/)
        return;
    if(freq < 1.0)
        freq = 440.0;

    fft_t *inf = new fft_t[OSCIL_SIZE / 2];
    for(int i = 0; i < OSCIL_SIZE / 2; i++)
        inf[i] = f[i];
    clearAll(f);
    clearDC(inf);

    float hc = 0.0, hs = 0.0;
    float basefreq = 30.0 * pow(10.0, Padaptiveharmonicsbasefreq / 128.0);
    float power    = (Padaptiveharmonicspower + 1.0) / 101.0;

    float rap      = freq / basefreq;

    rap = pow(rap, power);

    bool down = false;
    if(rap > 1.0) {
        rap  = 1.0 / rap;
        down = true;
    }

    for(int i = 0; i < OSCIL_SIZE / 2 - 2; i++) {
        float h    = i * rap;
        int      high = (int)(i * rap);
        float low  = fmod(h, 1.0);

        if(high >= (OSCIL_SIZE / 2 - 2))
            break;
        else {
            if(down) {
                f[high].real()     += inf[i].real() * (1.0 - low);
                f[high].imag()     += inf[i].imag() * (1.0 - low);
                f[high + 1].real() += inf[i].real() * low;
                f[high + 1].imag() += inf[i].imag() * low;
            }
            else {
                hc = inf[high].real() * (1.0 - low) + inf[high + 1].real() * low;
                hs = inf[high].imag() * (1.0 - low) + inf[high + 1].imag() * low;
            }
            if(fabs(hc) < 0.000001)
                hc = 0.0;
            if(fabs(hs) < 0.000001)
                hs = 0.0;
        }

        if(!down) {
            if(i == 0) { //corect the aplitude of the first harmonic
                hc *= rap;
                hs *= rap;
            }
            f[i] = fft_t(hc, hs);
        }
    }

    f[1] += f[0];
    clearDC(f);
    delete[] inf;
}

void OscilGen::adaptiveharmonicpostprocess(fft_t *f, int size)
{
    if(Padaptiveharmonics <= 1)
        return;
    fft_t *inf = new fft_t[size];
    float  par = Padaptiveharmonicspar * 0.01;
    par = 1.0 - pow((1.0 - par), 1.5);

    for(int i = 0; i < size; i++) {
        inf[i] = f[i] * par;
        f[i]   *= (1.0 - par);
    }


    if(Padaptiveharmonics == 2) { //2n+1
        for(int i = 0; i < size; i++)
            if((i % 2) == 0)
                f[i] += inf[i]; //i=0 pt prima armonica,etc.
    }
    else {  //celelalte moduri
        int nh = (Padaptiveharmonics - 3) / 2 + 2;
        int sub_vs_add = (Padaptiveharmonics - 3) % 2;
        if(sub_vs_add == 0) {
            for(int i = 0; i < size; i++) {
                if(((i + 1) % nh) == 0)
                    f[i] += inf[i];
            }
        }
        else {
            for(int i = 0; i < size / nh - 1; i++)
                f[(i + 1) * nh - 1] += inf[i];
        }
    }

    delete [] inf;
}

void OscilGen::newrandseed(unsigned int randseed)
{
    this->randseed = randseed;
}

bool OscilGen::needPrepare(void)
{
    bool outdated = false;

    //Check function parameters
    if((oldbasepar != Pbasefuncpar) || (oldbasefunc != Pcurrentbasefunc)
       || DIFF(hmagtype) || DIFF(waveshaping) || DIFF(waveshapingfunction))
        outdated = true;

    //Check filter parameters
    if(oldfilterpars != Pfiltertype * 256 + Pfilterpar1 + Pfilterpar2 * 65536
       + Pfilterbeforews * 16777216) {
        outdated      = true;
        oldfilterpars = Pfiltertype * 256 + Pfilterpar1 + Pfilterpar2 * 65536
                        + Pfilterbeforews * 16777216;
    }

    //Check spectrum adjustments
    if(oldsapars != Psatype * 256 + Psapar) {
        outdated  = true;
        oldsapars = Psatype * 256 + Psapar;
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
    if(oldharmonicshift != Pharmonicshift + Pharmonicshiftfirst * 256)
        outdated = true;

    return outdated == true || oscilprepared == false;
}

/*
 * Get the oscillator function
 */
short int OscilGen::get(float *smps, float freqHz, int resonance)
{
    if(needPrepare())
        prepare();

    int outpos =
        (int)((RND * 2.0 - 1.0) * (float) OSCIL_SIZE * (Prand - 64.0) / 64.0);
    outpos = (outpos + 2 * OSCIL_SIZE) % OSCIL_SIZE;


    clearAll(outoscilFFTfreqs);

    int nyquist = (int)(0.5 * SAMPLE_RATE / fabs(freqHz)) + 2;
    if(ADvsPAD)
        nyquist = (int)(OSCIL_SIZE / 2);
    if(nyquist > OSCIL_SIZE / 2)
        nyquist = OSCIL_SIZE / 2;

    //Process harmonics
    {
        int realnyquist = nyquist;

        if(Padaptiveharmonics != 0)
            nyquist = OSCIL_SIZE / 2;
        for(int i = 1; i < nyquist - 1; i++)
            outoscilFFTfreqs[i] = oscilFFTfreqs[i];

        adaptiveharmonic(outoscilFFTfreqs, freqHz);
        adaptiveharmonicpostprocess(&outoscilFFTfreqs[1], OSCIL_SIZE / 2 - 1);

        nyquist = realnyquist;
    }

    if(Padaptiveharmonics) { //do the antialiasing in the case of adaptive harmonics
        for(int i = nyquist; i < OSCIL_SIZE / 2; i++)
            outoscilFFTfreqs[i] = fft_t(0.0f, 0.0f);
    }

    // Randomness (each harmonic), the block type is computed
    // in ADnote by setting start position according to this setting
    if((Prand > 64) && (freqHz >= 0.0) && (!ADvsPAD)) {
        const float rnd = PI * pow((Prand - 64.0) / 64.0, 2.0);
        for(int i = 1; i < nyquist - 1; i++) //to Nyquist only for AntiAliasing
            outoscilFFTfreqs[i] *= std::polar(1.0f, (float)(rnd * i * RND));
    }

    //Harmonic Amplitude Randomness
    if((freqHz > 0.1) && (!ADvsPAD)) {
        unsigned int realrnd = rand();
        srand(randseed);
        float power     = Pamprandpower / 127.0;
        float normalize = 1.0 / (1.2 - power);
        switch(Pamprandtype) {
        case 1:
            power = power * 2.0 - 0.5;
            power = pow(15.0, power);
            for(int i = 1; i < nyquist - 1; i++)
                outoscilFFTfreqs[i] *= pow(RND, power) * normalize;
            break;
        case 2:
            power = power * 2.0 - 0.5;
            power = pow(15.0, power) * 2.0;
            float rndfreq = 2 * PI * RND;
            for(int i = 1; i < nyquist - 1; i++)
                outoscilFFTfreqs[i] *= pow(fabs(sin(i * rndfreq)), power)
                    * normalize;
            break;
        }
        srand(realrnd + 1);
    }

    if((freqHz > 0.1) && (resonance != 0))
        res->applyres(nyquist - 1, outoscilFFTfreqs, freqHz);

    rmsNormalize(outoscilFFTfreqs);

    if((ADvsPAD) && (freqHz > 0.1)) //in this case the smps will contain the freqs
        for(int i = 1; i < OSCIL_SIZE / 2; i++)
            smps[i - 1] = abs(outoscilFFTfreqs, i);
    else {
        fft->freqs2smps(outoscilFFTfreqs, smps);
        for(int i = 0; i < OSCIL_SIZE; i++)
            smps[i] *= 0.25;                     //correct the amplitude
    }

    if(Prand < 64)
        return outpos;
    else
        return 0;
}


/*
 * Get the spectrum of the oscillator for the UI
 */
void OscilGen::getspectrum(int n, float *spc, int what)
{
    if(n > OSCIL_SIZE / 2)
        n = OSCIL_SIZE / 2;

    for(int i = 1; i < n; i++) {
        if(what == 0)
            spc[i - 1] = abs(oscilFFTfreqs, i);
        else {
            if(Pcurrentbasefunc == 0)
                spc[i - 1] = ((i == 1) ? (1.0) : (0.0));
            else
                spc[i - 1] = abs(basefuncFFTfreqs, i);
        }
    }

    if(what == 0) {
        for(int i = 0; i < n; i++)
            outoscilFFTfreqs[i] = fft_t(spc[i], spc[i]);
        memset(outoscilFFTfreqs+n, 0, (OSCIL_SIZE / 2 - n) * sizeof(fft_t));
        adaptiveharmonic(outoscilFFTfreqs, 0.0);
        adaptiveharmonicpostprocess(outoscilFFTfreqs, n - 1);
        for(int i = 0; i < n; i++)
            spc[i] = outoscilFFTfreqs[i].imag();
    }
}


/*
 * Convert the oscillator as base function
 */
void OscilGen::useasbase()
{
    for(int i = 0; i < OSCIL_SIZE / 2; i++)
        basefuncFFTfreqs[i] = oscilFFTfreqs[i];

    oldbasefunc = Pcurrentbasefunc = 127;
    prepare();
}


/*
 * Get the base function for UI
 */
void OscilGen::getcurrentbasefunction(float *smps)
{
    if(Pcurrentbasefunc != 0)
        fft->freqs2smps(basefuncFFTfreqs, smps);
    else
        getbasefunction(smps);   //the sine case
}

void OscilGen::add2XML(XMLwrapper *xml)
{
    xml->addpar("harmonic_mag_type", Phmagtype);

    xml->addpar("base_function", Pcurrentbasefunc);
    xml->addpar("base_function_par", Pbasefuncpar);
    xml->addpar("base_function_modulation", Pbasefuncmodulation);
    xml->addpar("base_function_modulation_par1", Pbasefuncmodulationpar1);
    xml->addpar("base_function_modulation_par2", Pbasefuncmodulationpar2);
    xml->addpar("base_function_modulation_par3", Pbasefuncmodulationpar3);

    xml->addpar("modulation", Pmodulation);
    xml->addpar("modulation_par1", Pmodulationpar1);
    xml->addpar("modulation_par2", Pmodulationpar2);
    xml->addpar("modulation_par3", Pmodulationpar3);

    xml->addpar("wave_shaping", Pwaveshaping);
    xml->addpar("wave_shaping_function", Pwaveshapingfunction);

    xml->addpar("filter_type", Pfiltertype);
    xml->addpar("filter_par1", Pfilterpar1);
    xml->addpar("filter_par2", Pfilterpar2);
    xml->addpar("filter_before_wave_shaping", Pfilterbeforews);

    xml->addpar("spectrum_adjust_type", Psatype);
    xml->addpar("spectrum_adjust_par", Psapar);

    xml->addpar("rand", Prand);
    xml->addpar("amp_rand_type", Pamprandtype);
    xml->addpar("amp_rand_power", Pamprandpower);

    xml->addpar("harmonic_shift", Pharmonicshift);
    xml->addparbool("harmonic_shift_first", Pharmonicshiftfirst);

    xml->addpar("adaptive_harmonics", Padaptiveharmonics);
    xml->addpar("adaptive_harmonics_base_frequency", Padaptiveharmonicsbasefreq);
    xml->addpar("adaptive_harmonics_power", Padaptiveharmonicspower);

    xml->beginbranch("HARMONICS");
    for(int n = 0; n < MAX_AD_HARMONICS; n++) {
        if((Phmag[n] == 64) && (Phphase[n] == 64))
            continue;
        xml->beginbranch("HARMONIC", n + 1);
        xml->addpar("mag", Phmag[n]);
        xml->addpar("phase", Phphase[n]);
        xml->endbranch();
    }
    xml->endbranch();

    if(Pcurrentbasefunc == 127) {
        normalize(basefuncFFTfreqs);

        xml->beginbranch("BASE_FUNCTION");
        for(int i = 1; i < OSCIL_SIZE / 2; i++) {
            float xc = basefuncFFTfreqs[i].real();
            float xs = basefuncFFTfreqs[i].imag();
            if((fabs(xs) > 0.00001) && (fabs(xs) > 0.00001)) {
                xml->beginbranch("BF_HARMONIC", i);
                xml->addparreal("cos", xc);
                xml->addparreal("sin", xs);
                xml->endbranch();
            }
        }
        xml->endbranch();
    }
}

void OscilGen::getfromXML(XMLwrapper *xml)
{
    Phmagtype = xml->getpar127("harmonic_mag_type", Phmagtype);

    Pcurrentbasefunc    = xml->getpar127("base_function", Pcurrentbasefunc);
    Pbasefuncpar        = xml->getpar127("base_function_par", Pbasefuncpar);

    Pbasefuncmodulation = xml->getpar127("base_function_modulation",
                                         Pbasefuncmodulation);
    Pbasefuncmodulationpar1    = xml->getpar127("base_function_modulation_par1",
                                                Pbasefuncmodulationpar1);
    Pbasefuncmodulationpar2    = xml->getpar127("base_function_modulation_par2",
                                                Pbasefuncmodulationpar2);
    Pbasefuncmodulationpar3    = xml->getpar127("base_function_modulation_par3",
                                                Pbasefuncmodulationpar3);

    Pmodulation                = xml->getpar127("modulation", Pmodulation);
    Pmodulationpar1            = xml->getpar127("modulation_par1",
                                                Pmodulationpar1);
    Pmodulationpar2            = xml->getpar127("modulation_par2",
                                                Pmodulationpar2);
    Pmodulationpar3            = xml->getpar127("modulation_par3",
                                                Pmodulationpar3);

    Pwaveshaping               = xml->getpar127("wave_shaping", Pwaveshaping);
    Pwaveshapingfunction       = xml->getpar127("wave_shaping_function",
                                                Pwaveshapingfunction);

    Pfiltertype                = xml->getpar127("filter_type", Pfiltertype);
    Pfilterpar1                = xml->getpar127("filter_par1", Pfilterpar1);
    Pfilterpar2                = xml->getpar127("filter_par2", Pfilterpar2);
    Pfilterbeforews            = xml->getpar127("filter_before_wave_shaping",
                                                Pfilterbeforews);

    Psatype                    = xml->getpar127("spectrum_adjust_type", Psatype);
    Psapar                     = xml->getpar127("spectrum_adjust_par", Psapar);

    Prand                      = xml->getpar127("rand", Prand);
    Pamprandtype               = xml->getpar127("amp_rand_type", Pamprandtype);
    Pamprandpower              = xml->getpar127("amp_rand_power", Pamprandpower);

    Pharmonicshift             = xml->getpar("harmonic_shift",
                                             Pharmonicshift,
                                             -64,
                                             64);
    Pharmonicshiftfirst        = xml->getparbool("harmonic_shift_first",
                                                 Pharmonicshiftfirst);

    Padaptiveharmonics         = xml->getpar("adaptive_harmonics",
                                             Padaptiveharmonics,
                                             0,
                                             127);
    Padaptiveharmonicsbasefreq = xml->getpar(
        "adaptive_harmonics_base_frequency",
        Padaptiveharmonicsbasefreq,
        0,
        255);
    Padaptiveharmonicspower    = xml->getpar("adaptive_harmonics_power",
                                             Padaptiveharmonicspower,
                                             0,
                                             200);


    if(xml->enterbranch("HARMONICS")) {
        Phmag[0]   = 64;
        Phphase[0] = 64;
        for(int n = 0; n < MAX_AD_HARMONICS; n++) {
            if(xml->enterbranch("HARMONIC", n + 1) == 0)
                continue;
            Phmag[n]   = xml->getpar127("mag", 64);
            Phphase[n] = xml->getpar127("phase", 64);
            xml->exitbranch();
        }
        xml->exitbranch();
    }

    if(Pcurrentbasefunc != 0)
        changebasefunction();


    if(xml->enterbranch("BASE_FUNCTION")) {
        for(int i = 1; i < OSCIL_SIZE / 2; i++) {
            if(xml->enterbranch("BF_HARMONIC", i)) {
                basefuncFFTfreqs[i].real() = xml->getparreal("cos", 0.0);
                basefuncFFTfreqs[i].imag() = xml->getparreal("sin", 0.0);
                xml->exitbranch();
            }
        }
        xml->exitbranch();

        clearDC(basefuncFFTfreqs);
        normalize(basefuncFFTfreqs);
    }
}


//Define basic functions
#define FUNC(b) float basefunc_##b(float x, float a)

FUNC(pulse)
{
    return (fmod(x, 1.0) < a) ? -1.0 : 1.0;
}

FUNC(saw)
{
    if(a < 0.00001)
        a = 0.00001;
    else
    if(a > 0.99999)
        a = 0.99999;
    x = fmod(x, 1);
    if(x < a)
        return x / a * 2.0 - 1.0;
    else
        return (1.0 - x) / (1.0 - a) * 2.0 - 1.0;
}

FUNC(triangle)
{
    x = fmod(x + 0.25, 1);
    a = 1 - a;
    if(a < 0.00001)
        a = 0.00001;
    if(x < 0.5)
        x = x * 4 - 1.0;
    else
        x = (1.0 - x) * 4 - 1.0;
    x /= -a;
    if(x < -1.0)
        x = -1.0;
    if(x > 1.0)
        x = 1.0;
    return x;
}

FUNC(power)
{
    x = fmod(x, 1);
    if(a < 0.00001)
        a = 0.00001;
    else
    if(a > 0.99999)
        a = 0.99999;
    return pow(x, exp((a - 0.5) * 10.0)) * 2.0 - 1.0;
}

FUNC(gauss)
{
    x = fmod(x, 1) * 2.0 - 1.0;
    if(a < 0.00001)
        a = 0.00001;
    return exp(-x * x * (exp(a * 8) + 5.0)) * 2.0 - 1.0;
}

FUNC(diode)
{
    if(a < 0.00001)
        a = 0.00001;
    else
    if(a > 0.99999)
        a = 0.99999;
    a = a * 2.0 - 1.0;
    x = cos((x + 0.5) * 2.0 * PI) - a;
    if(x < 0.0)
        x = 0.0;
    return x / (1.0 - a) * 2 - 1.0;
}

FUNC(abssine)
{
    x = fmod(x, 1);
    if(a < 0.00001)
        a = 0.00001;
    else
    if(a > 0.99999)
        a = 0.99999;
    return sin(pow(x, exp((a - 0.5) * 5.0)) * PI) * 2.0 - 1.0;
}

FUNC(pulsesine)
{
    if(a < 0.00001)
        a = 0.00001;
    x = (fmod(x, 1) - 0.5) * exp((a - 0.5) * log(128));
    if(x < -0.5)
        x = -0.5;
    else
    if(x > 0.5)
        x = 0.5;
    x = sin(x * PI * 2.0);
    return x;
}

FUNC(stretchsine)
{
    x = fmod(x + 0.5, 1) * 2.0 - 1.0;
    a = (a - 0.5) * 4;
    if(a > 0.0)
        a *= 2;
    a = pow(3.0, a);
    float b = pow(fabs(x), a);
    if(x < 0)
        b = -b;
    return -sin(b * PI);
}

FUNC(chirp)
{
    x = fmod(x, 1.0) * 2.0 * PI;
    a = (a - 0.5) * 4;
    if(a < 0.0)
        a *= 2.0;
    a = pow(3.0, a);
    return sin(x / 2.0) * sin(a * x * x);
}

FUNC(absstretchsine)
{
    x = fmod(x + 0.5, 1) * 2.0 - 1.0;
    a = (a - 0.5) * 9;
    a = pow(3.0, a);
    float b = pow(fabs(x), a);
    if(x < 0)
        b = -b;
    return -pow(sin(b * PI), 2);
}

FUNC(chebyshev)
{
    a = a * a * a * 30.0 + 1.0;
    return cos(acos(x * 2.0 - 1.0) * a);
}

FUNC(sqr)
{
    a = a * a * a * a * 160.0 + 0.001;
    return -atan(sin(x * 2.0 * PI) * a);
}

typedef float(*base_func)(float,float);
base_func getBaseFunction(unsigned char func)
{
    if(!func)
        return NULL;

    if(func == 127) //should be the custom wave
        return NULL;

    func--;
    assert(func < 13);
    base_func functions[] = {
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
        basefunc_sqr,};
    return functions[func];
}

//And filters

#define FILTER(x) float osc_##x(unsigned int i, float par, float par2)
FILTER(lp)
{
    float gain = pow(1.0 - par * par * par * 0.99, i);
    float tmp  = par2 * par2 * par2 * par2 * 0.5 + 0.0001;
    if(gain < tmp)
        gain = pow(gain, 10.0) / pow(tmp, 9.0);
    return gain;
}

FILTER(hp1)
{
    float gain = 1.0 - pow(1.0 - par * par, i + 1);
    return pow(gain, par2 * 2.0 + 0.1);
}

FILTER(hp1b)
{
    if(par < 0.2)
        par = par * 0.25 + 0.15;
    float gain = 1.0 - pow(1.0 - par * par * 0.999 + 0.001, i * 0.05 * i + 1.0);
    float tmp  = pow(5.0, par2 * 2.0);
    return pow(gain, tmp);
}

FILTER(bp1)
{
    float gain = i + 1 - pow(2, (1.0 - par) * 7.5);
    gain = 1.0 / (1.0 + gain * gain / (i + 1.0));
    float tmp  = pow(5.0, par2 * 2.0);
    gain = pow(gain, tmp);
    if(gain < 1e-5)
        gain = 1e-5;
    return gain;
}

FILTER(bs1)
{
    float gain = i + 1 - pow(2, (1.0 - par) * 7.5);
    gain = pow(atan(gain / (i / 10.0 + 1)) / 1.57, 6);
    return pow(gain, par2 * par2 * 3.9 + 0.1);
}

FILTER(lp2)
{
    return (i + 1 > pow(2, (1.0 - par) * 10) ? 0.0 : 1.0) * par2 + (1.0 - par2);
}

FILTER(hp2)
{
    if(par == 1)
        return 1.0;
    return (i + 1 > pow(2, (1.0 - par) * 7) ? 1.0 : 0.0) * par2 + (1.0 - par2);
}

FILTER(bp2)
{
    return (fabs(pow(2, (1.0 - par) * 7) - i) > i / 2 + 1 ? 0.0 : 1.0) * par2 + (1.0 - par2);
}

FILTER(bs2)
{
    return (fabs(pow(2, (1.0 - par) * 7) - i) < i / 2 + 1 ? 0.0 : 1.0) * par2 + (1.0 - par2);
}

bool floatEq(float a, float b)
{
    const float fudge = .01;
    return a + fudge > b && a - fudge < b;
}

FILTER(cos)
{
    float tmp = pow(5.0, par2 * 2.0 - 1.0);
    tmp = pow(i / 32.0, tmp) * 32.0;
    if(floatEq(par2 * 127.0, 64.0))
        tmp = i;
    float gain  = cos(par * par * PI / 2.0 * tmp);
    gain *= gain;
    return gain;
}

FILTER(sin)
{
    float tmp = pow(5.0, par2 * 2.0 - 1.0);
    tmp = pow(i / 32.0, tmp) * 32.0;
    if(floatEq(par2 * 127.0, 64.0))
        tmp = i;
    float gain = sin(par * par * PI / 2.0 * tmp);
    gain *= gain;
    return gain;
}

FILTER(low_shelf)
{
    float p2 = 1.0 - par + 0.2;
    float x  = i / (64.0 * p2 * p2);
    if(x < 0.0)
        x = 0.0;
    else
        if(x > 1.0)
            x = 1.0;
    float tmp  = pow(1.0 - par2, 2.0);
    return cos(x * PI) * (1.0 - tmp) + 1.01 + tmp;
}

FILTER(s)
{
    unsigned int tmp  = (int) (pow(2.0, (1.0 - par) * 7.2));
    float gain = 1.0;
    if(i == tmp)
        gain = pow(2.0, par2 * par2 * 8.0);
    return gain;
}
#undef FILTER

typedef float(*filter_func)(unsigned int, float, float);
filter_func getFilter(unsigned char func)
{
    if(!func)
        return NULL;

    func--;
    assert(func < 13);
    filter_func functions[] = {
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
        osc_s};
    return functions[func];
}


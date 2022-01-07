/*
  ZynAddSubFX - a software synthesizer

  OscilGen.h - Waveform generator for ADnote
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef OSCIL_GEN_H
#define OSCIL_GEN_H

#include "../globals.h"
#include <rtosc/ports.h>
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"

namespace zyn {

class NoCopyNoMove
{
public:
    NoCopyNoMove() = default;
    NoCopyNoMove(const NoCopyNoMove& other) = delete;
    NoCopyNoMove& operator=(const NoCopyNoMove& other) = delete;
    NoCopyNoMove(NoCopyNoMove&& other) = delete;
    NoCopyNoMove& operator=(NoCopyNoMove&& other) = delete;
};

//Temporary args to safely create OscilGenBuffers
struct OscilGenBuffersCreator
{
    FFTwrapper* const fft;
    const int oscilsize;
    OscilGenBuffersCreator(FFTwrapper* fft, int oscilsize) :
        fft(fft), oscilsize(oscilsize) {}
};

//All temporary variables and buffers for OscilGen computations
class OscilGenBuffers : NoCopyNoMove
{
public:
    OscilGenBuffers(OscilGenBuffersCreator creator);
    ~OscilGenBuffers();
    void defaults();

private:
    // OscilGen needs to work with this data
    // Anyone else should not touch these
    friend class OscilGen;

    const int oscilsize;

    FFTfreqBuffer oscilFFTfreqs;
    fft_t *pendingfreqs;

    //This array stores some temporary data and it has OSCIL_SIZE elements
    FFTsampleBuffer tmpsmps;
    FFTfreqBuffer outoscilFFTfreqs;
    FFTsampleBuffer cachedbasefunc;
    bool cachedbasevalid;

    FFTfreqBuffer basefuncFFTfreqs; //Base function frequencies
    FFTfreqBuffer scratchFreqs; //Yet another tmp buffer

    //Internal Data
    unsigned char oldbasefunc, oldbasepar, oldhmagtype,
                  oldwaveshapingfunction, oldwaveshaping;
    int oldfilterpars, oldsapars, oldbasefuncmodulation,
        oldbasefuncmodulationpar1, oldbasefuncmodulationpar2,
        oldbasefuncmodulationpar3, oldharmonicshift;
    int oldmodulation, oldmodulationpar1, oldmodulationpar2,
        oldmodulationpar3;

    int    oscilprepared;   //1 if the oscil is prepared, 0 if it is not prepared and is need to call ::prepare() before ::get()

    float hmag[MAX_AD_HARMONICS], hphase[MAX_AD_HARMONICS]; //the magnituides and the phases of the sine/nonsine harmonics
};

class OscilGen:public Presets, NoCopyNoMove
{
    public:
        OscilGen(const SYNTH_T &synth, FFTwrapper *fft_, Resonance *res_);

        //You need to call this func if you need your own buffers for get() etc.
        OscilGenBuffersCreator createOscilGenBuffers() const;

        /**computes the full spectrum of oscil from harmonics,phases and basefunc*/
        void prepare(OscilGenBuffers& bfrs) const;
        void prepare() { return prepare(myBuffers()); }

        void prepare(OscilGenBuffers& bfrs, FFTfreqBuffer data) const;

        /**do the antialiasing(cut off higher freqs.),apply randomness and do a IFFT*/
        //returns where should I start getting samples, used in block type randomness
        short get(OscilGenBuffers& bfrs, float *smps, float freqHz, int resonance = 0) const;
        short get(float *smps, float freqHz, int resonance = 0) {
            return get(myBuffers(), smps, freqHz, resonance);
        }
        //if freqHz is smaller than 0, return the "un-randomized" sample for UI

        void getbasefunction(OscilGenBuffers& bfrs, FFTsampleBuffer smps) const;

        //called by UI
        void getspectrum(int n, float *spc, int what); //what=0 pt. oscil,1 pt. basefunc
        void getcurrentbasefunction(FFTsampleBuffer smps);
        /**convert oscil to base function*/
        void useasbase();

        void paste(OscilGen &o);
        void add2XML(XMLwrapper& xml) override;
        void defaults();
        void getfromXML(XMLwrapper& xml);

        void convert2sine();

        //Parameters

        /**
         * The hmag and hphase starts counting from 0, so the first harmonic(1) has the index 0,
         * 2-nd harmonic has index 1, ..the 128 harminic has index 127
         */
        unsigned char Phmag[MAX_AD_HARMONICS], Phphase[MAX_AD_HARMONICS]; //the MIDI parameters for mag. and phases


        /**The Type of magnitude:
         *   0 - Linear
         *   1 - dB scale (-40)
         *   2 - dB scale (-60)
         *   3 - dB scale (-80)
         *   4 - dB scale (-100)*/
        unsigned char Phmagtype;

        unsigned char Pcurrentbasefunc; //The base function used - 0=sin, 1=...
        unsigned char Pbasefuncpar; //the parameter of the base function

        unsigned char Pbasefuncmodulation; //what modulation is applied to the basefunc
        unsigned char Pbasefuncmodulationpar1, Pbasefuncmodulationpar2,
                      Pbasefuncmodulationpar3; //the parameter of the base function modulation

        unsigned char Pwaveshaping, Pwaveshapingfunction;
        unsigned char Pfiltertype, Pfilterpar1, Pfilterpar2;
        bool          Pfilterbeforews;
        unsigned char Psatype, Psapar; //spectrum adjust

        int Pharmonicshift; //how the harmonics are shifted
        int Pharmonicshiftfirst; //if the harmonic shift is done before waveshaping and filter

        unsigned char Pmodulation; //what modulation is applied to the oscil
        unsigned char Pmodulationpar1, Pmodulationpar2, Pmodulationpar3; //the parameter of the parameters

        /**Realtime parameters for ADnote*/

        /*the Randomness:
          64=no randomness
          63..0 - block type randomness - 0 is maximum
          65..127 - each harmonic randomness - 127 is maximum*/
        unsigned char Prand;
        unsigned char Pamprandpower, Pamprandtype; //amplitude randomness
        unsigned char Padaptiveharmonics; //the adaptive harmonics status (off=0,on=1,etc..)
        unsigned char Padaptiveharmonicsbasefreq; //the base frequency of the adaptive harmonic (30..3000Hz)
        unsigned char Padaptiveharmonicspower; //the strength of the effect (0=off,100=full)
        unsigned char Padaptiveharmonicspar; //the parameters in 2,3,4.. modes of adaptive harmonics



        //makes a new random seed for Amplitude Randomness
        //this should be called every note on event
        void newrandseed(unsigned int randseed);

        bool ADvsPAD; //!< true if it is used by PADsynth instead of ADsynth

        static const rtosc::MergePorts ports;
        static const rtosc::Ports      non_realtime_ports;
        static const rtosc::Ports      realtime_ports;

        /* Oscillator Frequencies -
         *  this is different than the harmonics set-up by the user,
         *  it may contain time-domain data if the antialiasing is turned off*/

        //Access m_myBuffers. Should be avoided.
        OscilGenBuffers& myBuffers() { return m_myBuffers; }

    private:

        //This has the advantage that it is the "old", "stable" code, and that
        //prepare can be re-used. The disadvantage is that, if multiple threads
        //work on this variable in parallel, race conditions would be possible.
        //So this might vanish, soon
        OscilGenBuffers m_myBuffers;

        FFTwrapper *fft;
        //computes the basefunction and make the FFT; newbasefunc<0  = same basefunc
        void changebasefunction(OscilGenBuffers& bfrs) const;
        //Waveshaping
        void waveshape(OscilGenBuffers& bfrs, FFTfreqBuffer freqs) const;

        //Filter the oscillator accotding to Pfiltertype and Pfilterpar
        void oscilfilter(fft_t *freqs) const;

        //Adjust the spectrum
        void spectrumadjust(fft_t *freqs) const;

        //Shift the harmonics
        void shiftharmonics(fft_t *freqs) const;

        //Do the oscil modulation stuff
        void modulation(OscilGenBuffers& bfrs, FFTfreqBuffer freqs) const;

        float userfunc(OscilGenBuffers& bfrs, float x) const;

    public:
        //Check system for needed updates
        bool needPrepare(OscilGenBuffers& bfrs) const;
        bool needPrepare() { return needPrepare(myBuffers()); }
    private:

        //Do the adaptive harmonic stuff
        void adaptiveharmonic(fft_t *f, float freq) const;

        //Do the adaptive harmonic postprocessing (2n+1,2xS,2xA,etc..)
        //this function is called even for the user interface
        //this can be called for the sine and components, and for the spectrum
        //(that's why the sine and cosine components should be processed with a separate call)
        void adaptiveharmonicpostprocess(fft_t *f, int size) const;

        Resonance *res;

        unsigned int randseed;
    public:
        const SYNTH_T &synth;
};

typedef float filter_func_t(unsigned int, float, float);
filter_func_t *getFilter(unsigned char func);
typedef float base_func_t(float, float);
base_func_t *getBaseFunction(unsigned char func);

}

#endif

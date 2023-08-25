/*
  ZynAddSubFX - a software synthesizer

  FilterParams.h - Parameters for filter
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef FILTER_PARAMS_H
#define FILTER_PARAMS_H

#include "../globals.h"
#include "../Misc/XMLwrapper.h"
#include "PresetsArray.h"

namespace zyn {

class FilterParams:public PresetsArray
{
    public:
        FilterParams(const AbsTime *time_ = nullptr);
        FilterParams(unsigned char Ptype_,
                     unsigned char Pfreq,
                     unsigned char Pq_,
                     consumer_location_t loc,
                     const AbsTime *time_ = nullptr);
        FilterParams(consumer_location_t loc,
                     const AbsTime *time_ = nullptr);
        ~FilterParams() override;

        void add2XML(XMLwrapper& xml) override;
        void add2XMLsection(XMLwrapper& xml, int n) override;
        void defaults();
        void getfromXML(XMLwrapper& xml);
        void getfromXMLsection(XMLwrapper& xml, int n);
        void paste(FilterParams &);
        void pasteArray(FilterParams &, int section);

        //! assignment operator-like function
        //! @warning function has been unused since 2004
        //!     (84ddf9c0132b6be8d685f01c6444edd8bc49bb0f). If you use it, make
        //!     sure it's still up-to-date and make tests
        void getfromFilterParams(const FilterParams *pars);

        float getfreq() const ;
        float getq() const ;
        float getfreqtracking(float notefreq) const ;
        float getgain() const ;

        unsigned Pcategory:4;  //!< Filter category (Analog/Formant/StVar/Moog/Comb)
        unsigned Ptype:8;      //!< Filter type  (for analog lpf,hpf,bpf..)
        unsigned Pstages:8;    //!< filter stages+1
        float    basefreq;     //!< Base cutoff frequency (Hz)
        float    baseq;        //!< Q parameters (resonance or bandwidth)
        float    freqtracking; //!< Tracking of center frequency with note frequency (percentage)
        float    gain;         //!< filter's output gain (dB)

        static float baseqFromOldPq(int Pq);
        static float gainFromOldPgain(int Pgain);
        static float basefreqFromOldPreq(int Pfreq);

        //Formant filter parameters
        unsigned char Pnumformants; //how many formants are used
        unsigned char Pformantslowness; //how slow varies the formants
        unsigned char Pvowelclearness; //how vowels are kept clean (how much try to avoid "mixed" vowels)
        unsigned char Pcenterfreq, Poctavesfreq; //the center frequency of the res. func., and the number of octaves

        struct Pvowels_t {
            struct formants_t {
                unsigned char loc; //!< only relevant for DynFilter's default values
                unsigned char freq, amp, q; //frequency,amplitude,Q
            } formants[FF_MAX_FORMANTS];
        } Pvowels[FF_MAX_VOWELS];

        unsigned char Psequencesize; //how many vowels are in the sequence
        unsigned char Psequencestretch; //how the sequence is stretched (how the input from filter envelopes/LFOs/etc. is "stretched")
        unsigned char Psequencereversed; //if the input from filter envelopes/LFOs/etc. is reversed(negated)
        struct {
            unsigned char nvowel; //the vowel from the position
        } Psequence[FF_MAX_SEQUENCE];

        float getcenterfreq() const ;
        float getoctavesfreq() const ;
        float getfreqpos(float freq) const ;
        float getfreqx(float x) const ;

        float getformantfreq(unsigned char freq) const ;
        float getformantamp(unsigned char amp) const ;
        float getformantq(unsigned char q) const ;

        void defaults(int n); //!< set default for formant @p n

        void updateLoc(int newloc);
        int loc; //!< consumer location
        bool changed;

        const AbsTime *time;
        int64_t last_update_timestamp; // timestamp of last update to this structure,
        // including any change to the vowels/formats

        static const rtosc::Ports ports;
    private:
        // common
        void setup();
        void updateLoc(int newloc, int n);

        //stored default parameters
        unsigned char Dtype;
        unsigned char Dfreq;
        unsigned char Dq;
};

}

#endif

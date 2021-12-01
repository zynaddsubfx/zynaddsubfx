/*
  ZynAddSubFX - a software synthesizer

  ADnote.h - The "additive" synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef AD_NOTE_H
#define AD_NOTE_H

#include "SynthNote.h"
#include "Envelope.h"
#include "LFO.h"
#include "Portamento.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/Controller.h"
#include "WatchPoint.h"

//Globals

/**FM amplitude tune*/
#define FM_AMP_MULTIPLIER 14.71280603f

#define OSCIL_SMP_EXTRA_SAMPLES 5

namespace zyn {

/**The "additive" synthesizer*/
class ADnote:public SynthNote
{
    public:
        /**Constructor.
         * @param pars Note Parameters
         * @param spars Synth Engine Agnostic Parameters*/
        ADnote(ADnoteParameters *pars, const SynthParams &spars,
                WatchManager *wm=0, const char *prefix=0);
        /**Destructor*/
        ~ADnote();

        /**Alters the playing note for legato effect*/
        void legatonote(const LegatoParams &pars);

        int noteout(float *outl, float *outr);
        void releasekey();
        bool finished() const;
        void entomb(void);


        virtual SynthNote *cloneLegato(void) override;
    private:

        void setupVoice(int nvoice);
        int  setupVoiceUnison(int nvoice);
        void setupVoiceDetune(int nvoice);
        void setupVoiceMod(int nvoice, bool first_run = true);
        VecWatchPoint watch_be4_add,watch_after_add, watch_punch, watch_legato;
        /**Changes the frequency of an oscillator.
         * @param nvoice voice to run computations on
         * @param in_freq new frequency*/
        void setfreq(int nvoice, float in_freq);
        /**Set the frequency of the modulator oscillator*/
        void setfreqFM(int nvoice, float in_freq);
        /**Computes relative frequency for unison and unison's vibratto.
         * Note: Must be called before setfreq* functions.*/
        void compute_unison_freq_rap(int nvoice);
        /**Compute parameters for next tick*/
        void computecurrentparameters();
        /**Initializes All Parameters*/
        void initparameters(WatchManager *wm, const char *prefix);
        /**Deallocate/Cleanup given voice*/
        void KillVoice(int nvoice);
        /**Deallocate Note resources and voice resources*/
        void KillNote();
        /**Get the Voice's base frequency*/
        inline float getvoicebasefreq(int nvoice, float adjust_log2 = 0.0f) const;
        /**Get modulator's base frequency*/
        inline float getFMvoicebasefreq(int nvoice) const;
        /**Compute the Oscillator's samples.
         * Affects tmpwave_unison and updates oscposhi/oscposlo*/
        inline void ComputeVoiceOscillator_LinearInterpolation(int nvoice);
        /**Compute the Oscillator's samples.
         * Affects tmpwave_unison and updates oscposhi/oscposlo
         * @todo remove this declaration if it is commented out*/
        inline void ComputeVoiceOscillator_SincInterpolation(int nvoice);
        /**Compute the Oscillator's samples.
         * Affects tmpwave_unison and updates oscposhi/oscposlo
         * @todo remove this declaration if it is commented out*/
        inline void ComputeVoiceOscillator_CubicInterpolation(int nvoice);
        /**Computes the Oscillator samples with mixing.
         * updates tmpwave_unison*/
        inline void ComputeVoiceOscillatorMix(int nvoice);
        /**Computes the Ring Modulated Oscillator.*/
        inline void ComputeVoiceOscillatorRingModulation(int nvoice);
        /**Computes the Frequency Modulated Oscillator.
         * @param FMmode modulation type 0=Phase 1=Frequency*/
        inline void ComputeVoiceOscillatorFrequencyModulation(int nvoice,
                                                              FMTYPE FMmode);
        //  inline void ComputeVoiceOscillatorFrequencyModulation(int nvoice);
        /**TODO*/
        inline void ComputeVoiceOscillatorPitchModulation(int nvoice);

        /**Generate Noise Samples for Voice*/
        inline void ComputeVoiceWhiteNoise(int nvoice);
        inline void ComputeVoicePinkNoise(int nvoice);
        inline void ComputeVoiceDC(int nvoice);

        /**Fadein in a way that removes clicks but keep sound "punchy"*/
        inline void fadein(float *smps) const;

        //GLOBALS
        ADnoteParameters &pars;
        unsigned char     stereo; //if the note is stereo (allows note Panning)
        float note_log2_freq;
        float velocity;

        ONOFFTYPE   NoteEnabled;

        /*****************************************************************/
        /*                    GLOBAL PARAMETERS                          */
        /*****************************************************************/

        struct Global {
            void kill(Allocator &memory);
            void initparameters(const ADnoteGlobalParam &param,
                                const SYNTH_T &synth,
                                const AbsTime &time,
                                class Allocator &memory,
                                float basefreq, float velocity,
                                bool stereo,
                                WatchManager *wm,
                                const char *prefix);
            /******************************************
            *     FREQUENCY GLOBAL PARAMETERS        *
            ******************************************/
            float Detune;  //cents

            Envelope *FreqEnvelope;
            LFO      *FreqLfo;

            /********************************************
            *     AMPLITUDE GLOBAL PARAMETERS          *
            ********************************************/
            float Volume;  // [ 0 .. 1 ]

            float Panning;  // [ 0 .. 1 ]

            Envelope *AmpEnvelope;
            LFO      *AmpLfo;

            float Fadein_adjustment;
            struct {
                int   Enabled;
                float initialvalue, dt, t;
            } Punch;

            /******************************************
            *        FILTER GLOBAL PARAMETERS        *
            ******************************************/
            ModFilter *Filter;
            Envelope  *FilterEnvelope;
            LFO       *FilterLfo;
        } NoteGlobalPar;



        /***********************************************************/
        /*                    VOICE PARAMETERS                     */
        /***********************************************************/
        struct Voice {
            void releasekey();
            void kill(Allocator &memory, const SYNTH_T &synth);
            /* If the voice is enabled */
            ONOFFTYPE Enabled;

            /* if AntiAliasing is enabled */
            bool AAEnabled;

            /* Voice Type (sound/noise)*/
            int noisetype;

            /* Filter Bypass */
            int filterbypass;

            /* Delay (ticks) */
            int DelayTicks;

            /* Waveform of the Voice */
            float *OscilSmp;

            /* preserved for phase mod PWM emulation. */
            int phase_offset;

            /* Range of waveform */
            float OscilSmpMin, OscilSmpMax;

            /************************************
            *     FREQUENCY PARAMETERS          *
            ************************************/
            int fixedfreq; //if the frequency is fixed to 440 Hz
            int fixedfreqET; //if the "fixed" frequency varies according to the note (ET)

            // cents = basefreq*VoiceDetune
            float Detune, FineDetune;

            // Bend adjustment
            float BendAdjust;

            float OffsetHz;

            Envelope *FreqEnvelope;
            LFO      *FreqLfo;


            /***************************
            *   AMPLITUDE PARAMETERS   *
            ***************************/

            /* Panning 0.0f=left, 0.5f - center, 1.0f = right */
            float Panning;
            float Volume;  // [-1.0f .. 1.0f]

            Envelope *AmpEnvelope;
            LFO      *AmpLfo;

            /*************************
            *   FILTER PARAMETERS    *
            *************************/
            ModFilter *Filter;
            Envelope  *FilterEnvelope;
            LFO       *FilterLfo;


            /****************************
            *   MODULLATOR PARAMETERS   *
            ****************************/

            FMTYPE FMEnabled;

            unsigned char FMFreqFixed;

            int FMVoice;

            // Voice Output used by other voices if use this as modullator
            float *VoiceOut;

            /* Wave of the Voice */
            float *FMSmp;

            smooth_float FMVolume;
            float FMDetune;  //in cents

            Envelope *FMFreqEnvelope;
            Envelope *FMAmpEnvelope;

            /********************************************************/
            /*    INTERNAL VALUES OF THE NOTE AND OF THE VOICES     */
            /********************************************************/

            //pinking filter (Paul Kellet)
            float pinking[14];

            //the size of unison for a single voice
            int unison_size;

            //the stereo spread of the unison subvoices (0.0f=mono,1.0f=max)
            float unison_stereo_spread;

            //fractional part (skip)
            float *oscposlo, *oscfreqlo;

            //integer part (skip)
            int *oscposhi, *oscfreqhi;

            //fractional part (skip) of the Modullator
            float *oscposloFM, *oscfreqloFM;

            //the unison base_value
            float *unison_base_freq_rap;

            //how the unison subvoice's frequency is changed (1.0f for no change)
            float *unison_freq_rap;

            //which subvoice has phase inverted
            bool *unison_invert_phase;

            //unison vibratto
            struct {
                float  amplitude; //amplitude which be added to unison_freq_rap
                float *step; //value which increments the position
                float *position; //between -1.0f and 1.0f
            } unison_vibratto;

            //integer part (skip) of the Modullator
            unsigned int *oscposhiFM, *oscfreqhiFM;

            //used to compute and interpolate the amplitudes of voices and modullators
            float oldamplitude, newamplitude,
                  FMoldamplitude, FMnewamplitude;

            //used by Frequency Modulation (for integration)
            float *FMoldsmp;

            //1 - if it is the fitst tick (used to fade in the sound)
            char firsttick;

        } NoteVoicePar[NUM_VOICES];

        //temporary buffer
        float  *tmpwavel;
        float  *tmpwaver;
        int     max_unison;
        float **tmpwave_unison;

        //Filter bypass samples
        float *bypassl, *bypassr;

        //interpolate the amplitudes
        float globaloldamplitude, globalnewamplitude;

        //Pointer to portamento if note has portamento
        Portamento *portamento;

        //how the fine detunes are made bigger or smaller
        float bandwidthDetuneMultiplier;
};

}

#endif

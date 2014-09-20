/*
  ZynAddSubFX - a software synthesizer

  Note.h - Abstract Base Class for synthesizers
  Copyright (C) 2010-2010 Mark McCurry
  Author: Mark McCurry

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
#ifndef SYNTH_NOTE_H
#define SYNTH_NOTE_H
#include "../globals.h"
#include "../Params/FilterParams.h"

class Allocator;
class Controller;
class Envelope;
struct SynthParams
{
    Allocator &memory;   //Memory Allocator for the Note to use
    Controller &ctl;
    float     frequency; //Note base frequency
    float     velocity;  //Velocity of the Note
    bool      portamento;//True if portamento is used for this note
    int       note;      //Integer value of the note
    bool      quiet;     //Initial output condition for legato notes
};

struct LegatoParams
{
    float frequency;
    float velocity;
    bool portamento;
    int midinote;
    bool externcall;
};
struct PunchState {
    void init(char Strength, char VelocitySensing,
            char Time, char Stretch, float velocity, float freq);
    bool   Enabled;
    float initialvalue, dt, t;
}; 

class SynthNote
{
    public:
        SynthNote(SynthParams &pars);
        virtual ~SynthNote() {}

        /**Compute Output Samples
         * @return 0 if note is finished*/
        virtual int noteout(float *outl, float *outr) = 0;

        //TODO fix this spelling error [noisey commit]
        /**Release the key for the note and start release portion of envelopes.*/
        virtual void relasekey() = 0;

        /**Return if note is finished.*/
        virtual bool finished() const = 0;

        virtual void legatonote(LegatoParams pars) = 0;
        /* For polyphonic aftertouch needed */
        void setVelocity(float velocity_);

    protected:
        /**Fadein in a way that removes clicks but keep sound "punchy"*/
        void fadein(float *smps) const;
        void fadeinCheap(float *smpl, float *smpr) const;
        void fadeout(float *smps) const;
        void applyPunch(float *outl, float *outr, PunchState &p);
        void applyPanning(float *outl, float *outr, float oldamp, float newamp,
                          float panl, float panr);
        void applyAmp(float *outl, float *outr, float oldamp, float newamp);
        float applyFixedFreqET(bool fixedfreq, float basefreq, int fixedfreqET, float detune, int midinote) const;
        float getPortamento(bool &portamento) const;

        // Legato transitions
        class Legato
        {
            public:
                Legato(float freq, float vel, int port,
                       int note, bool quiet);

                void apply(SynthNote &note, float *outl, float *outr);
                int update(LegatoParams pars);

            private:
                bool      silent;
                float     lastfreq;
                LegatoMsg msg;
                int       decounter;
                struct { // Fade In/Out vars
                    int   length;
                    float m, step;
                } fade;
                struct { // Note parameters
                    float freq, vel;
                    bool  portamento;
                    int   midinote;
                } param;

            public: /* Some get routines for legatonote calls (aftertouch feature)*/
                float getFreq() {return param.freq; }
                float getVelocity() {return param.vel; }
                bool  getPortamento() {return param.portamento; }
                int getMidinote() {return param.midinote; }
                void setSilent(bool silent_) {silent = silent_; }
                void setDecounter(int decounter_) {decounter = decounter_; }
        } legato;

        /************************************
         * Global Parameters Common To Each *
         ************************************/
            
        /******************************************
         *     FREQUENCY GLOBAL PARAMETERS        *
         ******************************************/
        float Detune;   //cents
        float basefreq;
        Envelope *FreqEnvelope;


        /********************************************
         *     AMPLITUDE GLOBAL PARAMETERS          *
         ********************************************/
        float volume;   // [ 0 .. 1 ]
        float panning;  // [ 0 .. 1 ]
            
        Envelope *AmpEnvelope;
            
        /******************************************
         *        FILTER GLOBAL PARAMETERS        *
         ******************************************/
        class Filter * GlobalFilterL, *GlobalFilterR;

        Envelope *FilterEnvelope;
        float FilterCenterPitch;  //octaves
        float FilterQ;
        float FilterFreqTracking;


        /******************************************
         *        OTHER  GLOBAL PARAMETERS        *
         ******************************************/
        float velocity;
        int midinote;
        float oldamplitude, newamplitude;
        bool  firsttick, portamento;
        bool  NoteEnabled;


        //Realtime Safe Memory Allocator For notes
        class Allocator &memory;
        const Controller &ctl;
};

#endif

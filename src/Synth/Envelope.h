/*
  ZynAddSubFX - a software synthesizer

  Envelope.h - Envelope implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "../globals.h"
#include "WatchPoint.h"

namespace zyn {

/**Implementation of a general Envelope*/
class Envelope
{
    public:
        /**Constructor*/
        Envelope(class EnvelopeParams &pars, float basefreq, float dt, WatchManager *m=0,
                const char *watch_prefix=0);
        /**Destructor*/
        ~Envelope(void);
        void releasekey(void);
        /**Push Envelope to finishing state*/
        void forceFinish(void);
        float envout(bool doWatch=true);
        float envout_dB(void);
        /**Determines the status of the Envelope
         * @return returns 1 if the envelope is finished*/
        bool finished(void) const;
        void watch(float time, float value);

    private:
        int   envpoints;     // number of envelope points
        int   envsustain;    // "-1" means disabled
        float envdt[MAX_ENVELOPE_POINTS];  //seconds
        float envval[MAX_ENVELOPE_POINTS]; // [0.0f .. 1.0f]
        float envcpy[MAX_ENVELOPE_CPOINTS];// [-1.0f .. 1.0f]
        float envstretch;    // how much lower notes are slower
        int   linearenvelope;
        int   mode;
        bool  repeating;

        int   currentpoint = 1;     //current envelope point (starts from 1)
        bool  forcedrelease;        // true if key release should jump to release segment
        bool  keyreleased = false;  // true if the key was released
        bool  envfinish = false;    // true if the end of release phase is reached
        float t = 0.0f;             // the time from the last point
        float tRelease = 0.0f;      // release duration
        float inct;                 // the time increment
        float envoutval = 0.0f;     // used to do the forced release

        VecWatchPoint watchOut;
};

}

#endif

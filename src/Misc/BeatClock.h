/*
  ZynAddSubFX - a software synthesizer

  Master.h - It sends Midi Messages to Parts, receives samples from parts,
             process them with system/insertion effects and mix them
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef BEATCLOCK_H
#define BEATCLOCK_H

#include "../globals.h"
#include "Time.h"


#pragma once

namespace zyn {

       enum beatclock_state_enum {
        bc_syncing = 1,
        bc_running = 2,
        bc_stopped = 3
    };

/**Beatclock with speed and phase estimator*/
class BeatClock
{
    public:
    BeatClock(const zyn::SYNTH_T& synth, AbsTime &time_);
    ~BeatClock();

    //~ void sppSync(unsigned long nanos);
    void sppPrepareSync(int beats);
    void sppSync(unsigned long nanos);

    void tick(unsigned long nanos);
    float getPhase();


    private:
    //~ float bpm;
    //~ float phase;

    //~ int songPosition;


    const SYNTH_T &synth;

    AbsTime &time;

    beatclock_state_enum state;//, oldState;

    // for bpm tracking
    int midiClockCounter, counterSppSync, newCounter, beatsSppSync; // clock tick counter
    long nanos, nanosLastTick, nanosSppSync; // number of sample the clock tick belongs to and its history value
    double dTime; // time between current and last clock tick
    double dtFiltered; // lp filtered dTime
    float outliersDt, outliersPhase; // outlier counter to speedup tempo change
    float phaseFiltered; // lp filtered phase offset of incoming clock pulses
    long bpm, oldbpm, newbpm, phase; // result
    long referenceTime;
    long tstamp;


};

}

#endif

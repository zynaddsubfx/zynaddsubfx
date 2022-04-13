#include "BeatClock.h"

namespace zyn {

BeatClock::BeatClock(const SYNTH_T &synth_, AbsTime &time_):
synth(synth_),
time(time_)
{
    dtFiltered = 0.0f;
    phaseFiltered = 0.0f;
    outliersDt = 0;
}

void BeatClock::sppPrepareSync(int beats)
{
    oldState = state;
    state = bc_syncing;
    songPosition = beats;
}

void BeatClock::sppSync(unsigned long nanos)
{
    if(state==bc_syncing) {
        time.tRef = nanos - ((float)beatsSppSync/4.0f) / ((float)bpm / 60.0f)*1000000000;
        printf("down beat: nanos = %lu  beatsSppSync = %d   bpm = %lu\n", nanos, beatsSppSync, bpm);
        printf("   nanosLastTick = %lu\n ", nanosLastTick);
        printf("  nanosSppSync  = %lu\n ", nanosSppSync);
        printf("  time.tRef = %lu\n ", time.tRef);
        printf("time.tStamp = %lu\n", time.tStamp);
        state = oldState;
    }

}

void BeatClock::tick(unsigned long nanos)
{

    if(bc_syncing)
        sppSync(nanos);

    ++midiClockCounter;

    dTime = (double)(nanos - nanosLastTick);
    // lp filter dTime
    double gain = 10.0 + (14.0 * (40000.0/dTime));
    if(fabs((dTime)-dtFiltered) < dtFiltered*0.2) {
        dtFiltered *= (gain-1.0)/gain; //
        dtFiltered += dTime/gain;
        if (outliersDt>=0.1f) outliersDt -= 0.1f;
    } else {
        //leave out outliers but reset filter if too much (->tempo change)
        if (++outliersDt > 2.0f)
        {
            dtFiltered = dTime;
            outliersDt = 0.0f;
        }
    }
    // round to integer bpm
    bpm = (int) round(2500000000.0/dtFiltered);

    // clalculate theroretical tick interval
    float tickInterval = 60.0f  * synth.samplerate / (bpm * 24.0f);
    // theoretical number of ticks until now
    float ticks = (nanos - referenceTime) / tickInterval;
    // phase = samples since theoretical last tick position
    float phaseRaw = nanos - (floor(ticks)*tickInterval);

    // lp filter phase
    if((fabs(phaseRaw-phase) < phaseFiltered*0.1f) && !std::isnan(phaseFiltered)) {
        phaseFiltered *= 47.0f/48.0f;
        phaseFiltered += phaseRaw/48.0f;
        outliersPhase -= 0.01f;
    } else {
        if (++outliersPhase > 2.0f)
        {
            phaseFiltered = phaseRaw;
            outliersPhase = 0.0f;
        }
    }
    phase = (int) roundf(phaseFiltered);
    nanosLastTick = nanos;

    if(oldbpm != bpm) {
        if(!newCounter)
            newbpm = bpm;
        if(++newCounter > 24) {
            oldbpm = bpm;
            newCounter = 0;
            time.bpm = bpm;
        }
    }

    if(midiClockCounter%24==0) { // 24 clock ticks = 1 quarter note length
        //~ printf("nanos = %lu \n", nanos);
        //~ printf("dTime = %f \n", dTime);
        //~ printf("gain = %f -> ", gain);

        //~ printf("dtFiltered = %f   dTime-dtFiltered = %f (outliers = %f)\n", dtFiltered, fabs(((float)dTime)-dtFiltered), outliersDt);
        //~ printf("tickInterval = %f -> ", tickInterval);
        //~ printf("(ticks) = %f   phaseRaw = %f   phaseFiltered = %f\n", floor(ticks), phaseRaw, phaseFiltered);
        //~ printf("bpm = %lu  phase = %lu\n", bpm, phase);
        printf("bpmRaw = %f \n", 2506000000.0/dtFiltered);
    }
}

}

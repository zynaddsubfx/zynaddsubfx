#include "../Misc/Allocator.h"
#include "../globals.h"
#include "../DSP/Value_Smoothing_Filter.h"

#pragma once


namespace zyn {

/**Comb Filter Bank for sympathetic Resonance*/
class CombFilterBank
{
    public:
    CombFilterBank(Allocator *alloc, unsigned int samplerate_, int buffersize_, float initgain);
    ~CombFilterBank();
    void filterout(float *smp);

    float delays[NUM_SYMPATHETIC_STRINGS]={};
    float inputgain;
    float outgain;
    float gainbwd;

    void setStrings(unsigned int nr, const float basefreq);

    /** Contact proximity to the string.
     *  0.0 = finger touching the string (maximum sensitivity, lower threshold)
     *  1.0 = no contact (minimum sensitivity, higher threshold)
     *  Controls how easily contact events are triggered. */
    float contactOffset = 1.0f;

    /** Contact material hardness / energy transfer.
     *  0.0 = soft material (no energy reflected back into string)
     *  1.0 = hard material (maximum energy reflected back)
     *  Controls how much of the detected contact excitation is fed back. */
    float contactStrength = 0.0f;

    /** Contact position along the string as fraction of delay (0..0.5) */
    float contactPosition = 0.25f;

    private:
    static float tanhX(const float x);
    float sampleLerp(const float *smp, const float pos) const;

    float* string_smps[NUM_SYMPATHETIC_STRINGS] = {};
    float baseFreq;
    unsigned int nrOfStrings=0;
    unsigned int pos_writer = 0;

    /* for smoothing gain jump when using binary valued sustain pedal */
    Value_Smoothing_Filter gain_smoothing;

    Allocator &memory;
    unsigned int mem_size=0;
    int samplerate=0;
    unsigned int buffersize=0;

    float hp_state[NUM_SYMPATHETIC_STRINGS] = {};
    float env[NUM_SYMPATHETIC_STRINGS] = {};
    float contactResponse[NUM_SYMPATHETIC_STRINGS] = {};
};

}
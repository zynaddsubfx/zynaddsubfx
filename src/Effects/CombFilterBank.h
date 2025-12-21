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
    void updateOffsets(float maxDrop);

    /** Individual delay times for each comb filter in seconds */
    float delays[NUM_SYMPATHETIC_STRINGS]={};

    /** Gain applied to the input signal before processing */
    float inputgain;

    /** Overall output gain applied after filter bank processing */
    float outgain;

    /** Feedback gain coefficient for the comb filter recursion */
    float gainbwd;

    void setStrings(unsigned int nr, const float basefreq);
    void setStrings(unsigned int nrOfStringsNew, unsigned int mem_size_new);

    /** Maximum drop amount for pitch modulation effects in octaves */
    float maxDrop = 2.0f;

    /** Rate at which pitch drops occur for modulation effects */
    float dropRate = 0.0f;

    /** Time constant for fade-in/fade-out effects in % default: 5/127 */
    float fadingTime = 0.03937007874f;

    /** Global pitch offset applied to all strings in semitones */
    float pitchOffset = 0.0f;

    /** Distance of contact 0 = full conctact, 1= no contact */
    float contactOffset = 1.0f;
    float contactStrength = 0.0f;

    private:
    static float tanhX(const float x);
    float sampleLerp(const float *smp, const float pos) const;

    /** Array of delay line buffers for each comb filter*/
    float* comb_smps[NUM_SYMPATHETIC_STRINGS] = {};

    /** Base frequency reference for tuning calculations in Hz */
    float baseFreq=110.0f;

    /** Current number of active sympathetic strings */
    unsigned int nrOfStrings=0;

    /** Write position pointer for circular buffer operation */
    unsigned int pos_writer = 0;

    /** Smoothing filter to prevent gain discontinuities when sustain pedal changes */
    Value_Smoothing_Filter gain_smoothing;

    /** Smoothing filter for gradual pitch offset changes */
    Value_Smoothing_Filter offset_smoothing;

    /** Memory allocator reference for dynamic buffer allocation */
    Allocator &memory;

    unsigned int mem_size=0;
    int samplerate=0;
    unsigned int buffersize=0;

    /** Sample counter for timing-based delay modulation */
    float sampleCounter = 0.0f;
};

}

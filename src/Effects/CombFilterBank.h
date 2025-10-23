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

    float delays[NUM_SYMPATHETIC_STRINGS]={};
    float inputgain;
    float outgain;
    float gainbwd;

    void setStrings(unsigned int nr, const float basefreq);
    void setStrings(unsigned int nrOfStringsNew, unsigned int mem_size_new);

    float maxDrop = 2.0f;
    float dropRate = 0.0f;
    float fadingTime = 0.03937007874f;
    float pitchOffset = 0.0f;

    private:
    static float tanhX(const float x);
    float sampleLerp(const float *smp, const float pos) const;

    float* comb_smps[NUM_SYMPATHETIC_STRINGS] = {};
    float baseFreq=110.0f;
    unsigned int nrOfStrings=0;
    unsigned int pos_writer = 0;

    /* for smoothing gain jump when using binary valued sustain pedal */
    Value_Smoothing_Filter gain_smoothing;
    Value_Smoothing_Filter offset_smoothing;

    Allocator &memory;
    unsigned int mem_size=0;
    int samplerate=0;
    unsigned int buffersize=0;
    float sampleCounter = 0.0f;


};

}

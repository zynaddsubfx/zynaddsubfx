#include <cmath>
#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "CombFilterBank.h"

namespace zyn {

    CombFilterBank::CombFilterBank(Allocator *alloc, unsigned int samplerate_, int buffersize_, float initgain):
    inputgain(1.0f),
    outgain(1.0f),
    gainbwd(initgain),
    baseFreq(110.0f),
    nrOfStrings(0),
    memory(*alloc),
    samplerate(samplerate_),
    buffersize(buffersize_)
    {
        // setup the smoother for gain parameter
        gain_smoothing.sample_rate(samplerate/16);
        gain_smoothing.thresh(0.02f); // TBD: 2% jump audible?
        gain_smoothing.cutoff(1.0f);
        gain_smoothing.reset(gainbwd);
        pos_writer = 0;
    }

    CombFilterBank::~CombFilterBank()
    {
        setStrings(0, baseFreq);
    }

    void CombFilterBank::setStrings(unsigned int nrOfStringsNew, const float baseFreqNew)
    {
        // limit nrOfStringsNew
        nrOfStringsNew = min(NUM_SYMPATHETIC_STRINGS,nrOfStringsNew);

        if(nrOfStringsNew == nrOfStrings && baseFreqNew == baseFreq)
            return;

        const unsigned int mem_size_new = (int)ceilf(( (float)samplerate/baseFreqNew*1.03f + buffersize + 2)/16) * 16;
        if(mem_size_new == mem_size)
        {
            if(nrOfStringsNew>nrOfStrings)
            {
                for(unsigned int i = nrOfStrings; i < nrOfStringsNew; ++i)
                {
                    string_smps[i] = memory.valloc<float>(mem_size);
                    memset(string_smps[i], 0, mem_size*sizeof(float));
                }
            }
            else if(nrOfStringsNew<nrOfStrings)
                for(unsigned int i = nrOfStringsNew; i < nrOfStrings; ++i)
                    memory.devalloc(string_smps[i]);
        } else
        {
            // free the old buffers (wrong size for baseFreqNew)
            for(unsigned int i = 0; i < nrOfStrings; ++i)
                memory.devalloc(string_smps[i]);

            // allocate buffers with new size
            for(unsigned int i = 0; i < nrOfStringsNew; ++i)
            {
                string_smps[i] = memory.valloc<float>(mem_size_new);
                memset(string_smps[i], 0, mem_size_new*sizeof(float));
            }
            // update mem_size and baseFreq
            mem_size = mem_size_new;
            baseFreq = baseFreqNew;
            // reset writer position
            pos_writer = 0;
        }
        // update nrOfStrings
        nrOfStrings = nrOfStringsNew;
    }

    inline float CombFilterBank::tanhX(const float x)
    {
        // Pade approximation of tanh(x) bound to [-1 .. +1]
        // https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
        const float x2 = x*x;
        return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2));
    }

    inline float CombFilterBank::sampleLerp(const float *smp, const float pos) const {
        int poshi = (int)pos; // integer part (pos >= 0)
        float poslo = pos - (float) poshi; // decimal part
        // linear interpolation between samples
        return smp[poshi] + poslo * (smp[(poshi+1)%mem_size]-smp[poshi]);
    }

    void CombFilterBank::filterout(float *smp)
    {
        // no string -> no sound
        if (nrOfStrings==0) return;

        // interpolate gainbuf values over buffer length using value smoothing filter (lp)
        // this should prevent popping noise when controlled binary with 0 / 127
        // new control rate = samplerate / 16
        const unsigned int gainbufsize = buffersize / 16;
        float gainbuf[gainbufsize]; // buffer for value smoothing filter
        if (!gain_smoothing.apply( gainbuf, gainbufsize, gainbwd ) ) // interpolate the gain value
            std::fill(gainbuf, gainbuf+gainbufsize, gainbwd); // if nothing to interpolate (constant value)

        for (unsigned int i = 0; i < buffersize; ++i)
        {
            // apply input gain
            const float input_smp = smp[i]*inputgain;

            for (unsigned int j = 0; j < nrOfStrings; ++j)
            {
                if (delays[j] == 0.0f) continue;
                assert(float(mem_size)>delays[j]);
                // calculate the feedback sample positions in the buffer
                const float pos_reader = fmodf(float(pos_writer+mem_size) - delays[j], float(mem_size));

                // sample at that position
                const float sample = sampleLerp(string_smps[j], pos_reader);
                string_smps[j][pos_writer] = input_smp + tanhX(sample*gainbuf[i/16]);
            }
            // mix output buffer samples to output sample
            smp[i]=0.0f;
            unsigned int nrOfActualStrings = 0;
            for (unsigned int j = 0; j < nrOfStrings; ++j)
                if (delays[j] != 0.0f) {
                    smp[i] += string_smps[j][pos_writer];
                    nrOfActualStrings++;
                }

            // apply output gain to sum of strings and
            // divide by nrOfStrings to get mean value
            // division by zero is catched at the beginning filterOut()
            smp[i] *= outgain / (float)nrOfActualStrings;

            // increment writing position
            ++pos_writer %= mem_size;
        }
    }
}

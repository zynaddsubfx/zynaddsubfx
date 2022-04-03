#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>

#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "CombFilterBank.h"

namespace zyn {

    CombFilterBank::CombFilterBank(Allocator *alloc, unsigned int samplerate_, int buffersize_):
    inputgain(1.0f),
    outgain(1.0f),
    baseFreq(110.0f),
    nrOfStrings(0),
    memory(*alloc),
    samplerate(samplerate_),
    buffersize(buffersize_)
    {
        // setup the smoother for gain parameter
        gain_smoothing.sample_rate(samplerate>>4);
        gain_smoothing.thresh(0.02f); // TBD: 2% jump audible?
        gain_smoothing.cutoff(1.0f);
    }

    CombFilterBank::~CombFilterBank()
    {
        setStrings(0, baseFreq);
    }

    void CombFilterBank::setStrings(unsigned int nrOfStringsNew, float baseFreqNew)
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
                    output[i] = memory.valloc<float>(mem_size);
                    memset(output[i], 0, mem_size*sizeof(float));
                }
            }
            else if(nrOfStringsNew<nrOfStrings)
                for(unsigned int i = nrOfStringsNew; i < nrOfStrings; ++i)
                    memory.devalloc(output[i]);
        } else
        {
            // free the old buffers (wrong size for baseFreqNew)
            for(unsigned int i = 0; i < nrOfStrings; ++i)
                memory.devalloc(output[i]);

            // allocate buffers with new size
            for(unsigned int i = 0; i < nrOfStringsNew; ++i)
            {
                output[i] = memory.valloc<float>(mem_size_new);
                memset(output[i], 0, mem_size_new*sizeof(float));
            }
            // update mem_size and baseFreq
            mem_size = mem_size_new;
            baseFreq = baseFreqNew;
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

    inline float CombFilterBank::sampleLerp(float *smp, float pos) {
        int poshi = (int)pos; // integer part (pos >= 0)
        float poslo = pos - (float) poshi; // decimal part
        // linear interpolation between samples
        return smp[poshi] + poslo * (smp[poshi+1]-smp[poshi]);
    }

    void CombFilterBank::filterout(float *smp)
    {
        float gainbuf[buffersize>>4];
        if (!gain_smoothing.apply( gainbuf, buffersize>>4, gainbwd ) )
            for (unsigned int i = 0; i < buffersize>>4; i ++) gainbuf[i] = gainbwd;

        for (unsigned int i = 0; i < buffersize; ++i)
        {
            for (unsigned int j = 0; j < nrOfStrings; ++j)
            {
                // calculate the feedback sample positions in the buffer
                const float pos = float(mem_size-buffersize+i) - delays[j];
                // sample at that position
                //~ if (pos < 0.0f || pos >= mem_size) printf("pos: %f\n", pos);
                const float sample = sampleLerp(output[j], pos);
                output[j][mem_size-buffersize+i] = smp[i]*inputgain
                    + tanhX(sample*gainbuf[i>>4]);
            }
            // mix output buffer samples to output sample
            smp[i]=0.0f;
            for (unsigned int j = 0; j < nrOfStrings; ++j)
                smp[i] += output[j][mem_size-buffersize+i];

            // apply output gain but
            smp[i] *= outgain / (nrOfStrings!=0 ? (float)nrOfStrings : 1.0f);
        }
        // shift the buffer content one buffersize to the left
        for(unsigned int j = 0; j < nrOfStrings; ++j)
            memmove(&output[j][0], &output[j][buffersize], (mem_size-buffersize)*sizeof(float));
    }
}

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
        gain_smoothing.cutoff(1.0f);
        gain_smoothing.sample_rate(samplerate/16);
        gain_smoothing.thresh(0.02f);
        gain_smoothing.reset(gainbwd);
        pos_writer = 0;
        for (unsigned int j=0; j<NUM_SYMPATHETIC_STRINGS; j++) env[j] = 0.1f;
    }

    CombFilterBank::~CombFilterBank()
    {
        setStrings(0, baseFreq);
    }

    void CombFilterBank::setStrings(unsigned int nrOfStringsNew, const float baseFreqNew)
    {
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
            for(unsigned int i = 0; i < nrOfStrings; ++i)
                memory.devalloc(string_smps[i]);

            for(unsigned int i = 0; i < nrOfStringsNew; ++i)
            {
                string_smps[i] = memory.valloc<float>(mem_size_new);
                memset(string_smps[i], 0, mem_size_new*sizeof(float));
            }
            mem_size = mem_size_new;
            baseFreq = baseFreqNew;
            pos_writer = 0;
        }
        nrOfStrings = nrOfStringsNew;
    }

    inline float CombFilterBank::tanhX(const float x)
    {
        const float x2 = x*x;
        return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2));
    }

    inline float CombFilterBank::sampleLerp(const float *smp, const float pos) const {
        int poshi = (int)pos;
        float poslo = pos - (float) poshi;
        if (poslo > 0.001)
            return smp[poshi] + poslo * (smp[(poshi+1)%mem_size]-smp[poshi]);
        else
            return smp[poshi];
    }

    void CombFilterBank::filterout(float *smp)
    {
        if (nrOfStrings==0) return;

        const unsigned int gainbufsize = buffersize / 16;
        STACKALLOC(float, gainbuf, gainbufsize);
        if (!gain_smoothing.apply( gainbuf, gainbufsize, gainbwd ) )
            std::fill(gainbuf, gainbuf+gainbufsize, gainbwd);

        for (unsigned int i = 0; i < buffersize; ++i)
        {
            const float input_smp = smp[i]*inputgain;

            for (unsigned int j = 0; j < nrOfStrings; ++j)
            {
                if (delays[j] == 0.0f) continue;

                float sMain = 0.0f;
                float sLeft = 0.0f;
                float sRight = 0.0f;

                const float baseDelay = delays[j];
                const float contactPos = contactPosition;

                auto readTaps = [&](float d, float weight) {
                    if (weight <= 0.0f) return;

                    float dm = min(d, float(mem_size));
                    sMain += sampleLerp(string_smps[j], fmodf(float(pos_writer + mem_size) - dm, float(mem_size))) * weight;

                    float dl = min(dm * contactPos, float(mem_size));
                    float dr = min(dm * (1.0f - contactPos), float(mem_size));
                    sLeft  += sampleLerp(string_smps[j], (pos_writer + mem_size - int(dl)) % mem_size) * weight;
                    sRight += sampleLerp(string_smps[j], (pos_writer + mem_size - int(dr)) % mem_size) * weight;
                };

                readTaps(baseDelay, 1.0f);

                const float damp_range = 0.5f;
                float wr = (1.0f - damp_range) + damp_range * contactPos;
                float wl = (1.0f - damp_range) + damp_range * (1.0f - contactPos);

                float contactIn = (wl * sLeft + wr * sRight) / (wr + wl);

                contactIn -= hp_state[j];
                hp_state[j] += 0.001f * contactIn;

                env[j] += 0.0001f * (fabsf(sMain) - env[j]);
                float thresh = 3.5f * contactOffset * env[j];
                float excess = fmaxf(0.0f, contactIn - thresh);

                float approximity = 1.0f - contactOffset;
                contactResponse[j] *= 0.5f;
                if (excess > 0.0f) {
                    float drive = 4.0f + 16.0f * approximity;
                    float n = 4.0f;
                    float shapedExcess = excess * drive / powf(1.0f + powf(fabsf(excess * drive), n), 1.0f / n);
                    contactResponse[j] += 0.5f * shapedExcess;
                }

                const float hockeyFactor = powf(approximity, 16);
                const float w_cont = contactStrength * (contactResponse[j] + hockeyFactor * (1.0f - contactResponse[j]));
                const float delta = contactIn - sMain;

                const float feedback = tanhX((sMain + w_cont * delta) * gainbuf[i/16]);
                string_smps[j][pos_writer] = input_smp + feedback;
            }

            smp[i]=0.0f;
            unsigned int nrOfActualStrings = 0;
            for (unsigned int j = 0; j < nrOfStrings; ++j)
                if (delays[j] != 0.0f) {
                    smp[i] += string_smps[j][pos_writer];
                    nrOfActualStrings++;
                }

            smp[i] *= outgain / (float)nrOfActualStrings;

            ++pos_writer %= mem_size;
        }
    }
}
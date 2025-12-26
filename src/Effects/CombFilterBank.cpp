#include <cmath>
#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "CombFilterBank.h"

//maxDrop = (float)(PmaxDrop+1)/32.0f
#define maxDrop_min 0.03125f // 1/32

namespace zyn {

    struct FloatTuple {
        float A;  // Gain or Phase for read head A
        float B;  // Gain or Phase for read head B
    };

    CombFilterBank::CombFilterBank(Allocator *alloc, unsigned int samplerate_, int buffersize_, float initgain):
    inputgain(1.0f),
    outgain(1.0f),
    gainbwd(initgain),
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
        // setup the smoother for offset parameter
        offset_smoothing.sample_rate(samplerate);
        offset_smoothing.thresh(0.02f); // TBD: 2% jump audible?
        offset_smoothing.cutoff(0.2f);
        offset_smoothing.reset(0.0f);
        pos_writer = 0;
        for (unsigned int j=0; j<NUM_SYMPATHETIC_STRINGS; j++) env[j] = 0.1f;
    }

    CombFilterBank::~CombFilterBank()
    {
        setStrings(0, baseFreq);
    }

    void CombFilterBank::setStrings(unsigned int nrOfStringsNew, const float baseFreqNew)
    {
        // limit nrOfStringsNew
        nrOfStringsNew = min(NUM_SYMPATHETIC_STRINGS,nrOfStringsNew);

        if((nrOfStringsNew == nrOfStrings && baseFreqNew == baseFreq) || baseFreqNew < 0.85f)
            return;

        baseFreq = baseFreqNew;
        const unsigned int mem_size_new = (int)ceilf(( (float)samplerate/baseFreqNew*1.03f + buffersize + 2)/16) * 16;
        setStrings(nrOfStringsNew, mem_size_new);
    }

    void CombFilterBank::setStrings(unsigned int nrOfStringsNew, unsigned int mem_size_new)
    {

        if(mem_size_new == mem_size)
        {
            if(nrOfStringsNew>nrOfStrings)
            {
                // allocate more combs of the same mem_size
                for(unsigned int i = nrOfStrings; i < nrOfStringsNew; ++i)
                {
                    comb_smps[i] = memory.valloc<float>(mem_size);
                    memset(comb_smps[i], 0, mem_size*sizeof(float));
                }
            }
            else if(nrOfStringsNew<nrOfStrings) // free some combs
                for(unsigned int i = nrOfStringsNew; i < nrOfStrings; ++i)
                    memory.devalloc(comb_smps[i]);
        } else // different mem_size
        {
            // free the old combs (wrong size for baseFreqNew)
            for(unsigned int i = 0; i < nrOfStrings; ++i)
                memory.devalloc(comb_smps[i]);

            // allocate buffers with new size
            for(unsigned int i = 0; i < nrOfStringsNew; ++i)
            {
                comb_smps[i] = memory.valloc<float>(mem_size_new);
                memset(comb_smps[i], 0, mem_size_new*sizeof(float));
            }
            // update mem_size and baseFreq
            mem_size = mem_size_new;
            // reset writer position
            pos_writer = 0;
        }
        // update nrOfStrings
        nrOfStrings = nrOfStringsNew;
    }

    inline float CombFilterBank::sampleLerp(const float *smp, const float pos) const {
        int poshi = (int)pos; // integer part (pos >= 0)
        float poslo = pos - (float) poshi; // decimal part
        // linear interpolation between samples
        if (poslo > 0.001)
            return smp[poshi] + poslo * (smp[(poshi+1)%mem_size]-smp[poshi]);
        else
            return smp[poshi];
    }

    inline float CombFilterBank::tanhX(const float x)
    {
        // Pade approximation of tanh(x) bound to [-1 .. +1]
        // https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
        const float x2 = x*x;
        return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2));
    }


    /**
     * Generates two 180° phase shifted rising sawtooth signals
     * at positive dropRate: 0 ... +maxDrop
     * at negative dropRate: 0 ... -maxDrop
     */
    inline FloatTuple generatePhasedSawtooth(float& masterCounter, float dropRate, float maxDrop) {
        FloatTuple result;

        // increase(decrease) master
        masterCounter += dropRate;

        if (dropRate >= 0) {
            // Positive direction: 0 ... +maxDrop
            if (masterCounter >= maxDrop) {
                masterCounter -= floorf(masterCounter);
            }

            result.A = masterCounter;
            result.B = masterCounter + (maxDrop * 0.5f);
            if (result.B >= maxDrop) {
                result.B -= floorf(result.B);
            }

        } else {
            // Negative direction: 0 ... -maxDrop
            if (masterCounter <= -maxDrop) {
                masterCounter = 0;
            }

            result.A = masterCounter;
            result.B = masterCounter - (maxDrop * 0.5f);
            if (result.B <= -maxDrop) {
                result.B -= floorf(result.B);
            }
        }
        return result;
    }


    /**
     * Calculates symmetric equal-power crossfade gains for two read heads
     *
     * @param normalizedPhase Normalized phase position in range [0.0, 1.0]
     * @param crossoverWidth Width of transition zones in range [0.0, 1.0]
     * @return CrossfadeGains structure with gainA and gainB (gainA² + gainB² = 1.0)
     */
    inline FloatTuple calculateSymmetricCrossfade(float phase, float crossoverWidth) {

        FloatTuple gains;

        // crossoverWidth = 0 → hard switch
        if (crossoverWidth <= 1e-6f) {
            gains.A = (phase < 0.5f) ? 1.0f : 0.0f;
            gains.B = (phase < 0.5f) ? 0.0f : 1.0f;
            return gains;
        }

        // Calculate zone boundaries
        const float stableZone = 0.5f * (1.0 - crossoverWidth);  // Length of each stable zone
        const float transitionZone = crossoverWidth*0.5f;     // Length of each transition zone

        // Zone 1: A stable (left side)
        const float zone1End = stableZone;
        // Zone 2: A→B crossfade
        const float zone2End = zone1End + transitionZone;
        // Zone 3: B stable (center)
        const float zone3End = zone2End + stableZone;
        // Zone 4: B→A crossfade
        // zone4End = 1.0 (implicit)

        if (phase < zone1End) {
            // Zone 1: A on, B off
            gains.A = 1.0f;
            gains.B = 0.0f;
        }
        else if (phase < zone2End) {
            // Zone 2: A→B linear crossfade
            float fadeFactor = (phase - zone1End) / transitionZone;
            assert(fadeFactor <= 1.0f);
            assert(fadeFactor >= 0.0f);
            gains.A = (1.0f - fadeFactor);
            gains.B = (fadeFactor);
        }
        else if (phase < zone3End) {
            // Zone 3: A off, B on
            gains.A = 0.0f;
            gains.B = 1.0f;
        }
        else {
            // Zone 4: B→A linear crossfade
            float fadeFactor = (phase - zone3End) / transitionZone;
            assert(fadeFactor <= 1.0f);
            assert(fadeFactor >= 0.0f);
            gains.A = (fadeFactor);
            gains.B = (1.0f - fadeFactor);
        }

        return gains;
    }

void CombFilterBank::filterout(float *smp)
{
    const unsigned int gain_bufsize = buffersize / 16;
    float gainbuf[gain_bufsize];
    if (!gain_smoothing.apply(gainbuf, gain_bufsize, gainbwd))
        std::fill(gainbuf, gainbuf + gain_bufsize, gainbwd);

    float offsetbuf[buffersize];
    if (!offset_smoothing.apply(offsetbuf, buffersize, pitchOffset))
        std::fill(offsetbuf, offsetbuf + buffersize, pitchOffset);

    for (unsigned int i = 0; i < buffersize; ++i)
    {
        const float input_smp = smp[i] * inputgain;
        const float current_gain = gainbuf[i / 16];

        // Pitch drop pre-calculations
        const bool isPitchDrop = maxDrop > maxDrop_min;
        FloatTuple phases, gains;
        if (isPitchDrop) {
            phases = generatePhasedSawtooth(sampleCounter, dropRate, maxDrop);
            float normalizedPhase = fmodf(fabsf(sampleCounter) / maxDrop + 0.5f, 1.0f);
            gains = calculateSymmetricCrossfade(normalizedPhase, fadingTime);
        }

        for (unsigned int j = 0; j < nrOfStrings; ++j)
        {
            if (delays[j] == 0.0f) continue;

            // --- 1. Sample Acquisition (Unified for Normal and PitchDrop) ---
            float sMain = 0.0f;
            float sLeft = 0.0f;
            float sRight = 0.0f;

            const float baseDelay = delays[j] * powf(2.0f, offsetbuf[i]);
            const float contactPos = 0.25f * (offsetbuf[i] + 1.0f);

            // Helper to read all 3 taps (main, left, right) for a given delay
            auto readTaps = [&](float d, float weight) {
                if (weight <= 0.0f) return;

                // Main loop delay
                float dm = min(d, float(mem_size));
                sMain += sampleLerp(comb_smps[j], fmodf(float(pos_writer + mem_size) - dm, float(mem_size))) * weight;

                // Contact segments
                float dl = min(dm * contactPos, float(mem_size));
                float dr = min(dm * (1.0f - contactPos), float(mem_size));
                sLeft  += sampleLerp(comb_smps[j], (pos_writer + mem_size - int(dl)) % mem_size) * weight;
                sRight += sampleLerp(comb_smps[j], (pos_writer + mem_size - int(dr)) % mem_size) * weight;
            };

            if (isPitchDrop) {
                readTaps(baseDelay * powf(2.0f, phases.A), gains.A);
                readTaps(baseDelay * powf(2.0f, phases.B), gains.B);
            } else {
                readTaps(baseDelay, 1.0f);
            }

            // --- 2. Contact Logic (Unified) ---
            const float damp_range = 0.5f;
            float wr = (1.0f - damp_range) + damp_range * contactPos;
            float wl = (1.0f - damp_range) + damp_range * (1.0f - contactPos);

            float contactIn = (wl * sLeft + wr * sRight) / (wr + wl);

            // DC removal
            contactIn -= hp_state[j];
            hp_state[j] += 0.001f * contactIn;

            // Envelope and Threshold
            env[j] += 0.0001f * (fabsf(sMain) - env[j]);
            float thresh = 3.5f * contactOffset * env[j];
            float excess = fmaxf(0.0f, contactIn - thresh);

            // Response State
            float approximity = 1.0f - contactOffset;
            contactResponse[j] *= 0.5f;
            if (excess > 0.0f) {
                float drive = 4.0f + 16.0f * approximity;
                float n = 4.0f;
                float shapedExcess = excess * drive / powf(1.0f + powf(fabsf(excess * drive), n), 1.0f / n);
                contactResponse[j] += 0.5f * shapedExcess;
            }

            // --- 3. Feedback and Write ---
            const float hockeyFactor = powf(approximity, 16);
            const float w_cont = contactStrength * (contactResponse[j] + hockeyFactor * (1.0f - contactResponse[j]));
            const float delta = contactIn - sMain;

            const float feedback = tanhX((sMain + w_cont * delta) * current_gain);
            comb_smps[j][pos_writer] = input_smp + feedback;
        }

        // --- 4. Output Mixing ---
        smp[i] = 0.0f;
        unsigned int nrOfNonZeroStrings = 0;
        for (unsigned int j = 0; j < nrOfStrings; ++j) {
            if (delays[j] != 0.0f) {
                smp[i] += comb_smps[j][pos_writer];
                nrOfNonZeroStrings++;
            }
        }

        smp[i] *= outgain;
        if (nrOfNonZeroStrings > 0) smp[i] /= (float)nrOfNonZeroStrings;

        ++pos_writer %= mem_size;
    }
}
}

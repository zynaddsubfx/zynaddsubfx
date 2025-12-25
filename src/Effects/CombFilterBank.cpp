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

        // interpolate gainbuf values over buffer length using value smoothing filter (lp)
        // this should prevent popping noise when controlled binary with 0 / 127
        // new control rate = samplerate / 16
        const unsigned int gain_bufsize = buffersize / 16;
        //~ const unsigned int offset_bufsize = buffersize / 4;
        float gainbuf[gain_bufsize]; // buffer for value smoothing filter
        if (!gain_smoothing.apply( gainbuf, gain_bufsize, gainbwd ) ) // interpolate the gain value
            std::fill(gainbuf, gainbuf+gain_bufsize, gainbwd); // if nothing to interpolate (constant value)

        float offsetbuf[buffersize]; // buffer for value smoothing filter
        if (!offset_smoothing.apply( offsetbuf, buffersize, pitchOffset ) ) // interpolate the offset value
            std::fill(offsetbuf, offsetbuf+buffersize, pitchOffset); // if nothing to interpolate (constant value)

        for (unsigned int i = 0; i < buffersize; ++i)
        {

            const float input_smp = smp[i] * inputgain;
            if (maxDrop <= maxDrop_min)
            {   //Pitch drop deactivated
                for (unsigned int j = 0; j < nrOfStrings; ++j)
                {
                    if (delays[j] == 0.0f) continue;
                    // apply pitchoffset to comb delay
                    //~ if (contactOffset<1.0f) {
                    if (true) {

                        // Base delay length for this comb / string instance.
                        // Clamped to ring buffer size for safety.
                        const float baseDelay = min(delays[j], float(mem_size));

                        // Contact position along the string, mapped from [-1,1] → [0,0.5]
                        // Due to symmetry only half the string is modeled explicitly.
                        const float contactPos = 0.25f * (offsetbuf[i] + 1.0f); // 0 … 0.5

                        // --- Delay lengths for main string and the two partial segments ---
                        // Main delay represents the full string loop.
                        const float delayMain  = min(baseDelay, float(mem_size));

                        // Left/right partial delays represent the two string segments
                        // split by the contact position.
                        const float delayLeft  = min(baseDelay * contactPos, float(mem_size));
                        const float delayRight = min(baseDelay * (1.0f - contactPos), float(mem_size));

                        // --- Compute read positions in the circular buffer ---
                        // Main read uses fractional delay (interpolated).
                        const float posMain  = fmodf(float(pos_writer + mem_size) - delayMain,
                                                     float(mem_size));

                        // Partial reads currently use integer delay indexing
                        // (implicitly band-limited only by later processing).
                        const float posLeft  = ((pos_writer + mem_size) - int(delayLeft)) % mem_size;
                        const float posRight = ((pos_writer + mem_size) - int(delayRight)) % mem_size;

                        // --- Sample delay line at the computed positions ---
                        float sMain  = sampleLerp(comb_smps[j], posMain);
                        float sLeft  = sampleLerp(comb_smps[j], posLeft);
                        float sRight = sampleLerp(comb_smps[j], posRight);

                        // --- Frequency-dependent weighting of partial segments ---
                        // Longer segments are slightly damped to reduce excessive low-frequency energy.
                        // This effectively biases spectral contribution depending on contact position.
                        const float damp_range = 0.5f; // heuristic, could be parameterized or removed

                        float wr = (1.0f - damp_range) + damp_range * contactPos;
                        float wl = (1.0f - damp_range) + damp_range * (1.0f - contactPos);

                        // Normalize combined energy of both partial taps
                        float w_ges = wr + wl;

                        // Contact excitation signal derived from partial segments.
                        // This represents local string motion at the contact point.
                        float contactIn = (wl * sLeft + wr * sRight) / w_ges;

                        // --- DC removal on contact signal ---
                        // Slow high-pass to avoid DC buildup from asymmetric nonlinear processing.
                        contactIn -= hp_state[j];            // subtract DC estimate
                        hp_state[j] += 0.001f * contactIn;   // very slow LP tracking DC

                        // --- Envelope follower on main string signal ---
                        // Acts as a global energy reference (implicitly part of a control loop).
                        // Note: this envelope is influenced by the feedback decision below.
                        env[j] += 0.0001f * (fabsf(sMain) - env[j]);

                        // Contact threshold derived from envelope and user parameter.
                        // contactOffset scales the geometric "distance" to contact.
                        float thresh = 3.5f * contactOffset * env[j];

                        // Amount by which local contact motion exceeds the threshold.
                        // Half-wave rectification introduces a unilateral contact condition.
                        float excess = contactIn - thresh;
                        excess = fmaxf(0.0f, excess);
                        //~ excess *= excess; // TBD: Try out

                        // --- Contact response state ---
                        // contactResponse[j] is a first-order leaky IIR state driven by contact excess.
                        // It provides temporal smoothing of contact activity and a bounded memory,
                        // but does not accumulate unbounded energy.
                        // This effectively models penetration depth, but is envelope-dependent.
                        float approximity = 1.0f - contactOffset;
                        contactResponse[j] *= 0.5f;
                        if (excess > 0.0f) {


                            //~ excess += (approximity*approximity)*2.0f;

                            // drive rises nonlinearly, so low offsets stay very soft
                            float drive = 4.0f + 8.0f * approximity;
                            // exponent controls knee hardness
                            float n = 4.0f;//2.5f + 2.0f * approximity;
                            // nonlinear shaper
                            float shapedExcess =
                                    excess * drive /
                                    powf(1.0f + powf(fabsf(excess * drive), n), 1.0f / n);

                            contactResponse[j] += 0.5f * shapedExcess;

                        }


                        // --- Feedback mixing ---
                        // Contact strength is modulated by contact response.
                        // This couples contact dynamics back into the main feedback loop.
                        const float w_cont = contactStrength * (contactResponse[j] + powf(approximity, 16) * (1.0f - contactResponse[j]));
                        //~ const float w_cont = contactStrength;

                        // Difference between contact-local motion and global string motion.
                        // Injects localized disturbance rather than absolute signal.
                        const float delta = contactIn - sMain;

                        // Final feedback signal:
                        // - main string signal
                        // - plus weighted contact-induced deviation
                        // - passed through a saturator for energy stability
                        const float feedback =
                            tanhX((sMain + w_cont * delta) * gainbuf[i / 16]);

                        // --- Write back into comb buffer ---
                        // External excitation (input_smp) plus stabilized feedback.
                        comb_smps[j][pos_writer] = input_smp + feedback;

                        if(i==0 && j==0) {
                            printf("\ncontactStrength: %f\n", contactStrength);
                            printf("contactPos: %f\n", contactPos);
                            //~ printf("w_ges: %f\n", w_ges);
                            //~ printf("contactIn: %f\n", fabsf(contactIn));
                             printf("contactOffset: %f\n", contactOffset);
                             printf("thresh: %f\n", thresh);
                            printf("contactResponse: %f\n", fabsf(contactResponse[j]));
                            printf("w_cont: %f\n", w_cont);
                            //~ printf("feedback: %f\n", feedback);
                        }



                    }
                    else {

                        const float delay = min(delays[j] * powf(2.0f, offsetbuf[i]), float(mem_size));
                        // calculate reading position and care for ring buffer range
                        const float pos_reader = fmodf(float(pos_writer + mem_size) - delay, float(mem_size));
                        // sample at that position
                        const float feedbackSample = sampleLerp(comb_smps[j], pos_reader);
                        // add saturated feedback to comb
                        comb_smps[j][pos_writer] = input_smp + tanhX(feedbackSample * gainbuf[i/16]);
                    }
                }
            }
            else
            {   // Pitch drop Feature
                // instead of reading with constant delay or with offset
                // pitch drop keeps constantly changing the delay
                // because the delay is limited, we use two alternating reading heads
                // they jump back to 0 while their gain is 0

                // calculate phases for both reading heads
                const FloatTuple phases = generatePhasedSawtooth(sampleCounter, dropRate, maxDrop);
                const float normalizedPhase = fmodf(fabsf(sampleCounter) / maxDrop + 0.5f, 1.0f);
                // calculate the gains with given cross fadeing time
                const FloatTuple gains = calculateSymmetricCrossfade(normalizedPhase, fadingTime);

                for (unsigned int j = 0; j < nrOfStrings; ++j)
                {
                    if (delays[j] == 0.0f) continue;
                    const float baseDelay = delays[j] * powf(2.0f, offsetbuf[i]);

                    // Reading Head A
                    float sampleA = 0.0f;
                    if (gains.A > 0.0f)
                    {
                        const float delay1 = min(baseDelay * powf(2.0f, phases.A), float(mem_size));
                        const float pos_reader1 = fmodf(float(pos_writer + mem_size - delay1), float(mem_size));
                        sampleA = sampleLerp(comb_smps[j], pos_reader1) * gains.A;

                    }

                    // Reading Head B
                    float sampleB = 0.0f;
                    if (gains.B > 0.0f) {
                        const float delay2 = min(baseDelay * powf(2.0f, phases.B), float(mem_size));
                        const float pos_reader2 = fmodf(float(pos_writer + mem_size) - delay2, float(mem_size));
                        sampleB = sampleLerp(comb_smps[j], pos_reader2)* gains.B;

                    }

                    float feedbackSample = (sampleA + sampleB);
                    comb_smps[j][pos_writer] = input_smp + tanhX(feedbackSample * gainbuf[i/16]);
                }
            }
            // mix output buffer samples to output sample
            smp[i]=0.0f;
            unsigned int nrOfNonZeroStrings = 0;
            for (unsigned int j = 0; j < nrOfStrings; ++j)
                if (delays[j] != 0.0f) {
                    smp[i] += comb_smps[j][pos_writer];
                    nrOfNonZeroStrings++;
                }

            // apply output gain to sum of strings and
            // divide by nrOfStrings to get mean value
            smp[i] *= outgain;
            if(nrOfNonZeroStrings>0)
                smp[i] /= (float)nrOfNonZeroStrings;

        // proceed to next position in ringbuffer
        ++pos_writer %= mem_size;
        }
    }
}

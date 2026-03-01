#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>

#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "CombFilter.h"
#include "AnalogFilter.h"

/**
 * @file CombFilter.cpp
 * @brief Implementation of a digital comb filter with variable feedback and interpolation.
 * * Theory based on "Introduction to Digital Filters with Audio Applications" by Julius O. Smith III.
 * This implementation supports both Feed-Forward (FIR) and Feed-Backward (IIR) structures.
 * @see https://www.dsprelated.com/freebooks/filters/Analysis_Digital_Comb_Filter.html
 */

namespace zyn {

/**
 * @brief Constructor for the CombFilter.
 * Initializes ring buffers for input and output and calculates the required buffer size
 * based on the lowest supported frequency (25Hz) to ensure enough delay memory.
 * * @param alloc Pointer to the real-time safe Memory Allocator.
 * @param Ftype Filter type (determines feed-forward and feed-backward gains).
 * @param Ffreq Initial frequency in Hz.
 * @param Fq Filter quality factor (resonance/feedback strength).
 * @param srate System sample rate.
 * @param bufsize Size of the processing audio block.
 * @param Plpf_ Initial Low-Pass Filter coefficient for the feedback loop.
 * @param Phpf_ Initial High-Pass Filter coefficient for the feedback loop.
 */
CombFilter::CombFilter(Allocator *alloc, unsigned char Ftype, float Ffreq, float Fq,
    unsigned int srate, int bufsize, unsigned char Plpf_, unsigned char Phpf_)
    : Filter(srate, bufsize), gain(1.0f), q(Fq), type(Ftype), Plpf(127), Phpf(0),
    lpf(nullptr), hpf(nullptr), memory(*alloc)
{
    // Worst case: looking back from smps[0] at 25Hz using higher order interpolation.
    // Calculation: (Samplerate / 25Hz) + buffer safety margin.
    mem_size = (int)ceilf((float)samplerate / 25.0) + buffersize + 2;
    input = (float*)memory.alloc_mem(mem_size * sizeof(float));
    output = (float*)memory.alloc_mem(mem_size * sizeof(float));
    memset(input, 0, mem_size * sizeof(float));
    memset(output, 0, mem_size * sizeof(float));

    setfreq_and_q(Ffreq, q);
    settype(type);
    setlpf(Plpf_);
    sethpf(Phpf_);
}

/**
 * @brief Destructor. Releases allocated memory for buffers and internal filters.
 */
CombFilter::~CombFilter(void)
{
    memory.dealloc(input);
    memory.dealloc(output);
    if (lpf) memory.dealloc(lpf);
    if (hpf) memory.dealloc(hpf);
}

/**
 * @brief Pade approximation of the hyperbolic tangent function.
 * Used as a saturation function for the feedback loop to prevent clipping/instability
 * and to add "analog-style" warmth to high resonance settings.
 * * @param x Input signal value.
 * @return float Approximated tanh(x) result bounded to [-1.0, +1.0].
 */
inline float CombFilter::tanhX(const float x)
{
    const float x2 = x * x;
    return (x * (105.0f + 10.0f * x2) / (105.0f + (45.0f + x2) * x2));
}

/**
 * @brief Linear interpolation between samples in the ring buffer.
 * Essential for sub-sample delay times, allowing for precise frequency tuning.
 * * @param smp Pointer to the ring buffer (input or output).
 * @param pos Floating point index position within the buffer.
 * @return float Interpolated sample value.
 */
inline float CombFilter::sampleLerp(float *smp, float pos) {
    int poshi = (int)pos;
    float poslo = pos - (float)poshi;
    int poshi_next = (poshi + 1) % mem_size;
    return smp[poshi] + poslo * (smp[poshi_next] - smp[poshi]);
}

/**
 * @brief Main audio processing routine.
 * Implements the comb filter difference equation by mixing the input signal with
 * a delayed version of itself. Includes saturation and optional filtering in the loop.
 * * @param smp The audio buffer to be processed "in-place".
 */
void CombFilter::filterout(float *smp)
{
    // 1. Copy incoming samples into the input ring buffer (handling wrap-around)
    int endSize = mem_size - inputIndex;
    if (endSize < buffersize) {
        memcpy(&input[inputIndex], smp, endSize * sizeof(float));
        memcpy(&input[0], &smp[endSize], (buffersize - endSize) * sizeof(float));
    } else {
        memcpy(&input[inputIndex], smp, buffersize * sizeof(float));
    }

    // 2. Sample-by-sample loop
    for (int i = 0; i < buffersize; i++) {
        const float inputPos = fmodf(inputIndex + i - delay + mem_size, mem_size);
        const float outputPos = fmodf(outputIndex - delay + mem_size, mem_size);

        // Calculate feedback with forward and backward gains
        float feedback = tanhX(
            gainfwd * sampleLerp(input, inputPos) -
            gainbwd * sampleLerp(output, outputPos));

        // Optional: Apply filtering within the feedback loop
        if (lpf) lpf->filterSample(feedback);
        if (hpf) hpf->filterSample(feedback);

        smp[i] = smp[i] * gain + feedback;

        // Write resulting sample to the output ring buffer
        output[outputIndex] = smp[i];

        // Apply output stage gain
        smp[i] *= outgain;

        outputIndex = (outputIndex + 1) % mem_size;
    }

    // Update input pointer for next block
    inputIndex = (inputIndex + buffersize) % mem_size;
}

/**
 * @brief Sets both frequency and resonance (Q) simultaneously.
 */
void CombFilter::setfreq_and_q(float freq, float q)
{
    setfreq(freq);
    setq(q);
}

/**
 * @brief Sets the filter frequency.
 * Adjusts the delay length based on the frequency, compensating for the
 * phase delays introduced by the internal LPF and HPF to keep the filter in tune.
 * * @param freq_ Frequency in Hz (limited between 25Hz and 40kHz).
 */
void CombFilter::setfreq(float freq_)
{
    freq = limit(freq_, 25.0f, 40000.0f);
    delay = ((float)samplerate) / freq - lpfDelay - hpfDelay;
}

/**
 * @brief Sets the filter resonance (Q).
 * Uses a cube-root scaling factor for a more musical/logarithmic feel of the parameter.
 */
void CombFilter::setq(float q_)
{
    q = cbrtf(0.0015f * q_);
    settype(type);
}

/**
 * @brief Sets the input gain of the filter.
 * @param dBgain Gain in decibels.
 */
void CombFilter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
}

/**
 * @brief Configures the comb filter topology.
 * Determines how much of the forward (FIR) and backward (IIR) signal is mixed
 * into the output, and controls the phase (positive/negative feedback).
 * * @param type_ Type index (0-5) representing different routing modes.
 */
void CombFilter::settype(unsigned char type_)
{
    type = type_;
    switch (type) {
        case 0: default: gainfwd = 0.0f;  gainbwd = q;     break; // IIR Positive
        case 1:          gainfwd = q;     gainbwd = 0.0f;  break; // FIR Positive
        case 2:          gainfwd = q;     gainbwd = q;     break; // Mix Positive
        case 3:          gainfwd = 0.0f;  gainbwd = -q;    break; // IIR Negative
        case 4:          gainfwd = -q;    gainbwd = 0.0f;  break; // FIR Negative
        case 5:          gainfwd = -q;    gainbwd = -q;    break; // Mix Negative
    }
}

/**
 * @brief Configures the High-Pass Filter within the feedback loop.
 * Also calculates the group delay of the HPF to offset the main delay line
 * and maintain frequency accuracy.
 * * @param _Phpf HPF parameter value (0 = Off, >0 = frequency).
 */
void CombFilter::sethpf(unsigned char _Phpf)
{
    if(Phpf == _Phpf) return;
    Phpf = _Phpf;
    if(Phpf == 0) {
        if (hpf) { memory.dealloc(hpf); hpf = nullptr; }
        hpfDelay = 0.0f;
    } else {
        const float fr = expf(sqrtf(Phpf / 127.0f) * logf(10000.0f)) + 20.0f;

        if(hpf == nullptr)
            hpf = memory.alloc<AnalogFilter>(1, fr, 1, 0, samplerate, buffersize);
        else
            hpf->setfreq(fr);

        const float a = expf(-2.0f * PI * fr / samplerate);
        // Approximation of filter phase delay for compensation
        hpfDelay = -a / (1.0f + a);
    }
    setfreq(freq);
}

/**
 * @brief Configures the Low-Pass Filter within the feedback loop.
 * Similar to sethpf, this adjusts lpfDelay to ensure the comb filter's
 * fundamental frequency remains stable regardless of LPF settings.
 * * @param _Plpf LPF parameter value (127 = Off, <127 = frequency).
 */
void CombFilter::setlpf(unsigned char _Plpf)
{
    if(Plpf == _Plpf) return;
    Plpf = _Plpf;
    if(Plpf == 127) {
        if (lpf) { memory.dealloc(lpf); lpf = nullptr; }
        lpfDelay = 0.0f;
    } else {
        const float fr = expf(sqrtf(Plpf / 127.0f) * logf(25000.0f)) + 40.0f;
        if(!lpf)
            lpf = memory.alloc<AnalogFilter>(0, fr, 1, 0, samplerate, buffersize);
        else
            lpf->setfreq(fr);

        const float a = expf(-2.0f * PI * fr / samplerate);
        // Approximation of filter phase delay for compensation
        lpfDelay = a / (1.0f - a);
    }
    setfreq(freq);
}

} // end namespace zyn

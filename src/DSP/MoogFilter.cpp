#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>

#include "../Misc/Util.h"
#include "MoogFilter.h"

// theory from "THE ART OF VA FILTER DESIGN"
// by Vadim Zavalishin

namespace zyn{

MoogFilter::MoogFilter(unsigned char Ftype, float Ffreq, float Fq,
    unsigned int srate, int bufsize)
    :Filter(srate, bufsize), sr(srate), gain(1.0f)
{
    setfreq_and_q(Ffreq/srate, Fq);
    settype(Ftype); // q must be set before
    for (unsigned int i = 0; i<(sizeof(state)/sizeof(*state)); i++)
    {
        state[i] = 0.0f;
    }
}

MoogFilter::~MoogFilter(void)
{

}

inline float MoogFilter::tanhX(const float x) const
{
    // Pade approximation of tanh(x) bound to [-1 .. +1]
    // https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
    float x2 = x*x;
    return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2));
}


inline float MoogFilter::tanhXdivX(float x) const
{
    // Pade approximation for tanh(x)/x used in filter stages
    float x2 = x*x;
    //~ return ((15.0+x2)/(15.0+6.0*x2)); // more accurate but instable at high frequencies
    return (1.0f-(0.35f*x2)+(0.1f*x2*x2));
}

inline float MoogFilter::step(float input)
{
    // transconductance
    // gM(vIn) = tanh( vIn ) / vIn
    float gm0 = tanhXdivX(state[0]);
    float gm1 = tanhXdivX(state[1]);
    float gm2 = tanhXdivX(state[2]);
    float gm3 = tanhXdivX(state[3]);


    // pre calc often used terms
    float ctgm0 = c*gm0;
    float ctgm1 = c*gm1;
    float ctgm2 = c*gm2;
    float ctgm3 = c*gm3;

    // denominators
    float d0 = 1.0f / (1.0f + ctgm0);
    float d1 = 1.0f / (1.0f + ctgm1);
    float d2 = 1.0f / (1.0f + ctgm2);
    float d3 = 1.0f / (1.0f + ctgm3);

    // pre calc often used term
    float gm1td2tgm2td3 = gm1 * d2 * gm2 * d3;

    // instantaneous response estimate
    float y3Estimate =
        cp4 * d0 * gm0 * d1 * gm1td2tgm2td3 * input +
        cp3 * gm0 * d1 * gm1td2tgm2td3      * d0 * state[0] +
        cp2 * gm1td2tgm2td3                 * d1 * state[1] +
        c   * gm2 * d3                      * d2 * state[2] +
                                              d3 * state[3];

    // mix input and gained feedback estimate for
    // cheaper feedback gain compensation. Idea from 
    // Antti Huovilainen and Vesa Välimäki, "NEW APPROACHES TO DIGITAL SUBTRACTIVE SYNTHESIS"
    float u = input - tanhX(feedbackGain * (y3Estimate - 0.5f*input));
    // output of all stages
    float y0 = gm0 * d0 * (state[0] + c * u);
    float y1 = gm1 * d1 * (state[1] + c * y0);
    float y2 = gm2 * d2 * (state[2] + c * y1);
    float y3 = gm3 * d3 * (state[3] + c * y2);

    // update state
    state[0] += ct2 * (u - y0);
    state[1] += ct2 * (y0 - y1);
    state[2] += ct2 * (y1 - y2);
    state[3] += ct2 * (y2 - y3);

    // calculate multimode filter output
    return (a0 * u
          + a1 * y0
          + a2 * y1
          + a3 * y2
          + a4 * y3);
}

void MoogFilter::filterout(float *smp)
{
    for (int i = 0; i < buffersize; i ++)
    {
        smp[i] = step(tanhX(smp[i]*gain));
        smp[i] *= outgain;
    }
}

void MoogFilter::setfreq_and_q(float frequency, float q_)
{
    setfreq(frequency/sr);
    setq(q_);
}

inline float MoogFilter::tan_2(const float x) const
{
    //Pade approximation tan(x) hand tuned to map fCutoff
    float x2 = x*x;
    //~ return ((9.54f*x*((11.08f - x2)))/(105.0f - x2*(45.0f + x2)));
    return (x+0.15f*x2+0.3f*x2*x2);
}

void MoogFilter::setfreq(float ff)
{
    // pre warp cutoff to map to reality
    c = tan_2(PI * ff);    
    // limit cutoff to prevent overflow
    c = limit(c,0.0006f,1.5f);
    // pre calculate some stuff outside the hot zone
    ct2 = c * 2.0f;
    cp2 = c * c;
    cp3 = cp2 * c;
    cp4 = cp2 * cp2;
}

void MoogFilter::setq(float q)
{
    // flattening the Q input
    feedbackGain = cbrtf(q/1000.0f)*4.0f + 0.1f;
    // compensation factor for passband reduction by the negative feedback
    passbandCompensation = 1.0f + limit(feedbackGain, 0.0f, 1.0f);
}

void MoogFilter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
}

void MoogFilter::settype(unsigned char ftype)
{
    switch (ftype)
    {
        case 0:
            a0 = 1.0f; a1 =-4.0f; a2 = 6.0f; a3 =-4.0f; a4 = 1.0f;
            break;
        case 1:
            a0 = 0.0f; a1 = 0.0f; a2 = 4.0f; a3 =-8.0f; a4 = 4.0f;
            break;
        case 2:
        default:
            a0 = 0.0f; a1 = 0.0f; a2 = 0.0f; a3 = 0.0f; a4 = passbandCompensation;
            break;
    }
}

};

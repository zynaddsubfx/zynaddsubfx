#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>
#include "../Misc/Util.h"
#include "SallenKeyFilter.h"

namespace zyn {

SallenKeyFilter::SallenKeyFilter(unsigned char Ftype, float Ffreq, float Fq, unsigned int srate, int bufsize) : Filter(srate, bufsize), sr(srate) {
    setfreq_and_q(Ffreq/srate, Fq);
    m_dAlpha0 = 0.0;
    
    m_LPF1.m_fSampleRate = srate;
    m_LPF2.m_fSampleRate = srate;
    m_HPF1.m_fSampleRate = srate;
    m_dSaturation = 1.0f;
    
}

SallenKeyFilter::~SallenKeyFilter() {
}

void SallenKeyFilter::filterout(float *smp) {
    for (int i = 0; i < buffersize; i++) {
        smp[i] = step(smp[i]);
    }
}

void SallenKeyFilter::setfreq(float freq) {
    // prewarp for BZT
    float wd = 2*PI*freq;          
    float T  = 1.0f/(float)sr;             
    float wa = (2.0f/T)*tan(wd*T/2.0f); 
    float g  = wa*T/2.0f;    

    // G - the feedforward coeff in the VA One Pole - now with resonance compensation
    float G = g / (1.0f + g * (1.0f - 1.0f / m_dK));

    // set alphas
    m_LPF1.m_fAlpha = G;
    m_LPF2.m_fAlpha = G;
    m_HPF1.m_fAlpha = G;

    // set betas all are in the form of  <something>/((1 + g)
    m_LPF2.m_fBeta = (m_dK - m_dK*G)/(1.0f + g);
    m_HPF1.m_fBeta = -1.0f/(1.0f + g);

    // set m_dAlpha0 variable
    m_dAlpha0 = 1.0f/(1.0f - m_dK*G + m_dK*G*G);

}

void SallenKeyFilter::setq(float q_) {
    m_dK = q_/250.0f + 0.1f;

}

void SallenKeyFilter::setfreq_and_q(float frequency, float q)
{
   setq(q); 
   setfreq(frequency);
    
}

void SallenKeyFilter::setgain(float gain_) {
    gain = dB2rap(gain_);
}

void SallenKeyFilter::settype(unsigned char ftype_) {
    ftype = ftype_;
}

inline float SallenKeyFilter::tanhX(const float x) const
{
    // Pade approximation of tanh(x) bound to [-1 .. +1]
    // https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
    float x2 = x*x;
    return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2));
}

inline float SallenKeyFilter::step(float input) {

    // process input through LPF1
    double y1 = m_LPF1.doFilter(input);

    // form feedback value
    double S35 = m_HPF1.getFeedbackOutput() + m_LPF2.getFeedbackOutput(); 

    // calculate u
    double u = m_dAlpha0*(y1 + S35);

    // Regular Version
    u = tanhX(m_dSaturation*u);

    // feed it to LPF2
    double y = m_dK*m_LPF2.doFilter(u);

    // feed y to HPF
    m_HPF1.doFilter(y);

    // auto-normalize
    if(m_dK > 0)
        y *= 1/m_dK;

    return y;
}

}  // namespace zyn

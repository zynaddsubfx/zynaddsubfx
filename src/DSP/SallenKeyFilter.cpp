#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>
#include "../Misc/Util.h"
#include "SallenKeyFilter.h"

#define smoothing_size 8

namespace zyn {

SallenKeyFilter::SallenKeyFilter(unsigned char Ftype, float Ffreq, float Fq, unsigned int srate, int bufsize) : 
    Filter(srate, bufsize), sr(srate), freqbufsize(bufsize/smoothing_size) {
    setfreq_and_q(Ffreq/srate, Fq);
    m_dAlpha0 = 0.0;
    
    m_LPF1.m_fSampleRate = srate;
    m_LPF2.m_fSampleRate = srate;
    m_HPF1.m_fSampleRate = srate;
    m_dSaturation = 1.0f;
    input_old = 0.0f;
    
    
    freq_smoothing.sample_rate(samplerate_f/8);
    freq_smoothing.thresh(2.0f); // 2Hz
    
}

SallenKeyFilter::~SallenKeyFilter() {
}

void SallenKeyFilter::filterout(float *smp) {
    
    float freqbuf[freqbufsize];
    
    if ( freq_smoothing.apply( freqbuf, freqbufsize, freq ) )
    {
        /* in transition, need to do fine grained interpolation */
        for(int j = 0; j < freqbufsize; ++j)
        {
            updateFilters(freqbuf[j]);
            singlefilterout(&smp[j*smoothing_size], smoothing_size);
        }
    }
    else
    {
        /* stable state, just use one coeff */
        updateFilters(freq);
        singlefilterout(smp, buffersize);
    }
    
    
}
void SallenKeyFilter::singlefilterout(float *smp, unsigned int bufsize)
{
    for (unsigned int i = 0; i < bufsize; i++) {
        float input = tanhX(smp[i]*gain);
        // 2x oversampling
        smp[i] = step((input_old+input)*0.5f );
        smp[i] += step(input);
        smp[i] *= 0.5f;
        input_old = input;
        
        smp[i] *= outgain;
    }
}

void SallenKeyFilter::setfreq(float freq_) {
    // limit frequency - with oversampling
    freq = std::min(freq_, (float)sr*0.5f);
}


void SallenKeyFilter::updateFilters(float freq) {
    
    // prewarp for BZT
    float wd = 2*PI*freq;          
    float T  = 0.5f/(float)sr;       // 0.5 due to 2x oversampling      
    float wa = (2.0f/T)*tan(wd*T*0.5f); 
    float g  = wa*T*0.5f;    

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

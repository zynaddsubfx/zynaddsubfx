/*
  ZynAddSubFX - a software synthesizer

  Unison.cpp - Unison effect (multivoice chorus)
  Copyright (C) 2002-2009 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include <math.h>
#include <stdio.h>
#include "Unison.h"

Unison::Unison(int update_period_samples_, float max_delay_sec_) {
    update_period_samples = update_period_samples_;
    max_delay = (int)(max_delay_sec_ * (float)SAMPLE_RATE + 1);
    if(max_delay < 10)
        max_delay = 10;
    delay_buffer = new float[max_delay];
    delay_k      = 0;
    base_freq    = 1.0f;
    unison_bandwidth_cents = 10.0f;

    ZERO_float(delay_buffer, max_delay);

    uv = NULL;
    update_period_sample_k = 0;
    first_time = 0;

    set_size(1);
}

Unison::~Unison() {
    delete [] delay_buffer;
    if(uv)
        delete [] uv;
}

void Unison::set_size(int new_size) {
    if(new_size < 1)
        new_size = 1;
    unison_size = new_size;
    if(uv)
        delete [] uv;
    uv = new UnisonVoice[unison_size];
    first_time = true;
    update_parameters();
}

void Unison::set_base_frequency(float freq) {
    base_freq = freq;
    update_parameters();
}

void Unison::set_bandwidth(float bandwidth) {
    if(bandwidth < 0)
        bandwidth = 0.0f;
    if(bandwidth > 1200.0f)
        bandwidth = 1200.0f;

    printf("bandwidth %g\n", bandwidth);
#warning \
    : todo: if bandwidth is too small the audio will be self canceled (because of the sign change of the outputs)
    unison_bandwidth_cents = bandwidth;
    update_parameters();
}

void Unison::update_parameters() {
    if(!uv)
        return;
    float increments_per_second = SAMPLE_RATE
                                  / (float) update_period_samples;
//	printf("#%g, %g\n",increments_per_second,base_freq);
    for(int i = 0; i < unison_size; ++i) {
        float base = powf(UNISON_FREQ_SPAN, RND * 2.0f - 1.0f);
        uv[i].relative_amplitude = base;
        float period = base / base_freq;
        float m      = 4.0f / (period * increments_per_second);
        if(RND < 0.5f)
            m = -m;
        uv[i].step = m;
//		printf("%g %g\n",uv[i].relative_amplitude,period);
    }

    float max_speed = powf(2.0f, unison_bandwidth_cents / 1200.0f);
    unison_amplitude_samples = 0.125f
                               * (max_speed - 1.0f) * SAMPLE_RATE / base_freq;
    printf("unison_amplitude_samples %g\n", unison_amplitude_samples);

#warning \
    todo: test if unison_amplitude_samples is to big and reallocate bigger memory
    if(unison_amplitude_samples >= max_delay - 1)
        unison_amplitude_samples = max_delay - 2;

    update_unison_data();
}

void Unison::process(int bufsize, float *inbuf, float *outbuf) {
    if(!uv)
        return;
    if(!outbuf)
        outbuf = inbuf;

    float volume    = 1.0f / sqrt(unison_size);
    float xpos_step = 1.0f / (float) update_period_samples;
    float xpos      = (float) update_period_sample_k * xpos_step;
    for(int i = 0; i < bufsize; ++i) {
        if((update_period_sample_k++) >= update_period_samples) {
            update_unison_data();
            update_period_sample_k = 0;
            xpos = 0.0f;
        }
        xpos += xpos_step;
        float in = inbuf[i], out = 0.0f;

        float sign = 1.0f;
        for(int k = 0; k < unison_size; ++k) {
            float vpos = uv[k].realpos1
                         * (1.0f - xpos) + uv[k].realpos2 * xpos;        //optimize
            float pos = delay_k + max_delay - vpos - 1.0f;  //optimize
            int   posi;
            float posf;
            F2I(pos, posi); //optimize!
            if(posi >= max_delay)
                posi -= max_delay;
            posf = pos - floor(pos);
            out +=
                ((1.0f
                  - posf) * delay_buffer[posi] + posf
                 * delay_buffer[posi + 1]) * sign;
            sign = -sign;
        }
        outbuf[i] = out * volume;
//		printf("%d %g\n",i,outbuf[i]);
        delay_buffer[delay_k] = in;
        if((++delay_k) >= max_delay)
            delay_k = 0;
    }
}

void Unison::update_unison_data() {
    if(!uv)
        return;

    for(int k = 0; k < unison_size; ++k) {
        float pos  = uv[k].position;
        float step = uv[k].step;
        pos += step;
        if(pos <= -1.0f) {
            pos  = -1.0f;
            step = -step;
        }
        if(pos >= 1.0f) {
            pos  = 1.0f;
            step = -step;
        }
        float vibratto_val = (pos - 0.333333333f * pos * pos * pos) * 1.5f; //make the vibratto lfo smoother
#warning \
        I will use relative amplitude, so the delay might be bigger than the whole buffer
#warning \
        I have to enlarge (reallocate) the buffer to make place for the whole delay
        float newval = 1.0f + 0.5f
                       * (vibratto_val
                          + 1.0f) * unison_amplitude_samples
                       * uv[k].relative_amplitude;

        if(first_time)
            uv[k].realpos1 = uv[k].realpos2 = newval;
        else {
            uv[k].realpos1 = uv[k].realpos2;
            uv[k].realpos2 = newval;
        }

        uv[k].position = pos;
        uv[k].step     = step;
    }
    if(first_time)
        first_time = false;
}

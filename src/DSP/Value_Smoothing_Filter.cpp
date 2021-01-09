
/*******************************************************************************/
/* Copyright (C) 2008-2020 Jonathan Moore Liles                                */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/

#include "Value_Smoothing_Filter.h"
#include <math.h>

/* compensate for missing nonlib macro */
#define assume_aligned(x) (x)

void
Value_Smoothing_Filter::sample_rate ( nframes_t n )
{
    const float FS = n;
    const float T = 0.05f;

    w = _cutoff / (FS * T);
}

bool
Value_Smoothing_Filter::apply( sample_t * __restrict__ dst, nframes_t nframes, float gt )
{
    if ( _reset_on_next_apply )
    {
        reset( gt );
        _reset_on_next_apply = false;
        return false;
    }

    if ( target_reached(gt) )
        return false;

    sample_t * dst_ = (sample_t*) assume_aligned(dst);

    const float a = 0.07f;
    const float b = 1 + a;

    const float gm = b * gt;

    float g1 = this->g1;
    float g2 = this->g2;

    for (nframes_t i = 0; i < nframes; i++)
    {
        g1 += w * (gm - g1 - a * g2);
        g2 += w * (g1 - g2);
        dst_[i] = g2;
    }

    g2 += 1e-10f;               /* denormal protection */

    if ( fabsf( gt - g2 ) < 0.0001f )
        g2 = gt;

    this->g1 = g1;
    this->g2 = g2;

    return true;
}

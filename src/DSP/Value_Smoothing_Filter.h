
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

#ifndef VALUE_SMOOTHING_FILTER_H
#define VALUE_SMOOTHING_FILTER_H

typedef unsigned long nframes_t;
typedef float sample_t;

class Value_Smoothing_Filter
{
    float w, g1, g2;

    float _cutoff;

    bool _reset_on_next_apply;

public:

    Value_Smoothing_Filter ( )
        {
            g1 = g2 = 0;
            _cutoff = 10.0f;
            _reset_on_next_apply = false;
        }

    void reset_on_next_apply ( bool v ) { _reset_on_next_apply = v; }

    void cutoff ( float v ) { _cutoff = v; }

    void reset ( float v ) { g2 = g1 = v; }

    inline bool target_reached ( float gt ) const { return gt == g2; }

    void sample_rate ( nframes_t n );

    bool apply( sample_t * __restrict__ dst, nframes_t nframes, float gt );
};

#endif

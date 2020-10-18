/*
  ZynAddSubFX - a software synthesizer

  WaveTable.cpp - WaveTable implementation
  Copyright (C) 2020-2020 Johannes Lorenz

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

#include <cmath>
#include "WaveTable.h"

namespace zyn {

const Tensor1<WaveTable::float32>& WaveTable::get(float32 freq) const
{
    std::size_t num_freqs = freqs.size(), i;
    float bestFreqDist = 9999999;
    std::size_t bestI = 0;
    for(i = 0; i < num_freqs; ++i) // TODO: std::lower_bound?
    {
        float curFreqDist = fabsf(freqs[i] - freq);
        if(bestFreqDist > curFreqDist)
        {
            bestFreqDist = curFreqDist;
            bestI = i;
        }
        else
        {
            assert(i > 0);
            bestI = i - 1; // it got worse, so take the previous freq
            break;
        }
    }
    assert(bestI < num_freqs);
    std::size_t random_seed = 0; // TODO: fix
    return data[random_seed][bestI];
}

void WaveTable::insert(Tensor3<float32> &data,
                       Tensor1<float32> &freqs,
                       Tensor1<IntOrFloat> &semantics,
                       bool invalidate)
{
    pointer_swap(this->data, data);
    pointer_swap(this->freqs, freqs);
    pointer_swap(this->semantics, semantics);
    (void)invalidate; // future enhancement
}

WaveTable::WaveTable(std::size_t buffersize) :
    semantics(Shape1{num_semantics}),
    freqs(Shape1{num_freqs}),
    data(Shape3{num_semantics, num_freqs, buffersize})
{

}

}


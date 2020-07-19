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

#include "WaveTable.h"

namespace zyn {

Tensor1<const WaveTable::float32> WaveTable::get(WaveTable::float32 freq) const
{
    std::size_t num_freqs = freqs.shape().dim[0], i;
    for(i = 0; i < num_freqs; ++i) // TODO: use binary search
    {
        if(freqs[i] == freq)
            break;
    }
    assert(i < num_freqs);
    return Tensor1<const WaveTable::float32>(data[0][i].shape(), data[0][i].data());
}

void WaveTable::insert(Tensor3<float> &data,
                       Tensor1<float32> &freqs,
                       Tensor1<float32> &semantics,
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


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

const Tensor1<WaveTable::float32> &WaveTable::get(WaveTable::float32 freq) const
{
    (void) freq;
    /* TODO: this is obviously wrong yet, we need to return slices from data, */
    /*       not from freqs. This is for removing compiler warnings yet */
    return freqs;
}

void WaveTable::insert(Tensor3<float> data,
                       Tensor1<WaveTable::float32> freqs,
                       Tensor1<WaveTable::float32> semantics,
                       bool invalidate)
{
    (void)data;
    (void)freqs;
    (void)semantics;
    (void)invalidate;
}

WaveTable::WaveTable(std::size_t buffersize) :
    semantics(Shape1{num_semantics}),
    freqs(Shape1{num_freqs}),
    data(Shape3{num_semantics, num_freqs, buffersize})
{

}

}


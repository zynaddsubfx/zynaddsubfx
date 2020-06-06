/*
  ZynAddSubFX - a software synthesizer

  WaveTable.h - WaveTable definition
  Copyright (C) 2020-2020 Johannes Lorenz
  Author: Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef WAVETABLE_H
#define WAVETABLE_H

#include "../Misc/Allocator.h"

namespace zyn {

struct Shape1  { int dim[1]; };
struct Shape2  { int dim[2]; };
struct Shape3  { int dim[3]; };

template <class T>
struct Tensor3 {
    Tensor3(Shape3, Allocator&) {}
    T    ***data;
    Shape3  shape;

    Tensor3() = default;
    Tensor3(Tensor3&& other) = default;
    Tensor3& operator=(Tensor3&& other) = default;
};
template <class T>
struct Tensor2 {
    Tensor2(Shape2, Allocator&) {}
    T     **data;
    Shape2  shape;

    Tensor2() = default;
    Tensor2(Tensor2&& other) = default;
    Tensor2& operator=(Tensor2&& other) = default;
};
template <class T>
struct Tensor1 {
    Tensor1(Shape1, Allocator&) {}
    T      *data;
    Shape1  shape;

    Tensor1() = default;
    Tensor1(Tensor1&& other) = default;
    Tensor1& operator=(Tensor1&& other) = default;
};

class WaveTable
{
    using float32 = float;
    Tensor3<float32> data;  //!< time=col,freq=row,semantics(oscil param or random seed)=depth
    Tensor1<float32> freqs; //!< The frequency of each 'row'
    Tensor1<float32> semantics; //!< E.g. oscil params or random seed (e.g. 0...127)

    enum class WtMode
    {
        freq_smps, // (freq)->samples
        freqseed_smps, // (freq, seed)->samples
        freqwave_smps // (freq, wave param)->samples
    };
public:
    //! Return sample slice for given frequency
    const Tensor1<float32>& get(float32 freq) const; // works for both seed and seedless setups
    // future extensions
    // Tensor2<float32> get_antialiased(void); // works for seed and seedless setups
    // Tensor2<float32> get_wavetablemod(float32 freq);

    //! Insert generated data into this object
    //! If this is only adding new random seeds, then the rest of the data does
    //! not need to be purged
    //! @param semantics seed or param
    void insert(Tensor3<float> data, Tensor1<float32> freqs, Tensor1<float32> semantics, bool invalidate=true);

    // future extension
    // Used to determine if new random seeds are needed
    // int number_of_remaining_seeds(void);

    WaveTable() = default;
    WaveTable(WaveTable&& other) = default;
    WaveTable& operator=(WaveTable&& other) = default;
};

}

#endif // WAVETABLE_H

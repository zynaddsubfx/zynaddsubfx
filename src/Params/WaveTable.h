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

#include <cassert>
#include <algorithm>

namespace zyn {

namespace detail
{
    //! check that arrays @p arr1 and @p arr2 are equal in range 0...Idx
    template<std::size_t Idx>
    constexpr bool check_equal_until(const std::size_t* arr1, const std::size_t* arr2) { return arr1[Idx] == arr2[Idx] && check_equal_until<Idx-1>(arr1, arr2); }
    template<>
    constexpr bool check_equal_until<0>(const std::size_t* arr1, const std::size_t* arr2) { return arr1[0] == arr2[0]; }

    //! multiply the first (Idx+1) members (0...Idx) of array @p arr
    template<std::size_t Idx>
    constexpr std::size_t mult(const std::size_t* arr) { return arr[Idx] * mult<Idx-1>(arr); }
    template<>
    constexpr std::size_t mult<0>(const std::size_t* arr) { return arr[0]; }
}

template<std::size_t N>
class Shape
{
public:
    std::size_t dim[N];
    constexpr bool operator==(const Shape<N>& other) const {
        return detail::check_equal_until<N-1>(dim, other.dim); }
    constexpr std::size_t volume() const { return detail::mult<N-1>(dim); }
    //! projection of shape onto first dimension
    //! the first dimension will be removed
    Shape<N-1> proj() const
    {
        // TODO: don't need for loop (use consexpr)?
        Shape<N-1> res;
        for(std::size_t i = 1; i < N; ++i)
            res.dim[i-1] = dim[i];
        return res;
    }
};

//! Tensor base class
//! contains a possibly multi dimensional array
template <std::size_t N, class T>
class TensorBase
{
protected:
    T       *m_data;
    Shape<N> m_shape;
    bool     m_owner = true; //! says if memory is owned by us
    void cleanup() { if(m_owner) { delete[] m_data; m_owner = false; } }
public:
    TensorBase(Shape<N> shape, T* data) :
        m_data(data),
        m_shape(shape),
        m_owner(false)
    {}
    TensorBase(Shape<N> shape) :
        m_data(new T[shape.volume()]),
        m_shape(shape)
    {}
    ~TensorBase() { cleanup(); }

    TensorBase(TensorBase&& other) {
        m_data = other.m_data; m_shape = other.m_shape;
        m_owner = other.m_owner;
        other.m_owner = false;
    }
    TensorBase& operator=(TensorBase&& other) {
        cleanup();
        m_data = other.m_data; m_shape = other.m_shape;
        m_owner = other.m_owner;
        other.m_owner = false;
    }

    Shape<N> shape() const { return m_shape; }

    bool operator==(const TensorBase<N, T>& other) const {
        return shape() == other.shape() &&
                std::equal(m_data, m_data + (shape().volume()), other.m_data); }

    bool operator!=(const TensorBase<N, T>& other) const {
        return !operator==(other); }
};

// all dimensions other than 1
template <std::size_t N, class T>
class Tensor : public TensorBase<N, T>
{
    using base_type = TensorBase<N, T>;
public:
    using TensorBase<N, T>::TensorBase;
    //! access slice with index @p i
    Tensor<N-1, T> operator[](std::size_t i) {
        return std::move(Tensor<N-1, T>(base_type::shape().proj(), base_type::m_data + i * base_type::shape().proj().volume())); }
    const Tensor<N-1, const T> operator[](std::size_t i) const {
        return std::move(Tensor<N-1, const T>(base_type::shape().proj(), base_type::m_data + i * base_type::shape().proj().volume())); }


    template<std::size_t N2, class X2>
    friend void pointer_swap(Tensor<N2, X2>&, Tensor<N2, X2>&);
};

// dimension 1
template <class T>
class Tensor<1, T> : public TensorBase<1, T>
{
    using base_type = TensorBase<1, T>;
public:
    using TensorBase<1, T>::TensorBase;
    //! raw access into 1D array
    T* data() { return base_type::m_data; }
    const T* data() const { return base_type::m_data; }
    //! raw indexing into 1D array
    T& operator[](std::size_t i) { return base_type::m_data[i]; }
    const T& operator[](std::size_t i) const { return base_type::m_data[i]; }

    template<std::size_t N2, class X2>
    friend void pointer_swap(Tensor<N2, X2>&, Tensor<N2, X2>&);
};

template<class T> using Tensor1 = Tensor<1, T>;
template<class T> using Tensor2 = Tensor<2, T>;
template<class T> using Tensor3 = Tensor<3, T>;

using Shape1 = Shape<1>;
using Shape2 = Shape<2>;
using Shape3 = Shape<3>;

//! swap data of two equally sized tensors
template<std::size_t N, class T>
void pointer_swap(Tensor<N, T>& t1, Tensor<N, T>& t2) {
    assert(t1.m_shape == t2.m_shape);
    std::swap(t1.m_data, t2.m_data);
}

class WaveTable
{
public:
    using float32 = float;
    // pure guesses for what sounds good:
    constexpr const static std::size_t num_freqs = 10;
    constexpr const static std::size_t num_semantics = 128;

    enum class WtMode
    {
        freq_smps, // (freq)->samples
        freqseed_smps, // (freq, seed)->samples
        freqwave_smps // (freq, wave param)->samples
    };

private:
    Tensor1<float32> semantics; //!< E.g. oscil params or random seed (e.g. 0...127)
    Tensor1<float32> freqs; //!< The frequency of each 'row'
    Tensor3<float32> data;  //!< time=col,freq=row,semantics(oscil param or random seed)=depth
    WtMode m_mode;

public:

    void setMode(WtMode mode) { m_mode = mode; }

    //! Return sample slice for given frequency
    Tensor1<const float32> get(float32 freq) const; // works for both seed and seedless setups
    // future extensions
    // Tensor2<float32> get_antialiased(void); // works for seed and seedless setups
    // Tensor2<float32> get_wavetablemod(float32 freq);

    //! Insert generated data into this object
    //! If this is only adding new random seeds, then the rest of the data does
    //! not need to be purged
    //! @param semantics seed or param
    void insert(Tensor3<float>& data, Tensor1<float32>& freqs, Tensor1<float32>& semantics, bool invalidate=true);

    // future extension
    // Used to determine if new random seeds are needed
    // std::size_t number_of_remaining_seeds(void);

    WaveTable(std::size_t buffersize);
    WaveTable(WaveTable&& other) = default;
    WaveTable& operator=(WaveTable&& other) = default;
};

}

#endif // WAVETABLE_H

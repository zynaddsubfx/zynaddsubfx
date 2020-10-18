/*
  ZynAddSubFX - a software synthesizer

  WaveTable.h - WaveTable declarations
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
#include <cstddef>
#include <algorithm>
#include <iostream>

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
        // TODO: don't need for loop (use constexpr)?
        Shape<N-1> res;
        for(std::size_t i = 1; i < N; ++i)
            res.dim[i-1] = dim[i];
        return res;
    }

    Shape<N+1> prepend_dim(std::size_t to_prepend) const
    {
        Shape<N+1> res;
        res.dim[0] = to_prepend;
        for(std::size_t i = 0; i < N; ++i)
            res.dim[i+1] = dim[i];
        return res;
    }
};

//! Common Tensor base class for all dimensions
template <std::size_t N, class T>
class TensorBase
{
protected:
    std::size_t m_size;
    bool     m_owner = true; //! says if memory is owned by us

    TensorBase() : m_size(0), m_owner(false) {}
    TensorBase(std::size_t size) : m_size(size), m_owner(true) {};

    TensorBase(TensorBase&& other) {
        m_size = other.size;
        m_owner = other.m_owner;
        other.m_owner = false;
    }
    TensorBase& operator=(TensorBase&& other) {
        m_size = other.m_size;
        m_owner = other.m_owner;
        other.m_owner = false;
    }
public:
    std::size_t size() const { return m_size; }
protected:
    void reserve_size(std::size_t new_size) { m_size = new_size; m_owner = true; }
    bool operator==(const TensorBase<N, T>& other) const {
#if 0
        return shape() == other.shape();/* &&
                std::equal(m_data, m_data + (shape().volume()), other.m_data);*/ }
#else
        return m_size == other.m_size;
#endif
    }

};

//! Tensor class for all dimensions != 1
template <std::size_t N, class T>
class Tensor : public TensorBase<N, T>
{
    using base_type = TensorBase<N, T>;
    Tensor<N-1, T>* m_data;
    void init_shape_alloced(const Shape<N>& shape)
    {
        Shape<N-1> proj = shape.proj();
        for(std::size_t i = 0; i < base_type::size(); ++i)
        {
            m_data[i].init_shape(proj);
        }
    }

public:
    Tensor() : m_data(nullptr) {}
    Tensor(const Shape<N>& shape) :
        TensorBase<N, T> (shape.dim[0]),
        m_data(new Tensor<N-1, T>[base_type::size()])
    {
        init_shape_alloced(shape);
    }
    void init_shape(const Shape<N>& shape)
    {
        base_type::reserve_size(shape.dim[0]);
        m_data = new Tensor<N-1, T>[base_type::size()];
        init_shape_alloced(shape);
    }
    ~Tensor()
    {
        if(base_type::m_owner) { delete[] m_data; }
    }
    Tensor& operator=(Tensor&& other) {
        base_type::operator=(other);
        m_data = other.m_data;
    }

    //! access slice with index @p i
    Tensor<N-1, T>& operator[](std::size_t i) { return m_data[i]; }
    const Tensor<N-1, T>& operator[](std::size_t i) const { return m_data[i]; }

    bool operator==(const Tensor<N, T>& other) const
    {
        bool equal = base_type::operator==(other);
        for(std::size_t i = 0; equal && i < base_type::size(); ++i)
        {
            equal = m_data[i].operator==(other.m_data[i]);
        }
        return equal;
    }

    bool operator!=(const Tensor<N, T>& other) const {
        return !operator==(other); }

    // testing only:
    std::size_t set_data(const T* new_data)
    {
        std::size_t consumed = 0;
        for(std::size_t i = 0; i < base_type::size(); ++i)
        {
            consumed += m_data[i].set_data(new_data + consumed);
        }
        return consumed;
    }

    Shape<N> shape() const {
        if(base_type::size()) {
            return m_data[0].shape().prepend_dim(base_type::size());
        }
        else {
            Shape<N> res;
            for(std::size_t i = 0; i < N; ++i) res.dim[i] = 0;
            return res;
        }
    }

    template<std::size_t N2, class X2>
    friend void pointer_swap(Tensor<N2, X2>&, Tensor<N2, X2>&);
};

//! Tensor class for dimension 1
template <class T>
class Tensor<1, T> : public TensorBase<1, T>
{
    using base_type = TensorBase<1, T>;
    T* m_data;

public:
    Tensor() : m_data(nullptr) {}
    Tensor(std::size_t size) : TensorBase<1, T>(size), m_data(new T[size]) {}
    Tensor(const Shape<1>& shape) : Tensor(shape.dim[0]){}
    ~Tensor() { if(base_type::m_owner) { delete[] m_data; } }
    Tensor& operator=(Tensor&& other) {
        base_type::operator=(other);
        m_data = other.m_data;
    }

    void init_shape(const Shape<1>& shape)
    {
        base_type::reserve_size(shape.dim[0]);
        m_data = new T[base_type::size()];
    }

    //! raw access into 1D array
    T* data() { return m_data; }
    const T* data() const { return m_data; }
    //! raw indexing into 1D array
    T& operator[](std::size_t i) { return m_data[i]; }
    const T& operator[](std::size_t i) const { return m_data[i]; }

    bool operator==(const Tensor<1, T>& other) const {
        return base_type::operator==(other) &&
            std::equal(m_data, m_data + base_type::size(), other.m_data);
    }

    bool operator!=(const Tensor<1, T>& other) const {
        return !operator==(other); }

    std::size_t set_data(const T* new_data)
    {
        std::copy(new_data, new_data+base_type::size(), m_data);
        return base_type::size();
    }

    Shape<1> shape() const { return Shape<1>{base_type::size()}; }

    template<std::size_t N2, class X2>
    friend void pointer_swap(Tensor<N2, X2>&, Tensor<N2, X2>&);
};

template<class T> using Tensor1 = Tensor<1, T>;
template<class T> using Tensor2 = Tensor<2, T>;
template<class T> using Tensor3 = Tensor<3, T>;

using Shape1 = Shape<1>;
using Shape2 = Shape<2>;
using Shape3 = Shape<3>;

//! swap data of two tensors
template<std::size_t N, class T>
void pointer_swap(Tensor<N, T>& t1, Tensor<N, T>& t2)
{
    std::swap(t1.m_size, t2.m_size);
    std::swap(t1.m_data, t2.m_data);
    std::swap(t1.m_owner, t2.m_owner);
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
    const Tensor1<float32> &get(float32 freq) const; // works for both seed and seedless setups
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

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
#include "WaveTableFwd.h"

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

/**
    Common Tensor base class for all dimensions

    m_usable <= m_size_planned <= m_capacity
    m_usable: data that has been generated and can be used
    m_size_planned: data that shall be generated (after generation,
                    m_usable == m_size_planned)
    m_capacity: allocated size
*/
template <std::size_t N, class T>
class TensorBase
{
protected:
    std::size_t m_capacity;
    bool m_owner = true; //!< says if memory is owned by us

    TensorBase() : m_capacity(0), m_owner(false) {}
    TensorBase(std::size_t capacity, std::size_t newsize) : m_capacity(capacity), m_owner(true) {};
    TensorBase(const TensorBase& other) = delete;
    TensorBase& operator=(const TensorBase& other) = delete;
    TensorBase(TensorBase&& other) = delete;
    TensorBase& operator=(TensorBase&& other) = delete;


#if 0
    TensorBase(TensorBase&& other) {
        m_capacity = other.capacity;
        m_size_planned = other.m_size_planned;
        m_usable = other.m_usable;
        m_owner = other.m_owner;
        other.m_owner = false;
    }
    TensorBase& operator=(TensorBase&& other) {
        m_capacity = other.m_capacity;
        m_size_planned = other.m_size_planned;
        m_usable = other.m_usable;
        m_owner = other.m_owner;
        other.m_owner = false;
    }
    // TODO: check rule of 5
#endif
public:
    std::size_t capacity() const { return m_capacity; }

    int size() const { return capacity(); }

protected:
    void set_capacity(std::size_t new_capacity) { m_capacity = new_capacity; m_owner = true; }
    bool operator==(const TensorBase<N, T>& other) const {
#if 0
        return shape() == other.shape();/* &&
                std::equal(m_data, m_data + (shape().volume()), other.m_data);*/ }
#else
        return m_capacity == other.m_capacity;
#endif
    }

    void swapWith(Tensor<N,T>& other)
    {
        std::swap(m_capacity, other.m_capacity);
        std::swap(m_owner, other.m_owner);
    }
};

//! Ringbuffer without buffer - only size, reader and writer
//! @invariant r<=w_delayed<=w<r (in a cyclic sense)
class AbstractRingbuffer
{
public:
    AbstractRingbuffer(int newsize) { resize(newsize); }

    int read_space() const
    {
        // "r->w" > 0 => OK, can read
        // "r->w" == 0 => can not read
        return (w_delayed - r + m_size) % m_size;
    }

    int write_space() const
    {
        // "w->r" (ringwise) > 1 => OK, can write
        // "w->r" (ringwise) == 1 => write would cause "r==w" (increase read space) => can not read
        // "w->r" (ringwise) == 0 => forbidden (see case ==1)
        return ((r-1) - w + m_size) % m_size;
    }

    int write_space_delayed() const
    {
        // same reasoning as for read_space
        return (w - w_delayed + m_size) % m_size;
    }

    void inc_write_pos(int amnt) { assert(amnt <= write_space()); w = (amnt+w) % m_size; }
    void inc_write_pos_delayed(int amnt = 1) { assert(amnt <= write_space_delayed()); w_delayed = (amnt+w_delayed) % m_size; }
    void inc_read_pos() { assert(m_size == 1 || 1 <= read_space()); /* <- too many consumed? */ r = (1+r) % m_size; }

    int read_pos() const { return r; }
    int write_pos() const { return w; }
    int write_pos_delayed() const { return w_delayed; }

    int size() const { return m_size; }

    void swapWith(AbstractRingbuffer& other)
    {
        std::swap(m_size, other.m_size);
        std::swap(r, other.r);
        std::swap(w_delayed, other.w_delayed);
        std::swap(w, other.w);
    }

    void dump() const
    {
        printf("RINGBUFFER: size: %d, r/w_delayed/w: %d %d %d\n",
               m_size, r, w_delayed, w);
    }

private:

    // TODO: only used by CTOR -> move to CTOR initializer list
    void resize(int newsize)
    {
        r = w = w_delayed = 0;
        m_size = newsize;
    }

    // Invariant: r<=w_delayed<=w<r (in a cyclic sense)
    //! read position, marks next element to read. can read until w_delayed.
    int r;
    //! write position, marks the element that will be written to next (may be
    //! reserved for write already if w_delayed < w). can write until w.
    int w_delayed;
    //! write position, marks the element that will be reserved for a write
    //! next (but maybe not yet written). can write until r-1.
    int w;

    int m_size;
};

//! Tensor class for all dimensions != 1
template <std::size_t N, class T>
class Tensor : public TensorBase<N, T>
{
    using base_type = TensorBase<N, T>;

    //! allow using some functions, but only inside of Tensor
    class SubTensor : public Tensor<N-1, T>
    {
    public:
        SubTensor() = default;
        using Tensor<N-1, T>::init_shape;
    };

    SubTensor* m_data;
    void init_shape_alloced(const Shape<N>& shape)
    {
        Shape<N-1> proj = shape.proj();
        for(std::size_t i = 0; i < base_type::capacity(); ++i)
        {
            m_data[i].init_shape(proj);
        }
    }

protected:
    Tensor() : m_data(nullptr) {}
    void init_shape(const Shape<N>& shape)
    {
        assert(!m_data);
        base_type::set_capacity(shape.dim[0]);
        m_data = new SubTensor[base_type::capacity()];
        init_shape_alloced(shape);
    }

public:
    Tensor(const Shape<N>& shape, const Shape<N>& ) :
        TensorBase<N, T> (shape.dim[0], shape.dim[0]),
        m_data(new SubTensor[base_type::capacity()])
    {
        init_shape_alloced(shape);
    }

    ~Tensor()
    {
        delete[] m_data;
    }

    Tensor& operator=(Tensor&& other) {
        base_type::operator=(other);
        m_data = other.m_data;
        return *this;
    }

    //! access slice with index @p i
    Tensor<N-1, T>& operator[](std::size_t i) { return m_data[i]; }
    const Tensor<N-1, T>& operator[](std::size_t i) const { return m_data[i]; }

    bool operator==(const Tensor<N, T>& other) const
    {
        bool equal = base_type::operator==(other);
        for(std::size_t i = 0; equal && i < base_type::capacity(); ++i)
        {
            equal = m_data[i].operator==(other.m_data[i]);
        }
        return equal;
    }

    bool operator!=(const Tensor<N, T>& other) const {
        return !operator==(other); }

    void fillWithZeroes()
    {
        for(std::size_t i = 0; i < base_type::size(); ++i)
        {
            m_data[i].fillWithZeroes();
        }
    }

    // testing only:
    std::size_t set_data_using_deep_copy(const T* new_data)
    {
        std::size_t consumed = 0;
        for(std::size_t i = 0; i < base_type::size(); ++i)
        {
            consumed += m_data[i].set_data_using_deep_copy(new_data + consumed);
        }
        return consumed;
    }

    Shape<N> capacity_shape() const {
        if(base_type::capacity()) {
            return m_data[0].capacity_shape().prepend_dim(base_type::capacity());
        }
        else {
            Shape<N> res;
            for(std::size_t i = 0; i < N; ++i) res.dim[i] = 0;
            return res;
        }
    }

/*    template<std::size_t N2, class X2>
    friend void pointer_swap(Tensor<N2, X2>&, Tensor<N2, X2>&);*/

    void swapWith(Tensor<N,T>& other)
    {
        TensorBase<N, T>::swapWith(other);
        std::swap(m_data, other.m_data);
    }
};

//! Tensor class for dimension 1
template <class T>
class Tensor<1, T> : public TensorBase<1, T>
{
    using base_type = TensorBase<1, T>;
    T* m_data;

protected:
    Tensor() : m_data(nullptr) {}
    void init_shape(const Shape<1>& shape)
    {
        assert(!m_data);
        base_type::set_capacity(shape.dim[0]);
        if(base_type::capacity())
            m_data = new T[base_type::capacity()];
    }
public:
    Tensor(std::size_t capacity, std::size_t ) : TensorBase<1, T>(capacity, capacity),
        m_data(capacity ? new T[capacity] : nullptr) {}
    Tensor(const Shape<1>& shape, const Shape<1>& ) : Tensor(shape.dim[0], shape.dim[0]){}
    ~Tensor() { delete[] m_data; }
    Tensor& operator=(Tensor&& other) {
        base_type::operator=(other);
        m_data = other.m_data;
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

    void fillWithZeroes()
    {
        std::fill_n(m_data, base_type::size(), 0);
    }

    std::size_t set_data_using_deep_copy(const T* new_data)
    {
        std::copy(new_data, new_data+base_type::size(), m_data);
        return base_type::capacity();
    }

    Shape<1> capacity_shape() const { return Shape<1>{base_type::capacity()}; }

    void swapWith(Tensor<1,T>& other)
    {
        TensorBase<1, T>::swapWith(other);
        std::swap(m_data, other.m_data);
    }
};

using Shape1 = Shape<1>;
using Shape2 = Shape<2>;
using Shape3 = Shape<3>;

#if 0
//! swap data of two tensors
template<std::size_t N, class T>
void pointer_swap(Tensor<N, T>& t1, Tensor<N, T>& t2)
{
    std::swap(t1.m_capacity, t2.m_capacity);
    std::swap(t1.m_size_planned, t2.m_size_planned);
    std::swap(t1.m_usable, t2.m_usable);
    std::swap(t1.m_data, t2.m_data);
    std::swap(t1.m_owner, t2.m_owner);
}
#endif

class Tensor3ForWaveTable : public Tensor<3, wavetable_types::float32>, public AbstractRingbuffer
{
    using base_type = Tensor<3, wavetable_types::float32>;
public:
    using base_type::base_type;

    Tensor3ForWaveTable(const Shape<3>& shape, const Shape<3>& )
        : base_type(shape, shape), AbstractRingbuffer(shape.dim[0]) {}

    int size() const { return base_type::size(); }

/*  template<std::size_t N2, class X2>
    friend void pointer_swap(Tensor<N2, X2>&, Tensor<N2, X2>&);*/

    void swapWith(Tensor3ForWaveTable& other)
    {
        base_type::swapWith(other);
        AbstractRingbuffer::swapWith(other);
    }
};

/**
    All pre-computed data that ADnote needs for generating a note
    @note This class will only ever reside in the RT thread. Non-RT threads
          may generate the required tensors, but only send them via bToU.
          At no time, non-RT threads access content directly.
*/
class WaveTable
{
public:
    using float32 = wavetable_types::float32;
    using IntOrFloat = wavetable_types::IntOrFloat;

    // pure guesses for what sounds good:
    constexpr const static std::size_t num_freqs = 10;
    constexpr const static std::size_t num_semantics = 128;

    enum class WtMode
    {
        freq_smps, // (freq)->samples
        freqseed_smps, // (seed, freq)->samples
        freqwave_smps // (wave param, freq)->samples
    };

private:
    Tensor1<IntOrFloat> semantics; //!< E.g. oscil params or random seed (e.g. 0...127)
    Tensor1<float32> freqs; //!< The frequency of each 'row'
    Tensor3ForWaveTable data;  //!< time=col,freq=row,semantics(oscil param or random seed)=depth
    Tensor3ForWaveTable next_tensor3; //!< can be safely refilled while data is in use, is swapped with data

    // permanent pointers, required to be able to transfer pointers through uToB
    const Tensor1<IntOrFloat>* semantics_addr = &semantics;
    const Tensor1<float32>* freqs_addr = &freqs;

    WtMode m_mode;

    int timestamp_requested = 0, timestamp_current = 0;

    Tensor3ForWaveTable& get_tensor3(bool next) { return (next?next_tensor3:data); }
    const Tensor3ForWaveTable& get_tensor3(bool next) const { return (next?next_tensor3:data); }
public:

    // ringbuffer stuff
    int size_next_semantics() const { return next_tensor3.size(); }
    int write_space_semantics(bool next_tensor) const { return get_tensor3(next_tensor).write_space(); }
    int write_space_delayed_semantics(bool next_tensor) const { return get_tensor3(next_tensor).write_space_delayed(); }
    int write_pos_semantics() const { return data.write_pos(); }
    int write_pos_delayed_semantics(bool next_tensor) const { return get_tensor3(next_tensor).write_pos_delayed(); }
    void inc_read_pos_semantics() { data.inc_read_pos(); }
    void inc_write_pos_semantics(bool next_tensor, int amnt) { get_tensor3(next_tensor).inc_write_pos(amnt); }
    void inc_write_pos_delayed_semantics(bool next_tensor, int amnt) { get_tensor3(next_tensor).inc_write_pos_delayed(amnt); }

    void dump_rb(bool next_tensor) const { get_tensor3(next_tensor).AbstractRingbuffer::dump(); }

    // timestamp/outdated stuff
    void set_outdated(int timestamp) { assert(timestamp != timestamp_current); timestamp_requested = timestamp; }
    void set_not_outdated_anymore(int timestamp) { assert(timestamp == timestamp_requested); timestamp_current = timestamp; }
    bool outdated() const { return timestamp_requested != timestamp_current; }
    bool is_correct_timestamp(int timestamp) const { return timestamp_requested == timestamp; }

    // data access
    const Tensor1<IntOrFloat>* const* get_semantics_addr() const { return &semantics_addr; }
    const Tensor1<float32>* const* get_freqs_addr() const { return &freqs_addr; }
    void swapSemantics(Tensor1<IntOrFloat>& unused) { semantics.swapWith(unused); }
    void swapFreqs(Tensor1<float32>& unused) { freqs.swapWith(unused); }
    void swapTensor3With(Tensor3ForWaveTable& unused) { next_tensor3.swapWith(unused); }

    void setMode(WtMode mode) { m_mode = mode; }
    WtMode mode() const { return m_mode; }

    //! Return sample slice for given frequency
    const Tensor1<float32> &get(float32 freq); // works for both seed and seedless setups
    // future extensions
    // Tensor2<float32> get_antialiased(void); // works for seed and seedless setups
    // Tensor2<float32> get_wavetablemod(float32 freq);

    void setSemantic(std::size_t i, IntOrFloat val) { semantics[i] = val; }
    void setFreq(std::size_t i, float32 val) { freqs[i] = val; }
    void setDataAt(int semanticIdx, int freqIdx, float bufIdx, float to) { data[semanticIdx][freqIdx][bufIdx] = to; }
    void swapDataAt(int semanticIdx, int freqIdx, Tensor1<float32>& new_data, bool next_tensor) {
        (next_tensor ? (next_tensor3) : (data))[semanticIdx][freqIdx].swapWith(new_data); }
    void swapTensor3s(int timestamp) { assert(timestamp == timestamp_requested); data.swapWith(next_tensor3); timestamp_current = timestamp_requested; }

    //! Insert generated data into this object
    //! If this is only adding new random seeds, then the rest of the data does
    //! not need to be purged
    //! @param semantics seed or param
    //void insert(Tensor3<float32> &data, Tensor1<float32>& freqs, Tensor1<IntOrFloat> &semantics, bool invalidate=true);

    //void insert(Tensor1<float32> &buffer, bool invalidate=true);

    // future extension
    // Used to determine if new random seeds are needed
    // std::size_t number_of_remaining_seeds(void);

    WaveTable(std::size_t buffersize);

    WaveTable(const WaveTable& other) = delete;
    WaveTable& operator=(const WaveTable& other) = delete;
    WaveTable(WaveTable&& other) = delete;
    WaveTable& operator=(WaveTable&& other) = delete;
};

}

#endif // WAVETABLE_H

/*
  ZynAddSubFX - a software synthesizer

  RtQueue.h - Fixed size Queue for use in RT context
  Copyright (C) 2024 Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <array>
#include <tuple>
#include <stdexcept>
#include <optional>

template<typename T, std::size_t Size>
class RTQueue {
public:
    RTQueue() : head_(0), tail_(0), count_(0) {}

    void push(const T& item) {
        if (count_ == Size) {
            throw std::overflow_error("Queue is full");
        }
        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % Size;
        ++count_;
    }

    void pop() {
        if (count_ == 0) {
            throw std::logic_error("Queue is empty");
        }
        head_ = (head_ + 1) % Size;
        --count_;
    }

    T front() const {
        if (count_ == 0) {
            throw std::logic_error("Queue is empty");
        }
        return buffer_[head_];
    }
    
    bool full() const {
        return count_ == Size;
    }

    bool empty() const {
        return count_ == 0;
    }

private:
    std::array<T, Size> buffer_;
    std::size_t head_;
    std::size_t tail_;
    std::size_t count_;
};

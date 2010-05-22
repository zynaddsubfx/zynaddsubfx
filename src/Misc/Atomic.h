/*
  ZynAddSubFX - a software synthesizer

  Atomic.h - Simple Atomic operation wrapper
  Copyright (C) 2009-2009 Mark McCurry
  Author: Mark McCurry

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
#ifndef ATOM_H
#define ATOM_H

#include <pthread.h>

/**Very simple threaded value container*/
template<class T>
class Atomic
{
    public:
        /**Initializes Atom
         * @param val the value of the atom*/
        Atomic(const T &val);
        ~Atomic();

        void operator=(const T &val);
        T operator()() const;
        T operator++();
        T operator--();
    private:
        mutable pthread_mutex_t mutex;
        T value;
};
#include "Atomic.cpp"
#endif


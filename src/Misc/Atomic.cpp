/*
  ZynAddSubFX - a software synthesizer

  Atomic.cpp - Simple Atomic operation wrapper
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

template<class T>
Atomic<T>::Atomic(const T &val)
    :value(val)
{
    pthread_mutex_init(&mutex, NULL);
}

template<class T>
Atomic<T>::~Atomic()
{
    pthread_mutex_destroy(&mutex);
}

template<class T>
void Atomic<T>::operator=(const T &nval)
{
    pthread_mutex_lock(&mutex);
    value = nval;
    pthread_mutex_unlock(&mutex);
}

template<class T>
T Atomic<T>::operator()() const
{
    T tmp;
    pthread_mutex_lock(&mutex);
    tmp = value;
    pthread_mutex_unlock(&mutex);
    return tmp;
}

template<class T>
T Atomic<T>::operator++()
{
    T tmp;
    pthread_mutex_lock(&mutex);
    tmp = ++value;
    pthread_mutex_unlock(&mutex);
    return tmp;
}

template<class T>
T Atomic<T>::operator--(){
    T tmp;
    pthread_mutex_lock(&mutex);
    tmp = --value;
    pthread_mutex_unlock(&mutex);
    return tmp;
}

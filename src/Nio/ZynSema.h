/*
  ZynAddSubFX - a software synthesizer

  ZynSema.h - Semaphore Wrapper
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef ZYNSEMA_H
#define ZYNSEMA_H

#if defined _MSC_VER

// avoid double inclusion if win socket headers
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_

#include <windows.h>
#include <synchapi.h>  // WaitOnAddress / WakeByAddress*
#include <atomic>

namespace zyn {

enum { PTHREAD_PROCESS_PRIVATE, PTHREAD_PROCESS_SHARED };

// TODO: This is AI code
class ZynSema
{
public:
    ZynSema() noexcept
    : _count(0)
    {}

    ~ZynSema() = default;

    // s is ignored (POSIX pshared flag, meaningless on Windows)
    int init(int /*s*/, int v) noexcept
    {
        _count.store(v, std::memory_order_release);
        return 0;
    }

    int post() noexcept
    {
        LONG prev = _count.fetch_add(1, std::memory_order_release);

        // If there were waiters, wake one
        if (prev < 0)
        {
            WakeByAddressSingle(&_count);
        }
        return 0;
    }

    int wait() noexcept
    {
        LONG value = _count.fetch_sub(1, std::memory_order_acquire);

        if (value > 0)
            return 0;

        // We must wait
        for (;;)
        {
            LONG expected = value - 1;
            WaitOnAddress(&_count, &expected, sizeof(expected), INFINITE);

            value = _count.load(std::memory_order_acquire);
            if (value >= 0)
                return 0;
        }
    }

    int trywait() noexcept
    {
        LONG value = _count.load(std::memory_order_acquire);

        while (value > 0)
        {
            if (_count.compare_exchange_weak(
                value, value - 1,
                std::memory_order_acquire,
                std::memory_order_relaxed))
            {
                return 0;
            }
        }
        return -1; // EAGAIN equivalent
    }

    int getvalue() noexcept
    {
        return static_cast<int>(_count.load(std::memory_order_relaxed));
    }

private:
    std::atomic<LONG> _count;
};

}

#elif defined __APPLE__ || defined WIN32

#include <pthread.h>

namespace zyn {

class ZynSema
{
public:
    ZynSema (void) : _count (0)
    {
    }

    ~ZynSema (void)
    {
        pthread_mutex_destroy (&_mutex);
        pthread_cond_destroy (&_cond);
    }

    int init (int, int v)
    {
        _count = v;
        return pthread_mutex_init (&_mutex, 0) || pthread_cond_init (&_cond, 0);
    }

    int post (void)
    {
        pthread_mutex_lock (&_mutex);
        if (++_count == 1) pthread_cond_signal (&_cond);
        pthread_mutex_unlock (&_mutex);
        return 0;
    }

    int wait (void)
    {
        pthread_mutex_lock (&_mutex);
        while (_count < 1) pthread_cond_wait (&_cond, &_mutex);
        --_count;
        pthread_mutex_unlock (&_mutex);
        return 0;
    }

    int trywait (void)
    {
        if (pthread_mutex_trylock (&_mutex)) return -1;
        if (_count < 1)
        {
            pthread_mutex_unlock (&_mutex);
            return -1;
        }
        --_count;
        pthread_mutex_unlock (&_mutex);
        return 0;
    }

    int getvalue (void) const
    {
        return _count;
    }


private:
    int              _count;
    pthread_mutex_t  _mutex;
    pthread_cond_t   _cond;
};

}

#else // POSIX semaphore

#include <pthread.h>  // for the "s" parameter to "init"
#include <semaphore.h>

namespace zyn {

class ZynSema
{
public:
    ZynSema (void)
    {
    }
    ~ZynSema (void)
    {
        sem_destroy (&_sema);
    }
    int init (int s, int v)
    {
        return sem_init (&_sema, s, v);
    }
    int post (void)
    {
        return sem_post (&_sema);
    }
    int wait (void)
    {
        return sem_wait (&_sema);
    }
    int trywait (void)
    {
        return sem_trywait (&_sema);
    }
    int getvalue(void)
    {
        int v = 0;
        sem_getvalue(&_sema, &v);
        return v;
    }

private:
    sem_t _sema;
};

}

#endif // POSIX semapore

#endif // ZYNSEMA_H

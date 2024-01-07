/*
  ZynAddSubFX - a software synthesizer

  Util.cpp - Miscellaneous functions
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "globals.h"
#include "Util.h"
#include <vector>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef HAVE_SCHEDULER
#include <sched.h>
#endif

#define errx(...) {}
#ifndef errx
#include <err.h>
#endif

#include <rtosc/rtosc.h>

namespace zyn {

bool isPlugin = false;

prng_t prng_state = 0x1234;

void chua_attractor(float &x, float &y, float &z, float alpha, float beta, float mu0, float mu1, float speed) {
    //~ const int ovsmpl = speed * 8
    const float dt = speed / 32;
    //~ for(int i = 0; i<ovsmpl; i++) {
        const float ht = mu1 * x + 0.5f * (mu0 - mu1) * (fabs(x + 1.0f) - fabs(x - 1.0f));
        const float x_out = x + dt * (alpha * (y - x - ht));
        const float y_out = y  + dt * (x - y + z);
        const float z_out = z + dt * (-beta * y);
        x = x_out;
        y = y_out;
        z = z_out;
    //~ }
    
}


/*
 * Transform the velocity according the scaling parameter (velocity sensing)
 */
float VelF(float velocity, unsigned char scaling)
{
    float x;
    x = powf(VELOCITY_MAX_SCALE, (64.0f - scaling) / 64.0f);
    if((scaling == 127) || (velocity > 0.99f))
        return 1.0f;
    else
        return powf(velocity, x);
}

char *fast_strcpy(char *dest, const char *src, size_t buffersize)
{
    *dest = 0;
    return strncat(dest, src, buffersize-1);
}

/*
 * Get the detune in cents
 */
float getdetune(unsigned char type,
                unsigned short int coarsedetune,
                unsigned short int finedetune)
{
    float det = 0.0f, octdet = 0.0f, cdet = 0.0f, findet = 0.0f;
    //Get Octave
    int octave = coarsedetune / 1024;
    if(octave >= 8)
        octave -= 16;
    octdet = octave * 1200.0f;

    //Coarse and fine detune
    int cdetune = coarsedetune % 1024;
    if(cdetune > 512)
        cdetune -= 1024;

    int fdetune = finedetune - 8192;

    switch(type) {
//	case 1: is used for the default (see below)
        case 2:
            cdet   = fabsf(cdetune * 10.0f);
            findet = fabsf(fdetune / 8192.0f) * 10.0f;
            break;
        case 3:
            cdet   = fabsf(cdetune * 100.0f);
            findet = powf(10, fabsf(fdetune / 8192.0f) * 3.0f) / 10.0f - 0.1f;
            break;
        case 4:
            cdet   = fabsf(cdetune * 701.95500087f); //perfect fifth
            findet =
                (powf(2, fabsf(fdetune / 8192.0f) * 12.0f) - 1.0f) / 4095 * 1200;
            break;
        //case ...: need to update N_DETUNE_TYPES, if you'll add more
        default:
            cdet   = fabsf(cdetune * 50.0f);
            findet = fabsf(fdetune / 8192.0f) * 35.0f; //almost like "Paul's Sound Designer 2"
            break;
    }
    if(finedetune < 8192)
        findet = -findet;
    if(cdetune < 0)
        cdet = -cdet;

    det = octdet + cdet + findet;
    return det;
}


bool fileexists(const char *filename)
{
    struct stat tmp;
    int result = stat(filename, &tmp);
    if(result >= 0)
        return true;

    return false;
}

void set_realtime()
{
#ifdef HAVE_SCHEDULER
    sched_param sc;
    sc.sched_priority = 60;
    //if you want get "sched_setscheduler undeclared" from compilation,
    //you can safely remove the following line:
    sched_setscheduler(0, SCHED_FIFO, &sc);
    //if (err==0) printf("Real-time");
#endif
}



#ifdef WIN32
#include <windows.h>

//https://stackoverflow.com/questions/5801813/c-usleep-is-obsolete-workarounds-for-windows-mingw
void os_usleep(long usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#else

void os_usleep(long length)
{
    usleep(length);
}
#endif

//!< maximum length a pid has on any POSIX system
//!< this is an estimation, but more than 12 looks insane
constexpr std::size_t max_pid_len = 12;

//!< safe pid length guess, posix conform
std::size_t os_guess_pid_length()
{
    const char* pid_max_file = "/proc/sys/kernel/pid_max";
    if(-1 == access(pid_max_file, R_OK)) {
        return max_pid_len;
    }
    else {
        std::ifstream is(pid_max_file);
        if(!is.good())
            return max_pid_len;
        else {
            std::string s;
            is >> s;
            for(const auto& c : s)
                if(c < '0' || c > '9')
                    return max_pid_len;
            return std::min(s.length(), max_pid_len);
        }
    }
}

//!< returns pid padded, posix conform
std::string os_pid_as_padded_string()
{
    char result_str[max_pid_len << 1];
    std::fill_n(result_str, max_pid_len, '0');
    std::size_t written = snprintf(result_str + max_pid_len, max_pid_len,
        "%d", (int)getpid());
    // the below pointer should never cause segfaults:
    return result_str + max_pid_len + written - os_guess_pid_length();
}

std::string legalizeFilename(std::string filename)
{
    for(int i = 0; i < (int) filename.size(); ++i) {
        char c = filename[i];
        if(!(isdigit(c) || isalpha(c) || (c == '-') || (c == ' ')))
            filename[i] = '_';
    }
    return filename;
}

void invSignal(float *sig, size_t len)
{
    for(size_t i = 0; i < len; ++i)
        sig[i] *= -1.0f;
}

float SYNTH_T::numRandom()
{
    return RND;
}

float interpolate(const float *data, size_t len, float pos)
{
    assert(len > (size_t)pos + 1);
    const int l_pos      = (int)pos,
              r_pos      = l_pos + 1;
    const float leftness = pos - l_pos;
    return data[l_pos] * leftness + data[r_pos] * (1.0f - leftness);
}

float cinterpolate(const float *data, size_t len, float pos)
{
    const unsigned int i_pos = pos,
                       l_pos = i_pos % len,
                       r_pos = l_pos + 1 < len ? l_pos + 1 : 0;
    const float leftness = pos - i_pos;
    return data[l_pos] * leftness + data[r_pos] * (1.0f - leftness);
}

char *rtosc_splat(const char *path, std::set<std::string> v)
{
    char argT[v.size()+1];
    rtosc_arg_t arg[v.size()];
    unsigned i=0;
    for(auto &vv : v) {
        argT[i]  = 's';
        arg[i].s = vv.c_str();
        i++;
    }
    argT[v.size()] = 0;

    size_t len = rtosc_amessage(0, 0, path, argT, arg);
    char *buf = new char[len];
    rtosc_amessage(buf, len, path, argT, arg);
    return buf;
}

}

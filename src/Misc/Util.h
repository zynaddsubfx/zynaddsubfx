/*
  ZynAddSubFX - a software synthesizer

  Util.h - Miscellaneous functions
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <stdint.h>
#include <algorithm>
#include <set>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

namespace zyn {

extern bool isPlugin;
bool fileexists(const char *filename);

using std::min;
using std::max;

/**
 * Copy string to another memory location, including the terminating zero byte
 * @param dest Destination memory location
 * @param src Source string
 * @param buffersize Maximal number of bytes that you can write to @p dest
 * @return A pointer to @p dest
 * @warning @p dest and @p src shall not overlap
 * @warning if buffersize is larger than strlen(src)+1, unused bytes in @p dest
 *   are not overwritten. Secure information may be released. Don't use this if
 *   you want to send the string somewhere else, e.g. via IPC.
 */
char *fast_strcpy(char *dest, const char *src, size_t buffersize);

//Velocity Sensing function
extern float VelF(float velocity, unsigned char scaling);

#define N_DETUNE_TYPES 4 //the number of detune types
extern float getdetune(unsigned char type,
                       unsigned short int coarsedetune,
                       unsigned short int finedetune);

/**Try to set current thread to realtime priority program priority
 * \todo see if the right pid is being sent
 * \todo see if this is having desired effect, if not then look at
 * pthread_attr_t*/
void set_realtime();

/**Os independent sleep in microsecond*/
void os_usleep(long length);

//! returns pid padded to maximum pid length, posix conform
std::string os_pid_as_padded_string();

std::string legalizeFilename(std::string filename);

void invSignal(float *sig, size_t len);

template<class T>
std::string stringFrom(T x)
{
    std::stringstream ss;
    ss << x;
    return ss.str();
}

template<class T>
std::string to_s(T x)
{
    return stringFrom(x);
}

template<class T>
T stringTo(const char *x)
{
    std::string str = x != NULL ? x : "0"; //should work for the basic float/int
    std::stringstream ss(str);
    T ans;
    ss >> ans;
    return ans;
}



template<class T>
T limit(T val, T min, T max)
{
    return val < min ? min : (val > max ? max : val);
}

template<class T>
bool inRange(T val, T min, T max)
{
    return val >= min && val <= max;
}

template<class T>
T array_max(const T *data, size_t len)
{
    T max = 0;

    for(unsigned i = 0; i < len; ++i)
        if(max < data[i])
            max = data[i];
    return max;
}

//Random number generator

typedef uint32_t prng_t;
extern prng_t prng_state;

// Portable Pseudo-Random Number Generator
inline prng_t prng_r(prng_t &p)
{
    return p = p * 1103515245 + 12345;
}

inline prng_t prng(void)
{
    return prng_r(prng_state) & 0x7fffffff;
}

inline void sprng(prng_t p)
{
    prng_state = p;
}

/*
 * The random generator (0.0f..1.0f)
 */
#ifndef INT32_MAX_FLOAT
#define INT32_MAX_FLOAT   0x7fffff80	/* the float mantissa is only 24-bit */
#endif
#define RND (prng() / (INT32_MAX_FLOAT * 1.0f))

//Linear Interpolation
float interpolate(const float *data, size_t len, float pos);

//Linear circular interpolation
float cinterpolate(const float *data, size_t len, float pos);

template<class T>
static inline void nullify(T &t) {delete t; t = NULL; }
template<class T>
static inline void arrayNullify(T &t) {delete [] t; t = NULL; }

char *rtosc_splat(const char *path, std::set<std::string>);

/**
 * Port macros - these produce easy and regular port definitions for common
 * types
 */
#define rParamZyn(name, ...) \
  {STRINGIFY(name) "::i",  rProp(parameter) rMap(min, 0) rMap(max, 127) DOC(__VA_ARGS__), NULL, rParamICb(name)}

#define rPresetType \
{"preset-type:", rProp(internal) rDoc("clipboard type of object"), 0, \
    [](const char *, rtosc::RtData &d){ \
        rObject *obj = (rObject*)d.obj; \
        d.reply(d.loc, "s", obj->type);}}

// let only realtime pastes reply,
// because non-realtime pastes need the free on non-realtime side (same thread)
#define rPasteInternal(isRt) \
rPresetType, \
{"paste:b", rProp(internal) rDoc("paste port"), 0, \
    [](const char *m, rtosc::RtData &d){ \
        printf("rPaste...\n"); \
        rObject &paste = **(rObject **)rtosc_argument(m,0).b.data; \
        rObject &o = *(rObject*)d.obj;\
        o.paste(paste);\
        rObject* ptr = &paste;\
        if(isRt)\
            d.reply("/free", "sb", STRINGIFY(rObject), sizeof(rObject*), &ptr);\
        else \
            delete ptr;}}

#define rArrayPasteInternal(isRt) \
{"paste-array:bi", rProp(internal) rDoc("array paste port"), 0, \
    [](const char *m, rtosc::RtData &d){ \
        printf("rArrayPaste...\n"); \
        rObject &paste = **(rObject **)rtosc_argument(m,0).b.data; \
        int field = rtosc_argument(m,1).i; \
        rObject &o = *(rObject*)d.obj;\
        o.pasteArray(paste,field);\
        rObject* ptr = &paste;\
        if(isRt)\
            d.reply("/free", "sb", STRINGIFY(rObject), sizeof(rObject*), &ptr);\
        else\
            delete ptr;}}

#define rPaste rPasteInternal(false)
#define rPasteRt rPasteInternal(true)
#define rArrayPaste rArrayPasteInternal(false)
#define rArrayPasteRt rArrayPasteInternal(true)

}

#define rUnit(x) rMap(unit, x)

#endif

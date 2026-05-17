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
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cinttypes>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <pwd.h>
    #include <cstdlib>
    #include <fcntl.h>
    #include <err.h>
#endif

#ifdef HAVE_SCHEDULER
#include <sched.h>
#endif

#ifndef errx
#define errx(...) {}
#endif

#include <rtosc/rtosc.h>

namespace zyn {

bool isPlugin = false;

prng_t prng_state = 0x1234;

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


std::uint32_t os_getpid()
{
#ifdef _MSC_VER
    return static_cast<std::uint32_t>(GetCurrentProcessId());
#else
    return static_cast<std::uint32_t>(getpid());
#endif
}

#ifndef _MSC_VER
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
        "%" PRIu32, os_getpid());
    // the below pointer should never cause segfaults:
    return result_str + max_pid_len + written - os_guess_pid_length();
}

#endif

#ifdef WIN32
static std::string windows_get_path(REFKNOWNFOLDERID rfid)
{
    PWSTR wide_path = nullptr;

    HRESULT hr = SHGetKnownFolderPath(rfid, 0, nullptr, &wide_path);
    if (FAILED(hr) || !wide_path)
        return {};

    // Calculate size for UTF-16 -> UTF-8 conversion
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_path, -1,
        nullptr, 0, nullptr, nullptr);
    if (utf8_size <= 0) {
        CoTaskMemFree(wide_path);
        return {};
    }

    // Do the conversion
    std::string result(utf8_size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide_path, -1,
        result.data(), utf8_size, nullptr, nullptr);

    CoTaskMemFree(wide_path);
    return result;
}
#endif

std::filesystem::path os_homepath()
{
#ifdef WIN32
    return windows_get_path(FOLDERID_Profile);
#else

    // Prefer HOME variable
    if (const char* home = std::getenv("HOME")) {
        if (*home)
            return home;
    }

    // Fallback: passwd database
    const struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir)
        return pw->pw_dir;

    return {};
#endif
}

std::filesystem::path os_localpath()
{
#ifdef WIN32
    return std::filesystem::path(windows_get_path(FOLDERID_Profile)) / "zynaddsubfx";
#else
    // Prefer XDG variable
    if (const char* home = std::getenv("HOME")) {
        if (*home)
            return std::filesystem::path(home) / "zynaddsubfx";
    }
    // Fallback: via HOME path
    return os_homepath() / ".local" / "share" / "zynaddsubfx";
#endif
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
#ifdef NDEBUG
    (void)len;
#else
    assert(len > (size_t)pos + 1 && pos >= 0);
#endif
    const unsigned int l_pos      = (int)pos;
    const unsigned int r_pos      = l_pos + 1;
    const float rightness = pos - (float)l_pos;
    return data[l_pos] + (data[r_pos] - data[l_pos]) * rightness;
}

float cinterpolate(const float *data, size_t len, float pos)
{
    const unsigned int i_pos = (int)pos;
    const unsigned int l_pos = i_pos % len;
    const unsigned int r_pos = (l_pos + 1) < len ? l_pos + 1 : 0;
    const float rightness = pos - (float)i_pos;
    return data[l_pos] + (data[r_pos] - data[l_pos]) * rightness;
}

char *rtosc_splat(const char *path, std::set<std::string> v)
{
    STACKALLOC(char, argT, v.size()+1);
    STACKALLOC(rtosc_arg_t, arg, v.size());
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

void expanddirname(std::string &dirname) {
    if (dirname.empty())
        return;

    // if the directory name starts with a ~ and the $HOME variable is
    // defined in the environment, replace ~ by the content of $HOME
    if (dirname.at(0) == '~') {
        char *home_dirname = getenv("HOME");
        if (home_dirname != NULL) {
            dirname = std::string(home_dirname) + dirname.substr(1);
        }
    }

#ifdef ZYN_DATADIR
    {
        std::string var = "$ZYN_DATADIR";
        size_t pos = dirname.find(var);
        if (pos != std::string::npos) {
            dirname.replace(pos, var.length(), ZYN_DATADIR);
        }
    }
#endif

    normalizedirsuffix(dirname);
}

void normalizedirsuffix(std::string &dirname) {
    if(((dirname[dirname.size() - 1]) != '/')
       && ((dirname[dirname.size() - 1]) != '\\'))
        dirname += "/";
}

}

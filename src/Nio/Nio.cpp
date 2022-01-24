/*
  ZynAddSubFX - a software synthesizer

  Nio.cpp - IO Wrapper Layer
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "Nio.h"
#include "OutMgr.h"
#include "InMgr.h"
#include "EngineMgr.h"
#include "MidiIn.h"
#include "AudioOut.h"
#include "WavEngine.h"
#include "../Misc/Config.h"
#include <cstring>
#include <iostream>
#include <algorithm>

namespace zyn {

using std::string;
using std::set;
using std::cerr;
using std::endl;

#ifndef IN_DEFAULT
#define IN_DEFAULT "NULL"
#endif
#ifndef OUT_DEFAULT
#define OUT_DEFAULT "NULL"
#endif

InMgr     *in  = NULL;
OutMgr    *out = NULL;
EngineMgr *eng = NULL;
string     postfix;

bool   Nio::autoConnect     = false;
bool   Nio::pidInClientName = false;
string Nio::defaultSource   = IN_DEFAULT;
string Nio::defaultSink     = OUT_DEFAULT;

void Nio::init(const SYNTH_T &synth, const oss_devs_t& oss_devs,
               class Master *master)
{
    in  = &InMgr::getInstance(); //Enable input wrapper
    out = &OutMgr::getInstance(&synth); //Initialize the Output Systems
    eng = &EngineMgr::getInstance(&synth, &oss_devs); //Initialize the Engines

    in->setMaster(master);
    out->setMaster(master);
}

bool Nio::start()
{
    if(eng)
        return eng->start();
    else
        return false;
}

void Nio::stop()
{
    if(eng)
        eng->stop();
}

void Nio::setDefaultSource(string name)
{
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    defaultSource = name;
}

void Nio::setDefaultSink(string name)
{
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    defaultSink = name;
}

bool Nio::setSource(string name)
{
    return in->setSource(name);
}

bool Nio::setSink(string name)
{
    return out->setSink(name);
}

void Nio::setPostfix(std::string post)
{
    postfix = post;
}

std::string Nio::getPostfix(void)
{
    return postfix;
}

set<string> Nio::getSources(void)
{
    set<string> sources;
    for(std::list<Engine *>::iterator itr = eng->engines.begin();
        itr != eng->engines.end(); ++itr)
        if(dynamic_cast<MidiIn *>(*itr))
            sources.insert((*itr)->name);
    return sources;
}

set<string> Nio::getSinks(void)
{
    set<string> sinks;
    for(std::list<Engine *>::iterator itr = eng->engines.begin();
        itr != eng->engines.end(); ++itr)
        if(dynamic_cast<AudioOut *>(*itr))
            sinks.insert((*itr)->name);
    return sinks;
}

string Nio::getSource()
{
    return in->getSource();
}

string Nio::getSink()
{
    return out->getSink();
}

#if JACK
#include <jack/jack.h>
void Nio::preferredSampleRate(unsigned &rate)
{
#if __linux__
    //avoid checking in with jack if it's off
    FILE *ps = popen("ps aux", "r");
    char buffer[4096];
    while(fgets(buffer, sizeof(buffer), ps))
        if(strstr(buffer, "jack"))
            break;
    pclose(ps);

    if(!strstr(buffer, "jack"))
        return;
#endif

    jack_client_t *client = jack_client_open("temp-client",
                                             JackNoStartServer, 0);
    if(client) {
        rate = jack_get_sample_rate(client);
        jack_client_close(client);
    }
}
#else
void Nio::preferredSampleRate(unsigned &)
{}
#endif

void Nio::masterSwap(Master *master)
{
    in->setMaster(master);
    out->setMaster(master);
}

void Nio::waveNew(class WavFile *wave)
{
    out->wave->newFile(wave);
}

void Nio::waveStart(void)
{
    out->wave->Start();
}

void Nio::waveStop(void)
{
    out->wave->Stop();
}

void Nio::waveEnd(void)
{
    out->wave->destroyFile();
}

void Nio::setAudioCompressor(bool isEnabled)
{
    out->setAudioCompressor(isEnabled);
}

bool Nio::getAudioCompressor(void)
{
    return out->getAudioCompressor();
}

}

#include "Nio.h"
#include "OutMgr.h"
#include "InMgr.h"
#include "EngineMgr.h"
#include "MidiIn.h"
#include "AudioOut.h"
#include <iostream>
#include <algorithm>
using std::string;
using std::set;
using std::cerr;
using std::endl;

InMgr     *in  = NULL;
OutMgr    *out = NULL;
EngineMgr *eng = NULL;
string     postfix;

bool   Nio::autoConnect = false;
string Nio::defaultSource;
string Nio::defaultSink;

void Nio::init ( void )
{
    in  = &InMgr::getInstance(); //Enable input wrapper
    out = &OutMgr::getInstance(); //Initialize the Output Systems
    eng = &EngineMgr::getInstance(); //Initialize The Engines
}

bool Nio::start()
{
    init();
    return eng->start();
}

void Nio::stop()
{
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

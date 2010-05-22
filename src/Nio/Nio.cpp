#include "Nio.h"
#include "OutMgr.h"
#include "InMgr.h"
#include "EngineMgr.h"
#include "MidiIn.h"
#include "AudioOut.h"
#include <iostream>
using namespace std;

Nio &Nio::getInstance()
{
    static Nio instance;
    return instance;
}

Nio::Nio()
:in(InMgr::getInstance()),//Enable input wrapper
    out(OutMgr::getInstance()),//Initialize the Output Systems
    eng(EngineMgr::getInstance())//Initialize The Engines
{}

Nio::~Nio()
{
    stop();
}

void Nio::start()
{
    eng.start();//Drivers start your engines!
}

void Nio::stop()
{
    eng.stop();
}
    
int Nio::setDefaultSource(string name)
{
    if(name.empty())
        return 0;

    if(!eng.setInDefault(name)) {
        cerr << "There is no input for " << name << endl;
        return false;
    }
    return 0;
}
    

int Nio::setDefaultSink(string name)
{
    if(name.empty())
        return 0;

    if(!eng.setOutDefault(name)) {
        cerr << "There is no output for " << name << endl;
    }
    return 0;
}

bool Nio::setSource(string name)
{
     return in.setSource(name);
}

bool Nio::setSink(string name)
{
     return out.setSink(name);
}
        
set<string> Nio::getSources() const
{
    set<string> sources;
    for(list<Engine *>::iterator itr = eng.engines.begin();
            itr != eng.engines.end(); ++itr)
        if(dynamic_cast<MidiIn *>(*itr))
            sources.insert((*itr)->name);
    return sources;
}

set<string> Nio::getSinks() const
{
    set<string> sinks;
    for(list<Engine *>::iterator itr = eng.engines.begin();
            itr != eng.engines.end(); ++itr)
        if(dynamic_cast<AudioOut *>(*itr))
            sinks.insert((*itr)->name);
    return sinks;
}
        
string Nio::getSource() const
{
     return in.getSource();
}

string Nio::getSink() const
{
     return out.getSink();
}


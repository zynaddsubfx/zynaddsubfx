#include "EngineMgr.h"
#include <algorithm>
#include <iostream>
#include "InMgr.h"
#include "OutMgr.h"
#include "AudioOut.h"
#include "MidiIn.h"
#include "NulEngine.h"
#if OSS
#include "OssEngine.h"
#endif
#if ALSA
#include "AlsaEngine.h"
#endif
#if JACK
#include "JackEngine.h"
#endif
#if PORTAUDIO
#include "PaEngine.h"
#endif

using namespace std;

EngineMgr *sysEngine;

EngineMgr::EngineMgr()
{
    Engine *defaultEng = NULL;

    //conditional compiling mess (but contained)
    engines.push_back(defaultEng = new NulEngine(sysOut));
#if OSS
#if OSS_DEFAULT
    engines.push_back(defaultEng = new OssEngine(sysOut));
#else
    engines.push_back(new OssEngine(sysOut));
#endif
#endif
#if ALSA
#if ALSA_DEFAULT
    engines.push_back(defaultEng = new AlsaEngine(sysOut));
#else
    engines.push_back(new AlsaEngine(sysOut));
#endif
#endif
#if JACK
#if JACK_DEFAULT
    engines.push_back(defaultEng = new JackEngine(sysOut));
#else
    engines.push_back(new JackEngine(sysOut));
#endif
#endif
#if PORTAUDIO
#if PORTAUDIO_DEFAULT
    engines.push_back(defaultEng = new PaEngine(sysOut));
#else
    engines.push_back(new PaEngine(sysOut));
#endif
#endif

    defaultOut = dynamic_cast<AudioOut *>(defaultEng);

    defaultIn = dynamic_cast<MidiIn *>(defaultEng);
};

EngineMgr::~EngineMgr()
{
    for(list<Engine*>::iterator itr = engines.begin();
            itr != engines.end(); ++itr) {
            delete *itr;
    }
}

Engine *EngineMgr::getEng(string name)
{
    transform(name.begin(), name.end(), name.begin(), ::toupper);
    for(list<Engine*>::iterator itr = engines.begin();
            itr != engines.end(); ++itr) {
        if((*itr)->name == name) {
            return *itr;
        }
    }
    return NULL;
}

void EngineMgr::start()
{
    if(!(defaultOut&&defaultIn)) {
        cerr << "ERROR: It looks like someone broke the Nio Output\n"
             << "       Attempting to recover by defaulting to the\n"
             << "       Null Engine." << endl;
        defaultOut = dynamic_cast<AudioOut *>(getEng("NULL"));
        defaultIn  = dynamic_cast<MidiIn *>(getEng("NULL"));
    }

    sysOut->currentOut = defaultOut;
    sysIn->current     = defaultIn;

    //open up the default output(s)
    cout << "Starting Audio: " << defaultOut->name << endl;
    defaultOut->setAudioEn(true);
    if(defaultOut->getAudioEn()) {
        cout << "Audio Started" << endl;
    }
    else { 
        cerr << "ERROR: The default audio output failed to open!" << endl;
        sysOut->currentOut = dynamic_cast<AudioOut *>(sysEngine->getEng("NULL"));
        sysOut->currentOut->setAudioEn(true);
    }

    cout << "Starting MIDI: " << defaultIn->name << endl;
    defaultIn->setMidiEn(true);
    if(defaultIn->getMidiEn()) {
        cout << "MIDI Started" << endl;
    }
    else { //recover
        cerr << "ERROR: The default MIDI input failed to open!" << endl;
        sysIn->current = dynamic_cast<MidiIn *>(sysEngine->getEng("NULL"));
        sysIn->current->setMidiEn(true);
    }
}

void EngineMgr::stop()
{
    for(list<Engine*>::iterator itr = engines.begin();
            itr != engines.end(); ++itr)
        (*itr)->Stop();
}

bool EngineMgr::setInDefault(string name)
{
        MidiIn *chosen;
        if((chosen = dynamic_cast<MidiIn *>(getEng(name)))){ //got the input
                 defaultIn = chosen;
                 return true;
        }
        return false;
}

bool EngineMgr::setOutDefault(string name)
{
        AudioOut *chosen;
        if((chosen = dynamic_cast<AudioOut *>(getEng(name)))){ //got the output
                 defaultOut = chosen;
                 return true;
        }
        return false;
}



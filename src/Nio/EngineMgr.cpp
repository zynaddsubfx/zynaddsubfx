#include "EngineMgr.h"
#include <algorithm>
#include <iostream>
#include "AudioOut.h"
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
    cout << "jack go" << endl;
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


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
    engines["NULL"] = defaultEng = new NulEngine(sysOut);
#if OSS
#if OSS_DEFAULT
    engines["OSS"] = defaultEng = new OssEngine(sysOut);
#else
    engines["OSS"] = new OssEngine(sysOut);
#endif
#endif
#if ALSA
#if ALSA_DEFAULT
    engines["ALSA"] = defaultEng = new AlsaEngine(sysOut);
#else
    engines["ALSA"] = new AlsaEngine(sysOut);
#endif
#endif
#if JACK
#if JACK_DEFAULT
    engines["JACK"] = defaultEng = new JackEngine(sysOut);
#else
    engines["JACK"] = new JackEngine(sysOut);
#endif
#endif
#if PORTAUDIO
#if PORTAUDIO_DEFAULT
    engines["PA"] = defaultEng = new PaEngine(sysOut);
#else
    engines["PA"] = new PaEngine(sysOut);
#endif
#endif

};

EngineMgr::~EngineMgr()
{
    for(map<string, Engine*>::iterator itr = engines.begin();
            itr != engines.end(); ++itr) {
            delete itr->second;
    }
}

Engine *EngineMgr::getEng(string name)
{
    Engine *ans = NULL;

    transform(name.begin(), name.end(), name.begin(), ::toupper);
    for(map<string, Engine*>::iterator itr = engines.begin();
            itr != engines.end(); ++itr) {
        if(itr->first == name) {
            ans = itr->second;
            break;
        }
    }
    return ans;
}


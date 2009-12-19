#include "OutMgr.h"
#include <algorithm>
#include <iostream>
#include "AudioOut.h"
#include "../Misc/Master.h"
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

using namespace std;

OutMgr *sysOut;

OutMgr::OutMgr(Master *nmaster)
    :running(false),numRequests(0)
{
    master = nmaster;

    //initialize mutex
    pthread_mutex_init(&close_m,     NULL);
    pthread_mutex_init(&mutex,       NULL);
    pthread_mutex_init(&processing,  NULL);
    pthread_cond_init(&needsProcess, NULL);

    //init samples
    outr = new REALTYPE[SOUND_BUFFER_SIZE];
    outl = new REALTYPE[SOUND_BUFFER_SIZE];

    //conditional compiling mess (but contained)
    managedOuts["NULL"] = defaultOut = new NulEngine(this);
#if OSS
#if OSS_DEFAULT
    managedOuts["OSS"] = defaultOut = new OssEngine(this);
#else
    managedOuts["OSS"] = new OssEngine(this);
#endif
#endif
#if ALSA
#if ALSA_DEFAULT
    managedOuts["ALSA"] = defaultOut = new AlsaEngine(this);
#else
    managedOuts["ALSA"] = new AlsaEngine(this);
#endif
#endif
#if JACK
#if JACK_DEFAULT
    managedOuts["JACK"] = defaultOut = new JackEngine(this);
#else
    managedOuts["JACK"] = new JackEngine(this);
#endif
#endif
    defaultOut->out(Stereo<Sample>(Sample(SOUND_BUFFER_SIZE * 20, 0.0),
                                   Sample(SOUND_BUFFER_SIZE * 20, 0.0)));

};

OutMgr::~OutMgr()
{
    for(map<string,AudioOut*>::iterator itr = managedOuts.begin();
            itr != managedOuts.end(); ++itr) {
            itr->second->Stop();
    }
    running = false;
    pthread_mutex_lock(&processing);
    pthread_cond_signal(&needsProcess);
    pthread_mutex_unlock(&processing);
    pthread_mutex_lock(&close_m);
    pthread_mutex_unlock(&close_m);
    for(map<string,AudioOut*>::iterator itr = managedOuts.begin();
            itr != managedOuts.end(); ++itr) {
            delete itr->second;
    }
    pthread_mutex_destroy(&close_m);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&processing);
    pthread_cond_destroy(&needsProcess);
#warning TODO deallocate Engines (or have possible issues)
}

void *_outputThread(void *arg)
{
    return (static_cast<OutMgr*>(arg))->outputThread();
}

void *OutMgr::outputThread()
{

    pthread_mutex_lock(&close_m);
    //open up the default output
    if(!defaultOut->Start())//there should be a better failsafe
        cerr << "ERROR: The default Audio Output Failed to Open!" << endl;

    //setup
    running=true;
    init=true;
    bool doWait=false;
    while(running()) {

        if(false) {
            cout << "Status: ";
            pthread_mutex_lock(&mutex);
            cout << managedOuts.size() << "-";
            cout << unmanagedOuts.size();
            pthread_mutex_unlock(&mutex);
            cout << " outs, ";
            cout << doWait;
            cout << " waits, ";
            cout << numRequests();
            cout << " requests" << endl;
        }


        pthread_mutex_lock(&(master->mutex));
        master->AudioOut(outl,outr);
        pthread_mutex_unlock(&(master->mutex));

        smps = Stereo<Sample>(Sample(SOUND_BUFFER_SIZE, outl),
                Sample(SOUND_BUFFER_SIZE, outr));
        pthread_mutex_unlock(&processing);

        pthread_mutex_lock(&mutex);
        if(false)
            cout << "output to ";
        for(map<string,AudioOut*>::iterator itr = managedOuts.begin();
                itr != managedOuts.end(); ++itr) {
            if(itr->second->isEnabled())
                itr->second->out(smps);
            if(false)
                cout << itr->second << " ";
        }
        for(list<AudioOut*>::iterator itr = unmanagedOuts.begin();
                itr != unmanagedOuts.end(); ++itr) {
            (*itr)->out(smps);
            if(false)
                cout << *itr << " ";
        }
        if(false)
            cout << endl;
        pthread_mutex_unlock(&mutex);

        //wait for next run
        --numRequests;
        doWait = (numRequests()<1);
        pthread_mutex_lock(&processing);
        if(doWait) {
            pthread_cond_wait(&needsProcess, &processing);
            pthread_mutex_unlock(&processing);
        }
        else
            if(false)
                cout << "Run Forest Run!" << endl;

    }
    pthread_mutex_unlock(&close_m);
    return NULL;
}

void OutMgr::run()
{
    pthread_attr_t attr;
    //threadStop = false;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&outThread, &attr, _outputThread, this);
}

AudioOut *OutMgr::getOut(string name)
{
    transform(name.begin(), name.end(), name.begin(), ::toupper);
    return managedOuts[name];
}

void OutMgr::add(AudioOut *driver)
{
    pthread_mutex_lock(&mutex);
    unmanagedOuts.push_back(driver);
    if(running())//hotplug
        driver->Start();
    pthread_mutex_unlock(&mutex);
}

void OutMgr::remove(AudioOut *out)
{
    pthread_mutex_lock(&mutex);
    unmanagedOuts.remove(out);
    out->Stop();//tells engine to stop
    out->out(Stereo<Sample>(Sample(SOUND_BUFFER_SIZE),
                Sample(SOUND_BUFFER_SIZE)));//gives a dummy sample to make sure it is not stuck
    pthread_mutex_unlock(&mutex);
}

int OutMgr::getRunning()
{
    return numRequests();
}

int OutMgr::requestSamples(unsigned int n)
{
    numRequests = numRequests() + n;

    pthread_mutex_lock(&processing);
    pthread_cond_signal(&needsProcess);
    pthread_mutex_unlock(&processing);
    return 0;
}


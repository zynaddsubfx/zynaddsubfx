#include "OutMgr.h"
#include <algorithm>
#include <iostream>
#include "AudioOut.h"
#include "Engine.h"
#include "EngineMgr.h"
#include "../Misc/Master.h"
#include "../Misc/Util.h"//for set_realtime()

using namespace std;

OutMgr *sysOut;

OutMgr::OutMgr(Master *nmaster)
    :running(false)
{
    master = nmaster;

    //initialize mutex
    pthread_mutex_init(&mutex, NULL);
    sem_init(&requested, PTHREAD_PROCESS_PRIVATE, 0);

    //init samples
    outr = new REALTYPE[SOUND_BUFFER_SIZE];
    outl = new REALTYPE[SOUND_BUFFER_SIZE];
};

OutMgr::~OutMgr()
{
    for(map<string,AudioOut*>::iterator itr = managedOuts.begin();
            itr != managedOuts.end(); ++itr) {
            itr->second->Stop();
    }
    running = false;
    sem_post(&requested);

    pthread_join(outThread, NULL);

    pthread_mutex_destroy(&mutex);
    sem_destroy(&requested);
}

void *_outputThread(void *arg)
{
    return (static_cast<OutMgr*>(arg))->outputThread();
}

void *OutMgr::outputThread()
{
    defaultOut = dynamic_cast<AudioOut *>(sysEngine->defaultEng);
    if(!defaultOut) {
        cerr << "ERROR: It looks like someone broke the Nio Output\n"
             << "       Attempting to recover by defaulting to the\n"
             << "       Null Engine." << endl;
        defaultOut = dynamic_cast<AudioOut *>(sysEngine->getEng("NULL"));
    }

    set_realtime();
    //open up the default output
    if(!defaultOut->Start())//there should be a better failsafe
        cerr << "ERROR: The default Audio Output Failed to Open!" << endl;


    //setup
    running     = true;
    init        = true;
    while(running()) {

        if(false) {
            cout << "Status: ";
            pthread_mutex_lock(&mutex);
            cout << managedOuts.size() << "-";
            cout << unmanagedOuts.size();
            pthread_mutex_unlock(&mutex);
            cout << " outs, ";
            cout << getRunning();
            cout << " requests" << endl;
        }


        pthread_mutex_lock(&(master->mutex));
        master->AudioOut(outl,outr);
        pthread_mutex_unlock(&(master->mutex));

        smps = Stereo<Sample>(Sample(SOUND_BUFFER_SIZE, outl),
                Sample(SOUND_BUFFER_SIZE, outr));

        //this mutex might be redundant
        pthread_mutex_lock(&mutex);

        for(list<Engine*>::iterator itr = sysEngine->engines.begin();
                itr != sysEngine->engines.end(); ++itr) {
            AudioOut *out = dynamic_cast<AudioOut *>(*itr);
            if(out && out->isRunning() && out->getAudioEn())
                out->out(smps);
        }

        for(list<AudioOut*>::iterator itr = unmanagedOuts.begin();
                itr != unmanagedOuts.end(); ++itr) {
            (*itr)->out(smps);
        }

        pthread_mutex_unlock(&mutex);

        //wait for next run
        sem_wait(&requested);
    }
    pthread_exit(NULL);
    return NULL;
}

void OutMgr::run()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&outThread, &attr, _outputThread, this);
}

AudioOut *OutMgr::getOut(string name)
{
    return dynamic_cast<AudioOut *>(sysEngine->getEng(name));
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

    //gives a dummy sample to make sure it is not stuck
    out->out(Stereo<Sample>(Sample(SOUND_BUFFER_SIZE, 0.0),
                            Sample(SOUND_BUFFER_SIZE, 0.0)));
    pthread_mutex_unlock(&mutex);
}

int OutMgr::getRunning()
{
    int tmp;
    sem_getvalue(&requested, &tmp);
    if(tmp < 0)
        tmp = 0;
    return tmp;
}

void OutMgr::requestSamples(unsigned int n)
{
    for(unsigned int i = 0; i < n; ++i)
        sem_post(&requested);
}


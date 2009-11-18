#include "OutMgr.h"
#include <algorithm>
#include <iostream>
#include "AudioOut.h"
#include "../Misc/Master.h"
using namespace std;

OutMgr *sysOut;
//typedef enum
//{
//    JACK_OUTPUT;
//    ALSA_OUTPUT;
//    OSS_OUTPUT;
//    WINDOWS_OUTPUT;
//    WAV_OUTPUT;
//} outputDriver;

OutMgr::OutMgr(Master *nmaster)
    :numRequests(0)
{
    running = false;
    master = nmaster;
    //initialize mutex
    pthread_mutex_init(&mutex,       NULL);
    pthread_mutex_init(&processing,  NULL);
    //pthread_mutex_init(&request_m,   NULL);
    pthread_cond_init(&needsProcess, NULL);
    //init samples
    outr = new REALTYPE[SOUND_BUFFER_SIZE];
    outl = new REALTYPE[SOUND_BUFFER_SIZE];
};

OutMgr::~OutMgr()
{
    running = false;
    pthread_mutex_lock(&close_m);
    pthread_cond_wait(&close_cond, &close_m);
    pthread_mutex_unlock(&close_m);
}

void *_outputThread(void *arg)
{
    return (static_cast<OutMgr*>(arg))->outputThread();
}

void *OutMgr::outputThread()
{
    pthread_mutex_lock(&mutex);
    for(list<AudioOut*>::iterator itr = outs.begin(); itr != outs.end(); ++itr)
        (*itr)->Start();
    pthread_mutex_unlock(&mutex);

    running=true;
    init=true;
    bool doWait=false;
    int lRequests;
    while(running){
        //pthread_mutex_lock(&request_m);
        //lRequests=numRequests--;
        //pthread_mutex_unlock(&request_m);

        --numRequests;

        pthread_mutex_lock(&mutex);
        if(true)
        {
            cout << "Status: ";
            cout << outs.size();
            cout << " outs, ";
            cout << doWait;
            cout << " waits, ";
            cout << numRequests();
            cout << " requests" << endl;
        }
        pthread_mutex_unlock(&mutex);

        doWait = (numRequests()<1);

        pthread_mutex_lock(&processing);
        if(doWait) {
            pthread_cond_wait(&needsProcess, &processing);
        }
        else
            if(true)
                cout << "Run Forest Run!" << endl;

        pthread_mutex_lock(&(master->mutex));
        master->AudioOut(outl,outr);
        pthread_mutex_unlock(&(master->mutex));

        smps = Stereo<Sample>(Sample(SOUND_BUFFER_SIZE, outl),
                Sample(SOUND_BUFFER_SIZE, outr));
        pthread_mutex_unlock(&processing);

        pthread_mutex_lock(&mutex);
        if(false)
            cout << "output to ";
        for(list<AudioOut*>::iterator itr = outs.begin(); itr != outs.end(); ++itr) {
            (*itr)->out(smps);
            if(false)
                cout << *itr << " ";
        }
        if(false)
            cout << endl;
        pthread_mutex_unlock(&mutex);

    }
    pthread_mutex_lock(&close_m);
    pthread_cond_signal(&close_cond);
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


void OutMgr::add(AudioOut *driver)
{
    pthread_mutex_lock(&mutex);
    outs.push_back(driver);
    if(running)//hotplug
        driver->Start();
    pthread_mutex_unlock(&mutex);
}

void OutMgr::remove(AudioOut *out)
{
    pthread_mutex_lock(&mutex);
    outs.remove(out);
    out->Stop();//tells engine to stop
    out->out(Stereo<Sample>(Sample(SOUND_BUFFER_SIZE),
                Sample(SOUND_BUFFER_SIZE)));//gives a dummy sample to make sure it is not stuck
    pthread_mutex_unlock(&mutex);
}

int OutMgr::getRunning()
{
    //int tmp;
    //pthread_mutex_lock(&request_m);
    //tmp=eumRequests;
    //pthread_mutex_unlock(&request_m);
    return numRequests();//tmp;
}

int OutMgr::requestSamples()
{
    //pthread_mutex_lock(&request_m);
    ++numRequests;
    //pthread_mutex_unlock(&request_m);

    pthread_mutex_lock(&processing);
    pthread_cond_signal(&needsProcess);
    pthread_mutex_unlock(&processing);
    return 0;
}

//int OutMgr::enable(outputDriver out);
//int OutMgr::disable(outputDriver out);


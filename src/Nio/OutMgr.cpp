#include "OutMgr.h"
#include <algorithm>
#include <iostream>
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
{
    master = nmaster;
    //initialize mutex
    pthread_mutex_init(&mutex,       NULL);
    pthread_mutex_init(&processing,  NULL);
    pthread_cond_init(&needsProcess, NULL);
    //init samples
    outr = new REALTYPE[SOUND_BUFFER_SIZE];
    outl = new REALTYPE[SOUND_BUFFER_SIZE];
};

void *_outputThread(void *arg)
{
    return (static_cast<OutMgr*>(arg))->outputThread();
}

void *OutMgr::outputThread()
{
    pthread_mutex_lock(&mutex);
    cout << "run start" << endl;
    for(int i = 0; i < outs.size(); ++i)
        outs[i]->Start();
    cout << "running" << endl;
    pthread_mutex_unlock(&mutex);
    running=true;
    init=true;
    while(running){
        cout << "OutMgr THREAD" << endl;
        pthread_mutex_lock(&processing);
        cout << "OutMgr wait" << endl;
        if(init||pthread_cond_wait(&needsProcess, &processing));
        //init=false;FIXME
        //make master use samples
        cout << "have some food" << endl;
        master->AudioOut(outl,outr);
        smps = Stereo<Sample>(Sample(SOUND_BUFFER_SIZE, outl), 
                              Sample(SOUND_BUFFER_SIZE, outr));
        pthread_mutex_unlock(&processing);
        
        pthread_mutex_lock(&mutex);
        for(int i = 0; i < outs.size(); ++i)
            outs[i]->out(smps);
        pthread_mutex_unlock(&mutex);

    }
    return NULL;
}

void OutMgr::run()
{
    int chk;
    pthread_attr_t attr;
    //threadStop = false;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&outThread, &attr, _outputThread, this);
}


int OutMgr::add(AudioOut *driver)
{
    pthread_mutex_lock(&mutex);
    outs.push_back(driver);
    pthread_mutex_unlock(&mutex);
}

int OutMgr::remove(AudioOut *out)
{
    pthread_mutex_lock(&mutex);
    //vector<AudioOut>::iterator it;
    //outs.remove(out);
    pthread_mutex_unlock(&mutex);
}

int OutMgr::requestSamples()
{
    cout << "me hungry" << endl;
    pthread_mutex_lock(&processing);
    pthread_cond_signal(&needsProcess);
    cout << "me start fire" << endl;
    pthread_mutex_unlock(&processing);
}

//int OutMgr::enable(outputDriver out);
//int OutMgr::disable(outputDriver out);


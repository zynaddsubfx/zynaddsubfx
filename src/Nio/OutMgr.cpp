#include "OutMgr.h"
#include <algorithm>
#include <iostream>
#include "AudioOut.h"
#include "Engine.h"
#include "EngineMgr.h"
#include "InMgr.h"
#include "WavEngine.h"
#include "../Misc/Master.h"
#include "../Misc/Util.h" //for set_realtime()
#include "../Samples/Sample.h" //for resampling

using namespace std;

OutMgr &OutMgr::getInstance()
{
    static OutMgr instance;
    return instance;
}

OutMgr::OutMgr()
    :wave(new WavEngine()),
      priBuf(new float[4096],
             new float[4096]), priBuffCurrent(priBuf),
      master(Master::getInstance())
{
    currentOut = NULL;
    stales     = 0;
    master     = Master::getInstance();

    //init samples
    outr = new float[SOUND_BUFFER_SIZE];
    outl = new float[SOUND_BUFFER_SIZE];
}

OutMgr::~OutMgr()
{
    delete wave;
    delete [] priBuf.l;
    delete [] priBuf.r;
    delete [] outr;
    delete [] outl;
}

/* Sequence of a tick
 * 1) lets see if we have any stuff to do via midi
 * 2) Lets do that stuff
 * 3) Lets see if the event queue has anything for us
 * 4) Lets empty that out
 * 5) Lets remove old/stale samples
 * 6) Lets see if we need to generate samples
 * 7) Lets generate some
 * 8) Lets return those samples to the primary and secondary outputs
 * 9) Lets wait for another tick
 */
const Stereo<float *> OutMgr::tick(unsigned int frameSize)
{
    pthread_mutex_lock(&(master.mutex));
    InMgr::getInstance().flush();
    pthread_mutex_unlock(&(master.mutex));
    //SysEv->execute();
    removeStaleSmps();
    while(frameSize > storedSmps()) {
        pthread_mutex_lock(&(master.mutex));
        master.AudioOut(outl, outr);
        pthread_mutex_unlock(&(master.mutex));
        addSmps(outl, outr);
    }
    Stereo<float *> ans = priBuffCurrent;
    ans.l -= frameSize;
    ans.r -= frameSize;
    //cout << storedSmps() << '=' << frameSize << endl;
    return priBuf;
}

AudioOut *OutMgr::getOut(string name)
{
    return dynamic_cast<AudioOut *>(EngineMgr::getInstance().getEng(name));
}

string OutMgr::getDriver() const
{
    return currentOut->name;
}

bool OutMgr::setSink(string name)
{
    AudioOut *sink = getOut(name);

    if(!sink)
        return false;

    if(currentOut)
        currentOut->setAudioEn(false);

    currentOut = sink;
    currentOut->setAudioEn(true);

    bool success = currentOut->getAudioEn();

    //Keep system in a valid state (aka with a running driver)
    if(!success)
        (currentOut = getOut("NULL"))->setAudioEn(true);

    return success;
}

string OutMgr::getSink() const
{
    if(currentOut)
        return currentOut->name;
    else {
        cerr << "BUG: No current output in OutMgr " << __LINE__ << endl;
        return "ERROR";
    }
    return "ERROR";
}

void OutMgr::addSmps(float *l, float *r)
{
    //allow wave file to syphon off stream
    wave->push(Stereo<float *>(l, r), SOUND_BUFFER_SIZE);

    if(currentOut->getSampleRate() != SAMPLE_RATE) { //we need to resample
        //cout << "BAD RESAMPLING" << endl;
        Stereo<Sample> smps(Sample(SOUND_BUFFER_SIZE, l), Sample(
                                SOUND_BUFFER_SIZE,
                                r));
        smps.l.resample(SAMPLE_RATE, currentOut->getSampleRate());
        smps.r.resample(SAMPLE_RATE, currentOut->getSampleRate());
        memcpy(priBuffCurrent.l, smps.l.c_buf(), SOUND_BUFFER_SIZE
               * sizeof(float));
        memcpy(priBuffCurrent.r, smps.r.c_buf(), SOUND_BUFFER_SIZE
               * sizeof(float));
    }
    else { //just copy the samples
        memcpy(priBuffCurrent.l, l, SOUND_BUFFER_SIZE * sizeof(float));
        memcpy(priBuffCurrent.r, r, SOUND_BUFFER_SIZE * sizeof(float));
    }
    priBuffCurrent.l += SOUND_BUFFER_SIZE;
    priBuffCurrent.r += SOUND_BUFFER_SIZE;
    stales += SOUND_BUFFER_SIZE;
}

void OutMgr::removeStaleSmps()
{
    if(!stales)
        return;

    //memset is possibly unneeded
    memset(priBuf.l, '0', 4096 * sizeof(float));
    memset(priBuf.r, '0', 4096 * sizeof(float));
    priBuffCurrent = priBuf;
    stales = 0;
}

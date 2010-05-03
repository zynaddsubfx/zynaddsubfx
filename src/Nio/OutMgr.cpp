#include "OutMgr.h"
#include <algorithm>
#include <iostream>
#include "AudioOut.h"
#include "Engine.h"
#include "EngineMgr.h"
#include "InMgr.h"
#include "../Misc/Master.h"
#include "../Misc/Util.h"//for set_realtime()

using namespace std;

OutMgr *sysOut;

OutMgr::OutMgr(Master *nmaster)
    :priBuf(new REALTYPE[4096],new REALTYPE[4096]),priBuffCurrent(priBuf)
{
    currentOut = NULL;
    stales = 0;
    master = nmaster;

    //init samples
    outr = new REALTYPE[SOUND_BUFFER_SIZE];
    outl = new REALTYPE[SOUND_BUFFER_SIZE];
};

OutMgr::~OutMgr()
{
    delete [] outr;
    delete [] outl;
}

#if 0 //reenable to get secondary inputs working
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
#endif

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
const Stereo<REALTYPE *> OutMgr::tick(unsigned int frameSize)
{
    pthread_mutex_lock(&(master->mutex));
    sysIn->flush();
    pthread_mutex_unlock(&(master->mutex));
    //SysEv->execute();
    removeStaleSmps();
    while(frameSize > storedSmps()) {
        {//get more stuff
            pthread_mutex_lock(&(master->mutex));
            master->AudioOut(outl, outr);
            pthread_mutex_unlock(&(master->mutex));
            addSmps(outl,outr);
        }
    }
    makeStale(frameSize);
    return priBuf;
}

AudioOut *OutMgr::getOut(string name)
{
    return dynamic_cast<AudioOut *>(sysEngine->getEng(name));
}

string OutMgr::getDriver() const
{
    return currentOut->name;
}

void OutMgr::run()
{
    defaultOut = dynamic_cast<AudioOut *>(sysEngine->defaultEng);
    if(!defaultOut) {
        cerr << "ERROR: It looks like someone broke the Nio Output\n"
             << "       Attempting to recover by defaulting to the\n"
             << "       Null Engine." << endl;
        defaultOut = dynamic_cast<AudioOut *>(sysEngine->getEng("NULL"));
    }

    currentOut = defaultOut;
    //open up the default output
    if(!defaultOut->Start()) {
        cerr << "ERROR: The default Audio Output Failed to Open!" << endl;
    }
    else {
        currentOut = defaultOut = dynamic_cast<AudioOut *>(sysEngine->getEng("NULL"));
        defaultOut->Start();
    }
}

bool OutMgr::setSink(string name)
{
    AudioOut *sink = NULL;
    for(list<Engine*>::iterator itr = sysEngine->engines.begin();
            itr != sysEngine->engines.end(); ++itr) {
        AudioOut *out = dynamic_cast<AudioOut *>(*itr);
        if(out && out->name == name)
            sink = out;
    }

    if(!sink)
        return false;

    if(currentOut)
        currentOut->setAudioEn(false);

    currentOut = sink;
    currentOut->setAudioEn(true);
    return currentOut->getAudioEn();
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

void OutMgr::addSmps(REALTYPE *l, REALTYPE *r)
{
    Stereo<Sample> smps(Sample(SOUND_BUFFER_SIZE, l), Sample(SOUND_BUFFER_SIZE, r));

    if(currentOut->getSampleRate() != SAMPLE_RATE) { //we need to resample
        smps.l().resample(SAMPLE_RATE,currentOut->getSampleRate());
        smps.r().resample(SAMPLE_RATE,currentOut->getSampleRate());
    }

    memcpy(priBuffCurrent.l(), smps.l().c_buf(), SOUND_BUFFER_SIZE*sizeof(REALTYPE));
    memcpy(priBuffCurrent.r(), smps.r().c_buf(), SOUND_BUFFER_SIZE*sizeof(REALTYPE));
    priBuffCurrent.l() += SOUND_BUFFER_SIZE;
    priBuffCurrent.r() += SOUND_BUFFER_SIZE;
}

void OutMgr::makeStale(unsigned int size)
{
    stales = size;
}

void OutMgr::removeStaleSmps()
{
    int toShift = priBuf.l() + stales - priBuffCurrent.l();
    memmove(priBuf.l(), priBuf.l()+stales, toShift);
    memmove(priBuf.r(), priBuf.r()+stales, toShift);
    priBuffCurrent.l() = toShift + priBuf.l();
    priBuffCurrent.r() = toShift + priBuf.r();
}


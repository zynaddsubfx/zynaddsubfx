/*
    AudioOut.cpp

    Copyright 2009, Alan Calvert

    This file is part of yoshimi, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    yoshimi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <cstring>

using namespace std;

#include "../Misc/Master.h"
#include "AudioOut.h"

AudioOut::AudioOut(OutMgr *out)
    :samplerate(SAMPLE_RATE),nsamples(SOUND_BUFFER_SIZE),
     manager(out),enabled(false)
{
    pthread_mutex_init(&outBuf_mutex, NULL);
    pthread_cond_init (&outBuf_cv, NULL);
}

AudioOut::~AudioOut()
{
#warning TODO destroy other mutex
}

void AudioOut::out(Stereo<Sample> smps)
{
    if(samplerate != SAMPLE_RATE) {
        smps.l().resample(SAMPLE_RATE,samplerate);//we need to resample
        smps.r().resample(SAMPLE_RATE,samplerate);//we need to resample
    }

    pthread_mutex_lock(&outBuf_mutex);
    outBuf.push_back(smps);
    pthread_cond_signal(&outBuf_cv);
    pthread_mutex_unlock(&outBuf_mutex);
}

const Stereo<Sample> AudioOut::popOne()
{
    const int BUFF_SIZE = 6;
    Stereo<Sample> ans;
    pthread_mutex_lock(&outBuf_mutex);
    bool isEmpty = outBuf.empty();
    pthread_mutex_unlock(&outBuf_mutex);

    if(isEmpty)//fetch samples if possible
    {
        if(manager->getRunning() < BUFF_SIZE)
            manager->requestSamples(BUFF_SIZE-manager->getRunning());
        if(true)
            cout << "-----------------Starvation------------------"<< endl;
        return current;
    }
    else
    {
        pthread_mutex_lock(&outBuf_mutex);
        ans = outBuf.front();
        outBuf.pop_front();
        if(outBuf.size()+manager->getRunning()<BUFF_SIZE)
            manager->requestSamples(BUFF_SIZE - (outBuf.size()
                        + manager->getRunning()));
        if(false)
            cout << "AudioOut "<< outBuf.size()<< '+' << manager->getRunning() << endl;
        pthread_mutex_unlock(&outBuf_mutex);
    }
    current=ans;
    return ans;
}

//hopefully this does not need to be called
//(it can cause a horrible mess with the current starvation behavior)
void AudioOut::putBack(const Stereo<Sample> smp)
{
    pthread_mutex_lock(&outBuf_mutex);
    outBuf.push_front(smp);
    pthread_mutex_unlock(&outBuf_mutex);
}

const Stereo<Sample> AudioOut::getNext(int smps)
{

    if(smps<1)
        smps = nsamples;

    Stereo<Sample> ans = popOne();

    //if everything is perfectly configured, this should not need to loop
    while(ans.l().size()!=smps) {
        if(ans.l().size() > smps) {
            //subsample/putback excess
#warning TODO check for off by one errors
            putBack(Stereo<Sample>(ans.l().subSample(smps,ans.l().size()),
                                   ans.l().subSample(smps,ans.l().size())));
            ans = Stereo<Sample>(ans.l().subSample(0,smps),
                                   ans.l().subSample(0,smps));
        }
        else {
            Stereo<Sample> next = popOne();
            ans.l().append(next.l());
            ans.r().append(next.r());
        }
    }

    return ans;
}

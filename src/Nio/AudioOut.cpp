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
    :manager(out),threadStop(false),enabled(false)
{
    cout << out;
    pthread_mutex_init(&outBuf_mutex, NULL);
    pthread_cond_init (&outBuf_cv, NULL);
}

void AudioOut::out(const Stereo<Sample> smps)
{
    pthread_mutex_lock(&outBuf_mutex);
    outBuf.push(smps);
    pthread_cond_signal(&outBuf_cv);
    pthread_mutex_unlock(&outBuf_mutex);
}

const Stereo<Sample> AudioOut::getNext()
{
    const int BUFF_SIZE = 4;
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
        outBuf.pop();
        if(outBuf.size()+manager->getRunning()<4)
            manager->requestSamples(BUFF_SIZE - (outBuf.size()
                        + manager->getRunning()));
        if(false)
            cout << "AudioOut "<< outBuf.size()<< '+' << manager->getRunning() << endl;
        pthread_mutex_unlock(&outBuf_mutex);
    }
    current=ans;
    return ans;
}

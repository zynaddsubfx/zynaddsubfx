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
    :manager(out),threadStop(false)
{
    cout << out;
    pthread_mutex_init(&outBuf_mutex, NULL);
    pthread_cond_init (&outBuf_cv, NULL);
}

//bool AudioOut::prepAudiobuffers(unsigned int nframes, bool with_interleaved)
//{
//    if (nframes > 0)
//    {
//        buffersize = nframes;
//        outl = new float[buffersize];
//        outr = new float[buffersize];
//        if (outl == NULL || outr == NULL)
//            goto bail_out;
//        memset(outl, 0, buffersize * sizeof(float));
//        memset(outr, 0, buffersize * sizeof(float));
//        if (with_interleaved)
//        {
//            shortInterleaved = new short int[buffersize * 2];
//            if (shortInterleaved == NULL)
//                goto bail_out;
//            memset(shortInterleaved, 0, buffersize * 2 * sizeof(short int));
//        }
//        return true;
//    }
//
//bail_out:
//    cerr << "Error, failed to allocate audio buffers, size " << buffersize << endl;
//    if (outl != NULL)
//        delete [] outl;
//    outl = NULL;
//    if (outr != NULL)
//        delete [] outr;
//    outr = NULL;
//    if (with_interleaved)
//    {
//        if (shortInterleaved != NULL)
//            delete [] shortInterleaved;
//        shortInterleaved = NULL;
//    }
//    return false;
//}
//
//
//bool AudioOut::getAudio(bool lockrequired)
//{
//    if (NULL != master && outr != NULL && outl != NULL)
//        return master->MasterAudio(outl, outr, lockrequired);
//    return false;
//}
//
//
//bool AudioOut::getAudioInterleaved(bool lockrequired)
//{
//    if (shortInterleaved != NULL)
//    {
//        if (getAudio(lockrequired))
//        {
//            int idx = 0;
//            double scaled;
//            for (int frame = 0; frame < buffersize; ++frame)
//            {   // with a nod to libsamplerate ...
//                scaled = outl[frame] * (8.0 * 0x10000000);
//                shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
//                scaled = outr[frame] * (8.0 * 0x10000000);
//                shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
//            }
//            return true;
//        }
//    }
//    return false;
//}
//
//
//void AudioOut::silenceBuffers()
//{
//        memset(outl, 0, buffersize * sizeof(float));
//        memset(outr, 0, buffersize * sizeof(float));
//}
//
//
//void AudioOut::dimBuffers()
//{
//    for (int i = 0; i < buffersize; ++i)
//    {
//        outl[i] *= 0.84033613;   // -1.5dB
//        outr[i] *= 0.84033613;
//    }
//}

void AudioOut::out(const Stereo<Sample> smps)
{
    pthread_mutex_lock(&outBuf_mutex);
    outBuf.push(smps);
    pthread_cond_signal(&outBuf_cv);
    pthread_mutex_unlock(&outBuf_mutex);
}

const Stereo<Sample> AudioOut::getNext()
{
    Stereo<Sample> ans;
    pthread_mutex_lock(&outBuf_mutex);
    bool isEmpty = outBuf.empty();
    pthread_mutex_unlock(&outBuf_mutex);
    if(isEmpty)//fetch samples if possible
    {
        //cerr << manager << endl;
        if(manager->getRunning()<4)
            manager->requestSamples();
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
            manager->requestSamples();
        if(true)
            cout << "AudioOut "<< outBuf.size()<< '+' << manager->getRunning() << endl;
        pthread_mutex_unlock(&outBuf_mutex);
    }
    current=ans;
    return ans;
}

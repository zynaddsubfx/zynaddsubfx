/*
    MusicIO.h

    Copyright 2009, Alan Calvert
    Copyright 2009, James Morris

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

#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

class AudioOut
{
    public:
        AudioOut();
        virtual ~AudioOut() { };

        virtual bool openAudio()=0;
        virtual bool Start()=0;
        virtual void Stop()=0;
        virtual void Close()=0;
        virtual void out(const Stereo<Sample> smps)=0;
        //bool prepAudiobuffers(unsigned int buffersize, bool with_interleaved);
        //void silenceBuffers();
        //void dimBuffers();
        /**Determines if new operator should/can be used*/
        //virtual bool isSingleton() const {return true;};

    protected:
        //float      *outl;
        //float      *outr;
        //short int  *shortInterleaved;
        //int buffersize;
};

#endif


#include "Sample.h"

Sample::Sample(const Sample &smp)
    : bufferSize(smp.bufferSize)
{
    buffer=new float[bufferSize];
    for(int i=0;i<bufferSize;++i)
        *(i+buffer)=*(i+smp.buffer);
}
    
Sample::Sample(const int &length)
    : bufferSize(length)
{
    if(length<1)
      bufferSize=1;
    buffer=new float[bufferSize];
    clear();
}

Sample::Sample(float *input,const int &length)
    : bufferSize(length)
{
    if(length>0){
        buffer=new float[length];
        for(int i=0;i<length;++i)
            *(buffer+i)=*(input+i);
    }
    else{
        buffer=new float[1];
        bufferSize=1;
        *buffer=0;
    }
}

Sample::~Sample()
{
  delete[] buffer;
}

void Sample::clear()
{
    for(int i=0;i<bufferSize;++i)
        *(i+buffer)=0;
}

void Sample::operator=(const Sample &smp)
{
    /**\todo rewrite to be less repetitive*/
    if(bufferSize==smp.bufferSize)
    {
        for(int i=0;i<bufferSize;++i)
            *(i+buffer)=*(i+smp.buffer);
    }
    else
    {
        delete[] buffer;
        buffer=new float[smp.bufferSize];
        bufferSize=smp.bufferSize;
        for(int i=0;i<bufferSize;++i)
            *(i+buffer)=*(i+smp.buffer);
    }
}


/*
  ZynAddSubFX - a software synthesizer
 
  Recorder.C - Records sound to a file
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License 
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "Recorder.h"

Recorder::Recorder(){
    recordbuf_16bit=new short int [SOUND_BUFFER_SIZE*2];
    status=0;file=-1;
    sampleswritten=0;
    notetrigger=0;
    for (int i=0;i<SOUND_BUFFER_SIZE*2;i++){
	recordbuf_16bit[i]=0;
    };
};

Recorder::~Recorder(){
    if (recording()==1) stop();
    delete [] recordbuf_16bit;
};

int Recorder::preparefile(char *filename_,int overwrite){
    if (overwrite==0) file=open(filename_,O_CREAT|O_EXCL|O_WRONLY|O_BINARY,00444+00222);
	    else file=open(filename_,O_CREAT|O_WRONLY|O_TRUNC|O_BINARY,00444+00222);//overwrite if the file exists
    if (file==-1) {
	if (errno==EEXIST) return(1);//file exists already
	    else return(2);//Access Denied or any other problem
    };    
    status=1;//ready
    
    //prepare the space fot the wav header
    //the header itself, will be written when the file is closed
    unsigned char zerobuf[44]; for (int i=0;i<44;i++) zerobuf[i]=0;
    write(file,zerobuf,44);

    return(0);
};

void Recorder::start(){
    notetrigger=0;
    status=2;//recording
};

void Recorder::stop(){
    unsigned int chunksize;
    lseek(file,0,SEEK_SET);

    write(file,"RIFF",4);
    chunksize=sampleswritten*4+36;
    write(file,&chunksize,4);

    write(file,"WAVEfmt ",8);
    chunksize=16;
     write(file,&chunksize,4);
    unsigned short int formattag=1;//uncompresed wave
     write(file,&formattag,2);
    unsigned short int nchannels=2;//stereo
     write(file,&nchannels,2);
    unsigned int samplerate=SAMPLE_RATE;//samplerate
     write(file,&samplerate,4);
    unsigned int bytespersec=SAMPLE_RATE*4;//bytes/sec
     write(file,&bytespersec,4);
    unsigned short int blockalign=4;//2 channels * 16 bits/8
     write(file,&blockalign,2);
    unsigned short int bitspersample=16;
     write(file,&bitspersample,2);
    
    write(file,"data",4);
    chunksize=sampleswritten*blockalign;
    write(file,&chunksize,4);

    close(file);
    file=-1;
    status=0;
    sampleswritten=0;
};

void Recorder::pause(){
    status=0;
};

int Recorder::recording(){
    if ((status==2)&&(notetrigger!=0)) return(1);
	else return(0);
};

void Recorder::recordbuffer(REALTYPE *outl,REALTYPE *outr){
    int tmp;
    if (status!=2) return;
    for (int i=0;i<SOUND_BUFFER_SIZE;i++){
	tmp=(int)(outl[i]*32767.0);
	if (tmp<-32768) tmp=-32768;
	if (tmp>32767) tmp=32767;
	recordbuf_16bit[i*2]=tmp;

	tmp=(int)(outr[i]*32767.0);
	if (tmp<-32768) tmp=-32768;
	if (tmp>32767) tmp=32767;
	recordbuf_16bit[i*2+1]=tmp;
    };
    if (write(file,recordbuf_16bit,SOUND_BUFFER_SIZE*4)<SOUND_BUFFER_SIZE*4) {
	fprintf(stderr,"Error while recording !\n");
	stop();
	};
    sampleswritten+=SOUND_BUFFER_SIZE;
};

void Recorder::triggernow(){
    if (status==2) notetrigger=1;
};

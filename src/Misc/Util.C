/*
  ZynAddSubFX - a software synthesizer
 
  Util.C - Miscellaneous functions
  Copyright (C) 2002-2004 Nasca Octavian Paul
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

#include "Util.h"
#include <math.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int SAMPLE_RATE=44100;
int SOUND_BUFFER_SIZE=256;
int OSCIL_SIZE=512;

Buffer slbuf;//the Buffer used for save/load parameters to/from disk

Buffer masterdefaultsbuf;//this is used to store all parameters at start of program and reload when I need to clear all data (eg. before loading master parameters)
Buffer instrumentdefaultsbuf;//this is used to store all instrument parameters at start of program and reload when I need to clear all data (eg. before loading instrument parameters)
Buffer clipboardbuf;//buffer used for the clipboard

Config config;
REALTYPE *denormalkillbuf;


/*
 * Transform the velocity according the scaling parameter (velocity sensing)
 */
REALTYPE VelF(REALTYPE velocity,unsigned char scaling){
    REALTYPE x;
    x=pow(VELOCITY_MAX_SCALE,(64.0-scaling)/64.0);
    if ((scaling==127)||(velocity>0.99)) return(1.0);
       else return(pow(velocity,x));
};

/*
 * Get the detune in cents 
 */
REALTYPE getdetune(unsigned char type,unsigned short int coarsedetune,unsigned short int finedetune){
    REALTYPE det=0.0,octdet=0.0,cdet=0.0,findet=0.0;
    //Get Octave
    int octave=coarsedetune/1024;
    if (octave>=8) octave-=16;
    octdet=octave*1200.0;

    //Coarse and fine detune
    int cdetune=coarsedetune%1024;
    if (cdetune>512) cdetune-=1024;
    
    int fdetune=finedetune-8192;

    switch (type){
//	case 1: is used for the default (see below)
	case 2:	cdet=fabs(cdetune*10.0);
		findet=fabs(fdetune/8192.0)*10.0;
		break;
	case 3:	cdet=fabs(cdetune*100);
		findet=pow(10,fabs(fdetune/8192.0)*3.0)/10.0-0.1;
		break;
	case 4:	cdet=fabs(cdetune*701.95500087);//perfect fifth
		findet=(pow(2,fabs(fdetune/8192.0)*12.0)-1.0)/4095*1200;
		break;
      //case ...: need to update N_DETUNE_TYPES, if you'll add more
	default:cdet=fabs(cdetune*50.0);
		findet=fabs(fdetune/8192.0)*35.0;//almost like "Paul's Sound Designer 2"
		break;
    };
    if (finedetune<8192) findet=-findet;
    if (cdetune<0) cdet=-cdet;
    
    det=octdet+cdet+findet;
    return(det);
};


/*
 * Waveshape (this is called by OscilGen::waveshape and Distorsion::process)
 */

void waveshapesmps(int n,REALTYPE *smps,unsigned char type,unsigned char drive){
    int i;
    REALTYPE ws=drive/127.0;
    REALTYPE tmpv;

    switch(type){
	case 1:	ws=pow(10,ws*ws*3.0)-1.0+0.001;//Arctangent
		for (i=0;i<n;i++) 
		    smps[i]=atan(smps[i]*ws)/atan(ws);
		break;
	case 2:	ws=ws*ws*32.0+0.0001;//Asymmetric
		if (ws<1.0) tmpv=sin(ws)+0.1;
			else tmpv=1.1;
		for (i=0;i<n;i++) {
		    smps[i]=sin(smps[i]*(0.1+ws-ws*smps[i]))/tmpv;
		};
		break;
	case 3:	ws=ws*ws*ws*20.0+0.0001;//Pow
		for (i=0;i<n;i++) {
		    smps[i]*=ws;
		    if (fabs(smps[i])<1.0) {
			smps[i]=(smps[i]-pow(smps[i],3.0))*3.0;
			if (ws<1.0) smps[i]/=ws;
		    } else smps[i]=0.0;
		};
		break;
	case 4:	ws=ws*ws*ws*32.0+0.0001;//Sine
		if (ws<1.57) tmpv=sin(ws);
			else tmpv=1.0;
		for (i=0;i<n;i++) smps[i]=sin(smps[i]*ws)/tmpv;
		break;
	case 5:	ws=ws*ws+0.000001;//Quantisize
		for (i=0;i<n;i++) 
		    smps[i]=floor(smps[i]/ws+0.5)*ws;
		break;
	case 6:	ws=ws*ws*ws*32+0.0001;//Zigzag
		if (ws<1.0) tmpv=sin(ws);
			else tmpv=1.0;
		for (i=0;i<n;i++) 
  		    smps[i]=asin(sin(smps[i]*ws))/tmpv;
		break;
	case 7:	ws=pow(2.0,-ws*ws*8.0); //Limiter
		for (i=0;i<n;i++) {
		    REALTYPE tmp=smps[i];
		    if (fabs(tmp)>ws) {
			if (tmp>=0.0) smps[i]=1.0;
			    else smps[i]=-1.0;
		    } else smps[i]/=ws;
		};
		break;
	case 8:	ws=pow(2.0,-ws*ws*8.0); //Upper Limiter
		for (i=0;i<n;i++) {
		    REALTYPE tmp=smps[i];
		    if (tmp>ws) smps[i]=ws;
		    smps[i]*=2.0;
		};
		break;
	case 9:	ws=pow(2.0,-ws*ws*8.0); //Lower Limiter
		for (i=0;i<n;i++) {
		    REALTYPE tmp=smps[i];
		    if (tmp<-ws) smps[i]=-ws;
		    smps[i]*=2.0;
		};
		break;
	case 10:ws=(pow(2.0,ws*6.0)-1.0)/pow(2.0,6.0); //Inverse Limiter
		for (i=0;i<n;i++) {
		    REALTYPE tmp=smps[i];
		    if (fabs(tmp)>ws) {
			if (tmp>=0.0) smps[i]=tmp-ws;
			    else smps[i]=tmp+ws;
		    } else smps[i]=0;
		};
		break;
	case 11:ws=pow(5,ws*ws*1.0)-1.0;//Clip
		for (i=0;i<n;i++) 
		    smps[i]=smps[i]*(ws+0.5)*0.9999-floor(0.5+smps[i]*(ws+0.5)*0.9999);
		break;
	case 12:ws=ws*ws*ws*30+0.001;//Asym2
		if (ws<0.3) tmpv=ws;
		   else tmpv=1.0;
		for (i=0;i<n;i++) {
		    REALTYPE tmp=smps[i]*ws;
		    if ((tmp>-2.0) && (tmp<1.0)) smps[i]=tmp*(1.0-tmp)*(tmp+2.0)/tmpv;
			else smps[i]=0.0;
		};
		break;
	case 13:ws=ws*ws*ws*32.0+0.0001;//Pow2
		if (ws<1.0) tmpv=ws*(1+ws)/2.0;
			else tmpv=1.0;
		for (i=0;i<n;i++) {
		    REALTYPE tmp=smps[i]*ws;
		    if ((tmp>-1.0)&&(tmp<1.618034)) smps[i]=tmp*(1.0-tmp)/tmpv;
			else if (tmp>0.0) smps[i]=-1.0;
				else smps[i]=-2.0;
		};
		break;
	//update to Distorsion::changepar (Ptype max) if there is added more waveshapings functions
    };

};


/*
 * Save buffer to file
 */
int savebufferfile(Buffer *buf,const char *filename,int overwrite,int whatIsave){
    unsigned char *data=NULL;
    int n,file=0;
    
    n=buf->getsize();
    data=new unsigned char[n+1];
    buf->getalldata(n,data);
    
    if (overwrite==0) file=open(filename,O_CREAT|O_EXCL|O_WRONLY|O_BINARY,00444+00222);
	    else file=open(filename,O_CREAT|O_WRONLY|O_TRUNC|O_BINARY,00444+00222);//overwrite if the file exists
    if (file==-1) {
	if (errno==EEXIST) return(1);//file exists already
	    else return(2);//Access Denied or any other problem
    };

    //Save the id.
    char id[2]; id[0]='N';id[1]='P';
    write(file,&id,2);

    //Save the format descriptor (for future formats)
    unsigned char type=whatIsave,//what the data represents? ( Master/Effect/Voice 0x00..0x0f)
		  coding=0;//the encoding (Raw, 7 bit encoding,compressed) (0x00..0x07)
    unsigned char fmt=type+coding*0x10; //0x00..0x7f
    write(file,&fmt,1);
    
    //Save the CRC or 0 for no CRC
    unsigned CRC=0;//todo, if I do the CRC I do: crc=1+crc % 127;!
    write (file,&CRC,1);
    
    //Save the data
    int result=write(file,data,n);
    if (result== -1) return(2);
    close(file);
        
    delete (data);
    return(0);//OK
};

/*
 * Load the buffer from the file
 */
int loadbufferfile(Buffer *buf,const char *filename,int whatIload){
    int n=512,file=0,result,i;
    unsigned char b=0;    
    unsigned char *data=new unsigned char[n];

    buf->resetbuffer();
    buf->changemode(1);
    
    file=open(filename,O_RDONLY|O_BINARY,00444+00222);
    
    if (file==-1) {
	return(2);//Access Denied or any other problem
    };

    //Load the id.
    char id[2];
    read(file,&id,2);
    if ((id[0]!='N')||(id[1]!='P')) {
	close(file);
	return(3);//invalid data
    };
    
    //Load the format descriptor 
    unsigned char fmt; //0x00..0x7f
    read(file,&fmt,1);
    unsigned char type=fmt%0x10;
    if (type!=whatIload){
	close(file);
	return(4);//the data is loaded as something wrong (eg. a master is loaded as a instrument)
    };
//  coding=fmt/0x10;

    //load the crc
    unsigned char CRC; //0x00..0x7f
    read(file,&CRC,1);
//    if (CRC!=0){};CHECK IF IT IS OK

    //Load the data
    do {
	result=read(file,data,n);
	for (i=0;i<result;i++){
	    b=data[i];
	    buf->rwbyte(&b);//I need to write the data to a buffer and process it according to a format
	};
    } while (result!=0);
    
    if (result== -1) return(2);
    close(file);
    delete(data);
    return(0);//OK
};


/*
  ZynAddSubFX - a software synthesizer
 
  Resonance.C - Resonance
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

#include <math.h>
#include <stdlib.h>
#include "Resonance.h"


#include <stdio.h>

Resonance::Resonance(){
    defaults();
};

Resonance::~Resonance(){
};


void Resonance::defaults(){
    Penabled=0;
    PmaxdB=20;
    Pcenterfreq=64;//1 kHz
    Poctavesfreq=64;
    Pprotectthefundamental=0;
    ctlcenter=1.0;
    ctlbw=1.0;
    for (int i=0;i<N_RES_POINTS;i++) Prespoints[i]=64;
};

/*
 * Set a point of resonance function with a value
 */
void Resonance::setpoint(int n,unsigned char p){
    if ((n<0)||(n>=N_RES_POINTS)) return;
    Prespoints[n]=p;
};

/*
 * Apply the resonance to FFT data
 */
void Resonance::applyres(int n,REALTYPE *fftdata,REALTYPE freq){
    if (Penabled==0) return;//if the resonance is disabled
    REALTYPE sum=0.0,
	     l1=log(getfreqx(0.0)*ctlcenter),
	     l2=log(2.0)*getoctavesfreq()*ctlbw;

    for (int i=0;i<N_RES_POINTS;i++) if (sum<Prespoints[i]) sum=Prespoints[i];
    if (sum<1.0) sum=1.0;

    for (int i=1;i<n;i++){
        REALTYPE x=(log(freq*i)-l1)/l2;//compute where the n-th hamonics fits to the graph
	if (x<0.0) x=0.0;

	x*=N_RES_POINTS;
	REALTYPE dx=x-floor(x);x=floor(x);
	int kx1=(int)x; if (kx1>=N_RES_POINTS) kx1=N_RES_POINTS-1;
	int kx2=kx1+1;if (kx2>=N_RES_POINTS) kx2=N_RES_POINTS-1;
	REALTYPE y=(Prespoints[kx1]*(1.0-dx)+Prespoints[kx2]*dx)/127.0-sum/127.0;
	
	y=pow(10.0,y*PmaxdB/20.0);
	
	if ((Pprotectthefundamental!=0)&&(i==1)) y=1.0;
	
        fftdata[i]*=y;
        fftdata[OSCIL_SIZE-i]*=y;
    };    
};

/*
 * Smooth the resonance function
 */
void Resonance::smooth(){
    REALTYPE old=Prespoints[0];
    for (int i=0;i<N_RES_POINTS;i++){
        old=old*0.4+Prespoints[i]*0.6;
	Prespoints[i]=(int) old;
    };
    old=Prespoints[N_RES_POINTS-1];
    for (int i=N_RES_POINTS-1;i>0;i--){
        old=old*0.4+Prespoints[i]*0.6;
	Prespoints[i]=(int) old+1;
	if (Prespoints[i]>127) Prespoints[i]=127;
    };
};

/*
 * Randomize the resonance function
 */
void Resonance::randomize(int type){
    int r=(int)(RND*127.0);
    for (int i=0;i<N_RES_POINTS;i++){
	Prespoints[i]=r;
	if ((RND<0.1)&&(type==0)) r=(int)(RND*127.0);
	if ((RND<0.3)&&(type==1)) r=(int)(RND*127.0);
	if (type==2) r=(int)(RND*127.0);
    };
    smooth();
};

/*
 * Interpolate the peaks
 */
void Resonance::interpolatepeaks(int type){
    int x1=0,y1=Prespoints[0];
    for (int i=1;i<N_RES_POINTS;i++){
	if ((Prespoints[i]!=64)||(i+1==N_RES_POINTS)){
	    int y2=Prespoints[i];
	    for (int k=0;k<i-x1;k++){
		float x=(float) k/(i-x1);
		if (type==0) x=(1-cos(x*PI))*0.5;
		Prespoints[x1+k]=(int)(y1*(1.0-x)+y2*x);
	    };
	    x1=i;
	    y1=y2;
	};
    };
};

/*
 * Get the frequency from x, where x is [0..1]; x is the x coordinate
 */
REALTYPE Resonance::getfreqx(REALTYPE x){
    if (x>1.0) x=1.0;
    REALTYPE octf=pow(2.0,getoctavesfreq());
    return(getcenterfreq()/sqrt(octf)*pow(octf,x));
};

/*
 * Get the x coordinate from frequency (used by the UI)
 */
REALTYPE Resonance::getfreqpos(REALTYPE freq){
    return((log(freq)-log(getfreqx(0.0)))/log(2.0)/getoctavesfreq());
};

/*
 * Get the center frequency of the resonance graph
 */
REALTYPE Resonance::getcenterfreq(){
    return(10000.0*pow(10,-(1.0-Pcenterfreq/127.0)*2.0));
};

/*
 * Get the number of octave that the resonance functions applies to
 */
REALTYPE Resonance::getoctavesfreq(){
    return(0.25+10.0*Poctavesfreq/127.0);
};

void Resonance::sendcontroller(MidiControllers ctl,REALTYPE par){
    if (ctl==C_resonance_center) ctlcenter=par;
	else ctlbw=par;
};


/*
 * Save or load the parameters to/from the buffer
 */
void Resonance::saveloadbuf(Buffer *buf){
    unsigned char npar,n,tmp;

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n( Resonance parameters) \n");
#endif    

    tmp=0xfe;
    buf->rwbyte(&tmp);//if tmp!=0xfe error


    for (n=0x80;n<0xf0;n++){
	if (buf->getmode()==0) {
	    buf->rwbyte(&npar);
	    n=0;//force a loop until the end of parameters (0xff)
	} else npar=n;
	if (npar==0xff) break;
	
	switch(npar){
	    //Resonance parameters
	    case 0x80:	buf->rwbytepar(n,&Penabled);
			break;
	    case 0x81:	buf->rwbytepar(n,&PmaxdB);
			break;
	    case 0x82:	buf->rwbytepar(n,&Pcenterfreq);
			break;
	    case 0x83:	buf->rwbytepar(n,&Poctavesfreq);
			break;
	    case 0x85:	buf->rwbytepar(n,&Pprotectthefundamental);
			break;
	    case 0xA0:	if (buf->getmode()!=0){
    			    if ((buf->getminimal()==0) || (Penabled!=0)){
				unsigned short int tmp2=N_RES_POINTS;
				buf->rwbyte(&npar);
				buf->rwword(&tmp2);
				for (int i=0;i<N_RES_POINTS;i++) buf->rwbyte(&Prespoints[i]);
			    };
			} else {
			    unsigned short int tmp2;
			    buf->rwword(&tmp2);
			    for (int i=0;i<tmp2;i++) 
			      if (i<N_RES_POINTS) buf->rwbyte(&Prespoints[i]);
			    	else buf->rwbyte(&tmp);
			      
			    
			};
			break;
	};
    };

    
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
};


void Resonance::add2XML(XMLwrapper *xml){
    xml->addparbool("enabled",Penabled);
    if (Penabled==0) return;

    xml->addpar("max_db",PmaxdB);
    xml->addpar("center_freq",Pcenterfreq);
    xml->addpar("octaves_freq",Poctavesfreq);
    xml->addparbool("protect_fundamental_frequency",Pprotectthefundamental);
    xml->addpar("resonance_points",N_RES_POINTS);
    for (int i=0;i<N_RES_POINTS;i++){
	xml->beginbranch("RESPOINT",i);
	    xml->addpar("val",Prespoints[i]);
	xml->endbranch();
    };
};


void Resonance::getfromXML(XMLwrapper *xml){
    Penabled=xml->getparbool("enabled",Penabled);

    PmaxdB=xml->getpar127("max_db",PmaxdB);
    Pcenterfreq=xml->getpar127("center_freq",Pcenterfreq);
    Poctavesfreq=xml->getpar127("octaves_freq",Poctavesfreq);
    Pprotectthefundamental=xml->getparbool("protect_fundamental_frequency",Pprotectthefundamental);
    for (int i=0;i<N_RES_POINTS;i++){
	if (xml->enterbranch("RESPOINT",i)==0) continue;
	    Prespoints[i]=xml->getpar127("val",Prespoints[i]);
	  xml->exitbranch();
    };
};



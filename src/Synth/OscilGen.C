/*
  ZynAddSubFX - a software synthesizer
 
  OscilGen.C - Waveform generator for ADnote
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

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "OscilGen.h"

#include "../Misc/Util.h"

OscilGen::OscilGen(FFTwrapper *fft_,Resonance *res_){
    fft=fft_;
    res=res_;
    oscilFFTfreqs=new REALTYPE[OSCIL_SIZE];

    basefuncFFTfreqsQ=NULL;
    basefuncFFTfreqs=NULL;
    outoscilFFTfreqs=NULL;

    defaults();
};

OscilGen::~OscilGen(){
    if (basefuncFFTfreqs!=NULL) delete [] basefuncFFTfreqs;
    if (basefuncFFTfreqsQ!=NULL) delete [] basefuncFFTfreqsQ;
    delete [] oscilFFTfreqs;
};


void OscilGen::defaults(){
    oldbasefunc=0;oldbasepar=64;oldhmagtype=0;oldwaveshapingfunction=0;oldwaveshaping=64,oldnormalizemethod=0;
    oldbasefuncmodulation=0;oldharmonicshift=0;
    for (int i=0;i<MAX_AD_HARMONICS;i++){
	hmag[i]=0.0;
	hphase[i]=0.0;
	Phmag[i]=64;
	Phphase[i]=64;
    };
    Phmag[0]=127;
    Phmagtype=0;
    Prand=64;//no randomness

    Pcurrentbasefunc=0;
    Pbasefuncpar=64;

    Pbasefuncmodulation=0;
    Pbasefuncmodulationpar1=64;
    Pbasefuncmodulationpar2=0;

    Pwaveshapingfunction=0;
    Pwaveshaping=64;
    Pnormalizemethod=2;
    Pfiltertype=0;
    Pfilterpar=64;
    Pfilterbeforews=0;
    Psatype=0;
    Psapar=64;

    Pamprandpower=64;
    Pamprandtype=0;
    
    Pharmonicshift=0;
    Pharmonicshiftfirst=0;
    
    if (basefuncFFTfreqsQ!=NULL) {
	delete(basefuncFFTfreqsQ);
	basefuncFFTfreqsQ=NULL;
    };
    if (basefuncFFTfreqs!=NULL){
	delete(basefuncFFTfreqs);
	basefuncFFTfreqs=NULL;
    };
    
    for (int i=0;i<OSCIL_SIZE;i++) oscilFFTfreqs[i]=0.0;
    oscilprepared=0;
    oldfilterpars=0;oldsapars=0;
    prepare();
};

/* 
 * Base Functions - START 
 */
REALTYPE OscilGen::basefunc_pulse(REALTYPE x,REALTYPE a){
    return((fmod(x,1.0)<a)?-1.0:1.0);
};

REALTYPE OscilGen::basefunc_saw(REALTYPE x,REALTYPE a){
    if (a<0.00001) a=0.00001;
	else if (a>0.99999) a=0.99999;
    x=fmod(x,1);
    if (x<a) return(x/a*2.0-1.0);
	else return((1.0-x)/(1.0-a)*2.0-1.0);
};

REALTYPE OscilGen::basefunc_triangle(REALTYPE x,REALTYPE a){
    x=fmod(x+0.25,1);
    a=1-a;
    if (a<0.00001) a=0.00001;
    if (x<0.5) x=x*4-1.0;
	else x=(1.0-x)*4-1.0;
    x/=-a;
    if (x<-1.0) x=-1.0;
    if (x>1.0) x=1.0;
    return(x);
};

REALTYPE OscilGen::basefunc_power(REALTYPE x,REALTYPE a){
    x=fmod(x,1);
    if (a<0.00001) a=0.00001;
	else if (a>0.99999) a=0.99999;
    return(pow(x,exp((a-0.5)*10.0))*2.0-1.0);
};

REALTYPE OscilGen::basefunc_gauss(REALTYPE x,REALTYPE a){
    x=fmod(x,1)*2.0-1.0;
    if (a<0.00001) a=0.00001;
    return(exp(-x*x*(exp(a*8)+5.0))*2.0-1.0);
};

REALTYPE OscilGen::basefunc_diode(REALTYPE x,REALTYPE a){
    if (a<0.00001) a=0.00001;
	else if (a>0.99999) a=0.99999;
    a=a*2.0-1.0;
    x=cos((x+0.5)*2.0*PI)-a;
    if (x<0.0) x=0.0;
    return(x/(1.0-a)*2-1.0);
};

REALTYPE OscilGen::basefunc_abssine(REALTYPE x,REALTYPE a){
    x=fmod(x,1);
    if (a<0.00001) a=0.00001;
	else if (a>0.99999) a=0.99999;
    return(sin(pow(x,exp((a-0.5)*5.0))*PI)*2.0-1.0);
};

REALTYPE OscilGen::basefunc_pulsesine(REALTYPE x,REALTYPE a){
    if (a<0.00001) a=0.00001;
    x=(fmod(x,1)-0.5)*exp((a-0.5)*log(128));
    if (x<-0.5) x=-0.5;
	else if (x>0.5) x=0.5;
    x=sin(x*PI*2.0);
    return(x);
};

REALTYPE OscilGen::basefunc_stretchsine(REALTYPE x,REALTYPE a){
    x=fmod(x+0.5,1)*2.0-1.0;
    a=(a-0.5)*4;if (a>0.0) a*=2;
    a=pow(3.0,a);
    REALTYPE b=pow(fabs(x),a);
    if (x<0) b=-b;
    return(-sin(b*PI));
};

REALTYPE OscilGen::basefunc_chirp(REALTYPE x,REALTYPE a){
    x=fmod(x,1.0)*2.0*PI;
    a=(a-0.5)*4;if (a<0.0) a*=2.0;
    a=pow(3.0,a);
    return(sin(x/2.0)*sin(a*x*x));
};

REALTYPE OscilGen::basefunc_absstretchsine(REALTYPE x,REALTYPE a){
    x=fmod(x+0.5,1)*2.0-1.0;
    a=(a-0.5)*9;
    a=pow(3.0,a);
    REALTYPE b=pow(fabs(x),a);
    if (x<0) b=-b;
    return(-pow(sin(b*PI),2));
};

REALTYPE OscilGen::basefunc_chebyshev(REALTYPE x,REALTYPE a){
    a=a*a*a*30.0+1.0;
    return(cos(acos(x*2.0-1.0)*a));
};
/* 
 * Base Functions - END
 */


/* 
 * Get the base function
 */
void OscilGen::getbasefunction(REALTYPE *smps){
    int i;    
    REALTYPE par=(Pbasefuncpar+0.5)/128.0;
    if (Pbasefuncpar==64) par=0.5;
    
    REALTYPE basefuncmodulationpar1=Pbasefuncmodulationpar1/127.0,
	     basefuncmodulationpar2=Pbasefuncmodulationpar2/127.0;

    switch(Pbasefuncmodulation){
        case 1:basefuncmodulationpar1=(pow(2,basefuncmodulationpar1*5.0)-1.0)/10.0;
	       basefuncmodulationpar2=floor((pow(2,basefuncmodulationpar2*5.0)-1.0));
	       if (basefuncmodulationpar2<0.9999) basefuncmodulationpar2=-1.0;
	    break;
        case 2:basefuncmodulationpar1=(pow(2,basefuncmodulationpar1*5.0)-1.0)/10.0;
	       basefuncmodulationpar2=1.0+floor((pow(2,basefuncmodulationpar2*5.0)-1.0));
    	    break;
    };

    for (i=0;i<OSCIL_SIZE;i++) {
	REALTYPE t=i*1.0/OSCIL_SIZE;

	switch(Pbasefuncmodulation){
	    case 1:t=t*basefuncmodulationpar2+sin(t*2.0*PI)*basefuncmodulationpar1;//rev
		break;
	    case 2:t=t+sin(t*2.0*PI*basefuncmodulationpar2)*basefuncmodulationpar1;//sine
		break;
	};
	
	t=t-floor(t);
	
	switch (Pcurrentbasefunc){
    	    case 1:smps[i]=basefunc_triangle(t,par);
	        break;
	    case 2:smps[i]=basefunc_pulse(t,par);
	        break;
	    case 3:smps[i]=basefunc_saw(t,par);
	        break;
	    case 4:smps[i]=basefunc_power(t,par);
	        break;
	    case 5:smps[i]=basefunc_gauss(t,par);
	        break;
	    case 6:smps[i]=basefunc_diode(t,par);
	        break;
	    case 7:smps[i]=basefunc_abssine(t,par);
	        break;
	    case 8:smps[i]=basefunc_pulsesine(t,par);
	        break;
	    case 9:smps[i]=basefunc_stretchsine(t,par);
	       break;
	    case 10:smps[i]=basefunc_chirp(t,par);
	       break;
	    case 11:smps[i]=basefunc_absstretchsine(t,par);
	       break;
	    case 12:smps[i]=basefunc_chebyshev(t,par);
	       break;
	    default:smps[i]=-sin(2.0*PI*i/OSCIL_SIZE);
	};
    };
};

/* 
 * Filter the basefunction
 */
void OscilGen::oscilfilter(){
    if (Pfiltertype==0) return;
    REALTYPE par=1.0-Pfilterpar/128.0;
    REALTYPE max=0.0;
    for (int i=0;i<OSCIL_SIZE/2-1;i++){
	REALTYPE gain=1.0;
	switch(Pfiltertype){
	    case 1: gain=pow(1.0-par*par*par,i);//lp
		    break;
	    case 2: gain=1.0-pow(1.0-par*par,i+1);//hp1
		    break;
	    case 3: if (par<0.2) par=par*0.25+0.15;
		    gain=1.0-pow(1.0-par*par*0.999+0.001,i*0.05*i+1.0);//hp1b
		    gain*=gain*gain;
		    break;
	    case 4: gain=i+1-pow(2,(1.0-par)*7.5);//bp1
		    gain=1.0/(1.0+gain*gain/(i+1.0));
		    gain*=gain;
		    if (gain<1e-5) gain=1e-5;
		    break;
	    case 5: gain=i+1-pow(2,(1.0-par)*7.5);//bs1
		    gain=pow(atan(gain/(i/10.0+1))/1.57,6);
		    break;
	    case 6: gain=(i+1>pow(2,(1.0-par)*10)?0.0:1.0);//lp2
		    break;
	    case 7: gain=(i+1>pow(2,(1.0-par)*7)?1.0:0.0);//hp2
		    if (Pfilterpar==0) gain=1.0;
		    break;
	    case 8: gain=(fabs(pow(2,(1.0-par)*7)-i)>i/2+1?0.0:1.0);//bp2
		    break;

	    case 9: gain=(fabs(pow(2,(1.0-par)*7)-i)<i/2+1?0.0:1.0);//bs2
		    break;
	    case 10:gain=cos(par*par*PI/2.0*i);//cos
		   gain*=gain;
		   break;
	};
	oscilFFTfreqs[OSCIL_SIZE-i-1]*=gain;
	oscilFFTfreqs[i+1]*=gain;
	REALTYPE tmp=oscilFFTfreqs[OSCIL_SIZE-i-1]*oscilFFTfreqs[OSCIL_SIZE-i-1]+
		     oscilFFTfreqs[i+1]*oscilFFTfreqs[i+1];
	if (max<tmp) max=tmp;
    };
    
    if (max<1e-10) max=1.0;
    for (int i=1;i<OSCIL_SIZE;i++) oscilFFTfreqs[i]/=max; 
};
 
/* 
 * Change the base function
 */
void OscilGen::changebasefunction(){
    if (basefuncFFTfreqs!=NULL) {
		delete(basefuncFFTfreqs);
		basefuncFFTfreqs=NULL;
    };

    if (basefuncFFTfreqs==NULL) basefuncFFTfreqs=new REALTYPE[OSCIL_SIZE];
    if (Pcurrentbasefunc!=0) {
        // I use basefuncfreq for temporary store of the time-domain data 
	// because I don't wish "eat" too much memory with another array
        getbasefunction(basefuncFFTfreqs);
	fft->smps2freqs(basefuncFFTfreqs,NULL);//perform FFT
	basefuncFFTfreqs[0]=0.0;
    } else {
	delete basefuncFFTfreqs;
	basefuncFFTfreqs=NULL;
    }
    oscilprepared=0;
    oldbasefunc=Pcurrentbasefunc;
    oldbasepar=Pbasefuncpar;
    oldbasefuncmodulation=Pbasefuncmodulation;
    oldbasefuncmodulationpar1=Pbasefuncmodulationpar1;
    oldbasefuncmodulationpar2=Pbasefuncmodulationpar2;
    
};

/* 
 * Waveshape
 */
void OscilGen::waveshape(){
    int i;

    oldwaveshapingfunction=Pwaveshapingfunction;
    oldwaveshaping=Pwaveshaping;
    if (Pwaveshapingfunction==0) return;

    oscilFFTfreqs[0]=0.0;//remove the DC
    //reduce the amplitude of the freqs near the nyquist
    for (i=1;i<OSCIL_SIZE/8;i++) {
	REALTYPE tmp=i/(OSCIL_SIZE/8.0);
	oscilFFTfreqs[OSCIL_SIZE/2+i-1]*=tmp;
	oscilFFTfreqs[OSCIL_SIZE/2-i+1]*=tmp;
    };
    fft->freqs2smps(oscilFFTfreqs,NULL);
    //now the oscilFFTfreqs contains *time-domain data* of samples
    //I don't want to allocate another array for this

    //Normalize
    REALTYPE max=0.0;
    for (i=0;i<OSCIL_SIZE;i++) 
	if (max<fabs(oscilFFTfreqs[i])) max=fabs(oscilFFTfreqs[i]);
    if (max<0.00001) max=1.0;
    max=1.0/max;for (i=0;i<OSCIL_SIZE;i++) oscilFFTfreqs[i]*=max;
    
    //Do the waveshaping
    waveshapesmps(OSCIL_SIZE,oscilFFTfreqs,Pwaveshapingfunction,Pwaveshaping);
    
    fft->smps2freqs(oscilFFTfreqs,NULL);//perform FFT
};

/* 
 * Adjust the spectrum
 */
void OscilGen::spectrumadjust(){
    if (Psatype==0) return;
    REALTYPE par=Psapar/127.0;
    switch(Psatype){
	case 1: par=1.0-par*2.0;
		if (par>=0.0) par=pow(5.0,par);
		    else par=pow(8.0,par);
		break;
	case 2: par=pow(10.0,(1.0-par)*3.0)*0.00095;
		break;
    };
    REALTYPE max=0.0;
    for (int i=0;i<OSCIL_SIZE/2-1;i++){ 
	REALTYPE tmp=pow(oscilFFTfreqs[OSCIL_SIZE-i-1],2)+pow(oscilFFTfreqs[i+1],2.0);
	if (max<tmp) max=tmp;
    };
    max=sqrt(max);
    if (max<1e-8) max=1.0;
    
    for (int i=0;i<OSCIL_SIZE/2-1;i++){
        REALTYPE mag=sqrt(pow(oscilFFTfreqs[OSCIL_SIZE-i-1],2)+pow(oscilFFTfreqs[i+1],2.0))/max;
	REALTYPE phase=atan2(oscilFFTfreqs[i+1],oscilFFTfreqs[OSCIL_SIZE-i-1]);
	
	switch (Psatype){
	    case 1: mag=pow(mag,par);
		    break;
	    case 2: if (mag<par) mag=0.0;
		    break;
	};
	oscilFFTfreqs[OSCIL_SIZE-i-1]=mag*cos(phase);
	oscilFFTfreqs[i+1]=mag*sin(phase);
    };
    
};

void OscilGen::shiftharmonics(){
    if (Pharmonicshift==0) return;
    
    REALTYPE hc,hs;
    int harmonicshift=-Pharmonicshift;
    
    if (harmonicshift>0){
	for (int i=OSCIL_SIZE/2-2;i>=0;i--){ 
	    int oldh=i-harmonicshift;
	    if (oldh<0){
		hc=0.0;
		hs=0.0;
	    } else {
		hc=oscilFFTfreqs[oldh+1];
		hs=oscilFFTfreqs[OSCIL_SIZE-oldh-1];
	    };
	    oscilFFTfreqs[i+1]=hc;
	    oscilFFTfreqs[OSCIL_SIZE-i-1]=hs;
	};
    } else {
	for (int i=0;i<OSCIL_SIZE/2-1;i++){ 
	    int oldh=i+abs(harmonicshift);
	    if (oldh>=(OSCIL_SIZE/2-1)){
		hc=0.0;
		hs=0.0;
	    } else {
		hc=oscilFFTfreqs[oldh+1];
		hs=oscilFFTfreqs[OSCIL_SIZE-oldh-1];
		if (fabs(hc)<0.000001) hc=0.0;
		if (fabs(hs)<0.000001) hs=0.0;
	    };
	    
	    oscilFFTfreqs[i+1]=hc;
	    oscilFFTfreqs[OSCIL_SIZE-i-1]=hs;
	};
    };
    
    oscilFFTfreqs[0]=0.0;
};

/* 
 * Prepare the Oscillator
 */
void OscilGen::prepare(){
   int i,j,k;
   REALTYPE a,b,c,d,hmagnew;

   
   if ((oldbasepar!=Pbasefuncpar)||(oldbasefunc!=Pcurrentbasefunc)||
	(oldbasefuncmodulation!=Pbasefuncmodulation)||
        (oldbasefuncmodulationpar1!=Pbasefuncmodulationpar1)||
	(oldbasefuncmodulationpar2!=Pbasefuncmodulationpar2)) 
	 changebasefunction();

   for (i=0;i<MAX_AD_HARMONICS;i++) hphase[i]=(Phphase[i]-64.0)/64.0*PI/(i+1);

   for (i=0;i<MAX_AD_HARMONICS;i++){
      hmagnew=1.0-fabs(Phmag[i]/64.0-1.0);
      switch(Phmagtype){
	case 1:hmag[i]=exp(hmagnew*log(0.01)); break;
	case 2:hmag[i]=exp(hmagnew*log(0.001));break;
	case 3:hmag[i]=exp(hmagnew*log(0.0001));break;
	case 4:hmag[i]=exp(hmagnew*log(0.00001));break;
	default:hmag[i]=1.0-hmagnew;
	        break; 
      };

      if (Phmag[i]<64) hmag[i]=-hmag[i];
   };
    
   //remove the harmonics where Phmag[i]==64
   for (i=0;i<MAX_AD_HARMONICS;i++) if (Phmag[i]==64) hmag[i]=0.0;


   for (i=0;i<OSCIL_SIZE;i++) oscilFFTfreqs[i]=0.0;
   if (Pcurrentbasefunc==0) {//the sine case
	for (i=0;i<MAX_AD_HARMONICS;i++){
	    oscilFFTfreqs[i+1]=-hmag[i]*sin(hphase[i]*(i+1))/2.0;
	    oscilFFTfreqs[OSCIL_SIZE-i-1]=hmag[i]*cos(hphase[i]*(i+1))/2.0;
	};
   } else {
	for (j=0;j<MAX_AD_HARMONICS;j++){
	    if (Phmag[j]==64) continue;
	    for (i=1;i<OSCIL_SIZE/2;i++){
		k=i*(j+1);if (k>OSCIL_SIZE/2) break;
		a=basefuncFFTfreqs[i];
		b=basefuncFFTfreqs[OSCIL_SIZE-i];
		c=hmag[j]*cos(hphase[j]*k);
		d=hmag[j]*sin(hphase[j]*k);
		oscilFFTfreqs[k]+=a*c-b*d;
		oscilFFTfreqs[OSCIL_SIZE-k]+=a*d+b*c;
	    };
	};

   };

   if (Pharmonicshiftfirst!=0)  shiftharmonics();

   if (Pfilterbeforews==0){
        waveshape();
	oscilfilter();
    } else {
	oscilfilter();
        waveshape();
    };

   spectrumadjust();
   if (Pharmonicshiftfirst==0)  shiftharmonics();

   //normalize (sum or RMS) - the "Full RMS" normalisation is located in the "::get()" function
   if ((Pnormalizemethod==0)||(Pnormalizemethod==1)){
        REALTYPE sum=0;
	for (j=1;j<OSCIL_SIZE/2;j++) {
	    REALTYPE term=oscilFFTfreqs[j]*oscilFFTfreqs[j]+oscilFFTfreqs[OSCIL_SIZE-j]*oscilFFTfreqs[OSCIL_SIZE-j];
	    if (Pnormalizemethod==0) sum+=sqrt(term);
		else sum+=term;
    	};
        if (sum<0.000001) sum=1.0;
	if (Pnormalizemethod==1) sum=sqrt(sum)*2.0;
	sum*=0.5;   
	for (j=1;j<OSCIL_SIZE;j++) oscilFFTfreqs[j]/=sum; 
   };

   oscilFFTfreqs[0]=0.0;//remove the DC

   if (ANTI_ALIAS==0) {
	//Perform the IFFT in case of Antialiasing is disabled
	REALTYPE *tmp=new REALTYPE[OSCIL_SIZE];
	for (i=0;i<OSCIL_SIZE;i++) tmp[i]=oscilFFTfreqs[i];
	fft->freqs2smps(tmp,oscilFFTfreqs);
	//now the oscilFFTfreqs contains samples data in TIME-DOMAIN (despite it's name)
	delete(tmp);
   };
   oldhmagtype=Phmagtype;
   oldnormalizemethod=Pnormalizemethod;
   oldharmonicshift=Pharmonicshift+Pharmonicshiftfirst*256;

   oscilprepared=1;
};

/* 
 * Get the oscillator function
 */
short int OscilGen::get(REALTYPE *smps,REALTYPE freqHz){
    return(this->get(smps,freqHz,0));
};

/* 
 * Get the oscillator function
 */
short int OscilGen::get(REALTYPE *smps,REALTYPE freqHz,int resonance){
    int i;
    int nyquist,outpos;
    
    if ((oldbasepar!=Pbasefuncpar)||(oldbasefunc!=Pcurrentbasefunc)||(oldhmagtype!=Phmagtype)
	||(oldwaveshaping!=Pwaveshaping)||(oldwaveshapingfunction!=Pwaveshapingfunction)) oscilprepared=0;
    if (oldnormalizemethod!=Pnormalizemethod) oscilprepared=0;
    if (oldfilterpars!=Pfiltertype*256+Pfilterpar+Pfilterbeforews*65536){ 
	oscilprepared=0;
	oldfilterpars=Pfiltertype*256+Pfilterpar+Pfilterbeforews*65536;
    };
    if (oldsapars!=Psatype*256+Psapar){ 
	oscilprepared=0;
	oldsapars=Psatype*256+Psapar;
    };

    if ((oldbasefuncmodulation!=Pbasefuncmodulation)||
        (oldbasefuncmodulationpar1!=Pbasefuncmodulationpar1)||
	(oldbasefuncmodulationpar2!=Pbasefuncmodulationpar2)) 
	    oscilprepared=0;

    if (oldharmonicshift!=Pharmonicshift+Pharmonicshiftfirst*256) oscilprepared=0;
    
    if (oscilprepared!=1) prepare();

    outpos=(int)((RND*2.0-1.0)*(REALTYPE) OSCIL_SIZE*(Prand-64.0)/64.0);
    outpos=(outpos+2*OSCIL_SIZE) % OSCIL_SIZE;
    
    if (ANTI_ALIAS==0) {//Anti-Aliasing OFF
	for (i=0;i<OSCIL_SIZE;i++) smps[i]=oscilFFTfreqs[i];
	if (Prand!=64) return(outpos);
	    else return(0);
    };

    //Anti-Aliasing ON
    outoscilFFTfreqs=new REALTYPE[OSCIL_SIZE];
    nyquist=(int)(0.5*SAMPLE_RATE/fabs(freqHz))+2;
    if (nyquist>OSCIL_SIZE/2) nyquist=OSCIL_SIZE/2;
    
    for (i=0;i<OSCIL_SIZE;i++) outoscilFFTfreqs[i]=0.0;

    // Randomness (each harmonic), the block type is computed 
    // in ADnote by setting start position according to this setting
    if ((Prand>64)&&(freqHz>=0.0)){
        REALTYPE rnd,angle,a,b,c,d;
        rnd=PI*pow((Prand-64.0)/64.0,2.0);
        for (i=1;i<nyquist-1;i++){//to Nyquist only for AntiAliasing
    	    angle=rnd*i*RND;
	    a=oscilFFTfreqs[i];
	    b=oscilFFTfreqs[OSCIL_SIZE-i];
	    c=cos(angle);
	    d=sin(angle);
	    outoscilFFTfreqs[i]=a*c-b*d;
	    outoscilFFTfreqs[OSCIL_SIZE-i]=a*d+b*c;
	};	
    } else {
	for (i=1;i<nyquist-1;i++) {
	    outoscilFFTfreqs[i]=oscilFFTfreqs[i];
	    outoscilFFTfreqs[OSCIL_SIZE-i]=oscilFFTfreqs[OSCIL_SIZE-i];
	};
    };

    //Harmonic Amplitude Randomness
    if (freqHz>0.1) {
	REALTYPE power=Pamprandpower/127.0;
	REALTYPE normalize=1.0/(1.2-power);
	switch (Pamprandtype){
	    case 1: power=power*2.0-0.5;
		    power=pow(15.0,power);
		    for (i=1;i<nyquist-1;i++){
    	    		REALTYPE amp=pow(RND,power)*normalize;
			outoscilFFTfreqs[i]*=amp;
			outoscilFFTfreqs[OSCIL_SIZE-i]*=amp;
		    };
		    break;
	    case 2: power=power*2.0-0.5;
		    power=pow(15.0,power)*2.0;
		    REALTYPE rndfreq=2*PI*RND;
		    for (i=1;i<nyquist-1;i++){
    	    		REALTYPE amp=pow(fabs(sin(i*rndfreq)),power)*normalize;
			outoscilFFTfreqs[i]*=amp;
			outoscilFFTfreqs[OSCIL_SIZE-i]*=amp;
		    };
		    break;
	};	
    };


    if ((freqHz>0.1)&&(resonance!=0)) res->applyres(nyquist-1,outoscilFFTfreqs,freqHz);

    //Full RMS normalize
    if (Pnormalizemethod==2){
        REALTYPE sum=0;
	for (int j=1;j<OSCIL_SIZE/2;j++) {
	    REALTYPE term=outoscilFFTfreqs[j]*outoscilFFTfreqs[j]+outoscilFFTfreqs[OSCIL_SIZE-j]*outoscilFFTfreqs[OSCIL_SIZE-j];
	    sum+=term;
    	};
        if (sum<0.000001) sum=1.0;	
	sum=sqrt(sum);
	
	if ((freqHz>0.1)&&(resonance!=0)&&(res->Penabled!=0)) sum/=dB2rap(res->Pgain*0.5);//apply the ressonance gain
	
	for (int j=1;j<OSCIL_SIZE;j++) outoscilFFTfreqs[j]/=sum; 
   };


//    for (i=0;i<OSCIL_SIZE/2;i++) outoscilFFTfreqs[i+OSCIL_SIZE/2]*=-1.0;//correct the amplitude

    fft->freqs2smps(outoscilFFTfreqs,smps);

    for (i=0;i<OSCIL_SIZE;i++) smps[i]*=0.25;//correct the amplitude

    delete(outoscilFFTfreqs);
    outoscilFFTfreqs=NULL;
    if (Prand<64) return(outpos);
	else return(0);
};


/* 
 * Get the spectrum of the oscillator for the UI
 */
void OscilGen::getspectrum(int n, REALTYPE *spc,int what){
    if (n>OSCIL_SIZE/2) n=OSCIL_SIZE/2;
    for (int i=1;i<n;i++){
	if (what==0){
	    if (ANTI_ALIAS==0) spc[i-1]=0.0;//there is no spectrum available
	    else spc[i-1]=sqrt(oscilFFTfreqs[i]*oscilFFTfreqs[i]+
	    	 oscilFFTfreqs[OSCIL_SIZE-i]*oscilFFTfreqs[OSCIL_SIZE-i]);
	} else {
	    if (Pcurrentbasefunc==0) spc[i-1]=((i==1)?(1.0):(0.0));
	    else spc[i-1]=sqrt(basefuncFFTfreqs[i]*basefuncFFTfreqs[i]+
	    	 basefuncFFTfreqs[OSCIL_SIZE-i]*basefuncFFTfreqs[OSCIL_SIZE-i]);
	};
    };
}


/* 
 * Convert the oscillator as base function
 */
void OscilGen::useasbase(){
   int i;

   if (Pcurrentbasefunc==0) basefuncFFTfreqs=new REALTYPE[OSCIL_SIZE];
   
   for (i=0;i<OSCIL_SIZE;i++) basefuncFFTfreqs[i]=oscilFFTfreqs[i];

   oldbasefunc=Pcurrentbasefunc=127;

   prepare();
};


/* 
 * Get the base function for UI
 */
void OscilGen::getcurrentbasefunction(REALTYPE *smps){
    if (Pcurrentbasefunc!=0) {
	for (int i=0;i<OSCIL_SIZE;i++) smps[i]=basefuncFFTfreqs[i];
	fft->freqs2smps(smps,NULL);
    } else getbasefunction(smps);//the sine case
};


/* 
 * Quantisize and store the base function frequencies (used to save basefunction to buffer)
 */
void OscilGen::savebasefuncQ(){
    int i;
    short int data;
    REALTYPE x,max;
    max=0.0;
    
    if (basefuncFFTfreqsQ==NULL) 
	basefuncFFTfreqsQ=new unsigned short int[OSCIL_SIZE];
    for (i=0;i<OSCIL_SIZE;i++) basefuncFFTfreqsQ[i]=8192;//"8192"=0.0

    if (Pcurrentbasefunc==0) return;

    for (i=1;i<OSCIL_SIZE;i++) 
	if (max<fabs(basefuncFFTfreqs[i])) max=fabs(basefuncFFTfreqs[i]);
    if (max<0.00000001) max=1.0;
    
    for (i=1;i<OSCIL_SIZE/2;i++) {
	//Cosine Component
	x=basefuncFFTfreqs[i]/max;
	if (fabs(x)<0.00001) data=0;// -100dB
		else {
		    data=(short int) ((1.0+rap2dB(fabs(x))/100.0)*8191.0);
		    if (x<0.0) data=-data;
		};
	basefuncFFTfreqsQ[i*2-1]=data+8192;

	//Sine Component
	if (i!=(OSCIL_SIZE/2-1)) {
	    x=basefuncFFTfreqs[OSCIL_SIZE-i]/max;
	    if (fabs(x)<0.00001) data=0;// -100dB
		else {
		    data=(short int) ((1.0+rap2dB(fabs(x))/100.0)*8191.0);
		    if (x<0.0) data=-data;
		};
	    basefuncFFTfreqsQ[i*2]=data+8192;
	};
    };
};

/* 
 * Load the base function frequencies from quantised data (used to load basefunction from buffer)
 */
void OscilGen::loadbasefuncQ(){
    short int data,i;
    REALTYPE x;
    
    if (basefuncFFTfreqs==NULL) basefuncFFTfreqs=new REALTYPE[OSCIL_SIZE];
    for (i=0;i<OSCIL_SIZE;i++) basefuncFFTfreqs[i]=0.0;
    if (basefuncFFTfreqsQ==NULL) return;

    if (Pcurrentbasefunc==0) basefuncFFTfreqs=new REALTYPE[OSCIL_SIZE];
  
    for (i=0;i<OSCIL_SIZE;i++) basefuncFFTfreqs[i]=0.0;;

    
    for (i=1;i<OSCIL_SIZE/2;i++) {
	//Cosine Component
	data=basefuncFFTfreqsQ[i*2-1];
	if (data==8192) x=0.0;
		else {
		    x=dB2rap((fabs(data-8192.0)/8192-1.0)*100.0);
		    if (data<8192) x=-x;
		};
	basefuncFFTfreqs[i]=x;

	//Sine Component
	data=basefuncFFTfreqsQ[i*2];
	if (data==8192) x=0.0;
		else {
		    x=dB2rap((fabs(data-8192.0)/8192-1.0)*100.0);
		    if (data<8192) x=-x;
		};
	basefuncFFTfreqs[OSCIL_SIZE-i]=x;

    };
    
    delete basefuncFFTfreqsQ;
    basefuncFFTfreqsQ=NULL;
    oldbasefunc=Pcurrentbasefunc=127;
    oldbasepar=Pbasefuncpar;
};


/*
 * Save or load the parameters to/from the buffer
 */
void OscilGen::saveloadbuf(Buffer *buf){
    unsigned char npar,n,nharmonic,tmp;
    unsigned short int nh,tmpw;

    int custombasefuncloaded=0;
#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n( Oscil parameters) \n");
#endif    

    tmp=0xfe;
    buf->rwbyte(&tmp);//if tmp!=0xfe error

    if (buf->getmode()==0){
	Pnormalizemethod=0;
	for (nharmonic=0;nharmonic<MAX_AD_HARMONICS;nharmonic++){
	    Phmag[nharmonic]=64;
	    Phphase[nharmonic]=64;
	};
    };
    
    for (n=0x80;n<0xf0;n++){
	if (buf->getmode()==0) {
	    buf->rwbyte(&npar);
	    n=0;//force a loop until the end of parameters (0xff)
	} else npar=n;
	if (npar==0xff) break;
	
	switch(npar){
	    //Oscil parameters
	    case 0x80:	buf->rwbytepar(n,&Phmagtype);
			break;
	    case 0x81:	if (buf->getmode()!=0){
			    for (nharmonic=0;nharmonic<MAX_AD_HARMONICS;nharmonic++){
				if (Phmag[nharmonic]!=64) {
				    buf->rwbytepar(n,&nharmonic);
				    buf->rwbyte(&Phmag[nharmonic]);
				    buf->rwbyte(&Phphase[nharmonic]);
				};
			    };
			} else {			
			    buf->rwbytepar(n,&nharmonic);
			    if (nharmonic<MAX_AD_HARMONICS){//for safety
				buf->rwbyte(&Phmag[nharmonic]);
				buf->rwbyte(&Phphase[nharmonic]);
			    } else {
				buf->rwbyte(&tmp);
				buf->rwbyte(&tmp);
			    };
			    
			};
			break;
	    case 0x82:	buf->rwbytepar(n,&Pcurrentbasefunc);
			break;
	    case 0x83:	buf->rwbytepar(n,&Pbasefuncpar);
			break;
	    case 0x84:	buf->rwbytepar(n,&Prand);
			break;
	    case 0x85:	buf->rwbytepar(n,&Pwaveshaping);
			break;
	    case 0x86:	buf->rwbytepar(n,&Pwaveshapingfunction);
			break;
	    case 0x87:	if (buf->getmode()!=0){
			    if (Pcurrentbasefunc==127){
				savebasefuncQ();
				for (nh=0;nh<OSCIL_SIZE;nh++){
				    if (basefuncFFTfreqsQ[nh]!=8192) {
					buf->rwbyte(&n);
					buf->rwword(&nh);
					buf->rwword(&basefuncFFTfreqsQ[nh]);
				    };
				};
			    };
			} else {
			    buf->rwword(&nh);
			    if (nh<OSCIL_SIZE){//for safety
				if (basefuncFFTfreqsQ==NULL) {
				    basefuncFFTfreqsQ=new unsigned short int[OSCIL_SIZE];
				    for (int tmp=0;tmp<OSCIL_SIZE;tmp++) basefuncFFTfreqsQ[tmp]=8192;//"8192"=0.0
				};
				buf->rwword(&basefuncFFTfreqsQ[nh]);
				custombasefuncloaded=1;
			    } else {
				buf->rwword(&tmpw);
			    };
			    
			};
			break;
	    case 0x88:	buf->rwbytepar(n,&Pnormalizemethod);
			break;
	    case 0x89:	buf->rwbytepar(n,&Pfiltertype);
			break;
	    case 0x8A:	buf->rwbytepar(n,&Pfilterpar);
			break;
	    case 0x8B:	buf->rwbytepar(n,&Pfilterbeforews);
			break;
	    case 0x8C:	buf->rwbytepar(n,&Psatype);
			break;
	    case 0x8D:	buf->rwbytepar(n,&Psapar);
			break;
	    case 0x8E:	buf->rwbytepar(n,&Pamprandtype);
			break;
	    case 0x8F:	buf->rwbytepar(n,&Pamprandpower);
			break;
	};
    };

    
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    } else {
	if (custombasefuncloaded!=0) {
	    loadbasefuncQ();
	};
	prepare();
    };
};


void OscilGen::add2XML(XMLwrapper *xml){
    xml->addpar("harmonic_mag_type",Phmagtype);
    xml->addpar("normalize_method",Pnormalizemethod);

    xml->addpar("base_function",Pcurrentbasefunc);
    xml->addpar("base_function_par",Pbasefuncpar);
    xml->addpar("base_function_modulation",Pbasefuncmodulation);
    xml->addpar("base_function_modulation_par1",Pbasefuncmodulationpar1);
    xml->addpar("base_function_modulation_par2",Pbasefuncmodulationpar2);

    xml->addpar("wave_shaping",Pwaveshaping);
    xml->addpar("wave_shaping_function",Pwaveshapingfunction);

    xml->addpar("filter_type",Pfiltertype);
    xml->addpar("filter_par",Pfilterpar);
    xml->addpar("filter_before_wave_shaping",Pfilterbeforews);

    xml->addpar("spectrum_adjust_type",Psatype);
    xml->addpar("spectrum_adjust_par",Psapar);

    xml->addpar("rand",Prand);
    xml->addpar("amp_rand_type",Pamprandtype);
    xml->addpar("amp_rand_power",Pamprandpower);

    xml->addpar("harmonic_shift",Pharmonicshift);
    xml->addparbool("harmonic_shift_first",Pharmonicshiftfirst);

    xml->beginbranch("HARMONICS");
	for (int n=0;n<MAX_AD_HARMONICS;n++){
	    if ((Phmag[n]==64)&&(Phphase[n]==64)) continue;
	    xml->beginbranch("HARMONIC",n+1);
		xml->addpar("mag",Phmag[n]);
		xml->addpar("phase",Phphase[n]);
	    xml->endbranch();
	};
    xml->endbranch();
    
    if (Pcurrentbasefunc==127){
	REALTYPE max=0.0;

	for (int i=0;i<OSCIL_SIZE;i++) if (max<fabs(basefuncFFTfreqs[i])) max=fabs(basefuncFFTfreqs[i]);
	if (max<0.00000001) max=1.0;

	xml->beginbranch("BASE_FUNCTION");
	    for (int i=1;i<OSCIL_SIZE/2;i++){
		REALTYPE xc=basefuncFFTfreqs[i]/max;
	        REALTYPE xs=basefuncFFTfreqs[OSCIL_SIZE-i]/max;
		if ((fabs(xs)>0.00001)&&(fabs(xs)>0.00001)){
		    xml->beginbranch("BF_HARMONIC",i);
			xml->addparreal("cos",xc);
			xml->addparreal("sin",xs);
		    xml->endbranch();
		};
	    };
	xml->endbranch();
    };
};


void OscilGen::getfromXML(XMLwrapper *xml){

    Phmagtype=xml->getpar127("harmonic_mag_type",Phmagtype);
    Pnormalizemethod=xml->getpar127("normalize_method",Pnormalizemethod);

    Pcurrentbasefunc=xml->getpar127("base_function",Pcurrentbasefunc);
    Pbasefuncpar=xml->getpar127("base_function_par",Pbasefuncpar);

    Pbasefuncmodulation=xml->getpar127("base_function_modulation",Pbasefuncmodulation);
    Pbasefuncmodulationpar1=xml->getpar127("base_function_modulation_par1",Pbasefuncmodulationpar1);
    Pbasefuncmodulationpar2=xml->getpar127("base_function_modulation_par2",Pbasefuncmodulationpar2);


    Pwaveshaping=xml->getpar127("wave_shaping",Pwaveshaping);
    Pwaveshapingfunction=xml->getpar127("wave_shaping_function",Pwaveshapingfunction);

    Pfiltertype=xml->getpar127("filter_type",Pfiltertype);
    Pfilterpar=xml->getpar127("filter_par",Pfilterpar);
    Pfilterbeforews=xml->getpar127("filter_before_wave_shaping",Pfilterbeforews);

    Psatype=xml->getpar127("spectrum_adjust_type",Psatype);
    Psapar=xml->getpar127("spectrum_adjust_par",Psapar);

    Prand=xml->getpar127("rand",Prand);
    Pamprandtype=xml->getpar127("amp_rand_type",Pamprandtype);
    Pamprandpower=xml->getpar127("amp_rand_power",Pamprandpower);

    Pharmonicshift=xml->getpar("harmonic_shift",Pharmonicshift,-64,64);
    Pharmonicshiftfirst=xml->getparbool("harmonic_shift_first",Pharmonicshiftfirst);

    if (xml->enterbranch("HARMONICS")){
	Phmag[0]=64;Phphase[0]=64;
	for (int n=0;n<MAX_AD_HARMONICS;n++){
	    if (xml->enterbranch("HARMONIC",n+1)==0) continue;
		Phmag[n]=xml->getpar127("mag",64);
		Phphase[n]=xml->getpar127("phase",64);
	    xml->exitbranch();
	};
     xml->exitbranch();
    };
    
    if (Pcurrentbasefunc!=0) changebasefunction();
    
    
    if (xml->enterbranch("BASE_FUNCTION")){
	    for (int i=1;i<OSCIL_SIZE/2;i++){
		if (xml->enterbranch("BF_HARMONIC",i)){
			basefuncFFTfreqs[i]=xml->getparreal("cos",0.0);
			basefuncFFTfreqs[OSCIL_SIZE-i]=xml->getparreal("sin",0.0);
		    xml->exitbranch();
		};


	    };
	xml->exitbranch();

	REALTYPE max=0.0;

	basefuncFFTfreqs[0]=0.0;
	for (int i=0;i<OSCIL_SIZE;i++) if (max<fabs(basefuncFFTfreqs[i])) max=fabs(basefuncFFTfreqs[i]);
	if (max<0.00000001) max=1.0;

	for (int i=0;i<OSCIL_SIZE;i++) if (basefuncFFTfreqs[i]) basefuncFFTfreqs[i]/=max;
    };


};




/*
  ZynAddSubFX - a software synthesizer
 
  DynamicFilter.C - "WahWah" effect and others
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
#include "DynamicFilter.h"
#include <stdio.h>

DynamicFilter::DynamicFilter(int insertion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_){
    efxoutl=efxoutl_;
    efxoutr=efxoutr_;
        
    insertion=insertion_;
    
    Ppreset=0;
    filter=NULL;
    filterpars=new FilterParams(0,64,64);
    setpreset(Ppreset);
    cleanup();
};

DynamicFilter::~DynamicFilter(){
    delete(filterpars);
};


/*
 * Apply the effect
 */
void DynamicFilter::out(REALTYPE *smpsl,REALTYPE *smpsr){
    int i;
/*    REALTYPE lfol,lfor;
    COMPLEXTYPE clfol,clfor,out,tmp;
*/
//    lfo.effectlfoout(&lfol,&lfor);
/*    lfol*=depth*PI*2.0;lfor*=depth*PI*2.0;
    clfol.a=cos(lfol+phase)*fb;clfol.b=sin(lfol+phase)*fb;
    clfor.a=cos(lfor+phase)*fb;clfor.b=sin(lfor+phase)*fb;

    for (i=0;i<SOUND_BUFFER_SIZE;i++){	
	REALTYPE x=((REALTYPE) i)/SOUND_BUFFER_SIZE;
	REALTYPE x1=1.0-x;
	//left	
	tmp.a=clfol.a*x+oldclfol.a*x1;
	tmp.b=clfol.b*x+oldclfol.b*x1;
	
	out.a=tmp.a*oldl[oldk].a-tmp.b*oldl[oldk].b
	     +(1-fabs(fb))*smpsl[i]*panning;
	out.b=tmp.a*oldl[oldk].b+tmp.b*oldl[oldk].a;
	oldl[oldk].a=out.a;
	oldl[oldk].b=out.b;
	REALTYPE l=out.a*10.0*(fb+0.1);
	
	//right
	tmp.a=clfor.a*x+oldclfor.a*x1;
	tmp.b=clfor.b*x+oldclfor.b*x1;
	
	out.a=tmp.a*oldr[oldk].a-tmp.b*oldr[oldk].b
	     +(1-fabs(fb))*smpsr[i]*(1.0-panning);
	out.b=tmp.a*oldr[oldk].b+tmp.b*oldr[oldk].a;
	oldr[oldk].a=out.a;
	oldr[oldk].b=out.b;
	REALTYPE r=out.a*10.0*(fb+0.1);


	if (++oldk>=Pdelay) oldk=0;
	//LRcross
	efxoutl[i]=l*(1.0-lrcross)+r*lrcross;
	efxoutr[i]=r*(1.0-lrcross)+l*lrcross;
    };

    oldclfol.a=clfol.a;oldclfol.b=clfol.b;
    oldclfor.a=clfor.a;oldclfor.b=clfor.b;
*/
    //Insertion effect
    if (insertion!=0) {
        REALTYPE v1,v2;
	if (volume<0.5) {
		v1=1.0;
		v2=volume*2.0;
	} else {
		v1=(1.0-volume)*2.0;
		v2=1.0;
	};
	for (i=0;i<SOUND_BUFFER_SIZE;i++){
	    smpsl[i]=smpsl[i]*v1+efxoutl[i]*v2;
	    smpsr[i]=smpsr[i]*v1+efxoutr[i]*v2;
	};
    } else {//System effect
	for (i=0;i<SOUND_BUFFER_SIZE;i++){
	    efxoutl[i]*=2.0*volume;
	    efxoutr[i]*=2.0*volume;
	    smpsl[i]=efxoutl[i];
	    smpsr[i]=efxoutr[i];
	};
    };


};

/*
 * Cleanup the effect
 */
void DynamicFilter::cleanup(){
    reinitfilter();
};


/*
 * Parameter control
 */

void DynamicFilter::setdepth(unsigned char Pdepth){
    this->Pdepth=Pdepth;
    depth=(Pdepth/127.0);
};


void DynamicFilter::setvolume(unsigned char Pvolume){
    this->Pvolume=Pvolume;
    outvolume=Pvolume/127.0;
    if (insertion==0) volume=1.0;
	else volume=outvolume;
};

void DynamicFilter::setpanning(unsigned char Ppanning){
    this->Ppanning=Ppanning;
    panning=Ppanning/127.0;
};

void DynamicFilter::setlrcross(unsigned char Plrcross){
    this->Plrcross=Plrcross;
    lrcross=Plrcross/127.0;
};

void DynamicFilter::setampsns(unsigned char Pampsns){
    ampsns=Pampsns/127.0;
    if (Pampsnsinv!=0) ampsns=-ampsns;    
};

void DynamicFilter::reinitfilter(){
    if (filter!=NULL) delete(filter);
    filter=new Filter(filterpars);
};

void DynamicFilter::setpreset(unsigned char npreset){
    const int PRESET_SIZE=11;
    const int NUM_PRESETS=4;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
	//DynamicFilter1
	{127,64,70,0,0,62,60,105,25,0,64},
	//DynamicFilter2
	{127,64,73,106,0,101,60,105,17,0,64},
	//DynamicFilter3
	{127,64,63,0,1,100,112,105,31,0,42},
	//DynamicFilter4
	{93,64,25,0,1,66,101,11,47,0,86}};
	
    if (npreset>=NUM_PRESETS) npreset=NUM_PRESETS-1;
    for (int n=0;n<PRESET_SIZE;n++) changepar(n,presets[npreset][n]);
    if (insertion==0) changepar(0,presets[npreset][0]/2);//lower the volume if this is system effect
    Ppreset=npreset;
    
    reinitfilter();
};


void DynamicFilter::changepar(int npar,unsigned char value){
    switch(npar){
	case 0:	setvolume(value);
	        break;
	case 1:	setpanning(value);
	        break;
	case 2:	lfo.Pfreq=value;
	        lfo.updateparams();
		break;	
	case 3:	lfo.Prandomness=value;
	        lfo.updateparams();
		break;	
	case 4:	lfo.PLFOtype=value;
	        lfo.updateparams();
		break;	
	case 5:	lfo.Pstereo=value;
	        lfo.updateparams();
		break;	
	case 6:	setdepth(value);
	        break;
	case 7:	setampsns(value);
	        break;
	case 8:	setampsns(Pampsns);
	        break;
	case 9:	setlrcross(value);
	        break;
    };
};

unsigned char DynamicFilter::getpar(int npar){
    switch (npar){
	case 0:	return(Pvolume);
		break;
	case 1:	return(Ppanning);
		break;
	case 2:	return(lfo.Pfreq);
		break;
	case 3:	return(lfo.Prandomness);
		break;
	case 4:	return(lfo.PLFOtype);
		break;
	case 5:	return(lfo.Pstereo);
		break;
	case 6:	return(Pdepth);
		break;
	case 7:	return(Pampsns);
		break;
	case 8:	return(Pampsnsinv);
		break;
	case 9:	return(Plrcross);
		break;
	default:return (0);
    };
    
};





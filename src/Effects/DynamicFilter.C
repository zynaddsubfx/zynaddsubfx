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
    filterl=NULL;
    filterr=NULL;
    filterpars=new FilterParams(0,64,64);
    setpreset(Ppreset);
    cleanup();
};

DynamicFilter::~DynamicFilter(){
    delete(filterpars);
    delete(filterl);
    delete(filterr);
};


/*
 * Apply the effect
 */
void DynamicFilter::out(REALTYPE *smpsl,REALTYPE *smpsr){
    int i;
    
    if (filterpars->changed){
	filterpars->changed=false;
	cleanup();	
    };
    
    REALTYPE lfol,lfor;
    lfo.effectlfoout(&lfol,&lfor);
    lfol*=depth*5.0;lfor*=depth*5.0;
    REALTYPE freq=filterpars->getfreq();
    REALTYPE q=filterpars->getq();

    for (i=0;i<SOUND_BUFFER_SIZE;i++){	
	efxoutl[i]=smpsl[i];
	efxoutr[i]=smpsr[i];
	
	REALTYPE x=(fabs(smpsl[i])+fabs(smpsr[i]))*0.5;
	ms1=ms1*(1.0-ampsmooth)+x*x*ampsmooth+1e-10;
    };
    

    REALTYPE ampsmooth2=pow(ampsmooth,0.2)*0.3;
    ms2=ms2*(1.0-ampsmooth2)+ms1*ampsmooth2;
    ms3=ms3*(1.0-ampsmooth2)+ms2*ampsmooth2;
    ms4=ms4*(1.0-ampsmooth2)+ms3*ampsmooth2;
    REALTYPE rms=(sqrt(ms4)-0.25)*ampsns;

    REALTYPE frl=filterl->getrealfreq(freq+lfol+rms);
    REALTYPE frr=filterr->getrealfreq(freq+lfor+rms);
    
    filterl->setfreq_and_q(frl,q);
    filterr->setfreq_and_q(frr,q);
    
    
    filterl->filterout(efxoutl);
    filterr->filterout(efxoutr);

    //panning    
    for (i=0;i<SOUND_BUFFER_SIZE;i++){	
	efxoutl[i]*=panning;
	efxoutr[i]*=(1.0-panning);
    };
        

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
    ms1=0.0;
    ms2=0.0;
    ms3=0.0;
    ms3=0.0;
};


/*
 * Parameter control
 */

void DynamicFilter::setdepth(unsigned char Pdepth){
    this->Pdepth=Pdepth;
    depth=pow((Pdepth/127.0),2.0);
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


void DynamicFilter::setampsns(unsigned char Pampsns){
    ampsns=pow(Pampsns/127.0,2.5)*8;
    if (Pampsnsinv!=0) ampsns=-ampsns;    
    ampsmooth=exp(-Pampsmooth/127.0*10.0)*0.99;
    this->Pampsns=Pampsns;
};

void DynamicFilter::reinitfilter(){
    if (filterl!=NULL) delete(filterl);
    if (filterr!=NULL) delete(filterr);
    filterl=new Filter(filterpars);
    filterr=new Filter(filterpars);
};

void DynamicFilter::setpreset(unsigned char npreset){
    const int PRESET_SIZE=10;
    const int NUM_PRESETS=1;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
	//DynamicFilter1
	{127,64,80,0,0,64,0,80,0,60}};
	
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
	case 8:	Pampsnsinv=value;
		setampsns(Pampsns);
	        break;
	case 9:	Pampsmooth=value;
		setampsns(Pampsns);
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
	case 9:	return(Pampsmooth);
		break;
	default:return (0);
    };
    
};





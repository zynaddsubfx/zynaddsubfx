/*
  ZynAddSubFX - a software synthesizer
 
  Distorsion.C - Distorsion effect
  Copyright (C) 2002-2003 Nasca Octavian Paul
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../Misc/Util.h"
#include "Distorsion.h"

Distorsion::Distorsion(int insertion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_){
    efxoutl=efxoutl_;
    efxoutr=efxoutr_;

    lpfl=new AnalogFilter(2,22000,1,0);
    lpfr=new AnalogFilter(2,22000,1,0);
    hpfl=new AnalogFilter(3,20,1,0);
    hpfr=new AnalogFilter(3,20,1,0);
    

    insertion=insertion_;
    //default values
    Ppreset=0;
    Pvolume=50;
    Plrcross=40; 
    Pdrive=90;
    Plevel=64;
    Ptype=0;
    Pnegate=0;
    Plpf=127;
    Phpf=0;
    Pstereo=0;
    Pprefiltering=0;

    setpreset(Ppreset);    	   
    cleanup();
};

Distorsion::~Distorsion(){
    delete (lpfl);
    delete (lpfr);
    delete (hpfl);
    delete (hpfr);
    
};

/*
 * Cleanup the effect
 */
void Distorsion::cleanup(){
    lpfl->cleanup();
    hpfl->cleanup();
    lpfr->cleanup();
    hpfr->cleanup();
};


/*
 * Apply the filters
 */

void Distorsion::applyfilters(REALTYPE *efxoutl,REALTYPE *efxoutr){
    lpfl->filterout(efxoutl);
    hpfl->filterout(efxoutl);
    if (Pstereo!=0){//stereo
	lpfr->filterout(efxoutr);
        hpfr->filterout(efxoutr);
    };

};


/*
 * Effect output
 */
void Distorsion::out(REALTYPE *smpsl,REALTYPE *smpsr){
    int i;
    REALTYPE l,r,lout,rout;

    REALTYPE inputvol=pow(5.0,(Pdrive-32.0)/127.0);
    if (Pnegate!=0) inputvol*=-1.0;
    
    if (Pstereo!=0){//Stereo
	for (i=0;i<SOUND_BUFFER_SIZE;i++){
	    efxoutl[i]=smpsl[i]*inputvol*panning;
	    efxoutr[i]=smpsr[i]*inputvol*(1.0-panning);
	};
    } else {
	for (i=0;i<SOUND_BUFFER_SIZE;i++){
	    efxoutl[i]=( smpsl[i]*panning + smpsr[i]*(1.0-panning) ) * inputvol;
	};
    };
    
    if (Pprefiltering!=0) applyfilters(efxoutl,efxoutr);

    //no optimised, yet (no look table)
    waveshapesmps(SOUND_BUFFER_SIZE,efxoutl,Ptype+1,Pdrive);
    if (Pstereo!=0) waveshapesmps(SOUND_BUFFER_SIZE,efxoutr,Ptype+1,Pdrive);

    if (Pprefiltering==0) applyfilters(efxoutl,efxoutr);
    
    if (Pstereo==0) for (i=0;i<SOUND_BUFFER_SIZE;i++) efxoutr[i]=efxoutl[i];
    
    REALTYPE level=dB2rap(60.0*Plevel/127.0-40.0);
    for (i=0;i<SOUND_BUFFER_SIZE;i++){
	lout=efxoutl[i];
	rout=efxoutr[i];
	l=lout*(1.0-lrcross)+rout*lrcross;
	r=rout*(1.0-lrcross)+lout*lrcross;
	lout=l;rout=r;
	
	efxoutl[i]=lout*2.0*level;
	efxoutr[i]=rout*2.0*level;

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
 * Parameter control
 */
void Distorsion::setvolume(unsigned char Pvolume){
    this->Pvolume=Pvolume;

    if (insertion==0) {
	outvolume=pow(0.01,(1.0-Pvolume/127.0))*4.0;
	volume=1.0;
    } else {
	volume=outvolume=Pvolume/127.0;
    };
    if (Pvolume==0) cleanup();

};

void Distorsion::setpanning(unsigned char Ppanning){
    this->Ppanning=Ppanning;
    panning=(Ppanning+0.5)/127.0;
};


void Distorsion::setlrcross(unsigned char Plrcross){
    this->Plrcross=Plrcross;
    lrcross=Plrcross/127.0*1.0;
};

void Distorsion::setlpf(unsigned char Plpf){
    this->Plpf=Plpf;
    REALTYPE fr=exp(pow(Plpf/127.0,0.5)*log(25000.0))+40;
    lpfl->setfreq(fr);
    lpfr->setfreq(fr);
};

void Distorsion::sethpf(unsigned char Phpf){
    this->Phpf=Phpf;
    REALTYPE fr=exp(pow(Phpf/127.0,0.5)*log(25000.0))+20.0;
    hpfl->setfreq(fr);
    hpfr->setfreq(fr);
};


void Distorsion::setpreset(unsigned char npreset){
    const int PRESET_SIZE=11;
    const int NUM_PRESETS=6;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
	//Overdrive 1
	{127,64,35,56,70,0,0,96,0,0,0},
	//Overdrive 2
	{127,64,35,29,75,1,0,127,0,0,0},
	//A. Exciter 1
	{64,64,35,75,80,5,0,127,105,1,0},
	//A. Exciter 2
	{64,64,35,85,62,1,0,127,118,1,0},
	//Guitar Amp
	{127,64,35,63,75,2,0,55,0,0,0},
	//Quantisize
	{127,64,35,88,75,4,0,127,0,1,0}};


    if (npreset>=NUM_PRESETS) npreset=NUM_PRESETS-1;
    for (int n=0;n<PRESET_SIZE;n++) changepar(n,presets[npreset][n]);
    if (insertion==0) changepar(0,(int) (presets[npreset][0]/1.5));//lower the volume if this is system effect
    Ppreset=npreset;
    cleanup();
};


void Distorsion::changepar(int npar,unsigned char value){
    switch (npar){
	case 0: setvolume(value);
		break;
	case 1: setpanning(value);
		break;
	case 2: setlrcross(value);
		break;
	case 3: Pdrive=value;
		break;
	case 4: Plevel=value;
		break;
	case 5: if (value>12) value=12;//this must be increased if more distorsion types are added
		Ptype=value;
		break;
	case 6: if (value>1) value=1;
		Pnegate=value;
		break;
	case 7: setlpf(value);
		break;
	case 8: sethpf(value);
		break;
	case 9: if (value>1) value=1;
		Pstereo=value;
		break;
	case 10:Pprefiltering=value;
		break;
    };
};

unsigned char Distorsion::getpar(int npar){
    switch (npar){
	case 0: return(Pvolume);
		break;
	case 1: return(Ppanning);
		break;
	case 2: return(Plrcross);
		break;
	case 3: return(Pdrive);
		break;
	case 4: return(Plevel);
		break;
	case 5: return(Ptype);
		break;
	case 6: return(Pnegate);
		break;
	case 7: return(Plpf);
		break;
	case 8: return(Phpf);
		break;
	case 9: return(Pstereo);
		break;
	case 10:return(Pprefiltering);
		break;
    };
    return(0);//in case of bogus parameter number
};





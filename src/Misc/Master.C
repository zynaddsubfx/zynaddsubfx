/*
  ZynAddSubFX - a software synthesizer
 
  Master.C - It sends Midi Messages to Parts, receives samples from parts,
             process them with system/insertion effects and mix them
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

#include "Master.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

Master::Master(){
    swaplr=0;
    
    pthread_mutex_init(&mutex,NULL);
    fft=new FFTwrapper(OSCIL_SIZE);

    tmpmixl=new REALTYPE[SOUND_BUFFER_SIZE];
    tmpmixr=new REALTYPE[SOUND_BUFFER_SIZE];
    audiooutl=new REALTYPE[SOUND_BUFFER_SIZE];
    audiooutr=new REALTYPE[SOUND_BUFFER_SIZE];
    
    ksoundbuffersample=-1;//this is only time when this is -1; this means that the GetAudioOutSamples was never called
    ksoundbuffersamplelow=0.0;
    oldsamplel=0.0;oldsampler=0.0;
    shutup=0;
    for (int npart=0;npart<NUM_MIDI_PARTS;npart++) {
	vuoutpeakpart[npart]=1e-9;
	fakepeakpart[npart]=0;
    };
    
    for (int i=0;i<SOUND_BUFFER_SIZE;i++) {
	audiooutl[i]=0.0;
	audiooutr[i]=0.0;
    };

    for (int npart=0;npart<NUM_MIDI_PARTS;npart++)
	part[npart]=new Part(&microtonal,fft,&mutex);
    


    //Insertion Effects init        
    for (int nefx=0;nefx<NUM_INS_EFX;nefx++)
    	insefx[nefx]=new EffectMgr(1,&mutex);

    //System Effects init        
    for (int nefx=0;nefx<NUM_SYS_EFX;nefx++) {
	sysefx[nefx]=new EffectMgr(0,&mutex);
    };

    
    defaults();
};

void Master::defaults(){
    volume=1.0;
    setPvolume(80);
    setPkeyshift(64);
    
    for (int npart=0;npart<NUM_MIDI_PARTS;npart++){
	part[npart]->defaults();
	part[npart]->Prcvchn=npart%NUM_MIDI_CHANNELS;
    };

    partonoff(0,1);//enable the first part

    for (int nefx=0;nefx<NUM_INS_EFX;nefx++) {
    	insefx[nefx]->defaults();
	Pinsparts[nefx]=-1;
    };
    
    //System Effects init        
    for (int nefx=0;nefx<NUM_SYS_EFX;nefx++) {
	sysefx[nefx]->defaults();
	for (int npart=0;npart<NUM_MIDI_PARTS;npart++){
	    if (nefx==0) setPsysefxvol(npart,nefx,64);
	    else setPsysefxvol(npart,nefx,0);
	};
	for (int nefxto=0;nefxto<NUM_SYS_EFX;nefxto++)
	    setPsysefxsend(nefx,nefxto,0);
    };

    sysefx[0]->changeeffect(1);
};

/*
 * Note On Messages (velocity=0 for NoteOff)
 */
void Master::NoteOn(unsigned char chan,unsigned char note,unsigned char velocity){
    dump.dumpnote(chan,note,velocity);
    seq.recordnote(chan,note,velocity);

    noteon(chan,note,velocity);
};

/*
 * Internal Note On (velocity=0 for NoteOff)
 */
void Master::noteon(unsigned char chan,unsigned char note,unsigned char velocity){
    int npart;
    if (velocity!=0){
        for (npart=0;npart<NUM_MIDI_PARTS;npart++){
	    if (chan==part[npart]->Prcvchn){
		fakepeakpart[npart]=velocity*2;
		if (part[npart]->Penabled!=0) part[npart]->NoteOn(note,velocity,keyshift);
	    };
	};
    }else{
	this->NoteOff(chan,note);
    };
    HDDRecorder.triggernow();    
};

/*
 * Note Off Messages
 */
void Master::NoteOff(unsigned char chan,unsigned char note){
    dump.dumpnote(chan,note,0);
    seq.recordnote(chan,note,0);
    
    noteoff(chan,note);
};

/*
 * Internal Note Off
 */
void Master::noteoff(unsigned char chan,unsigned char note){
    int npart;
    for (npart=0;npart<NUM_MIDI_PARTS;npart++){
	if ((chan==part[npart]->Prcvchn) && (part[npart]->Penabled!=0))
	    part[npart]->NoteOff(note);	
    };
};

/*
 * Controllers 
 */
void Master::SetController(unsigned char chan,unsigned int type,int par){
    dump.dumpcontroller(chan,type,par);
    seq.recordcontroller(chan,type,par);
    
    setcontroller(chan,type,par);
};

/*
 * Internal Controllers 
 */
void Master::setcontroller(unsigned char chan,unsigned int type,int par){
    if ((type==C_dataentryhi)||(type==C_dataentrylo)||
       (type==C_nrpnhi)||(type==C_nrpnlo)){//Process RPN and NRPN by the Master (ignore the chan)
	ctl.setparameternumber(type,par);
	
	int parhi=-1,parlo=-1,valhi=-1,vallo=-1;
	if (ctl.getnrpn(&parhi,&parlo,&valhi,&vallo)==0){//this is NRPN
	    //fprintf(stderr,"rcv. NRPN: %d %d %d %d\n",parhi,parlo,valhi,vallo);
	    switch (parhi){
	       case 0x04://System Effects
	    	    if (parlo<NUM_SYS_EFX) {
			sysefx[parlo]->seteffectpar_nolock(valhi,vallo);
		    };
	            break;
	       case 0x08://Insertion Effects
	    	    if (parlo<NUM_INS_EFX) {
			insefx[parlo]->seteffectpar_nolock(valhi,vallo);
		    };
	            break;
	    
	    };
	};
    } else {//other controllers
	for (int npart=0;npart<NUM_MIDI_PARTS;npart++){//Send the controller to all part assigned to the channel
	    if ((chan==part[npart]->Prcvchn) && (part[npart]->Penabled!=0))
		part[npart]->SetController(type,par);
	};
    };
};


/*
 * Enable/Disable a part
 */
void Master::partonoff(int npart,int what){
    if (npart>=NUM_MIDI_PARTS) return;
    if (what==0){//disable part
	fakepeakpart[npart]=0;
	part[npart]->Penabled=0;
	part[npart]->cleanup();
	for (int nefx=0;nefx<NUM_INS_EFX;nefx++){
	    if (Pinsparts[nefx]==npart) {
	    insefx[nefx]->cleanup();
	};
    };
    } else {//enabled
	part[npart]->Penabled=1;
	fakepeakpart[npart]=0;
    };
};

/*
 * Master audio out (the final sound)
 */
void Master::AudioOut(REALTYPE *outl,REALTYPE *outr){
    int i,npart,nefx;

    //test!!!!!!!!!!!!! se poate bloca aici (mutex)
    while (1){	
	int type,par1,par2,again;
	char chan=0;//deocamdata
	again=seq.getevent(chan,&type,&par1,&par2);
//	if (type!=0) printf("%d %d %d %d\n",type,chan,par1,par2);
	
	if (again<=0) break;
    };
    //test end

    
    //Swaps the Left channel with Right Channel (if it is asked for)
    if (swaplr!=0){
        REALTYPE *tmp=outl;
	outl=outr;
	outr=tmp;
    };
    
    //clean up the output samples 
    for (i=0;i<SOUND_BUFFER_SIZE;i++) {
	outl[i]=0.0;
	outr[i]=0.0;
    };

    //Compute part samples and store them part[npart]->partoutl,partoutr
    for (npart=0;npart<NUM_MIDI_PARTS;npart++)
	if (part[npart]->Penabled!=0) part[npart]->ComputePartSmps();

    //Insertion effects 
    for (nefx=0;nefx<NUM_INS_EFX;nefx++){
	if (Pinsparts[nefx]>=0) {
	    int efxpart=Pinsparts[nefx];
	    if (part[efxpart]->Penabled!=0) 
		insefx[nefx]->out(part[efxpart]->partoutl,part[efxpart]->partoutr);
	};
    };
    
    
    //Apply the part volumes and pannings (after insertion effects)
    for (npart=0;npart<NUM_MIDI_PARTS;npart++){
	if (part[npart]->Penabled==0)  continue; 

	REALTYPE newvol_l=part[npart]->volume;
	REALTYPE newvol_r=part[npart]->volume;
	REALTYPE oldvol_l=part[npart]->oldvolumel;
	REALTYPE oldvol_r=part[npart]->oldvolumer;
	REALTYPE pan=part[npart]->panning;
	if (pan<0.5) newvol_l*=pan*2.0;
   	    else newvol_r*=(1.0-pan)*2.0;
	
	if (ABOVE_AMPLITUDE_THRESHOLD(oldvol_l,newvol_l)||
	    ABOVE_AMPLITUDE_THRESHOLD(oldvol_r,newvol_r)){//the volume or the panning has changed and needs interpolation
	    
	    for (i=0;i<SOUND_BUFFER_SIZE;i++) {
		REALTYPE vol_l=INTERPOLATE_AMPLITUDE(oldvol_l,newvol_l,i,SOUND_BUFFER_SIZE);
		REALTYPE vol_r=INTERPOLATE_AMPLITUDE(oldvol_r,newvol_r,i,SOUND_BUFFER_SIZE);
		part[npart]->partoutl[i]*=vol_l;
	        part[npart]->partoutr[i]*=vol_r;
	    };
	    part[npart]->oldvolumel=newvol_l;
	    part[npart]->oldvolumer=newvol_r;

	} else {
	    for (i=0;i<SOUND_BUFFER_SIZE;i++) {//the volume did not changed
		part[npart]->partoutl[i]*=newvol_l;
	        part[npart]->partoutr[i]*=newvol_r;
	    };
	};
    };


    //System effects
    for (nefx=0;nefx<NUM_SYS_EFX;nefx++){
	if (sysefx[nefx]->geteffect()==0) continue;//the effect is disabled

    	//Clean up the samples used by the system effects
	for (i=0;i<SOUND_BUFFER_SIZE;i++) {
	    tmpmixl[i]=0.0;
	    tmpmixr[i]=0.0;
	};

	//Mix the channels according to the part settings about System Effect
	for (npart=0;npart<NUM_MIDI_PARTS;npart++){
	    //skip if the part has no output to effect	    
	    if (Psysefxvol[nefx][npart]==0) continue;

	    //skip if the part is disabled
	    if (part[npart]->Penabled==0) continue;

	    //the output volume of each part to system effect
	    REALTYPE vol=sysefxvol[nefx][npart];
	    for (i=0;i<SOUND_BUFFER_SIZE;i++) {
		tmpmixl[i]+=part[npart]->partoutl[i]*vol;
		tmpmixr[i]+=part[npart]->partoutr[i]*vol;
	    };
	};
	
	// system effect send to next ones
	for (int nefxfrom=0;nefxfrom<nefx;nefxfrom++){
	    if (Psysefxsend[nefxfrom][nefx]!=0){
		REALTYPE v=sysefxsend[nefxfrom][nefx];
		for (i=0;i<SOUND_BUFFER_SIZE;i++) {
		    tmpmixl[i]+=sysefx[nefxfrom]->efxoutl[i]*v;
		    tmpmixr[i]+=sysefx[nefxfrom]->efxoutr[i]*v;
		};
	    };
	};

	sysefx[nefx]->out(tmpmixl,tmpmixr);
	
	//Add the System Effect to sound output
	REALTYPE outvol=sysefx[nefx]->sysefxgetvolume();
	for (i=0;i<SOUND_BUFFER_SIZE;i++) {
	    outl[i]+=tmpmixl[i]*outvol;
	    outr[i]+=tmpmixr[i]*outvol;
	};

    };

    //Mix all parts
    for (npart=0;npart<NUM_MIDI_PARTS;npart++){
	for (i=0;i<SOUND_BUFFER_SIZE;i++) {//the volume did not changed
    	    outl[i]+=part[npart]->partoutl[i];
	    outr[i]+=part[npart]->partoutr[i];
	};
    };
    
    //Insertion effects for Master Out
    for (nefx=0;nefx<NUM_INS_EFX;nefx++){
	if (Pinsparts[nefx] == -2) 
	    insefx[nefx]->out(outl,outr);
    };

    //Master Volume
    for (i=0;i<SOUND_BUFFER_SIZE;i++) {
        outl[i]*=volume;
        outr[i]*=volume;
    };

    //Peak computation (for vumeters)
    vuoutpeakl=1e-12;vuoutpeakr=1e-12;
    for (i=0;i<SOUND_BUFFER_SIZE;i++) {
        if (fabs(outl[i])>vuoutpeakl) vuoutpeakl=fabs(outl[i]);
        if (fabs(outr[i])>vuoutpeakr) vuoutpeakr=fabs(outr[i]);
    };
    if ((vuoutpeakl>1.0)||(vuoutpeakr>1.0)) vuclipped=1;
    if (vumaxoutpeakl<vuoutpeakl) vumaxoutpeakl=vuoutpeakl;
    if (vumaxoutpeakr<vuoutpeakr) vumaxoutpeakr=vuoutpeakr;
    
    //Part Peak computation (for Part vumeters or fake part vumeters)
    for (npart=0;npart<NUM_MIDI_PARTS;npart++){
	vuoutpeakpart[npart]=1.0e-12;
	if (part[npart]->Penabled!=0) {
    	    REALTYPE *outl=part[npart]->partoutl,
		     *outr=part[npart]->partoutr;
	    for (i=0;i<SOUND_BUFFER_SIZE;i++) {
		REALTYPE tmp=fabs(outl[i]+outr[i]);
    		if (tmp>vuoutpeakpart[npart]) vuoutpeakpart[npart]=tmp;
	    };
	    vuoutpeakpart[npart]*=volume;
	} else {
	    if (fakepeakpart[npart]>1) fakepeakpart[npart]--;
	};
    };


    //Shutup if it is asked (with fade-out)
    if (shutup!=0){
	for (i=0;i<SOUND_BUFFER_SIZE;i++) {
	    REALTYPE tmp=(SOUND_BUFFER_SIZE-i)/(REALTYPE) SOUND_BUFFER_SIZE;
	    outl[i]*=tmp;
	    outr[i]*=tmp;
	};
	ShutUp();
    };

    //update the LFO's time
    LFOParams::time++;

    if (HDDRecorder.recording()) HDDRecorder.recordbuffer(outl,outr);
    dump.inctick();
};

void Master::GetAudioOutSamples(int nsamples,int samplerate,REALTYPE *outl,REALTYPE *outr){
    if (ksoundbuffersample==-1){//first time
	    AudioOut(&audiooutl[0],&audiooutr[0]);
    	    ksoundbuffersample=0;
    };


    if (samplerate==SAMPLE_RATE){//no resample
	int ksample=0;
        while (ksample<nsamples){
	    outl[ksample]=audiooutl[ksoundbuffersample];
	    outr[ksample]=audiooutr[ksoundbuffersample];

	    ksample++;
	    ksoundbuffersample++;
	    if (ksoundbuffersample>=SOUND_BUFFER_SIZE){
		AudioOut(&audiooutl[0],&audiooutr[0]);
    		ksoundbuffersample=0;
	    };
	};
    } else {//Resample
	int ksample=0;
	REALTYPE srinc=SAMPLE_RATE/(REALTYPE)samplerate;

        while (ksample<nsamples){
	    if (ksoundbuffersample!=0){
		outl[ksample]=audiooutl[ksoundbuffersample]*ksoundbuffersamplelow
			+audiooutl[ksoundbuffersample-1]*(1.0-ksoundbuffersamplelow);
		outr[ksample]=audiooutr[ksoundbuffersample]*ksoundbuffersamplelow
			+audiooutr[ksoundbuffersample-1]*(1.0-ksoundbuffersamplelow);
	    } else {
		outl[ksample]=audiooutl[ksoundbuffersample]*ksoundbuffersamplelow
			+oldsamplel*(1.0-ksoundbuffersamplelow);
		outr[ksample]=audiooutr[ksoundbuffersample]*ksoundbuffersamplelow
			+oldsampler*(1.0-ksoundbuffersamplelow);
	    };
	    
	    ksample++;
	    
	    ksoundbuffersamplelow+=srinc;
	    if (ksoundbuffersamplelow>=1.0){
		ksoundbuffersample+=(int) floor(ksoundbuffersamplelow);
		ksoundbuffersamplelow=ksoundbuffersamplelow-floor(ksoundbuffersamplelow);
	    };
	    
	    if (ksoundbuffersample>=SOUND_BUFFER_SIZE){
		oldsamplel=audiooutl[SOUND_BUFFER_SIZE-1];
		oldsampler=audiooutr[SOUND_BUFFER_SIZE-1];
		AudioOut(&audiooutl[0],&audiooutr[0]);
    		ksoundbuffersample=0;
	    };
	};
    };
};


Master::~Master(){
    for (int npart=0;npart<NUM_MIDI_PARTS;npart++) delete (part[npart]);
    for (int nefx=0;nefx<NUM_INS_EFX;nefx++) delete (insefx[nefx]);
    for (int nefx=0;nefx<NUM_SYS_EFX;nefx++) delete (sysefx[nefx]);
    
    delete [] audiooutl;
    delete [] audiooutr;
    delete [] tmpmixl;
    delete [] tmpmixr;
    delete (fft);

    pthread_mutex_destroy(&mutex);
};


/*
 * Parameter control
 */
void Master::setPvolume(char Pvolume_){
    Pvolume=Pvolume_;
    volume=dB2rap((Pvolume-96.0)/96.0*40.0);
};

void Master::setPkeyshift(char Pkeyshift_){
    Pkeyshift=Pkeyshift_;
    keyshift=(int)Pkeyshift-64;
};


void Master::setPsysefxvol(int Ppart,int Pefx,char Pvol){
    Psysefxvol[Pefx][Ppart]=Pvol;
    sysefxvol[Pefx][Ppart]=pow(0.1,(1.0-Pvol/96.0)*2.0);
};

void Master::setPsysefxsend(int Pefxfrom,int Pefxto,char Pvol){
    Psysefxsend[Pefxfrom][Pefxto]=Pvol;
    sysefxsend[Pefxfrom][Pefxto]=pow(0.1,(1.0-Pvol/96.0)*2.0);
};


/*
 * Panic! (Clean up all parts and effects)
 */
void Master::ShutUp(){
    for (int npart=0;npart<NUM_MIDI_PARTS;npart++) {
	part[npart]->cleanup();
	fakepeakpart[npart]=0;    
    };
    for (int nefx=0;nefx<NUM_INS_EFX;nefx++) insefx[nefx]->cleanup();
    for (int nefx=0;nefx<NUM_SYS_EFX;nefx++) sysefx[nefx]->cleanup();
    vuresetpeaks();
    shutup=0;
};


/*
 * Reset peaks and clear the "cliped" flag (for VU-meter)
 */
void Master::vuresetpeaks(){
    vuoutpeakl=1e-9;vuoutpeakr=1e-9;vumaxoutpeakl=1e-9;vumaxoutpeakr=1e-9;
    vuclipped=0;
};


/*
 * Swap 2 effect (effect1<->effect2)
 */
void Master::swapcopyeffects(int what,int type,int neff1,int neff2){
    EffectMgr *eff1,*eff2;
    if (neff1==neff2) return;//to swap a effect with itself or copy to itself is meaningless

    if (type==0) {
	eff1=sysefx[neff1];
	eff2=sysefx[neff2];
    } else {
	eff1=insefx[neff1];
	eff2=insefx[neff2];
    };
    
    //get the eff2 parameters (it is needef for swapping)
    unsigned char effect=eff2->geteffect();
    unsigned char preset=eff2->getpreset();
    unsigned char par[128];
    for (int i=0;i<128;i++) par[i]=eff2->geteffectpar(i);
    //copy the eff1 to eff2
    eff2->changeeffect(eff1->geteffect());
    eff2->changepreset_nolock(eff1->getpreset());
    for (int i=0;i<128;i++) eff2->seteffectpar_nolock(i,eff1->geteffectpar(i));
    if (what==0){//if swapping is needed, copy the saved parameters to eff1
	eff1->changeeffect(effect);
	eff1->changepreset_nolock(preset);
	for (int i=0;i<128;i++) eff1->seteffectpar_nolock(i,par[i]);
    };
};



/*
 * Get the effect volume for the system effect
 */
void Master::saveloadbuf(Buffer *buf){
    unsigned char npar,n,tmp;

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n\n\n\n( MASTER parameters) \n");
    if (buf->getmode()==1)fprintf(stderr,"\nTest LOAD\n");
	else fprintf(stderr,"\nTest SAVE\n");
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
	    //Master
	    case 0x80:	buf->rwbytepar(n,&Pvolume);
			if (buf->getmode()==0) setPvolume(Pvolume);
			break;
	    case 0x81:	buf->rwbytepar(n,&Pkeyshift);
			if (buf->getmode()==0) setPkeyshift(Pkeyshift);
			break;
	    case 0xA0:	if (buf->getmode()!=0) {//save the Parts
			    for (unsigned char npart=0;npart<NUM_MIDI_PARTS;npart++){
				if ((buf->getminimal()!=0) && (part[npart]->Penabled==0)) continue;
				buf->rwbyte(&npar);
				buf->rwbyte(&npart);//Write the number of part
				part[npart]->saveloadbuf(buf,0);
			    };
			} else {//load the Part
			    unsigned char npart;
			    buf->rwbyte(&npart);
			    if (npart<NUM_MIDI_PARTS) {
				part[npart]->disablekitloading=0;
				part[npart]->saveloadbuf(buf,0);
			    } else buf->skipbranch();
			};
			break;
	    case 0xB0:	if (buf->getmode()!=0) {//SYSTEM EFFECTS
			    for (unsigned char neffect=0;neffect<NUM_SYS_EFX;neffect++){
				buf->rwbyte(&npar);
				buf->rwbyte(&neffect);
				sysefx[neffect]->saveloadbuf(buf);
			    };
			} else {
			    unsigned char neffect;
			    buf->rwbyte(&neffect);
			    if (neffect<NUM_SYS_EFX) sysefx[neffect]->saveloadbuf(buf);
			    else buf->skipbranch();
			};
			break;
	    case 0xB1:	if (buf->getmode()!=0) {//Part send to System Effects
			    for (unsigned char neffect=0;neffect<NUM_SYS_EFX;neffect++){
				for (unsigned char npart=0;npart<NUM_MIDI_PARTS;npart++){
				    buf->rwbyte(&npar);
				    buf->rwbyte(&npart);
				    buf->rwbyte(&neffect);
				    buf->rwbyte(&Psysefxvol[neffect][npart]);
				};
			    };
			} else {
			    unsigned char neffect,npart;
			    buf->rwbyte(&npart);
			    buf->rwbyte(&neffect);
			    if ((npart<NUM_MIDI_PARTS) && (neffect<NUM_SYS_EFX)){
				buf->rwbyte(&tmp);
				setPsysefxvol(npart,neffect,tmp);
			    } else buf->rwbyte(&tmp);
			};
			break;
	    case 0xB2:	if (buf->getmode()!=0) {//System Effect send to System Effect
			    for (unsigned char neff1=0;neff1<NUM_SYS_EFX;neff1++){
				for (unsigned char neff2=0;neff2<NUM_SYS_EFX;neff2++){
				    buf->rwbyte(&npar);
				    buf->rwbyte(&neff1);
				    buf->rwbyte(&neff2);
				    buf->rwbyte(&Psysefxsend[neff1][neff2]);
				};
			    };
			} else {
			    unsigned char neff1,neff2;
			    buf->rwbyte(&neff1);
			    buf->rwbyte(&neff2);
			    if ((neff1<NUM_SYS_EFX) && (neff2<NUM_SYS_EFX)){
				buf->rwbyte(&tmp);
				setPsysefxsend(neff1,neff2,tmp);
			    } else buf->rwbyte(&tmp);
			};
			break;
	    case 0xC0:	if (buf->getmode()!=0) {//INSERTION EFFECTS
			    for (unsigned char neffect=0;neffect<NUM_INS_EFX;neffect++){
				buf->rwbyte(&npar);
				buf->rwbyte(&neffect);
				insefx[neffect]->saveloadbuf(buf);
			    };
			} else {
			    unsigned char neffect;
			    buf->rwbyte(&neffect);
			    if (neffect<NUM_INS_EFX) insefx[neffect]->saveloadbuf(buf);
			    else buf->skipbranch();
			};
			break;
	    case 0xC1:	if (buf->getmode()!=0) {
			    for (unsigned int neffect=0;neffect<NUM_INS_EFX;neffect++){
				buf->rwbyte(&npar);
				buf->rwwordparwrap(neffect,&Pinsparts[neffect]);
			    };
			} else {
			    unsigned char neffect;
			    buf->rwbyte(&neffect);
			    buf->rwwordparwrap(npar,&Pinsparts[neffect]);
			    if (Pinsparts[neffect]>=NUM_MIDI_PARTS) Pinsparts[neffect]=-1;
			};
			break;
	    case 0xD0:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			microtonal.saveloadbuf(buf);
			break;
	    case 0xE0:	buf->rwbytepar(n,&ctl.NRPN.receive);
			break;

	};
    };


    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
};

void Master::exportbankasxmldirectory(const char *directory){
    char filename[1000],nostr[10];
    Part *tmppart=new Part(&microtonal,fft,&mutex);
    tmppart->Penabled=1;
    
    mkdir(directory,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    for (int slot=0;slot<128;slot++){
	tmppart->defaults();
	bank.loadfromslot(slot,&slbuf);
	slbuf.changemode(0);
	tmppart->saveloadbuf(&slbuf,1);
	snprintf((char *)tmppart->kit[0].Pname,PART_MAX_NAME_LEN,bank.getname(slot));
	snprintf((char *)tmppart->Pname,PART_MAX_NAME_LEN,bank.getname(slot));
	if (bank.emptyslot(slot)) continue;
	
	snprintf(nostr,10,"%4d",slot+1);
	for (int i=0;i<strlen(nostr);i++) if (nostr[i]==' ') nostr[i]='0';
	
	snprintf(filename,1000,"%s/%s-%s.xml",directory,nostr,bank.getname(slot));
	printf("%s\n",filename);
	tmppart->saveXML(filename);
    };
    
    
    delete (tmppart);
};




void Master::add2XML(XMLwrapper *xml){
    xml->addpar("volume",Pvolume);
    xml->addpar("key_shift",Pkeyshift);
    xml->addparbool("nrpn_receive",ctl.NRPN.receive);

    xml->beginbranch("MICROTONAL");
	microtonal.add2XML(xml);
    xml->endbranch();

    for (int npart=0;npart<NUM_MIDI_PARTS;npart++){
	xml->beginbranch("PART",npart);
	part[npart]->add2XML(xml);
	xml->endbranch();
    };
    
    xml->beginbranch("SYSTEM_EFFECTS");
	for (int nefx=0;nefx<NUM_SYS_EFX;nefx++){
	    xml->beginbranch("SYSEFFECT",nefx);
		xml->beginbranch("EFFECT");
		    sysefx[nefx]->add2XML(xml);
		xml->endbranch();

		for (int pefx=0;pefx<NUM_MIDI_PARTS;pefx++){
		    xml->beginbranch("VOLUME",pefx);
			xml->addpar("vol",Psysefxvol[nefx][pefx]);
		    xml->endbranch();    
		};

		for (int tonefx=nefx+1;tonefx<NUM_SYS_EFX;tonefx++){
		    xml->beginbranch("SENDTO",tonefx);
			xml->addpar("send_vol",Psysefxsend[nefx][tonefx]);
		    xml->endbranch();    
		};
		
		
	    xml->endbranch();
	};
    xml->endbranch();

    xml->beginbranch("INSERTION_EFFECTS");
	for (int nefx=0;nefx<NUM_INS_EFX;nefx++){
	    xml->beginbranch("INSEFFECT",nefx);
		xml->addpar("part",Pinsparts[nefx]);

		xml->beginbranch("EFFECT");
		    insefx[nefx]->add2XML(xml);
		xml->endbranch();
	    xml->endbranch();
	};
	
    xml->endbranch();
    
};


int Master::saveXML(char *filename){

    //sa pun aici un test daca exista fisierul

    XMLwrapper *xml=new XMLwrapper();

    xml->beginbranch("MASTER");
    add2XML(xml);
    xml->endbranch();

    xml->saveXMLfile(filename,0);
    delete (xml);
    return(0);
};



int Master::loadXML(char *filename){
    XMLwrapper *xml=new XMLwrapper();
    if (xml->loadXMLfile(filename)<0) {
	delete(xml);
	return(-1);
    };
    
    getfromXML(xml);
    
    delete(xml);
    return(0);
};

void Master::getfromXML(XMLwrapper *xml){
    if (xml->enterbranch("MASTER")){
	Pvolume=xml->getpar127("volume",Pvolume);
	Pkeyshift=xml->getpar127("key_shift",Pkeyshift);
	ctl.NRPN.receive=xml->getparbool("nrpn_receive",ctl.NRPN.receive);
	
	
	for (int npart=0;npart<NUM_MIDI_PARTS;npart++){
	    if (xml->enterbranch("PART",npart)==0) continue;
		part[npart]->getfromXML(xml);
	    xml->exitbranch();
	};

/*	if (xml->enterbranch("MICROTONAL")){
	    //
	    xml->exitbranch();
	};
	
	
	etc.
	
	
*/	
	xml->exitbranch();
    };
};





/*
  ZynAddSubFX - a software synthesizer
 
  ADnoteParameters.C - Parameters for ADnote (ADsynth)
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ADnoteParameters.h"

ADnoteParameters::ADnoteParameters(FFTwrapper *fft_){
    int nvoice;
    fft=fft_;
    //Default Parameters
    /* Frequency Global Parameters */
    GlobalPar.PStereo=1;//stereo
    GlobalPar.PDetune=8192;//zero
    GlobalPar.PCoarseDetune=0;
    GlobalPar.PDetuneType=1;
    GlobalPar.FreqEnvelope=new EnvelopeParams(0,0);
    GlobalPar.FreqEnvelope->ASRinit(64,50,64,60);
    GlobalPar.FreqLfo=new LFOParams(70,0,64,0,0,0,0,0);
    
    /* Amplitude Global Parameters */
    GlobalPar.PVolume=90;
    GlobalPar.PPanning=64;//center
    GlobalPar.PAmpVelocityScaleFunction=64;
    GlobalPar.AmpEnvelope=new EnvelopeParams(64,1);
    GlobalPar.AmpEnvelope->ADSRinit_dB(0,40,127,25);
    GlobalPar.AmpLfo=new LFOParams(80,0,64,0,0,0,0,1);
    GlobalPar.PPunchStrength=0;
    GlobalPar.PPunchTime=60;
    GlobalPar.PPunchStretch=64;
    GlobalPar.PPunchVelocitySensing=72;
    
    /* Filter Global Parameters*/
    GlobalPar.PFilterVelocityScale=64;
    GlobalPar.PFilterVelocityScaleFunction=64;
    GlobalPar.GlobalFilter=new FilterParams(2,94,40);
    GlobalPar.FilterEnvelope=new EnvelopeParams(0,1);
    GlobalPar.FilterEnvelope->ADSRinit_filter(64,40,64,70,60,64);
    GlobalPar.FilterLfo=new LFOParams(80,0,64,0,0,0,0,2);
    GlobalPar.Reson=new Resonance();

    for (nvoice=0;nvoice<NUM_VOICES;nvoice++){
	VoicePar[nvoice].Enabled=0;
	VoicePar[nvoice].Type=0;
	VoicePar[nvoice].Pfixedfreq=0;
	VoicePar[nvoice].PfixedfreqET=0;
	VoicePar[nvoice].Presonance=1;
	VoicePar[nvoice].Pfilterbypass=0;
	VoicePar[nvoice].Pextoscil=-1;
	VoicePar[nvoice].PextFMoscil=-1;
	VoicePar[nvoice].Poscilphase=64;
	VoicePar[nvoice].PFMoscilphase=64;
	VoicePar[nvoice].PDelay=0;
	VoicePar[nvoice].PVolume=100;
	VoicePar[nvoice].PVolumeminus=0;
	VoicePar[nvoice].PPanning=64;//center
	VoicePar[nvoice].PDetune=8192;//8192=0
	VoicePar[nvoice].PCoarseDetune=0;
	VoicePar[nvoice].PDetuneType=0;
	VoicePar[nvoice].PFreqLfoEnabled=0;
	VoicePar[nvoice].PFreqEnvelopeEnabled=0;
	VoicePar[nvoice].PAmpEnvelopeEnabled=0;
	VoicePar[nvoice].PAmpLfoEnabled=0;
	VoicePar[nvoice].PAmpVelocityScaleFunction=127;
	VoicePar[nvoice].PFilterEnabled=0;
	VoicePar[nvoice].PFilterEnvelopeEnabled=0;
	VoicePar[nvoice].PFilterLfoEnabled=0;
	VoicePar[nvoice].PFMEnabled=0;

	//I use the internal oscillator intern (-1)
	VoicePar[nvoice].PFMVoice=-1;

	VoicePar[nvoice].PFMVolume=90;
	VoicePar[nvoice].PFMVolumeDamp=64;
	VoicePar[nvoice].PFMDetune=8192;
	VoicePar[nvoice].PFMCoarseDetune=0;
	VoicePar[nvoice].PFMDetuneType=0;
	VoicePar[nvoice].PFMFreqEnvelopeEnabled=0;
	VoicePar[nvoice].PFMAmpEnvelopeEnabled=0;
	VoicePar[nvoice].PFMVelocityScaleFunction=64;	
	
	EnableVoice(nvoice);
    };
    VoicePar[0].Enabled=1;
};

/*
 * Init the voice parameters
 */
void ADnoteParameters::EnableVoice(int nvoice){
    VoicePar[nvoice].OscilSmp=new OscilGen(fft,GlobalPar.Reson);
    VoicePar[nvoice].FMSmp=new OscilGen(fft,NULL);
    VoicePar[nvoice].OscilSmp->prepare();
    VoicePar[nvoice].FMSmp->prepare();

    VoicePar[nvoice].AmpEnvelope=new EnvelopeParams(64,1);
    VoicePar[nvoice].AmpEnvelope->ADSRinit_dB(0,100,127,100);	
    VoicePar[nvoice].AmpLfo=new LFOParams(90,32,64,0,0,30,0,1);

    VoicePar[nvoice].FreqEnvelope=new EnvelopeParams(0,0);
    VoicePar[nvoice].FreqEnvelope->ASRinit(30,40,64,60);
    VoicePar[nvoice].FreqLfo=new LFOParams(50,40,0,0,0,0,0,0);

    VoicePar[nvoice].VoiceFilter=new FilterParams(2,50,60);
    VoicePar[nvoice].FilterEnvelope=new EnvelopeParams(0,0);
    VoicePar[nvoice].FilterEnvelope->ADSRinit_filter(90,70,40,70,10,40);
    VoicePar[nvoice].FilterLfo=new LFOParams(50,20,64,0,0,0,0,2);

    VoicePar[nvoice].FMFreqEnvelope=new EnvelopeParams(0,0);
    VoicePar[nvoice].FMFreqEnvelope->ASRinit(20,90,40,80);
    VoicePar[nvoice].FMAmpEnvelope=new EnvelopeParams(64,1);
    VoicePar[nvoice].FMAmpEnvelope->ADSRinit(80,90,127,100);
};


/*
 * Kill the voice
 */
void ADnoteParameters::KillVoice(int nvoice){
    delete (VoicePar[nvoice].OscilSmp);
    delete (VoicePar[nvoice].FMSmp);

    delete (VoicePar[nvoice].AmpEnvelope);
    delete (VoicePar[nvoice].AmpLfo);

    delete (VoicePar[nvoice].FreqEnvelope);
    delete (VoicePar[nvoice].FreqLfo);

    delete (VoicePar[nvoice].VoiceFilter);
    delete (VoicePar[nvoice].FilterEnvelope);
    delete (VoicePar[nvoice].FilterLfo);

    delete (VoicePar[nvoice].FMFreqEnvelope);
    delete (VoicePar[nvoice].FMAmpEnvelope);
};

ADnoteParameters::~ADnoteParameters(){
    delete(GlobalPar.FreqEnvelope);
    delete(GlobalPar.FreqLfo);
    delete(GlobalPar.AmpEnvelope);
    delete(GlobalPar.AmpLfo);
    delete(GlobalPar.GlobalFilter);
    delete(GlobalPar.FilterEnvelope);
    delete(GlobalPar.FilterLfo);
    delete(GlobalPar.Reson);

    for (int nvoice=0;nvoice<NUM_VOICES;nvoice++){
        KillVoice(nvoice);
    };
};

void ADnoteParameters::copypastevoice(int n,int what){
    if (what==0){//copy
	clipboardbuf.changemode(1);//write to buffer
	clipboardbuf.changeminimal(0);
	saveloadbufvoice(&clipboardbuf,n);
    } else {//paste
	clipboardbuf.changemode(0);//read from buffer
	saveloadbufvoice(&clipboardbuf,n);
    };
};

/*
 * Save or load the voice parameters to/from the buffer
 */
void ADnoteParameters::saveloadbufvoice(Buffer *buf,unsigned char nvoice){
    unsigned char npar,n,tmp;
    int fmon,min,fmexton;//fmon is 0 if there is no need to save the FM parameters

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n\n( ADnoteParameters VOICE %d) \n",nvoice);    
#endif

    tmp=0xfe;
    buf->rwbyte(&tmp);//if tmp!=0xfe error

    
    if (nvoice>=NUM_VOICES){//too big voice
	buf->skipbranch();
	return;
    };
    
    if ((buf->getminimal()!=0) && (buf->getmode()!=0)) min=1; else min=0;
    if (min && (VoicePar[nvoice].PFMEnabled==0)) fmon=1; else fmon=0;
    if (min && (VoicePar[nvoice].PFMVoice!= -1)) fmexton=1; else fmexton=0;
			    
    for (n=0x80;n<0xF0;n++){
	if (buf->getmode()==0) {
	    buf->rwbyte(&npar);
	    n=0;//force a loop until the end of parameters (0xff)
	} else npar=n;
	if (npar==0xff) break;

	switch(npar){
	    //Misc Voice Parameters
	    case 0x80:	buf->rwbytepar(n,&VoicePar[nvoice].Enabled);
			break;
	    case 0x81:	buf->rwbytepar(n,&VoicePar[nvoice].PDelay);
			break;
	    case 0x82:	buf->rwbytepar(n,&VoicePar[nvoice].PPanning);
			break;
	    case 0x83:	buf->rwbytepar(n,&VoicePar[nvoice].Poscilphase);
			break;
	    case 0x84:	buf->rwwordparwrap(n,&VoicePar[nvoice].Pextoscil);	    
			if (VoicePar[nvoice].Pextoscil>nvoice-1) VoicePar[nvoice].Pextoscil=-1;
			break;
	    case 0x85:	if ((VoicePar[nvoice].Pextoscil!= -1) && (min)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].OscilSmp->saveloadbuf(buf);
			break;
	    case 0x86:	buf->rwbytepar(n,&VoicePar[nvoice].Presonance);
			break;
	    case 0x87:	buf->rwbytepar(n,&VoicePar[nvoice].Type);
			break;
	    //Amplitude Global Parameters
	    case 0x90:	buf->rwbytepar(n,&VoicePar[nvoice].PVolume);
			break;
	    case 0x91:	buf->rwbytepar(n,&VoicePar[nvoice].PAmpVelocityScaleFunction);
			break;
	    case 0x92:	buf->rwbytepar(n,&VoicePar[nvoice].PAmpEnvelopeEnabled);
			break;
	    case 0x93:	buf->rwbytepar(n,&VoicePar[nvoice].PAmpLfoEnabled);
			break;
	    case 0x94:	buf->rwbytepar(n,&VoicePar[nvoice].PVolumeminus);
			break;
	    case 0x98:	if ((min) && (VoicePar[nvoice].PAmpLfoEnabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].AmpLfo->saveloadbuf(buf);
			break;
	    case 0x99:	if ((min) && (VoicePar[nvoice].PAmpEnvelopeEnabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].AmpEnvelope->saveloadbuf(buf);
			break;


	    //Frequency Voice Parameters
	    case 0xA0:	buf->rwwordpar(n,&VoicePar[nvoice].PDetune);
			break;
	    case 0xA1:	buf->rwwordpar(n,&VoicePar[nvoice].PCoarseDetune);
			break;
	    case 0xA2:	buf->rwbytepar(n,&VoicePar[nvoice].PFreqEnvelopeEnabled);
			break;
	    case 0xA3:	buf->rwbytepar(n,&VoicePar[nvoice].PFreqLfoEnabled);
			break;
	    case 0xA4:	buf->rwbytepar(n,&VoicePar[nvoice].PDetuneType);
			break;
	    case 0xA5:	buf->rwbytepar(n,&VoicePar[nvoice].Pfixedfreq);
			break;
	    case 0xA6:	buf->rwbytepar(n,&VoicePar[nvoice].PfixedfreqET);
			break;
	    case 0xA8:	if ((min) && (VoicePar[nvoice].PFreqLfoEnabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].FreqLfo->saveloadbuf(buf);
			break;
	    case 0xA9:	if ((min) && (VoicePar[nvoice].PFreqEnvelopeEnabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].FreqEnvelope->saveloadbuf(buf);
			break;

	    //Filter Voice Parameters
	    case 0xB0:	buf->rwbytepar(n,&VoicePar[nvoice].PFilterEnabled);
			break;
	    case 0xB1:	buf->rwbytepar(n,&VoicePar[nvoice].PFilterEnvelopeEnabled);
			break;
	    case 0xB2:	buf->rwbytepar(n,&VoicePar[nvoice].PFilterLfoEnabled);
			break;
	    case 0xB3:	buf->rwbytepar(n,&VoicePar[nvoice].Pfilterbypass);
			break;
	    case 0xB8:	if ((min) && (VoicePar[nvoice].PFilterEnabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].VoiceFilter->saveloadbuf(buf);
			break;
	    case 0xB9:	if ((min) && ((VoicePar[nvoice].PFilterEnabled==0) || (VoicePar[nvoice].PFilterLfoEnabled==0))) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].FilterLfo->saveloadbuf(buf);
			break;
	    case 0xBA:	if ((min) && ((VoicePar[nvoice].PFilterEnabled==0) || (VoicePar[nvoice].PFilterEnvelopeEnabled==0))) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].FilterEnvelope->saveloadbuf(buf);
			break;

	    //FM Voice Parameters
	    case 0xC0:	buf->rwbytepar(n,&VoicePar[nvoice].PFMEnabled);
			break;
	    case 0xC1:	if (fmon) break;
			buf->rwwordparwrap(n,&VoicePar[nvoice].PFMVoice);
			if (VoicePar[nvoice].PFMVoice>nvoice-1) VoicePar[nvoice].PFMVoice=-1;
			break;
	    case 0xC2:	if ((fmon)||(fmexton)) break;
			buf->rwbytepar(n,&VoicePar[nvoice].PFMoscilphase);
			break;
	    case 0xC3:	if (fmon) break;
			buf->rwwordparwrap(n,&VoicePar[nvoice].PextFMoscil);
			if (VoicePar[nvoice].PextFMoscil>nvoice-1) VoicePar[nvoice].PextFMoscil=-1;
			break;
	    case 0xC4:	if (fmon) break;
			buf->rwbytepar(n,&VoicePar[nvoice].PFMVolume);
			break;
	    case 0xC5:	if (fmon) break;
			buf->rwbytepar(n,&VoicePar[nvoice].PFMVolumeDamp);
			break;
	    case 0xC6:	if (fmon) break;
			buf->rwbytepar(n,&VoicePar[nvoice].PFMVelocityScaleFunction);
			break;
	    case 0xC7:	if ((fmon)||(fmexton)) break;
			buf->rwwordpar(n,&VoicePar[nvoice].PFMDetune);
			break;
	    case 0xC8:	if ((fmon)||(fmexton)) break;
			buf->rwwordpar(n,&VoicePar[nvoice].PFMCoarseDetune);
			break;
	    case 0xC9:	if (fmon) break;
			buf->rwbytepar(n,&VoicePar[nvoice].PFMAmpEnvelopeEnabled);
			break;
	    case 0xCA:	if ((fmon)||(fmexton)) break;
			buf->rwbytepar(n,&VoicePar[nvoice].PFMFreqEnvelopeEnabled);
			break;
	    case 0xCB:	if (fmon) break;
			buf->rwbytepar(n,&VoicePar[nvoice].PFMDetuneType);
			break;
	    case 0xD0:	if ((fmon) || (fmexton)) break;
			if ((VoicePar[nvoice].PextFMoscil!= -1) && (min)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].FMSmp->saveloadbuf(buf);
			break;
	    case 0xD1:	if (fmon) break;
			if ((VoicePar[nvoice].PFMAmpEnvelopeEnabled==0) && (min)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].FMAmpEnvelope->saveloadbuf(buf);
			break;
	    case 0xD2:	if ((fmon)||(fmexton)) break;
			if ((VoicePar[nvoice].PFMFreqEnvelopeEnabled==0) && (min)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			VoicePar[nvoice].FMFreqEnvelope->saveloadbuf(buf);
			break;
			
	};
    };
    
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
    
};

/*
 * Save or load the parameters to/from the buffer
 */
void ADnoteParameters::saveloadbuf(Buffer *buf){
    unsigned char npar,n,tmp;

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n( ADnoteParameters Global) \n");
#endif    

    tmp=0xfe;
    buf->rwbyte(&tmp);//if tmp!=0xfe error


    if (buf->getmode()==0){
	for (unsigned int nvoice=0;nvoice<NUM_VOICES;nvoice++){
	    VoicePar[nvoice].Enabled=0;
	};
    };

    for (n=0x80;n<0xF0;n++){
	if (buf->getmode()==0) {
	    buf->rwbyte(&npar);
	    n=0;//force a loop until the end of parameters (0xff)
	} else npar=n;
	if (npar==0xff) break;
	
	switch(npar){
	    //Misc Global Parameters
	    case 0x80:	buf->rwbytepar(n,&GlobalPar.PStereo);
			break;
	    case 0x81:	buf->rwbytepar(n,&GlobalPar.PPanning);
			break;
	    case 0x82:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalPar.Reson->saveloadbuf(buf);
			break;
	    //Amplitude Global Parameters
	    case 0x90:	buf->rwbytepar(n,&GlobalPar.PVolume);
			break;
	    case 0x91:	buf->rwbytepar(n,&GlobalPar.PAmpVelocityScaleFunction);
			break;
	    case 0x92:	buf->rwbytepar(n,&GlobalPar.PPunchStrength);
//			break;
	    case 0x93:	buf->rwbytepar(n,&GlobalPar.PPunchTime);
//			break;
	    case 0x94:	buf->rwbytepar(n,&GlobalPar.PPunchStretch);
//			break;
	    case 0x95:	buf->rwbytepar(n,&GlobalPar.PPunchVelocitySensing);
//			break;
	    case 0x98:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalPar.AmpLfo->saveloadbuf(buf);
			break;
	    case 0x99:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalPar.AmpEnvelope->saveloadbuf(buf);
			break;

			
	    //Frequency Global Parameters
	    case 0xA0:	buf->rwwordpar(n,&GlobalPar.PDetune);
			break;
	    case 0xA1:	buf->rwwordpar(n,&GlobalPar.PCoarseDetune);
			break;
	    case 0xA2:	buf->rwbytepar(n,&GlobalPar.PDetuneType);
			break;
	    case 0xA8:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalPar.FreqLfo->saveloadbuf(buf);
			break;
	    case 0xA9:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalPar.FreqEnvelope->saveloadbuf(buf);
			break;


	    //Filter Global Parameters
	    case 0xB0:	buf->rwbytepar(n,&GlobalPar.PFilterVelocityScale);
			break;
	    case 0xB1:	buf->rwbytepar(n,&GlobalPar.PFilterVelocityScaleFunction);
			break;
	    case 0xB8:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalPar.GlobalFilter->saveloadbuf(buf);
			break;
	    case 0xB9:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalPar.FilterLfo->saveloadbuf(buf);
			break;
	    case 0xBA:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalPar.FilterEnvelope->saveloadbuf(buf);
			break;

	    //Voices Parameters
	    case 0xC0:	if (buf->getmode()!=0) {
			    for (unsigned char nvoice=0;nvoice<NUM_VOICES;nvoice++){
				int mustbesaved=0;
				for (int i=0;i<NUM_VOICES;i++){
				    if ((VoicePar[i].Pextoscil!=1)||(VoicePar[i].PextFMoscil!=1))
					mustbesaved=1;
				};
				if ((buf->getminimal()!=0) && (VoicePar[nvoice].Enabled==0) &&
				    (mustbesaved!=0)) continue;
				buf->rwbyte(&npar);
				buf->rwbyte(&nvoice);//Write the number of voice 
				saveloadbufvoice(buf,nvoice);
			    };
			} else {
			    unsigned char nvoice;
			    buf->rwbyte(&nvoice);
			    saveloadbufvoice(buf,nvoice);
			};
			break;

	};
    };

    
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
};




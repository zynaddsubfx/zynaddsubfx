/*
  ZynAddSubFX - a software synthesizer
 
  SUBnoteParameters.C - Parameters for SUBnote (SUBsynth)  
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

#include "../globals.h"
#include "SUBnoteParameters.h"
#include <stdio.h>

SUBnoteParameters::SUBnoteParameters():Presets(){
    setpresettype("Psubsyth");
    AmpEnvelope=new EnvelopeParams(64,1);
    AmpEnvelope->ADSRinit_dB(0,40,127,25);
    FreqEnvelope=new EnvelopeParams(64,0);
    FreqEnvelope->ASRinit(30,50,64,60);
    BandWidthEnvelope=new EnvelopeParams(64,0);
    BandWidthEnvelope->ASRinit_bw(100,70,64,60);

    GlobalFilter=new FilterParams(2,80,40);
    GlobalFilterEnvelope=new EnvelopeParams(0,1);
    GlobalFilterEnvelope->ADSRinit_filter(64,40,64,70,60,64);
    
    defaults();
};


void SUBnoteParameters::defaults(){
    PVolume=96;    
    PPanning=64;
    PAmpVelocityScaleFunction=90;
    
    Pfixedfreq=0;
    PfixedfreqET=0;
    Pnumstages=2;
    Pbandwidth=40;
    Phmagtype=0;
    Pbwscale=64;
    Pstereo=1;
    Pstart=1;

    PDetune=8192;
    PCoarseDetune=0;
    PDetuneType=1;
    PFreqEnvelopeEnabled=0;
    PBandWidthEnvelopeEnabled=0;
    
    for (int n=0;n<MAX_SUB_HARMONICS;n++){
	Phmag[n]=0;
	Phrelbw[n]=64;
    };
    Phmag[0]=127;

    PGlobalFilterEnabled=0;
    PGlobalFilterVelocityScale=64;
    PGlobalFilterVelocityScaleFunction=64;

    AmpEnvelope->defaults();
    FreqEnvelope->defaults();
    BandWidthEnvelope->defaults();
    GlobalFilter->defaults();
    GlobalFilterEnvelope->defaults();

};



SUBnoteParameters::~SUBnoteParameters(){
    delete (AmpEnvelope);
    delete (FreqEnvelope);
    delete (BandWidthEnvelope);
    delete (GlobalFilter);
    delete (GlobalFilterEnvelope);
};


/*
 * Save or load the parameters to/from the buffer
 */
void SUBnoteParameters::saveloadbuf(Buffer *buf){
    unsigned char npar,n,nharmonic,tmp;

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n( SUBnotePparameters) \n");
#endif    

    tmp=0xfe;
    buf->rwbyte(&tmp);//if tmp!=0xfe error


    if (buf->getmode()==0){
	for (nharmonic=0;nharmonic<MAX_SUB_HARMONICS;nharmonic++){
	    Phmag[nharmonic]=0;
	    Phrelbw[nharmonic]=64;
	};
    };
    
    for (n=0x80;n<0xf0;n++){
	if (buf->getmode()==0) {
	    buf->rwbyte(&npar);
	    n=0;//force a loop until the end of parameters (0xff)
	} else npar=n;
	if (npar==0xff) break;
	
	switch(npar){
	    //Misc Parameters
	    case 0x80:	buf->rwbytepar(n,&Pstereo);
			break;
	    case 0x81:	buf->rwbytepar(n,&PPanning);
			break;
	    case 0x82:	buf->rwbytepar(n,&Phmagtype);
			break;
	    case 0x83:	if (buf->getmode()!=0){
			    for (nharmonic=0;nharmonic<MAX_SUB_HARMONICS;nharmonic++){
				if (Phmag[nharmonic]==0) continue;
				buf->rwbytepar(n,&nharmonic);
				buf->rwbyte(&Phmag[nharmonic]);
				buf->rwbyte(&Phrelbw[nharmonic]);
			    };
			} else {
			    buf->rwbytepar(n,&nharmonic);
			    if (nharmonic<MAX_SUB_HARMONICS){//for safety
				buf->rwbyte(&Phmag[nharmonic]);
				buf->rwbyte(&Phrelbw[nharmonic]);
			    } else{
				buf->rwbyte(&tmp);
				buf->rwbyte(&tmp);
			    };
			};
			break;
	    case 0x84:	buf->rwbytepar(n,&Pnumstages);
			break;
	    case 0x85:	buf->rwbytepar(n,&Pstart);
			break;
	    //Amplitude Parameters
	    case 0x90:	buf->rwbytepar(n,&PVolume);
			break;
	    case 0x91:	buf->rwbytepar(n,&PAmpVelocityScaleFunction);
			break;
	    case 0x98:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			AmpEnvelope->saveloadbuf(buf);
			break;
	    //Frequency Parameters
	    case 0xA0:	buf->rwwordpar(n,&PDetune);
			break;
	    case 0xA1:	buf->rwwordpar(n,&PCoarseDetune);
			break;
	    case 0xA2:	buf->rwbytepar(n,&PDetuneType);
			break;
	    case 0xA3:	buf->rwbytepar(n,&Pbandwidth);
			break;
	    case 0xA4:	buf->rwbytepar(n,&Pbwscale);
			break;
	    case 0xA5:	buf->rwbytepar(n,&PFreqEnvelopeEnabled);
			break;
	    case 0xA6:	buf->rwbytepar(n,&PBandWidthEnvelopeEnabled);
			break;
	    case 0xA7:	buf->rwbytepar(n,&Pfixedfreq);
			break;
	    case 0xA8:	if ((buf->getminimal()!=0) && (buf->getmode()!=0) 
			    && (PFreqEnvelopeEnabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			FreqEnvelope->saveloadbuf(buf);
			break;
	    case 0xA9:	if ((buf->getminimal()!=0) && (buf->getmode()!=0) 
			    && (PBandWidthEnvelopeEnabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			BandWidthEnvelope->saveloadbuf(buf);
			break;
	    case 0xAA:	buf->rwbytepar(n,&PfixedfreqET);
			break;
	    //Filter Parameters
	    case 0xB0:	buf->rwbytepar(n,&PGlobalFilterEnabled);
			break;
	    case 0xB1:	buf->rwbytepar(n,&PGlobalFilterVelocityScale);
			break;
	    case 0xB2:	buf->rwbytepar(n,&PGlobalFilterVelocityScaleFunction);
			break;
	    case 0xB8:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalFilter->saveloadbuf(buf);
			break;
	    case 0xB9:	if (buf->getmode()!=0) buf->rwbyte(&npar);
			GlobalFilterEnvelope->saveloadbuf(buf);
			break;
	};
    };

    
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
};


void SUBnoteParameters::add2XML(XMLwrapper *xml){
    xml->addpar("num_stages",Pnumstages);
    xml->addpar("harmonic_mag_type",Phmagtype);
    xml->addpar("start",Pstart);
    
    xml->beginbranch("HARMONICS");
	for (int i=0;i<MAX_SUB_HARMONICS;i++){
	    if (Phmag[i]==0) continue;
	    xml->beginbranch("HARMONIC",i);
		xml->addpar("mag",Phmag[i]);
		xml->addpar("relbw",Phrelbw[i]);
	    xml->endbranch();
	};
    xml->endbranch();

    xml->beginbranch("AMPLITUDE_PARAMETERS");
	xml->addparbool("stereo",Pstereo);
	xml->addpar("volume",PVolume);
	xml->addpar("panning",PPanning);
	xml->addpar("velocity_sensing",PAmpVelocityScaleFunction);
	xml->beginbranch("AMPLITUDE_ENVELOPE");
	    AmpEnvelope->add2XML(xml);
	xml->endbranch();
    xml->endbranch();

    xml->beginbranch("FREQUENCY_PARAMETERS");
	xml->addparbool("fixed_freq",Pfixedfreq);
	xml->addpar("fixed_freq_et",PfixedfreqET);

	xml->addpar("detune",PDetune);
	xml->addpar("coarse_detune",PCoarseDetune);
	xml->addpar("detune_type",PDetuneType);

	xml->addpar("bandwidth",Pbandwidth);
	xml->addpar("bandwidth_scale",Pbwscale);

	xml->addparbool("freq_envelope_enabled",PFreqEnvelopeEnabled);
	if (PFreqEnvelopeEnabled!=0){
	    xml->beginbranch("FREQUENCY_ENVELOPE");
	        FreqEnvelope->add2XML(xml);
	    xml->endbranch();
	};

	xml->addparbool("band_width_envelope_enabled",PBandWidthEnvelopeEnabled);
	if (PBandWidthEnvelopeEnabled!=0){
	    xml->beginbranch("BANDWIDTH_ENVELOPE");
	        BandWidthEnvelope->add2XML(xml);
	    xml->endbranch();
	};
    xml->endbranch();

    xml->beginbranch("FILTER_PARAMETERS");
	xml->addparbool("enabled",PGlobalFilterEnabled);
	if (PGlobalFilterEnabled!=0){
	    xml->beginbranch("FILTER");
		GlobalFilter->add2XML(xml);
	    xml->endbranch();

	    xml->addpar("filter_velocity_sensing",PGlobalFilterVelocityScaleFunction);
	    xml->addpar("filter_velocity_sensing_amplitude",PGlobalFilterVelocityScale);

	    xml->beginbranch("FILTER_ENVELOPE");
		GlobalFilterEnvelope->add2XML(xml);
	    xml->endbranch();
	};
    xml->endbranch();
};

void SUBnoteParameters::getfromXML(XMLwrapper *xml){
    Pnumstages=xml->getpar127("num_stages",Pnumstages);
    Phmagtype=xml->getpar127("harmonic_mag_type",Phmagtype);
    Pstart=xml->getpar127("start",Pstart);
    
    if (xml->enterbranch("HARMONICS")){
	Phmag[0]=0;
	for (int i=0;i<MAX_SUB_HARMONICS;i++){
	    if (xml->enterbranch("HARMONIC",i)==0) continue;
		Phmag[i]=xml->getpar127("mag",Phmag[i]);
		Phrelbw[i]=xml->getpar127("relbw",Phrelbw[i]);
	    xml->exitbranch();
	};
	xml->exitbranch();
    };

    if (xml->enterbranch("AMPLITUDE_PARAMETERS")){
	Pstereo=xml->getparbool("stereo",Pstereo);
	PVolume=xml->getpar127("volume",PVolume);
	PPanning=xml->getpar127("panning",PPanning);
	PAmpVelocityScaleFunction=xml->getpar127("velocity_sensing",PAmpVelocityScaleFunction);
	if (xml->enterbranch("AMPLITUDE_ENVELOPE")){
	    AmpEnvelope->getfromXML(xml);
	    xml->exitbranch();
	};
	xml->exitbranch();
    };

    if (xml->enterbranch("FREQUENCY_PARAMETERS")){
	Pfixedfreq=xml->getparbool("fixed_freq",Pfixedfreq);
	PfixedfreqET=xml->getpar127("fixed_freq_et",PfixedfreqET);

	PDetune=xml->getpar("detune",PDetune,0,16383);
	PCoarseDetune=xml->getpar("coarse_detune",PCoarseDetune,0,16383);
	PDetuneType=xml->getpar127("detune_type",PDetuneType);

	Pbandwidth=xml->getpar127("bandwidth",Pbandwidth);
	Pbwscale=xml->getpar127("bandwidth_scale",Pbwscale);

	PFreqEnvelopeEnabled=xml->getparbool("freq_envelope_enabled",PFreqEnvelopeEnabled);
	if (xml->enterbranch("FREQUENCY_ENVELOPE")){
	    FreqEnvelope->getfromXML(xml);
	    xml->exitbranch();
	};

	PBandWidthEnvelopeEnabled=xml->getparbool("band_width_envelope_enabled",PBandWidthEnvelopeEnabled);
	if (xml->enterbranch("BANDWIDTH_ENVELOPE")){
	    BandWidthEnvelope->getfromXML(xml);
	    xml->exitbranch();
	};
    
        xml->exitbranch();
    };
    
    if (xml->enterbranch("FILTER_PARAMETERS")){
	PGlobalFilterEnabled=xml->getparbool("enabled",PGlobalFilterEnabled);
	if (xml->enterbranch("FILTER")){
	    GlobalFilter->getfromXML(xml);
	    xml->exitbranch();
	};

	PGlobalFilterVelocityScaleFunction=xml->getpar127("filter_velocity_sensing",PGlobalFilterVelocityScaleFunction);
	PGlobalFilterVelocityScale=xml->getpar127("filter_velocity_sensing_amplitude",PGlobalFilterVelocityScale);

	if (xml->enterbranch("FILTER_ENVELOPE")){
	    GlobalFilterEnvelope->getfromXML(xml);
	    xml->exitbranch();
	};
	
	xml->exitbranch();
    };
};





/*
  ZynAddSubFX - a software synthesizer
 
  LFOParams.C - Parameters for LFO
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

#include <math.h>
#include <stdio.h>
#include "../globals.h"
#include "LFOParams.h"

int LFOParams::time;

LFOParams::LFOParams(char Pfreq_,char Pintensity_,char Pstartphase_, char PLFOtype_,char Prandomness_, char Pdelay_,char Pcontinous_,char fel_){
    Dfreq=Pfreq_;
    Dintensity=Pintensity_;
    Dstartphase=Pstartphase_;
    DLFOtype=PLFOtype_;
    Drandomness=Prandomness_;
    Ddelay=Pdelay_;
    Dcontinous=Pcontinous_;
    fel=fel_;
    time=0;
    
    defaults();
};

LFOParams::~LFOParams(){
};

void LFOParams::defaults(){
    Pfreq=Dfreq;
    Pintensity=Dintensity;
    Pstartphase=Dstartphase;
    PLFOtype=DLFOtype;
    Prandomness=Drandomness;
    Pdelay=Ddelay;
    Pcontinous=Dcontinous;
    Pfreqrand=0;
};

/*
 * Save or load the parameters to/from the buffer
 */
void LFOParams::saveloadbuf(Buffer *buf){
    unsigned char npar,n,tmp;

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n( LFO parameters) \n");
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
	    //LFO parameters
	    case 0x80:	buf->rwbytepar(n,&Pfreq);
			break;
	    case 0x81:	buf->rwbytepar(n,&Pintensity);
			break;
	    case 0x82:	buf->rwbytepar(n,&Pstartphase);
			break;
	    case 0x83:	buf->rwbytepar(n,&PLFOtype);
			break;
	    case 0x84:	buf->rwbytepar(n,&Prandomness);
			break;
	    case 0x85:	buf->rwbytepar(n,&Pdelay);
			break;
	    case 0x86:	buf->rwbytepar(n,&Pcontinous);
			break;
	    case 0x87:	buf->rwbytepar(n,&Pfreqrand);
			break;
	};
    };

    
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
};

void LFOParams::add2XML(XMLwrapper *xml){
    xml->addpar("freq",Pfreq);
    xml->addpar("intensity",Pintensity);
    xml->addpar("startphase",Pstartphase);
    xml->addpar("LFOtype",PLFOtype);
    xml->addpar("randomness",Prandomness);
    xml->addpar("freqrand",Pfreqrand);
    xml->addpar("delay",Pdelay);
    xml->addparbool("continous",Pcontinous);
};



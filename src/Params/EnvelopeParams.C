/*
  ZynAddSubFX - a software synthesizer
 
  EnvelopeParams.C - Parameters for Envelope
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

#include <math.h>
#include <stdlib.h>
#include "EnvelopeParams.h"

EnvelopeParams::EnvelopeParams(unsigned char Penvstretch_,unsigned char Pforcedrelease_){
    int i;
    
    PA_dt=10;PD_dt=10;PR_dt=10;PA_val=64;PD_val=64;PS_val=64;PR_val=64;
    
    for (i=0;i<MAX_ENVELOPE_POINTS;i++){
	Penvdt[i]=32;
	Penvval[i]=64;
    };
    Penvdt[0]=0;//no used
    Penvsustain=1;
    Penvpoints=1;
    Envmode=1;
    Penvstretch=Penvstretch_;
    Pforcedrelease=Pforcedrelease_;        
    Pfreemode=1;
    Plinearenvelope=0;
};

EnvelopeParams::~EnvelopeParams(){
};

REALTYPE EnvelopeParams::getdt(char i){
    REALTYPE result=(pow(2.0,Penvdt[i]/127.0*12.0)-1.0)*10.0;//miliseconds
    return(result);
};


/*
 * ADSR/ASR... initialisations
 */
void EnvelopeParams::ADSRinit(char A_dt,char D_dt,char S_val,char R_dt){
    Plinearenvelope=1;
    Envmode=1;
    PA_dt=A_dt;PD_dt=D_dt;PS_val=S_val;PR_dt=R_dt;
    Pfreemode=0;
    converttofree();
};

void EnvelopeParams::ADSRinit_dB(char A_dt,char D_dt,char S_val,char R_dt){
    Envmode=2;
    PA_dt=A_dt;PD_dt=D_dt;PS_val=S_val;PR_dt=R_dt;
    Pfreemode=0;
    converttofree();
};

void EnvelopeParams::ASRinit(char A_val,char A_dt,char R_val,char R_dt){
    Envmode=3;
    PA_val=A_val;PA_dt=A_dt;PR_val=R_val;PR_dt=R_dt;
    Pfreemode=0;
    converttofree();

};

void EnvelopeParams::ADSRinit_filter(char A_val,char A_dt,char D_val,char D_dt,char R_dt,char R_val){
    Envmode=4;
    PA_val=A_val;PA_dt=A_dt;PD_val=D_val;PD_dt=D_dt;PR_dt=R_dt;PR_val=R_val;
    Pfreemode=0;
    converttofree();
};

void EnvelopeParams::ASRinit_bw(char A_val,char A_dt,char R_val,char R_dt){
    Envmode=5;
    PA_val=A_val;PA_dt=A_dt;PR_val=R_val;PR_dt=R_dt;
    Pfreemode=0;
    converttofree();

};

/*
 * Convert the Envelope to freemode
 */
void EnvelopeParams::converttofree(){
    switch (Envmode){
	case 1:	Penvpoints=4;Penvsustain=2;
		Penvval[0]=0;Penvdt[1]=PA_dt;Penvval[1]=127;
		Penvdt[2]=PD_dt;Penvval[2]=PS_val;
		Penvdt[3]=PR_dt;Penvval[3]=0;    
		break;
	case 2: Penvpoints=4;Penvsustain=2;
		Penvval[0]=0;Penvdt[1]=PA_dt;
		Penvval[1]=127;Penvdt[2]=PD_dt;
		Penvval[2]=PS_val;Penvdt[3]=PR_dt;Penvval[3]=0;    
		break;	
	case 3:	Penvpoints=3;Penvsustain=1;
    		Penvval[0]=PA_val;Penvdt[1]=PA_dt;
		Penvval[1]=64;Penvdt[2]=PR_dt;Penvval[2]=PR_val;
		break;
	case 4:	Penvpoints=4;Penvsustain=2;
		Penvval[0]=PA_val;Penvdt[1]=PA_dt;
		Penvval[1]=PD_val;Penvdt[2]=PD_dt;Penvval[2]=64;
		Penvdt[3]=PR_dt;Penvval[3]=PR_val;
		break;
	case 5:	Penvpoints=3;Penvsustain=1;
    		Penvval[0]=PA_val;Penvdt[1]=PA_dt;
		Penvval[1]=64;Penvdt[2]=PR_dt;Penvval[2]=PR_val;
		break;
    };
};


/*
 * Save or load the parameters to/from the buffer
 */
void EnvelopeParams::saveloadbuf(Buffer *buf){
    unsigned char npar,n,tmp,np;

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n( Envelope parameters) \n");
#endif    

    tmp=0xfe;
    buf->rwbyte(&tmp);//if tmp!=0xfe error


    for (n=0x80;n<0xF0;n++){
	if (buf->getmode()==0) {
	    buf->rwbyte(&npar);
	    n=0;//force a loop until the end of parameters (0xff)
	} else npar=n;
	if (npar==0xff) break;
	
	switch(npar){
	    //Envelope parameters
	    case 0x80:	buf->rwbytepar(n,&Pfreemode);
			break;
	    case 0x81:	buf->rwbytepar(n,&Penvstretch);
			break;
	    case 0x82:	buf->rwbytepar(n,&Pforcedrelease);
			break;
	    case 0x83:	buf->rwbytepar(n,&Penvpoints); 
			break;
	    case 0x84:	buf->rwbytepar(n,&Penvsustain); 
			break;
	    case 0x85:	buf->rwbytepar(n,&Plinearenvelope); 
			break;
	    case 0x90:	if (buf->getmode()!=0){
			    for (np=0;np<Penvpoints;np++){
				buf->rwbytepar(n,&np);
				buf->rwbyte(&Penvdt[np]);
				buf->rwbyte(&Penvval[np]);
			    };
			} else {
			    buf->rwbytepar(np,&np);
			    if (np<MAX_ENVELOPE_POINTS){//for safety
				buf->rwbyte(&Penvdt[np]);
				buf->rwbyte(&Penvval[np]);
			    } else{
				buf->rwbyte(&tmp);
				buf->rwbyte(&tmp);
			    };
			};
			break;
	    case 0xA0:	buf->rwbytepar(n,&PA_dt);
			break;
	    case 0xA1:	buf->rwbytepar(n,&PD_dt);
			break;
	    case 0xA2:	buf->rwbytepar(n,&PR_dt);
			break;
	    case 0xA3:	buf->rwbytepar(n,&PA_val);
			break;
	    case 0xA4:	buf->rwbytepar(n,&PD_val);
			break;
	    case 0xA5:	buf->rwbytepar(n,&PS_val);
			break;
	    case 0xA6:	buf->rwbytepar(n,&PR_val);
			break;
	};
    };

    if (Pfreemode==0) converttofree();
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
};

void EnvelopeParams::add2XML(XMLwrapper *xml){
    xml->addparbool("freemode",Pfreemode);    
    xml->addpar("envpoints",Penvpoints);
    xml->addpar("envsustain",Penvsustain);
    xml->addpar("envstretch",Penvstretch);
    xml->addparbool("forcedrelease",Pforcedrelease);
    xml->addparbool("linearenvelope",Plinearenvelope);
    xml->addpar("A_dt",PA_dt);
    xml->addpar("D_dt",PD_dt);
    xml->addpar("R_dt",PR_dt);
    xml->addpar("A_val",PA_val);
    xml->addpar("D_val",PD_val);
    xml->addpar("S_val",PS_val);
    xml->addpar("R_val",PR_val);

    if (Pfreemode!=0){
	for (int i=0;i<Penvpoints;i++){
	    xml->beginbranch("POINT",i);
		if (i!=0) xml->addpar("envdt",Penvdt[i]);
		xml->addpar("envval",Penvval[i]);
	    xml->endbranch();
	};
    };
};




/*
  ZynAddSubFX - a software synthesizer
 
  Microtonal.C - Tuning settings and microtonal capabilities
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
#include <string.h>
#include "Microtonal.h"

#define MAX_LINE_SIZE 80

Microtonal::Microtonal(){
    Pname=new unsigned char[MICROTONAL_MAX_NAME_LEN];
    Pcomment=new unsigned char[MICROTONAL_MAX_NAME_LEN];
    defaults();
};

void Microtonal::defaults(){
    Pinvertupdown=0;
    Pinvertupdowncenter=60;
    octavesize=12;
    Penabled=0;
    PAnote=69;
    PAfreq=440.0;
    Pscaleshift=64;
    
    Pfirstkey=0;Plastkey=127;
    Pmiddlenote=60;Pmapsize=12;
    Pmappingenabled=0;
    
    for (int i=0;i<128;i++) Pmapping[i]=i;
    
    for (int i=0;i<MAX_OCTAVE_SIZE;i++){
	octave[i].tuning=tmpoctave[i].tuning=pow(2,(i%octavesize+1)/12.0);
	octave[i].type=tmpoctave[i].type=1;
	octave[i].x1=tmpoctave[i].x1=(i%octavesize+1)*100;
	octave[i].x2=tmpoctave[i].x2=0;
    };
    octave[11].type=2;octave[11].x1=2;octave[11].x2=1;
    for (int i=0;i<MICROTONAL_MAX_NAME_LEN;i++){
	Pname[i]='\0';
	Pcomment[i]='\0';
    };
    snprintf((char *) Pname,MICROTONAL_MAX_NAME_LEN,"12tET");
    snprintf((char *) Pcomment,MICROTONAL_MAX_NAME_LEN,"Equal Temperament 12 notes per octave");    
    Pglobalfinedetune=64;
};

Microtonal::~Microtonal(){
    delete (Pname);
    delete (Pcomment);
};

/*
 * Get the size of the octave
 */
unsigned char Microtonal::getoctavesize(){
    if (Penabled!=0) return(octavesize);
	else return(12);
};

/*
 * Get the frequency according the note number
 */
REALTYPE Microtonal::getnotefreq(int note,int keyshift){
    // in this function will appears many times things like this:
    // var=(a+b*100)%b 
    // I had written this way because if I use var=a%b gives unwanted results when a<0
    // This is the same with divisions.
    
    if ((Pinvertupdown!=0)&&((Pmappingenabled==0)||(Penabled==0))) note=(int) Pinvertupdowncenter*2-note;

    //compute global fine detune
    REALTYPE globalfinedetunerap=pow(2.0,(Pglobalfinedetune-64.0)/1200.0);//-64.0 .. 63.0 cents
        
    if (Penabled==0) return(pow(2.0,(note-PAnote+keyshift)/12.0)*PAfreq*globalfinedetunerap);//12tET
    
    int scaleshift=((int)Pscaleshift-64+(int) octavesize*100)%octavesize;

    //compute the keyshift
    REALTYPE rap_keyshift=1.0;
    if (keyshift!=0){
	int kskey=(keyshift+(int)octavesize*100)%octavesize;
	int ksoct=(keyshift+(int)octavesize*100)/octavesize-100;
	rap_keyshift=(kskey==0) ? (1.0):(octave[kskey-1].tuning);
	rap_keyshift*=pow(octave[octavesize-1].tuning,ksoct);
    };

    //if the mapping is enabled
    if (Pmappingenabled!=0){
	if ((note<Pfirstkey)||(note>Plastkey)) return (-1.0);
	//Compute how many mapped keys are from middle note to reference note
	//and find out the proportion between the freq. of middle note and "A" note
	int tmp=PAnote-Pmiddlenote,minus=0;
	if (tmp<0) { tmp=-tmp; minus=1; };
	int deltanote=0;
	for (int i=0;i<tmp;i++) if (Pmapping[i%Pmapsize]>=0) deltanote++;
	REALTYPE rap_anote_middlenote=(deltanote==0) ? (1.0) : (octave[(deltanote-1)%octavesize].tuning);
	if (deltanote!=0) rap_anote_middlenote*=pow(octave[octavesize-1].tuning,(deltanote-1)/octavesize);
	if (minus!=0) rap_anote_middlenote=1.0/rap_anote_middlenote;
	
	//Convert from note (midi) to degree (note from the tunning)
	int degoct=(note-(int)Pmiddlenote+(int) Pmapsize*200)/(int)Pmapsize-200;
	int degkey=(note-Pmiddlenote+(int)Pmapsize*100)%Pmapsize;
	degkey=Pmapping[degkey];
	if (degkey<0) return(-1.0);//this key is not mapped
	
	//invert the keyboard upside-down if it is asked for
	//TODO: do the right way by using Pinvertupdowncenter
	if (Pinvertupdown!=0){
	    degkey=octavesize-degkey-1;
	    degoct=-degoct;
	};
	//compute the frequency of the note
	degkey=degkey+scaleshift;
	degoct+=degkey/octavesize;
	degkey%=octavesize;

	REALTYPE freq=(degkey==0) ? (1.0):octave[degkey-1].tuning;
	freq*=pow(octave[octavesize-1].tuning,degoct);
	freq*=PAfreq/rap_anote_middlenote;
	freq*=globalfinedetunerap;
	if (scaleshift!=0) freq/=octave[scaleshift-1].tuning;
	return(freq*rap_keyshift);
    } else {//if the mapping is disabled
	int nt=note-PAnote+scaleshift;
	int ntkey=(nt+(int)octavesize*100)%octavesize;
	int ntoct=(nt-ntkey)/octavesize;    
    
	REALTYPE oct=octave[octavesize-1].tuning;
	REALTYPE freq=octave[(ntkey+octavesize-1)%octavesize].tuning*pow(oct,ntoct)*PAfreq;
	if (ntkey==0) freq/=oct;
        if (scaleshift!=0) freq/=octave[scaleshift-1].tuning;
//	fprintf(stderr,"note=%d freq=%.3f cents=%d\n",note,freq,(int)floor(log(freq/PAfreq)/log(2.0)*1200.0+0.5));
	freq*=globalfinedetunerap;
    	return(freq*rap_keyshift);
    };
};


/*
 * Convert a line to tunings; returns -1 if it ok
 */
int Microtonal::linetotunings(unsigned int nline,const char *line){
    int x1=-1,x2=-1,type=-1;
    REALTYPE x=-1.0,tmp,tuning;
    if (strstr(line,"/")==NULL){
	if (strstr(line,".")==NULL){// M case (M=M/1)
	    sscanf(line,"%d",&x1);
	    x2=1;
	    type=2;//division
	} else {// float number case
    	    sscanf(line,"%f",&x);
	    if (x<0.000001) return(1);
	    type=1;//float type(cents)
	};
    } else {// M/N case
	sscanf(line,"%d/%d",&x1,&x2);
	if ((x1<0)||(x2<0)) return(1);
	if (x2==0) x2=1;
	type=2;//division
    };
    
    if (x1<=0) x1=1;//not allow zero frequency sounds (consider 0 as 1)
    
    //convert to float if the number are too big
    if ((type==2)&&((x1>(128*128*128-1))||(x2>(128*128*128-1)))){
	type=1;
	x=((REALTYPE) x1)/x2;
    };
    switch (type){
	case 1:	x1=(int) floor(x);
		tmp=fmod(x,1.0);
		x2=(int) (floor (tmp*1e6));
		tuning=pow(2.0,x/1200.0);
		break;
	case 2:	x=((REALTYPE)x1)/x2;
		tuning=x;
		break;
    };
    
    tmpoctave[nline].tuning=tuning;
    tmpoctave[nline].type=type;
    tmpoctave[nline].x1=x1;
    tmpoctave[nline].x2=x2;
    
    return(-1);//ok
};

/*
 * Convert the text to tunnings
 */
int Microtonal::texttotunings(const char *text){
    unsigned int i,k=0,nl=0;
    char *lin;
    lin=new char[MAX_LINE_SIZE+1];
    while (k<strlen(text)){
	for (i=0;i<MAX_LINE_SIZE;i++){
	    lin[i]=text[k++];
	    if (lin[i]<0x20) break;
	};
	lin[i]='\0';
	if (strlen(lin)==0) continue;
	int err=linetotunings(nl,lin);
	if (err!=-1) {
	    delete [] lin;
	    return(nl);//Parse error
	};
	nl++;
    };
    delete [] lin;
    if (nl>MAX_OCTAVE_SIZE) nl=MAX_OCTAVE_SIZE;
    if (nl==0) return(-2);//the input is empty
    octavesize=nl;
    for (i=0;i<octavesize;i++){
	octave[i].tuning=tmpoctave[i].tuning;
	octave[i].type=tmpoctave[i].type;
	octave[i].x1=tmpoctave[i].x1;
	octave[i].x2=tmpoctave[i].x2;
    };
    return(-1);//ok
};

/*
 * Convert the text to mapping
 */
void Microtonal::texttomapping(const char *text){
    unsigned int i,k=0;
    char *lin;
    lin=new char[MAX_LINE_SIZE+1];
    for (i=0;i<128;i++) Pmapping[i]=-1;
    int tx=0;
    while (k<strlen(text)){
	for (i=0;i<MAX_LINE_SIZE;i++){
	    lin[i]=text[k++];
	    if (lin[i]<0x20) break;
	};
	lin[i]='\0';
	if (strlen(lin)==0) continue;

	if (sscanf(lin,"%d",&Pmapping[tx])==0) Pmapping[tx]=-1;
	if (Pmapping[tx]<-1) Pmapping[tx]=-1;
	
	if ((tx++)>127) tx=127;
    };
    delete [] lin;
    
    if (tx==0) tx=1;
    Pmapsize=tx;
};

/*
 * Convert tunning to text line
 */
void Microtonal::tuningtoline(int n,char *line,int maxn){
    if ((n>octavesize) || (n>MAX_OCTAVE_SIZE)) {
	line[0]='\0';
	return;
    };
    if (octave[n].type==1) snprintf(line,maxn,"%d.%d",octave[n].x1,octave[n].x2);
    if (octave[n].type==2) snprintf(line,maxn,"%d/%d",octave[n].x1,octave[n].x2);
};

 
int Microtonal::loadline(FILE *file,char *line){
    do {
    if (fgets(line,500,file)==0) return(1);
    } while (line[0]=='!');
    return(0);
};
/*
 * Loads the tunnings from a scl file
 */
int Microtonal::loadscl(const char *filename){
    FILE *file=fopen(filename, "r");
    char tmp[500];
    fseek(file,0,SEEK_SET);
    //loads the short description
    if (loadline(file,&tmp[0])!=0) return(2);
    for (int i=0;i<500;i++) if (tmp[i]<32) tmp[i]=0;
    snprintf((char *) Pname,MICROTONAL_MAX_NAME_LEN,"%s",tmp);
    snprintf((char *) Pcomment,MICROTONAL_MAX_NAME_LEN,"%s",tmp);
    //loads the number of the notes
    if (loadline(file,&tmp[0])!=0) return(2);
    int nnotes=MAX_OCTAVE_SIZE;
    sscanf(&tmp[0],"%d",&nnotes);
    if (nnotes>MAX_OCTAVE_SIZE) return (2);
    //load the tunnings
    for (int nline=0;nline<nnotes;nline++){
	if (loadline(file,&tmp[0])!=0) return(2);
	linetotunings(nline,&tmp[0]);
    };
    fclose(file);

    octavesize=nnotes;
    for (int i=0;i<octavesize;i++){
	octave[i].tuning=tmpoctave[i].tuning;
	octave[i].type=tmpoctave[i].type;
	octave[i].x1=tmpoctave[i].x1;
	octave[i].x2=tmpoctave[i].x2;
    };

    return(0);
};

/*
 * Loads the mapping from a kbm file
 */
int Microtonal::loadkbm(const char *filename){
    FILE *file=fopen(filename, "r");
    int x;
    char tmp[500];

    fseek(file,0,SEEK_SET);
    //loads the mapsize
    if (loadline(file,&tmp[0])!=0) return(2);
    if (sscanf(&tmp[0],"%d",&x)==0) return(2);
    if (x<1) x=0;if (x>127) x=127;//just in case...
    Pmapsize=x;
    //loads first MIDI note to retune
    if (loadline(file,&tmp[0])!=0) return(2);
    if (sscanf(&tmp[0],"%d",&x)==0) return(2);
    if (x<1) x=0;if (x>127) x=127;//just in case...
    Pfirstkey=x;
    //loads last MIDI note to retune
    if (loadline(file,&tmp[0])!=0) return(2);
    if (sscanf(&tmp[0],"%d",&x)==0) return(2);
    if (x<1) x=0;if (x>127) x=127;//just in case...
    Plastkey=x;
    //loads last the middle note where scale fro scale degree=0
    if (loadline(file,&tmp[0])!=0) return(2);
    if (sscanf(&tmp[0],"%d",&x)==0) return(2);
    if (x<1) x=0;if (x>127) x=127;//just in case...
    Pmiddlenote=x;
    //loads the reference note
    if (loadline(file,&tmp[0])!=0) return(2);
    if (sscanf(&tmp[0],"%d",&x)==0) return(2);
    if (x<1) x=0;if (x>127) x=127;//just in case...
    PAnote=x;
    //loads the reference freq.
    if (loadline(file,&tmp[0])!=0) return(2);
    REALTYPE tmpPAfreq=440.0;
    if (sscanf(&tmp[0],"%f",&tmpPAfreq)==0) return(2);
    PAfreq=tmpPAfreq;

    //the scale degree(which is the octave) is not loaded, it is obtained by the tunnings with getoctavesize() method
    if (loadline(file,&tmp[0])!=0) return(2);

    //load the mappings
    if (Pmapsize!=0){
	for (int nline=0;nline<Pmapsize;nline++){
	    if (loadline(file,&tmp[0])!=0) return(2);
	    if (sscanf(&tmp[0],"%d",&x)==0) x=-1;
	    Pmapping[nline]=x;
	};
	Pmappingenabled=1;
    } else {
	Pmappingenabled=0;
	Pmapping[0]=0;
	Pmapsize=1;
    };
    fclose(file);

    return(0);
};


/*
 * Save or load the parameters to/from the buffer
 */
void Microtonal::saveloadbuf(Buffer *buf){
    unsigned char npar,n,tmp;

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n( Microtonal parameters) \n");
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
	    //Microtonal parameters
	    case 0x80:	buf->rwbytepar(n,&Penabled);
			break;
	    case 0x81:	buf->rwbytepar(n,&Pinvertupdown);
			break;
	    case 0x82:	buf->rwbytepar(n,&Pinvertupdowncenter);
			break;
	    case 0x83:	buf->rwbytepar(n,&PAnote);
			break;
	    case 0x84:	if (buf->getmode()!=0){
			    buf->rwbyte(&npar);
			    unsigned short int freqint=(unsigned short int) floor(PAfreq);
			    if (freqint>16383) freqint=16383;
			    buf->rwword(&freqint);
			    unsigned short int freqmod=(unsigned short int)(fmod(PAfreq,1.0)*10000.0);
			    buf->rwword(&freqmod);
			} else {
			    unsigned short int freqint;
			    buf->rwword(&freqint);
			    unsigned short int freqmod;
			    buf->rwword(&freqmod);
			    PAfreq=freqint+freqmod/10000.0;
			};
			break;
	    case 0x85:	if ((buf->getmode()!=0) && (buf->getminimal()!=0) && (Penabled==0)) break;
			buf->rwbytepar(n,&Pscaleshift);
			break;
	    case 0x86:	if (buf->getmode()!=0){
			    if ((Penabled==0) && (buf->getminimal()!=0)) break;
			    for (unsigned char k=0;k<octavesize;k++){
				buf->rwbyte(&npar);
				tmp=0;buf->rwbyte(&tmp);//need in the future if the octave will be >127 keys
				buf->rwbyte(&k);
				buf->rwbyte(&octave[k].type);
				unsigned int x=octave[k].x1;
				tmp=x/16384;buf->rwbyte(&tmp);
				tmp=(x%16384)/128;buf->rwbyte(&tmp);
				tmp=x%128;buf->rwbyte(&tmp);
				x=octave[k].x2;
				tmp=x/16384;buf->rwbyte(&tmp);
				tmp=(x%16384)/128;buf->rwbyte(&tmp);
				tmp=x%128;buf->rwbyte(&tmp);
			    };
			} else {
			    buf->rwbyte(&tmp); //need in the future for octacesize>127
			    unsigned char k;			    
			    buf->rwbyte(&k);
			    if (k>=MAX_OCTAVE_SIZE) k=MAX_OCTAVE_SIZE;
			    buf->rwbyte(&octave[k].type);
			    unsigned int x=0;
			    buf->rwbyte(&tmp);x=tmp*(int) 16384;
			    buf->rwbyte(&tmp);x+=tmp*(int) 128;
			    buf->rwbyte(&tmp);x+=tmp;
			    octave[k].x1=x;
			    buf->rwbyte(&tmp);x=tmp*(int) 16384;
			    buf->rwbyte(&tmp);x+=tmp*(int) 128;
			    buf->rwbyte(&tmp);x+=tmp;
			    octave[k].x2=x;
			    
			    if (octave[k].type==1) {
				octave[k].tuning=pow(2.0
				,(octave[k].x1+octave[k].x2/(REALTYPE)1e6)/1200.0);
			    };
			    if (octave[k].type==2) {
				if (octave[k].x2==0) octave[k].x2=1;
				octave[k].tuning=octave[k].x1/(REALTYPE) octave[k].x2;
			    };
			    
			};
			break;
	    case 0x87:	if ((buf->getmode()!=0) && (buf->getminimal()!=0) && (Penabled==0)) break;
			buf->rwbytepar(n,&octavesize);
			break;
	    case 0x88:	if (buf->getmode()!=0){
			    if ((Penabled==0) && (buf->getminimal()!=0)) break;
			    buf->rwbyte(&npar);
			    int k=strlen((char *)Pname);
			    if (k>MICROTONAL_MAX_NAME_LEN-2) {
				k=MICROTONAL_MAX_NAME_LEN-2;
			    };
			    Pname[k]='\0';
			    for (int i=0;i<k+1;i++){
				unsigned char tmp=Pname[i];
				buf->rwbyte(&tmp);
			    };
			} else {
			    unsigned char k=0,tmp=1;
			    while ((tmp!=0)&&(k<=MICROTONAL_MAX_NAME_LEN-2)){
				buf->rwbyte(&tmp);
				Pname[k++]=tmp;
			    };
			    Pname[k]='\0';
			};
	    break;
	    case 0x89:	if (buf->getmode()!=0){
			    if ((Penabled==0) && (buf->getminimal()!=0)) break;
			    buf->rwbyte(&npar);
			    int k=strlen((char *)Pcomment);
			    if (k>MICROTONAL_MAX_NAME_LEN-2) {
				k=MICROTONAL_MAX_NAME_LEN-2;
			    };
			    Pcomment[k]='\0';
			    for (int i=0;i<k+1;i++){
				unsigned char tmp=Pcomment[i];
				buf->rwbyte(&tmp);
			    };
			} else {
			    unsigned char k=0,tmp=1;
			    while ((tmp!=0)&&(k<=MICROTONAL_MAX_NAME_LEN-2)){
				buf->rwbyte(&tmp);
				Pcomment[k++]=tmp;
			    };
			    Pcomment[k]='\0';
			};
	    break;
	    case 0x90:	if ((buf->getmode()!=0) && (buf->getminimal()!=0) && (Penabled==0)) break;
			buf->rwbytepar(n,&Pmappingenabled);
			break;
	    case 0x91:	if ((buf->getmode()!=0) && (buf->getminimal()!=0) && ((Penabled==0)||(Pmappingenabled==0))) break;
			buf->rwbytepar(n,&Pfirstkey);
			break;
	    case 0x92:	if ((buf->getmode()!=0) && (buf->getminimal()!=0) && ((Penabled==0)||(Pmappingenabled==0))) break;
			buf->rwbytepar(n,&Plastkey);
			break;
	    case 0x93:	if ((buf->getmode()!=0) && (buf->getminimal()!=0) && ((Penabled==0)||(Pmappingenabled==0))) break;
			buf->rwbytepar(n,&Pmiddlenote);
			break;
	    case 0x94:	if (buf->getmode()!=0){
			    if ((buf->getminimal()!=0) && ((Penabled==0)||(Pmappingenabled==0))) break;
			    buf->rwbyte(&npar);
			    buf->rwbyte(&Pmapsize);
			    for (unsigned char k=0;k<Pmapsize;k++){
			        unsigned short int tmp=16383;//-1
			        if (Pmapping[k]>=0) tmp=Pmapping[k];
			        buf->rwword(&tmp);
			    };
			} else {
			    buf->rwbyte(&Pmapsize);
			    for (unsigned char k=0;k<Pmapsize;k++){
			        unsigned short int tmp;
			        buf->rwword(&tmp);
			        if (tmp!=16383) Pmapping[k]=tmp;
				    else Pmapping[k]=-1;
			    };
			};
			break;
	    case 0x95:	buf->rwbytepar(n,&Pglobalfinedetune);
			break;

	};
    };

    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
};


void Microtonal::add2XML(XMLwrapper *xml){
    xml->addparstr("name",(char *) Pname);
    xml->addparstr("comment",(char *) Pcomment);

    xml->addparbool("invert_up_down",Pinvertupdown);
    xml->addparbool("invert_up_down_center",Pinvertupdowncenter);

    xml->addparbool("enabled",Penabled);
    xml->addpar("global_fine_detune",Pglobalfinedetune);

    xml->addpar("a_note",PAnote);
    xml->addparreal("a_freq",PAfreq);

    if (Penabled==0) return;

    xml->beginbranch("SCALE");
        xml->addpar("scale_shift",Pscaleshift);
	xml->addpar("first_key",Pfirstkey);
	xml->addpar("last_key",Plastkey);
	xml->addpar("middle_note",Pmiddlenote);

	xml->beginbranch("OCTAVE");
	    xml->addpar("octave_size",octavesize);
	    for (int i=0;i<octavesize;i++){
		xml->beginbranch("DEGREE",i);
		    if (octave[i].type==1){			
			xml->addparreal("cents",octave[i].tuning);
		    };
		    if (octave[i].type==2){			
			xml->addpar("numerator",octave[i].x1);
			xml->addpar("denominator",octave[i].x2);
		    };
		xml->endbranch();
	    };
	xml->endbranch();

	xml->beginbranch("KEYBOARD_MAPPING");
	    xml->addpar("map_size",Pmapsize);
	    xml->addpar("mapping_enabled",Pmappingenabled);
		for (int i=0;i<Pmapsize;i++){
		    xml->beginbranch("KEYMAP",i);
			xml->addpar("degree",Pmapping[i]);
		    xml->endbranch();
		};
	xml->endbranch();
    xml->endbranch();
};

void Microtonal::getfromXML(XMLwrapper *xml){
    xml->getparstr("name",(char *) Pname,MICROTONAL_MAX_NAME_LEN);
    xml->getparstr("comment",(char *) Pcomment,MICROTONAL_MAX_NAME_LEN);

    Pinvertupdown=xml->getparbool("invert_up_down",Pinvertupdown);
    Pinvertupdowncenter=xml->getparbool("invert_up_down_center",Pinvertupdowncenter);

    Penabled=xml->getparbool("enabled",Penabled);
    Pglobalfinedetune=xml->getpar127("global_fine_detune",Pglobalfinedetune);

    PAnote=xml->getpar127("a_note",PAnote);
    PAfreq=xml->getparreal("a_freq",PAfreq,1.0,10000.0);

    if (xml->enterbranch("SCALE")){
	Pscaleshift=xml->getpar127("scale_shift",Pscaleshift);
	Pfirstkey=xml->getpar127("first_key",Pfirstkey);
	Plastkey=xml->getpar127("last_key",Plastkey);
	Pmiddlenote=xml->getpar127("middle_note",Pmiddlenote);

	if (xml->enterbranch("OCTAVE")){
	    octavesize=xml->getpar127("octave_size",octavesize);
	    for (int i=0;i<octavesize;i++){
		if (xml->enterbranch("DEGREE",i)==0) continue;
		    octave[i].x2=0;
		    octave[i].tuning=xml->getparreal("cents",octave[i].tuning);
		    octave[i].x1=xml->getpar127("numerator",octave[i].x1);
		    octave[i].x2=xml->getpar127("denominator",octave[i].x2);
		    
		    if (octave[i].x2!=0) octave[i].type=2; 
			else octave[i].type=1;
		    
		xml->exitbranch();
	    };
	    xml->exitbranch();
	};

	if (xml->enterbranch("KEYBOARD_MAPPING")){
	    Pmapsize=xml->getpar127("map_size",Pmapsize);
	    Pmappingenabled=xml->getpar127("mapping_enabled",Pmappingenabled);
		for (int i=0;i<Pmapsize;i++){
		    if (xml->enterbranch("KEYMAP",i)==0) continue;
			Pmapping[i]=xml->getpar127("degree",Pmapping[i]);
		    xml->exitbranch();
		};
	    xml->exitbranch();
	};
	xml->exitbranch();
    };
};



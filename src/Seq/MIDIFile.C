/*
  ZynAddSubFX - a software synthesizer
 
  MIDIFile.C - MIDI file loader
  Copyright (C) 2003-2004 Nasca Octavian Paul
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
#include <string.h>
#include "MIDIFile.h"


MIDIFile::MIDIFile(){
    midifile=NULL;
    midifilesize=0;
    midifilek=0;
    midieof=false;
};

MIDIFile::~MIDIFile(){
    clearmidifile();
};

int MIDIFile::loadfile(char *filename){
    clearmidifile();
    
    FILE *file=fopen(filename,"r");
    if (file==NULL) return(-1);
    
    char header[4];
    memset(header,0,4);
    fread(header,4,1,file);

    //test to see if this a midi file
    if ((header[0]!='M')||(header[1]!='T')||(header[2]!='h')||(header[3]!='d')){
	fclose(file);
	return(-1);
    };
    
    //get the filesize
    fseek(file,0,SEEK_END);
    midifilesize=ftell(file);
    rewind(file);
    
    midifile=new unsigned char[midifilesize];
    memset(midifile,0,midifilesize);
    fread(midifile,midifilesize,1,file);
    fclose(file);

//    for (int i=0;i<midifilesize;i++) printf("%2x ",midifile[i]);
//    printf("\n");
    
    
    return(0);
};

int MIDIFile::parsemidifile(){

    //read the header
    int chunk=getint32();//MThd
    if (chunk!=0x4d546864) return(-1);
    int size=getint32();
    if (size!=6) return(-1);//header is always 6 bytes long

    int format=getint16();
    printf("format %d\n",format);

    int ntracks=getint16();//this is always if the format is "0"
    printf("ntracks %d\n",ntracks);

    if (ntracks>=NUM_MIDI_CHANNELS){
	ntracks=NUM_MIDI_CHANNELS;
    };
    
    int division=getint16();
    printf("division %d\n",division);
    if (division>=0){//delta time units in each a quater note
//	tick=???;
    } else {//SMPTE (frames/second and ticks/frame)
	printf("ERROR:in MIDIFile.C::parsemidifile() - SMPTE not implemented yet.");
    };    
    
//    for (int n=0;n<ntracks;n++){
	if (parsetrack(0)<0) {
	    clearmidifile();
	    return(-1);
	};
//    };

    return(0);
};

int MIDIFile::parsetrack(int ntrack){
    printf("\n--==*Reading track %d **==--\n",ntrack);

    int chunk=getint32();//MTrk
    if (chunk!=0x4d54726b) return(-1);

    int size=getint32();
    printf("%d\n",size);
//    if (size!=6) return(-1);//header is always 6 bytes long
/*
unsigned long ReadVarLen( void ) {
        unsigned long value;
        byte c;

        if ((value = getc(infile)) & 0x80) {
                value &= 0x7f;
                do  {
                        value = (value << 7) + ((c = getc(infile)) & 0x7f);
                }  while (c & 0x80);
        }
        return value;
}

*/
};

//private members

void MIDIFile::clearmidifile(){
    if (midifile!=NULL) delete(midifile);
    midifile=NULL;
    midifilesize=0;
    midifilek=0;
    midieof=false;
    
    data.tick=0.05;
    
};

unsigned char MIDIFile::getbyte(){
    if (midifilek>=midifilesize) {
	midieof=true;
	return(0);
    };

    return(midifile[midifilek++]);
};

unsigned int MIDIFile::getint32(){
    unsigned int result=0;
    for (int i=0;i<4;i++) {
	result=result*256+getbyte();
    };
    if (midieof) result=0;
    return(result);
};

unsigned short int MIDIFile::getint16(){
    unsigned short int result=0;
    for (int i=0;i<2;i++) {
	result=result*256+getbyte();
    };
    if (midieof) result=0;
    return(result);
};

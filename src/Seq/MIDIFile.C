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

    for (int i=0;i<midifilesize;i++) printf("%2x ",midifile[i]);
    printf("\n");
    
    
    return(0);
};

int MIDIFile::parsemidifile(){
    return(0);
};

//private members

void MIDIFile::clearmidifile(){
    if (midifile!=NULL) delete(midifile);
    midifile=NULL;
    midifilesize=0;

};


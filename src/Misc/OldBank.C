/*
  ZynAddSubFX - a software synthesizer
 
  OldBank.h - Instrument OldBank (Obsolete)
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

#include "OldBank.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

OldBank::OldBank(){
    bankfilename=NULL;bankfiletitle=NULL;lock=1;
    memset(&defaultinsname,0,PART_MAX_NAME_LEN);
    snprintf(defaultinsname,PART_MAX_NAME_LEN,"%s"," ");
    for (int i=0;i<128;i++){
	memset(&ins[i].name[0],0,PART_MAX_NAME_LEN);
	ins[i].size=0;
	ins[i].data=NULL;
    };
    
    struct stat statbuf;

};

OldBank::~OldBank(){
    savefile();
};


/*
 * Get the name of an instrument from the bank
 */
char *OldBank::getname (unsigned char ninstrument){
    if (emptyslot(ninstrument)) return (&defaultinsname[0]);
    return(&ins[ninstrument].name[0]);
};

/*
 * Get the numbered name of an instrument from the bank
 */
char *OldBank::getnamenumbered (unsigned char ninstrument){
    if (ninstrument>=128) return(&tmpinsname[0][0]);
    memset(&tmpinsname[ninstrument][0],0,PART_MAX_NAME_LEN+15);
    snprintf(&tmpinsname[ninstrument][0],PART_MAX_NAME_LEN,"%d. %s",ninstrument+1,getname(ninstrument));
    return(&tmpinsname[ninstrument][0]);
};

/*
 * Changes the name of an instrument
 */
void OldBank::setname(unsigned char ninstrument,const char *newname){
    if (emptyslot(ninstrument)) return;
    strncpy(&ins[ninstrument].name[0],newname,PART_MAX_NAME_LEN-1);
    ins[ninstrument].name[PART_MAX_NAME_LEN-1]='\0';//just in case
    getnamenumbered (ninstrument);
};

/*
 * Check if there is no instrument on a slot from the bank
 */
int OldBank::emptyslot(unsigned char ninstrument){
    if (ninstrument>=128) return(1);
    if (ins[ninstrument].data==NULL) return (1);
	else return(0);
};

/*
 * Removes the instrument from the bank
 */
void OldBank::clearslot(unsigned char ninstrument){
    if (ninstrument>=128) return;
    ins[ninstrument].name[0]='\0';
    ins[ninstrument].size=0;
    if (ins[ninstrument].data!=NULL) delete (ins[ninstrument].data);
    ins[ninstrument].data=NULL;
};

/*
 * Save the instrument to a slot (the instrument is stored in a buffer who was filled
 * with instrument parameters)
 */
void OldBank::savetoslot(unsigned char ninstrument,const char *name,Buffer *buf){
    if (ninstrument>=128) return;
    clearslot(ninstrument);
    ins[ninstrument].size=buf->getsize();
    ins[ninstrument].data=new unsigned char [buf->getsize()];
    buf->getalldata(buf->getsize(),ins[ninstrument].data);
    setname(ninstrument,name);
    savefile();
};

/*
 * Loads the instrument from the bank to a buffer (who will be "dumped" to instrument parameters)
 */
void OldBank::loadfromslot(unsigned char ninstrument,Buffer *buf){
    if (emptyslot(ninstrument)!=0) return;
    buf->putalldata(ins[ninstrument].size,ins[ninstrument].data);
    buf->changemode(0);
};

/*
 * Loads the bank from the current file 
 */
int OldBank::loadfile(){
    int file;

    file=open(bankfilename,O_RDONLY|O_BINARY,00444+00222);
    if (file==-1) return(2);//something went wrong (access denied,..etc.)

    //Load the id.
    char id[2];
    read(file,&id,2);
    if ((id[0]!='N')||(id[1]!='P')) {
	close(file);
	return(3);//invalid data
    };
    
    //Load the format descriptor 
    unsigned char fmt; //0x00..0x7f
    read(file,&fmt,1);
    unsigned char type=fmt%0x10;
    if (type!=3){
	close(file);
	return(4);//the data is loaded as something wrong (eg. a master is loaded as a instrument)
    };
    //coding=fmt/0x10;

    //load the crc
    unsigned char CRC; //0x00..0x7f
    read(file,&CRC,1);
    //if (CRC!=0){};CHECK IF IT IS OK
    
    //get the meta_data - this has to be changed in future
    unsigned short int metadatasize;
    read (file,&metadatasize,2);
    char tmp[2];
    read (file,&tmp[0],2);
    
    int err=0,ni=0;
    do {
	clearslot(ni);
	//get the slot number (used only to see if there is a error)
	unsigned char slotnr;
	read (file,&slotnr,1);
	if (slotnr!=ni){
	    err=1;
	    break;
	};
	//get the name
	unsigned char namesize;
	read (file,&namesize,1);
	memset(&ins[ni].name[0],0,PART_MAX_NAME_LEN);//????????
	read (file,&ins[ni].name[0],namesize);
	//get the data
	unsigned int datasize;
	read (file,&datasize,4);
	if (datasize!=0){
	    ins[ni].data=new unsigned char [datasize];
	    ins[ni].size=datasize;
	    for (unsigned int i=0;i<datasize;i++) ins[ni].data[i]=0xff;//if the file data will be not loaded (because of some error), the data is filled with "Buffer exit codes"
	    if (read (file,&ins[ni].data[0],datasize)==-1) {
		err=1;
		break;
	    };
	};
	//get the meta_data - this has to be changed in future
	unsigned short int metadatasize;
	read (file,&metadatasize,2);
	char tmp[2];
	read (file,&tmp[0],2);

        ni++;
    } while ((err==0)&&(ni<128));

    if (err!=0) {
	fprintf(stderr,"The bank file is corrupt.\n");
	if (ni<128) clearslot(ni);
    };
    close(file);
    return(0);
};

/*
 * Saves the bank to the current file
 */
int OldBank::savefile(){
    int file;
    if (lock!=0) return(2);
    file=open(bankfilename,O_CREAT|O_WRONLY|O_TRUNC|O_BINARY,00444+00222);//overwrite if the file exists
    if (file==-1) return(2);//something went wrong (access denied,..etc.)

    //Save the id.
    char id[2]; id[0]='N';id[1]='P';
    write(file,&id,2);

    //Save the format descriptor (for future formats)
    unsigned char type=3,//bank
		  coding=0;//the encoding (Raw, 7 bit encoding,compressed) (0x00..0x07)
    unsigned char fmt=type+coding*0x10; //0x00..0x7f
    write(file,&fmt,1);
    
    //Save the CRC or 0 for no CRC
    unsigned char CRC=0;//todo, if I do the CRC I do: crc=1+crc % 127;!
    write (file,&CRC,1);
    
    //write the bank meta_data buffer (for future versions)
    unsigned short int metadatasize=2;
    write (file,&metadatasize,2);
    char tmp[2]; tmp[0]=0xfe;tmp[1]=0xff;
    write (file,&tmp[0],metadatasize);	
    
    //Save the data
    for (int ni=0;ni<128;ni++){
	//write the slot number (used only to see if there is a error)
	unsigned char slotnr=ni;
	write (file,&slotnr,1);
	//write the instrument name
	unsigned char namesize=strlen(ins[ni].name);
	if (namesize>=PART_MAX_NAME_LEN) namesize=PART_MAX_NAME_LEN;
	write (file,&namesize,1);
	write (file,&ins[ni].name[0],namesize);
	//write the instrument data
	unsigned int datasize=ins[ni].size;
	write (file,&datasize,4);
	write (file,&ins[ni].data[0],datasize);
	//write the instrument meta_data buffer (for future versions)
	unsigned short int metadatasize=2;
	write (file,&metadatasize,2);
	char tmp[2]; tmp[0]=0xfe;tmp[1]=0xff;
	write (file,&tmp[0],metadatasize);	
    };

    close(file);    
    return(0);
};


/*
 * Change the current bank filename
 */
void OldBank::changebankfilename(const char *newbankfilename,int ro){
    if (bankfilename!=NULL) delete(bankfilename);
    bankfilename=new char [strlen(newbankfilename)+2];
    sprintf(bankfilename,"%s\0",newbankfilename);

    if (bankfiletitle!=NULL) delete(bankfiletitle);
    bankfiletitle=new char [strlen(newbankfilename)+50];
    if (ro==0) sprintf(bankfiletitle,"OldBank: %s\0",newbankfilename);
	else sprintf(bankfiletitle,"OldBank: (LOCKED) %s\0",newbankfilename);
};

/*
 * Load a bank from a file and makes it current
 */
int OldBank::loadfilebank(const char *newbankfilename){
    int file,err=0;
    if (bankfilename!=NULL) savefile();//save the current bank
    file=open(newbankfilename,O_RDWR|O_BINARY);
    if (file==-1) err=1;
    close(file);
    if (err!=0){
	file=open(newbankfilename,O_RDONLY|O_BINARY);
	if (file==-1) err=2;
	close(file);
    };
    if (err==2) return(2);//the file cannot be opened

    if (err==0) {
	changebankfilename(newbankfilename,0);
	lock=0;
    } else {
	changebankfilename(newbankfilename,1);
	lock=1;
    };
    
    if (loadfile()!=0) {
	lock=1;
	return(2);//something went wrong
    };
    if (err==0) return(0);//ok
	else return(1);//the file is openend R/O
};

/*
 * Save the bank to a file and makes it current
 */
int OldBank::savefilebank(const char *newbankfilename, int overwrite){
    int file;
    if (overwrite==0) file=open(newbankfilename,O_CREAT|O_EXCL|O_WRONLY|O_BINARY,00444+00222);
	    else file=open(newbankfilename,O_CREAT|O_WRONLY|O_TRUNC|O_BINARY,00444+00222);//overwrite if the file exists
    if (file==-1) {
	if (errno==EEXIST) return(1);//file exists already
           else return(2);//Access Denied or any other problem
    };
    lock=0;
    changebankfilename(newbankfilename,0);
    savefile();
    return(0);    
};

/*
 * Makes a new bank, put it on a file and makes it current bank
 */
int OldBank::newfilebank(const char *newbankfilename, int overwrite){
    int file;
    savefile();//saves the current bank before changing the file
    if (overwrite==0) file=open(newbankfilename,O_CREAT|O_EXCL|O_WRONLY|O_BINARY,00444+00222);
	    else file=open(newbankfilename,O_CREAT|O_WRONLY|O_TRUNC|O_BINARY,00444+00222);//overwrite if the file exists
    if (file==-1) {
	if (errno==EEXIST) return(1);//file exists already
           else return(2);//Access Denied or any other problem
    };
    lock=0;
    changebankfilename(newbankfilename,0);
    for (int i=0;i<128;i++) clearslot(i);
    savefile();
    return(0);    
};

/*
 * Check if the bank is locked (i.e. the file opened was readonly)
 */
int OldBank::locked(){
    return(lock);
};


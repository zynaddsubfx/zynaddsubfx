/*
  ZynAddSubFX - a software synthesizer
 
  Buffer.C - A buffer used to save/load parameters
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
#include "Buffer.h"

Buffer::Buffer(){
    mode=1;
    minimal=0;
    datak=datasize=0;
    data=firstdata=NULL;
};

Buffer::~Buffer(){
    resetbuffer();
};

/* 
 * Get or set byte to/from buffer (depends on the mode)
 */
void Buffer::rwbyte(unsigned char *b){
    if (data==NULL){//first time
	data=new list;
	firstdata=data;
	for (int i=0;i<BUFFER_CHUNK_SIZE;i++)
	 data->d[i]=0xff;
	 datak=0;
	 data->next=NULL;
    };
    
    if (mode==0) *b=data->d[datak];
	else {
	    data->d[datak]=*b;
	    datasize++;
	};

    datak++;

    if (datak>=BUFFER_CHUNK_SIZE) {
	if (mode==0) {//read from the buffer
	    if (data->next!=NULL) data=data->next;
		else *b=0xff;
	} else {//write to the buffer
	    list *tmp=new list;
	    tmp->next=NULL;
	    for (int i=0;i<BUFFER_CHUNK_SIZE;i++)
		tmp->d[i]=0xff;
	    data->next=tmp;
	    data=tmp;
	};
	datak=0;
    };

#ifdef DEBUG_BUFFER
    if (*b<16) fprintf(stderr,"0");
    fprintf(stderr,"[%d]%x ",mode,*b);
//    fprintf(stderr,"%x ",*b);
//    if (*b==0xff) fprintf(stderr,"*");
    fflush(stderr);
#endif
};

/* 
 * Get or set byte to/from buffer (depends on the mode)
 * Also get the byte with parameter number.
 */
void Buffer::rwbytepar(unsigned char npar,unsigned char *b){
    if (mode!=0) rwbyte(&npar);    
    rwbyte(b);
    if (mode==0) *b &= 0x7f;
#ifdef DEBUG_BUFFER
    fprintf(stderr," ");
#endif
};


/* 
 * Get or set word to/from buffer (depends on the mode)
 */
void Buffer::rwword(unsigned short int *s){
    unsigned char b1,b2;
    b1= *s/128;
    b2= *s%128;
    rwbyte(&b1);
    rwbyte(&b2);
    if (mode==0) *s=b1*128+b2;

#ifdef DEBUG_BUFFER
    fprintf(stderr," ");
#endif
};

/* 
 * Get or set word to/from buffer (depends on the mode)
 * Also get the parameter number.
 */
void Buffer::rwwordpar(unsigned char npar,unsigned short int *s){
    if (mode!=0) rwbyte(&npar);
    rwword(s);

#ifdef DEBUG_BUFFER
    fprintf(stderr," ");
#endif
};

/* 
 * Get or set word to/from buffer (depends on the mode)
 * Also get the parameter number.
 * Wrap if the word is less than 0.
 */
void Buffer::rwwordparwrap(unsigned char npar,short int *s){
    unsigned short int tmp;
    if (mode!=0) rwbyte(&npar);

    if (getmode()!=0) {
	tmp=(0x4000+ *s)%0x4000;
	rwword(&tmp);
    } else { 
	rwword(&tmp);
	*s =tmp;
	if (tmp>0x2000) *s=(signed short int)tmp-0x4000;
    };

#ifdef DEBUG_BUFFER
    fprintf(stderr," ");
#endif
};


/*
 * Reset the buffer
 */
void Buffer::resetbuffer(){
    datak=0;
    datasize=0;
    data=firstdata;
    
    if (data==NULL) return;
    while (data->next!=NULL){
	list *tmp=data;
	data=data->next;
	delete(tmp);
    };
    
    delete(data);
    data=firstdata=NULL;
    
};

/*
 * Change the mode: 0 => the buffer becomes a read buffer
          other value => the buffer becomes a write buffer
 */
void Buffer::changemode(int mode){

//    fprintf(stderr,"ch %d %d \n",this->mode,mode);
    if (mode!=0) resetbuffer();
	else {
	    datak=0;
	    data=firstdata;
	};
    this->mode=mode;
#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n\n-=CHANGING MODE (%d)=-\n\n",mode);
#endif
};

/*
 * Get the mode of the buffer (0=read, other value=write)
 */
int Buffer::getmode(){
    return(this->mode);
};

/*
 * Change the "minimal" value of the buffer
 * If it is different than 0, the program will not save into the buffer
 * unused data (like disabled parts..)
 */
void Buffer::changeminimal(int minimal){
    this->minimal=minimal;
};

/*
 * Get the "minimal" mode
 */
int Buffer::getminimal(){
    return(this->minimal);
};


/* 
 * skip if there are data that are saved on "better" specifications 
 * (eg. a part or a voice that are higher 
 *  than the number of theese suported by the program)
 * This is used if you load some data with an older version of the program
 */
void Buffer::skipbranch(){
    unsigned char tmp=0,k=1;
    int found0xfe=0;
    do {
        this->rwbyte(&tmp);
        if (tmp==0xfe) {
		k++;
		found0xfe=1;
	};
	if (tmp==0xff) k--;
    } while ((k>1)||(found0xfe==0));
};

/*
 * Dumps all data from the buffer (used to save to file)
 */
void Buffer::getalldata(int n,unsigned char *d){
#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n\n-= GET ALL DATA=-\n\n");
#endif
    int tmpmode=mode,tmpdatak=datak;
    list *tmpdata=data;
    mode=0;//read
    datak=0;
    data=firstdata;

    for (int k=0;k<n;k++) rwbyte(&d[k]);

    mode=tmpmode;
    datak=tmpdatak;
    data=tmpdata;
};

/*
 * Dumps all data to the buffer (used to load file)
 */
void Buffer::putalldata(int n,unsigned char *d){
#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n\n-= DUMP ALL DATA=-\n\n");
#endif
    resetbuffer();
    int tmpmode=mode;

    mode=1;//write
    for (int k=0;k<n;k++) rwbyte(&d[k]);
    datak=0;
    datasize=n;
};

/*
 * Get the size of the data stored in the buffer (used by save/load to/from file)
 */
int Buffer::getsize(){
    return(datasize);
};



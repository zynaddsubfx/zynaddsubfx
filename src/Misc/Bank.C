/*
  ZynAddSubFX - a software synthesizer
 
  Bank.h - Instrument Bank 
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

#include "Bank.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
*/
Bank::Bank(){
    for (int i=0;i<BANK_SIZE;i++){
	ins[i].used=false;
	memset(ins[i].name,0,PART_MAX_NAME_LEN+1);
	ins[i].filename=NULL;
    };

    loadbank("bank_xml");

};

Bank::~Bank(){
    clearbank();
};


/*
 * Get the name of an instrument from the bank
 */
char *Bank::getname (unsigned char ninstrument){
};

/*
 * Get the numbered name of an instrument from the bank
 */
char *Bank::getnamenumbered (unsigned char ninstrument){
};

/*
 * Changes the name of an instrument (and the filename)
 */
void Bank::setname(unsigned char ninstrument,const char *newname){
//    if (ninstrument<0)&&()
};

/*
 * Check if there is no instrument on a slot from the bank
 */
int Bank::emptyslot(unsigned char ninstrument){
};

/*
 * Removes the instrument from the bank
 */
void Bank::clearslot(unsigned char ninstrument){
};

/*
 * Save the instrument to a slot 
 */
void Bank::savetoslot(unsigned char ninstrument,const char *name,XMLwrapper *buf){
};

/*
 * Loads the instrument from the bank to a buffer
 */
void Bank::loadfromslot(unsigned char ninstrument,XMLwrapper *buf){

};


/*
/*
 * Load a bank from a file and makes it current
 */
int Bank::loadbank(const char *bankdirname){
    DIR *dir=opendir(bankdirname);

    clearbank();

    if (dir==NULL) return(-1);

    printf("%s/\n",bankdirname);
    struct dirent *fn;
    
    char *remainfiles[BANK_SIZE];
    memset(remainfiles,0,sizeof(remainfiles));
    int remainfilek=0;
        
    while(fn=readdir(dir)){
	if (fn->d_type!=DT_REG) continue;//this is not a regular file
	const char *filename= fn->d_name;
	
	//sa verific daca e si extensia dorita
	
	//verify if the name is like this NNNN-name (where N is a digit)
	int no=0,startname=0;
	
	for (int i=0;i<4;i++) {
	    if (strlen(filename)<=i) break;

	    if ((filename[i]>='0')&&(filename[i]<='9')) no=no*10+(filename[i]-'0');
		else if (startname!=0) startname=i;
	};
	
	
	
	if (no!=0){//the instrument position in the bank is found
	    addtobank(no-1,filename,&filename[startname]);
	} else {
	    addtobank(-1,filename,filename);
	};
	
    };
    
    
    closedir(dir);
};

/*
 * Makes a new bank, put it on a file and makes it current bank
 */
int Bank::newbank(const char *newbankdirname, int overwrite){
};

/*
 * Check if the bank is locked (i.e. the file opened was readonly)
 */
int Bank::locked(){
};


// private stuff

void Bank::clearbank(){
    for (int i=0;i<BANK_SIZE;i++) deletefrombank(i);
};

int Bank::addtobank(int pos, const char *filename, const char* name){
    if ((pos>=0)&&(pos<BANK_SIZE)){
	if (ins[pos].used) pos=-1;//force it to find a new free position
    } else if (pos>=BANK_SIZE) pos=-1;
    
    
    if (pos<0) {//find a free position
	for (int i=BANK_SIZE-1;i>=0;i--)
	    if (!ins[i].used) {
		pos=i;
		break;
	    };
	
    };
    
    if (pos<0) return (-1);//the bank is full

    printf("%s   %d\n",filename,pos);

    deletefrombank(pos);
    
    ins[pos].used=true;
    snprintf(ins[pos].name,PART_MAX_NAME_LEN,name);

    int len=strlen(filename);
    ins[pos].filename=new char[len+1];
    snprintf(ins[pos].filename,len,"%s",filename);
    
    return(0);
};

void Bank::deletefrombank(int pos){
    if ((pos<0)||(pos>=BANK_SIZE)) return;
    ins[pos].used=false;
    memset(ins[pos].name,0,PART_MAX_NAME_LEN+1);
    if (ins[pos].filename!=NULL) {
	delete (ins[pos].filename);
	ins[pos].filename=NULL;
    };
};


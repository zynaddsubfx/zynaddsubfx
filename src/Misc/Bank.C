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
#include <sys/stat.h>

Bank::Bank(){
    memset(defaultinsname,0,PART_MAX_NAME_LEN);
    snprintf(defaultinsname,PART_MAX_NAME_LEN,"%s"," ");
        
    for (int i=0;i<BANK_SIZE;i++){
	ins[i].used=false;
	ins[i].filename=NULL;
    };
    dirname=NULL;

    clearbank();

    loadbank("bank_xml");
    
    bankfiletitle=dirname;

};

Bank::~Bank(){
    clearbank();
};


/*
 * Get the name of an instrument from the bank
 */
char *Bank::getname (unsigned int ninstrument){
    if (emptyslot(ninstrument)) return (defaultinsname);
    return (ins[ninstrument].name);
};

/*
 * Get the numbered name of an instrument from the bank
 */
char *Bank::getnamenumbered (unsigned int ninstrument){
    if (emptyslot(ninstrument)) return (defaultinsname);
    snprintf(tmpinsname[ninstrument],PART_MAX_NAME_LEN+15,"%d. %s",ninstrument+1,getname(ninstrument));
    return(tmpinsname[ninstrument]);
};

/*
 * Changes the name of an instrument (and the filename)
 */
void Bank::setname(unsigned int ninstrument,const char *newname){
    if (emptyslot(ninstrument)) return;
    
    char newfilename[1000+1],tmpfilename[100+1];
    
    memset(newfilename,0,1001);
    memset(tmpfilename,0,101);
    snprintf(tmpfilename,100,"%4d-%s",ninstrument+1,newname);
    
    //add the zeroes at the start of filename
    for (int i=0;i<4;i++) if (tmpfilename[i]==' ') tmpfilename[i]='0';

    //make the filenames legal
    for (int i=0;i<strlen(tmpfilename);i++) {
	char c=tmpfilename[i];
	if ((c>='0')&&(c<='9')) continue;
	if ((c>='A')&&(c<='Z')) continue;
	if ((c>='a')&&(c<='z')) continue;
	if ((c=='-')||(c==' ')) continue;
	
	tmpfilename[i]='_';
    };

    snprintf(newfilename,1000,"%s/%s.xmlz",dirname,tmpfilename);

    rename(ins[ninstrument].filename,newfilename);
    snprintf(ins[ninstrument].name,PART_MAX_NAME_LEN,"%s",&tmpfilename[5]);
    
};

/*
 * Check if there is no instrument on a slot from the bank
 */
int Bank::emptyslot(unsigned int ninstrument){
    if (ninstrument>=BANK_SIZE) return (1);
    if (ins[ninstrument].filename==NULL) return(1);

    if (ins[ninstrument].used) return (0);
	else return(1);
};

/*
 * Removes the instrument from the bank
 */
void Bank::clearslot(unsigned int ninstrument){
    if (emptyslot(ninstrument)) return;
    remove(ins[ninstrument].filename);
    deletefrombank(ninstrument);
};

/*
 * Save the instrument to a slot 
 */
void Bank::savetoslot(unsigned int ninstrument,Part *part){
    clearslot(ninstrument);
    
    const int maxfilename=200;
    char tmpfilename[maxfilename+20];
    memset(tmpfilename,0,maxfilename+20);

    snprintf(tmpfilename,maxfilename,"%4d-%s",ninstrument+1,(char *)part->Pname);

    //add the zeroes at the start of filename
    for (int i=0;i<4;i++) if (tmpfilename[i]==' ') tmpfilename[i]='0';
    
    //make the filenames legal
    for (int i=0;i<strlen(tmpfilename);i++) {
	char c=tmpfilename[i];
	if ((c>='0')&&(c<='9')) continue;
	if ((c>='A')&&(c<='Z')) continue;
	if ((c>='a')&&(c<='z')) continue;
	if ((c=='-')||(c==' ')) continue;
	
	tmpfilename[i]='_';
    };

    strncat(tmpfilename,".xml",maxfilename+10);

    int fnsize=strlen(dirname)+strlen(tmpfilename)+10;
    char *filename=new char[fnsize+1];
    memset(filename,0,fnsize+2);

    if (config.cfg.GzipCompression) {
	tmpfilename[strlen(tmpfilename)]='z';
	tmpfilename[strlen(tmpfilename)+1]=0;
    };
    snprintf(filename,fnsize,"%s/%s",dirname,tmpfilename);

    remove(filename);
    part->saveXML(filename);
    addtobank(ninstrument,tmpfilename,(char *) part->Pname);
    
    delete(filename);
};

/*
 * Loads the instrument from the bank
 */
void Bank::loadfromslot(unsigned int ninstrument,Part *part){
    if (emptyslot(ninstrument)) return;
    
    part->defaultsinstrument();

   // printf("load:  %s\n",ins[ninstrument].filename);
    
    part->loadXMLinstrument(ins[ninstrument].filename);
    
};


/*
 * Load a bank from a file and makes it current
 */
int Bank::loadbank(const char *bankdirname){
    DIR *dir=opendir(bankdirname);
    clearbank();

    if (dir==NULL) return(-1);

    if (dirname!=NULL) delete(dirname);
    dirname=new char[strlen(bankdirname)+1];
    snprintf(dirname,strlen(bankdirname)+1,"%s",bankdirname);

   // printf("%s/\n",bankdirname);
    struct dirent *fn;
    
        
    while ((fn=readdir(dir))){
	if (fn->d_type!=DT_REG) continue;//this is not a regular file
	const char *filename= fn->d_name;
	
	//sa verific daca e si extensia dorita
	
	//verify if the name is like this NNNN-name (where N is a digit)
	int no=0;
	unsigned int startname=0;
	
	for (unsigned int i=0;i<4;i++) {
	    if (strlen(filename)<=i) break;

	    if ((filename[i]>='0')&&(filename[i]<='9')) {
		no=no*10+(filename[i]-'0');
		startname++;
	    };
	};
	
	
	if ((startname+1)<strlen(filename)) startname++;//to take out the "-"
	
	char name[PART_MAX_NAME_LEN+1]; 
	memset(name,0,PART_MAX_NAME_LEN+1);
	snprintf(name,PART_MAX_NAME_LEN,"%s",filename);
	
	//remove the file extension
	for (int i=strlen(name)-1;i>=2;i--){
	    if (name[i]=='.') {
		name[i]='\0';
		break;
	    };
	};
	
	if (no!=0){//the instrument position in the bank is found
	    addtobank(no-1,filename,&name[startname]);
	} else {
	    addtobank(-1,filename,name);
	};
	
    };
    
    
    closedir(dir);
    return(0);
};

/*
 * Makes a new bank, put it on a file and makes it current bank
 */
int Bank::newbank(const char *newbankdirname){
    int result=mkdir(newbankdirname,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (result<0) return(-1);
    
    return(loadbank(newbankdirname));
};

/*
 * Check if the bank is locked (i.e. the file opened was readonly)
 */
int Bank::locked(){
    return(dirname==NULL);
};

// private stuff

void Bank::clearbank(){
    for (int i=0;i<BANK_SIZE;i++) deletefrombank(i);
    if (dirname!=NULL) delete(dirname);
    dirname=NULL;
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

   // printf("%s   %d\n",filename,pos);

    deletefrombank(pos);
    
    ins[pos].used=true;
    snprintf(ins[pos].name,PART_MAX_NAME_LEN,"%s",name);

    snprintf(tmpinsname[pos],PART_MAX_NAME_LEN+10," ");

    int len=strlen(filename)+1+strlen(dirname);
    ins[pos].filename=new char[len+1];
    snprintf(ins[pos].filename,len+1,"%s/%s",dirname,filename);
    
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
    
    memset(tmpinsname[pos],0,PART_MAX_NAME_LEN+20);
    
};


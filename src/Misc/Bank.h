/*
  ZynAddSubFX - a software synthesizer
 
  Bank.C - Instrument Bank
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

#ifndef BANK_H
#define BANK_H

#include "../globals.h"
#include "XMLwrapper.h"
#include "Part.h"

#define BANK_SIZE 128

class Bank{
    public:
	Bank();
	~Bank();
	char *getname(unsigned int ninstrument);
	char *getnamenumbered(unsigned int ninstrument);
	void setname(unsigned int ninstrument,const char *newname);
	
	//returns 0 if the slot is not empty or 1 if the slot is empty
	int emptyslot(unsigned int ninstrument);

	void clearslot(unsigned int ninstrument);
	void savetoslot(unsigned int ninstrument,Part *part);
	void loadfromslot(unsigned int ninstrument,Part *part);

	int loadbank(const char *bankdirname);
//	int savebank(const char *newbankfilename,int overwrite);
	int newbank(const char *newbankdirname,int overwrite);
	
	char *bankfiletitle; //this is shown on the UI of the bank (the title of the window)
	int locked();
    private:
    
	//it adds a filename to the bank
	//if pos is -1 it try to find a position
	//returns -1 if the bank is full, or 0 if the instrument was added
	int addtobank(int pos,const char* filename,const char* name);
    
	void deletefrombank(int pos);
    
	void clearbank();
    
	char defaultinsname[PART_MAX_NAME_LEN];
	char tmpinsname[BANK_SIZE][PART_MAX_NAME_LEN+20];//this keeps the numbered names
    
	struct{
	    bool used;
	    char name[PART_MAX_NAME_LEN+1];
	    char *filename;
	}ins[BANK_SIZE];
	
	char *dirname;
};

#endif


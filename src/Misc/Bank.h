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
#include "Buffer.h"

class Bank{
    public:
	Bank();
	~Bank();
	char *getname(unsigned char ninstrument);
	char *getnamenumbered(unsigned char ninstrument);
	void setname(unsigned char ninstrument,const char *newname);
	int emptyslot(unsigned char ninstrument);
	void clearslot(unsigned char ninstrument);
	void savetoslot(unsigned char ninstrument,const char *name,Buffer *buf);
	void loadfromslot(unsigned char ninstrument,Buffer *buf);
	int loadfilebank(const char *newbankfilename);
	int savefilebank(const char *newbankfilename,int overwrite);
	int newfilebank(const char *newbankfilename,int overwrite);
	
	char *bankfiletitle; //this is shown on the UI of the bank (the title of the window)
	int locked();
    private:
	void changebankfilename(const char *newbankfilename,int ro);
	int savefile();
	int loadfile();

	int lock;
	char *bankfilename;
	char defaultinsname[PART_MAX_NAME_LEN],tmpinsname[128][PART_MAX_NAME_LEN+20];
	struct insstuct{
	    char name[PART_MAX_NAME_LEN];
	    unsigned int size;
	    unsigned char *data;
	} ins[128];
};

#endif


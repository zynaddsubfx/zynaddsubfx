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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

Bank::Bank(){
};

Bank::~Bank(){
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
 * Changes the name of an instrument
 */
void Bank::setname(unsigned char ninstrument,const char *newname){
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
 * Save the instrument to a slot (the instrument is stored in a buffer who was filled
 * with instrument parameters)
 */
void Bank::savetoslot(unsigned char ninstrument,const char *name,Buffer *buf){
};

/*
 * Loads the instrument from the bank to a buffer (who will be "dumped" to instrument parameters)
 */
void Bank::loadfromslot(unsigned char ninstrument,Buffer *buf){
};


/*
/*
 * Load a bank from a file and makes it current
 */
int Bank::loadfilebank(const char *newbankfilename){
};

/*
 * Save the bank to a file and makes it current
 */
int Bank::savefilebank(const char *newbankfilename, int overwrite){
};

/*
 * Makes a new bank, put it on a file and makes it current bank
 */
int Bank::newfilebank(const char *newbankfilename, int overwrite){
};

/*
 * Check if the bank is locked (i.e. the file opened was readonly)
 */
int Bank::locked(){
};


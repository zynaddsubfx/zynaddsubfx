/*
  ZynAddSubFX - a software synthesizer
 
  Buffer.h - A buffer used to save/load parameters
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

#ifndef BUFFER_H
#define BUFFER_H
#include "../globals.h"

//#define DEBUG_BUFFER

class Buffer{
#define BUFFER_CHUNK_SIZE 1000
    public:
	Buffer();
	~Buffer();
	int getmode();
	int getminimal();
	void changeminimal(int minimal);
	void changemode(int mode);
	void rwbyte(unsigned char *b);//read or write byte 
	void rwbytepar(unsigned char npar,unsigned char *b);//read or write a parameter

	void rwword(unsigned short int *s);//read or write word
	void rwwordpar(unsigned char npar,unsigned short int *s);
	void rwwordparwrap(unsigned char npar,short int *s);
	void skipbranch();
	void resetbuffer();

	int getsize();	
	void getalldata(int n,unsigned char *d);
	void putalldata(int n,unsigned char *d);

    private:

    struct list{
	unsigned char d[BUFFER_CHUNK_SIZE];
	struct list *next;
    } *data,*firstdata;
    
    int datak,datasize;
    int mode;//0 - read, 1 write
    int minimal;//If it is different than 0, the program will not save into the buffer unused data (like disabled parts..)
};

#endif

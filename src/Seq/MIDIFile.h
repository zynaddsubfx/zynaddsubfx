/*
  ZynAddSubFX - a software synthesizer
 
  MIDIFile.h - MIDI file loader
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

#ifndef MIDIFILE_H
#define MIDIFILE_H

class MIDIFile{
    public:
	MIDIFile();
	~MIDIFile();
	
	//returns -1 if there is an error, otherwise 0
	int loadfile(char *filename);
	
	//returns -1 if there is an error, otherwise 0
	int parsemidifile();
	
    private:
	unsigned char *midifile;
	int midifilesize,midifilek;
	bool midieof;
	
	void clearmidifile();
	
	//get a byte from the midifile
	unsigned char getbyte();
	
	//get a set of 4 bytes from the midifile
	unsigned int getint32();
	
	//get a word of 2 bytes from the midifile
	unsigned short int getint16();
	
};

#endif

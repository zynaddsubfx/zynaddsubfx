/*
  ZynAddSubFX - a software synthesizer
 
  Sequencer.h - The Sequencer
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

#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "../globals.h"
#include "MIDIFile.h"

class Sequencer{
    public:
	Sequencer();
	~Sequencer();
	
	//theese functions are called by the master and are ignored if the recorder/player are stopped
	void recordnote(char chan, char note, char vel);
	void recordcontroller(char chan,unsigned int type,int par);
	
	//this is only for player
	//it returns 1 if this must be called at least once more
	//it returns 0 if there are no more notes for the current time
	//or -1 if there is no note
	int getevent(char chan, int *type,int *par1, int *par2);

	//returns 0 if ok or -1 if there is a error loading file
	int importmidifile(char *filename);	
	
	void startplay();
	void stopplay();
	

	int play;	
	
    private:
    
	MIDIFile midifile;

    /* Events */
    struct event{
        int deltatime;
	int type,par1,par2;//type=1 for note, type=2 for controller
    } tmpevent;
    struct listpos{
	event ev;
        struct listpos *next;
    };
    struct list{
	listpos *first,*current;
	int size;//how many events are
	double length;//in seconds
    };
    struct {
	list track//the stored track
	,record;//the track being recorded
    } midichan[NUM_MIDI_CHANNELS];
    
    void writeevent(list *l,event *ev);
    void readevent(list *l,event *ev);
    
    void rewindlist(list *l);
    void deletelist(list *l);
    void deletelistreference(list *l);
    
    /* Timer */
    struct timestruct{
	double abs;//the time from the begining of the track
	double rel;//the time difference between the last and the current event
	double last;//the time of the last event (absolute, since 1 Jan 1970)
	//theese must be double, because the float's precision is too low
	//and all theese represents the time in seconds
    } rectime,playtime;
    
    void resettime(timestruct *t);
    void updatecounter(timestruct *t);//this updates the timer values

    /* Player only*/

    struct {
	event ev;
	double time;
    } nextevent;    
};

#endif


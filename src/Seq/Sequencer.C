/*
  ZynAddSubFX - a software synthesizer
 
  Sequencer.C - The Sequencer
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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>
#include <time.h>

#include "Sequencer.h"



Sequencer::Sequencer(){
    play=0;
    for (int i=0;i<NUM_MIDI_CHANNELS;i++){
	midichan[i].track.first=NULL;
	midichan[i].track.current=NULL;
	midichan[i].track.size=0;
	midichan[i].track.length=0.0;
	midichan[i].record.first=NULL;
	midichan[i].record.current=NULL;
	midichan[i].record.size=0;
	midichan[i].record.length=0.0;
    };
    
    resettime(&rectime);
    resettime(&playtime);
    nextevent.time=-1.0;//must be less than 0 
};

Sequencer::~Sequencer(){
    for (int i=0;i<NUM_MIDI_CHANNELS;i++){
	deletelist(&midichan[i].track);
	deletelist(&midichan[i].record);
    };
};


int Sequencer::importmidifile(char *filename){
    if (midifile.loadfile(filename)<0) return(-1);
    if (midifile.parsemidifile()<0) return(-1);
    return(0);
};



void Sequencer::startplay(){
    if (play!=0) return;
    play=1;
    resettime(&playtime);
    
    //test - canalul 1, deocamdata
    rewindlist(&midichan[0].track);
    
};
void Sequencer::stopplay(){
    if (play==0) return;
    play=0;
};


/************** Player stuff ***************/

int Sequencer::getevent(char chan,int *type,int *par1, int *par2){
    *type=0;
    if (play==0) return(-1);

    updatecounter(&playtime);

    if (nextevent.time>=playtime.abs) readevent(&midichan[chan].track,&nextevent.ev);
	else return(-1);
    if (nextevent.ev.type==-1) return(-1);

    *type=nextevent.ev.type;
    *par1=nextevent.ev.par1;
    *par2=nextevent.ev.par2;
    
    double dt=nextevent.ev.deltatime*0.001;
    nextevent.time+=dt;

    printf("%f   -  %d %d \n",nextevent.time,par1,par2);
    return(0);//?? sau 1
};


/************** Track stuff ***************/


void Sequencer::writeevent(list *l,event *ev){
    listpos *tmp=new listpos;
    tmp->next=NULL;
    tmp->ev=*ev;
    if (l->current!=NULL) l->current->next=tmp;
	else l->first=tmp;
    l->current=tmp;
    l->size++;
};

void Sequencer::readevent(list *l,event *ev){
    if (l->current==NULL) {
	ev->type=-1;
	return;
    };
    *ev=l->current->ev;
    l->current=l->current->next;
};


void Sequencer::rewindlist(list *l){
    l->current=l->first;
};

void Sequencer::deletelist(list *l){
    l->current=l->first;
    if (l->current==NULL) return;
    while (l->current->next!=NULL){
	listpos *tmp=l->current;
	l->current=l->current->next;
	delete(tmp);
    };
    deletelistreference(l);
};

void Sequencer::deletelistreference(list *l){
    l->current=l->first=NULL;
    l->size=0;
    l->length=0.0;
};

/************** Timer stuff ***************/

void Sequencer::resettime(timestruct *t){
    t->abs=0.0;
    t->rel=0.0;
    
    timeval tval;

    if (gettimeofday(&tval,NULL)==0)  
	t->last=tval.tv_sec+tval.tv_usec*0.000001;
    else t->last=0.0;
    
};

void Sequencer::updatecounter(timestruct *t){
    timeval tval;
    double current;
    if (gettimeofday(&tval,NULL)==0)  
	current=tval.tv_sec+tval.tv_usec*0.000001;
    else current=0.0;
    
    t->rel=current - t->last;
    t->abs+=t->rel;
    t->last=current;
    
//    printf("%f %f %f\n",t->last,t->abs,t->rel);
};


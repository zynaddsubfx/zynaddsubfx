/*
  ZynAddSubFX - a software synthesizer
 
  Sequencer.C - The Sequencer
  Copyright (C) 2003 Nasca Octavian Paul
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
    rec=0;
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
};

Sequencer::~Sequencer(){
    for (int i=0;i<NUM_MIDI_CHANNELS;i++){
	deletelist(&midichan[i].track);
	deletelist(&midichan[i].record);
    };
};

void Sequencer::recordnote(char chan, char note, char vel){
    if (rec==0) return;
    if (chan>=NUM_MIDI_CHANNELS) return;
    
    updatecounter(&rectime);
    int dt=(int) (rectime.rel*1000.0);
    tmpevent.deltatime=dt;

    tmpevent.type=1;
    tmpevent.par1=note;
    tmpevent.par2=vel;

    writeevent(&midichan[chan].record,&tmpevent);
    midichan[chan].record.length=rectime.abs;

    printf("Note %d %d %d \n",chan,note,vel);
};

void Sequencer::recordcontroller(char chan,unsigned int type,int par){
    if (rec==0) return;
    if (chan>=NUM_MIDI_CHANNELS) return;

    updatecounter(&rectime);
    int dt=(int) (rectime.rel*1000.0);
    tmpevent.deltatime=dt;

    tmpevent.type=2;
    tmpevent.par1=type;
    tmpevent.par2=par;

    writeevent(&midichan[chan].record,&tmpevent);
    midichan[chan].record.length=rectime.abs;

    printf("Ctl %d %d %d \n",chan,type,par);
};



void Sequencer::startrec(){
    if (rec!=0) return;
    resettime(&rectime);
    rec=1;
};

void Sequencer::stoprec(){
    if (rec==0) return;
    //for now, only record over track (erase the track)
    for (int i=0;i<NUM_MIDI_CHANNELS;i++){
	deletelist(&midichan[i].track);
	midichan[i].track=midichan[i].record;
	deletelistreference(&midichan[i].record);
    };
    rec=0;
};
	
void Sequencer::startplay(){
    if (play!=0) return;
    play=1;
    resettime(&playtime);
    
    //test - canalul 1, deocamdata
    rewindlist(&midichan[0].track);
    printf("\n\n===================================");
    do {
	readevent(&midichan[0].track,&tmpevent);
	printf("Play note %d %d \n",tmpevent.par1,tmpevent.par2);
    } while(tmpevent.type>=0);
    
};
void Sequencer::stopplay(){
    if (play==0) return;
    play=0;
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


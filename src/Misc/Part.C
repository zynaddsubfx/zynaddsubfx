/*
  ZynAddSubFX - a software synthesizer
 
  Part.C - Part implementation
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

#include "Part.h"
#include "Microtonal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Part::Part(Microtonal *microtonal_,FFTwrapper *fft_, pthread_mutex_t *mutex_){
    microtonal=microtonal_;    
    fft=fft_;
    mutex=mutex_;
    partoutl=new REALTYPE [SOUND_BUFFER_SIZE];
    partoutr=new REALTYPE [SOUND_BUFFER_SIZE];
    tmpoutl=new REALTYPE [SOUND_BUFFER_SIZE];
    tmpoutr=new REALTYPE [SOUND_BUFFER_SIZE];
    
    for (int n=0;n<NUM_KIT_ITEMS;n++){
	kit[n].Penabled=0;kit[n].Pmuted=0;
	kit[n].Pminkey=0;kit[n].Pmaxkey=127;
	kit[n].Padenabled=0;kit[n].Psubenabled=0;
	kit[n].Pname=new unsigned char [PART_MAX_NAME_LEN];
	for (int i=0;i<PART_MAX_NAME_LEN;i++) kit[n].Pname[i]='\0';
	kit[n].Psendtoparteffect=0;
	kit[n].adpars=NULL;kit[n].subpars=NULL;
    };
    kit[0].Penabled=1;
    kit[0].adpars=new ADnoteParameters(fft);
    kit[0].subpars=new SUBnoteParameters();
    ADPartParameters=kit[0].adpars;
    SUBPartParameters=kit[0].subpars;

    //Part's Insertion Effects init
    for (int nefx=0;nefx<NUM_PART_EFX;nefx++) {
    	partefx[nefx]=new EffectMgr(1,mutex);
	Pefxroute[nefx]=0;//route to next effect
    };

    for (int n=0;n<NUM_PART_EFX+1;n++) {
	partfxinputl[n]=new REALTYPE [SOUND_BUFFER_SIZE];
	partfxinputr[n]=new REALTYPE [SOUND_BUFFER_SIZE];
    };

    //parameters setup
    Penabled=0;
    Pminkey=0;
    Pmaxkey=127;
    Pnoteon=1;
    Ppolymode=1;
    setPvolume(96);
    Pkeyshift=64;
    Prcvchn=0;
    setPpanning(64);
    Pvelsns=64;
    Pveloffs=64;
    Pkeylimit=15;
    Pkitmode=0;
    Pdrummode=0;
    killallnotes=0;
    oldfreq=-1.0;

    PADnoteenabled=1;
    PSUBnoteenabled=0;
    
    int i,j;
    for (i=0;i<POLIPHONY;i++){
      partnote[i].status=KEY_OFF;
      partnote[i].note=-1;
      partnote[i].itemsplaying=0;
      for (j=0;j<NUM_KIT_ITEMS;j++){
        partnote[i].kititem[j].adnote=NULL;
        partnote[i].kititem[j].subnote=NULL;
      };
      partnote[i].time=0;
    };
    cleanup();    
    Pname=new unsigned char [PART_MAX_NAME_LEN];
    for (i=0;i<PART_MAX_NAME_LEN;i++) Pname[i]='\0';

    info.Ptype=0;
    memset(info.Pauthor,0,MAX_INFO_TEXT_SIZE+1);
    memset(info.Pcomments,0,MAX_INFO_TEXT_SIZE+1);
    
    oldvolumel=oldvolumer=0.5;
    lastnote=-1;disablekitloading=0;
};


/*
 * Cleanup the part
 */
void Part::cleanup(){
    for (int k=0;k<POLIPHONY;k++) KillNotePos(k);
    for (int i=0;i<SOUND_BUFFER_SIZE;i++){
	partoutl[i]=denormalkillbuf[i];
	partoutr[i]=denormalkillbuf[i];
	tmpoutl[i]=0.0;
	tmpoutr[i]=0.0;
    };
    ctl.resetall();
    for (int nefx=0;nefx<NUM_PART_EFX;nefx++) partefx[nefx]->cleanup();
    for (int n=0;n<NUM_PART_EFX+1;n++) {
	for (int i=0;i<SOUND_BUFFER_SIZE;i++){
	    partfxinputl[n][i]=denormalkillbuf[i];
	    partfxinputr[n][i]=denormalkillbuf[i];
	};
    };
};

Part::~Part(){
    cleanup();
    for (int n=0;n<NUM_KIT_ITEMS;n++){
	if (kit[n].adpars!=NULL) delete (kit[n].adpars);
	if (kit[n].subpars!=NULL) delete (kit[n].subpars);
    	kit[n].adpars=NULL;kit[n].subpars=NULL;
	delete(kit[n].Pname);
    };

    delete (Pname);
    delete (partoutl);
    delete (partoutr);
    delete (tmpoutl);
    delete (tmpoutr);
    for (int nefx=0;nefx<NUM_PART_EFX;nefx++) 
	delete (partefx[nefx]);
    for (int n=0;n<NUM_PART_EFX;n++) {
	delete (partfxinputl[n]);
	delete (partfxinputr[n]);
    };
};

/*
 * Note On Messages
 */
void Part::NoteOn(unsigned char note,unsigned char velocity,int masterkeyshift){
    int i,pos;    
    
    lastnote=note;    
    if ((note<Pminkey)||(note>Pmaxkey)) return;
    
    pos=-1;
    for (i=0;i<POLIPHONY;i++){
        if (partnote[i].status==KEY_OFF){
    	    pos=i;
	    break;
	};
    };

    if (Ppolymode==0){//if the mode is 'mono' turn off all other notes
	for (i=0;i<POLIPHONY;i++)
	    if (partnote[i].status==KEY_PLAYING) NoteOff(partnote[i].note);
	RelaseSustainedKeys();    
    };
    
    if (pos==-1){
        //test
	fprintf(stderr,"NOTES TOO MANY (> POLIPHONY) - (Part.C::NoteOn(..))\n");
    } else {
	if (Pnoteon!=0){
	    //start the note
    	    partnote[pos].status=KEY_PLAYING;
    	    partnote[pos].note=note;

	    //this computes the velocity sensing of the part 
	    REALTYPE vel=VelF(velocity/127.0,Pvelsns);

	    //compute the velocity offset
	    vel+=(Pveloffs-64.0)/64.0;
	    if (vel<0.0) vel=0.0; else if (vel>1.0) vel=1.0;

	    //compute the keyshift            
	    int partkeyshift=(int)Pkeyshift-64;
	    int keyshift=masterkeyshift+partkeyshift;

	    //initialise note frequency
	    REALTYPE notebasefreq;
	    if (Pdrummode==0){
		notebasefreq=microtonal->getnotefreq(note,keyshift);
		if (notebasefreq<0.0) return;//the key is no mapped
	    } else {
		notebasefreq=440.0*pow(2.0,(note-69.0)/12.0);
	    };
	    
	    //Portamento
	    if (oldfreq<1.0) oldfreq=notebasefreq;//this is only the first note is played
	    
	    int portamento=ctl.initportamento(oldfreq,notebasefreq);
	    
	    if (portamento!=0) ctl.portamento.noteusing=pos;
	    oldfreq=notebasefreq;
	    
	    partnote[pos].itemsplaying=0;
	    if (Pkitmode==0){//init the notes for the "normal mode"
		partnote[pos].kititem[0].sendtoparteffect=0;
        	if (PADnoteenabled!=0) partnote[pos].kititem[0].adnote=new ADnote(ADPartParameters,&ctl,notebasefreq,vel,portamento,note);
        	if (PSUBnoteenabled!=0) partnote[pos].kititem[0].subnote=new SUBnote(SUBPartParameters,&ctl,notebasefreq,vel,portamento,note);
		if ((PADnoteenabled!=0)||(PSUBnoteenabled!=0)) partnote[pos].itemsplaying++;
	    } else {//init the notes for the "kit mode"
		for (int item=0;item<NUM_KIT_ITEMS;item++){
		    if (kit[item].Pmuted!=0) continue;
		    if ((note<kit[item].Pminkey)||(note>kit[item].Pmaxkey)) continue;

		    int ci=partnote[pos].itemsplaying;//ci=current item

		    partnote[pos].kititem[ci].sendtoparteffect=( kit[item].Psendtoparteffect<NUM_PART_EFX ?
			    kit[item].Psendtoparteffect: NUM_PART_EFX);//if this parameter is 127 for "unprocessed"

        	    if ((kit[item].adpars!=NULL)&&(kit[item].Padenabled)!=0) 
		      partnote[pos].kititem[ci].adnote=new ADnote(kit[item].adpars,&ctl,notebasefreq,vel,portamento,note);

        	    if ((kit[item].subpars!=NULL)&&(kit[item].Psubenabled)!=0) 
		      partnote[pos].kititem[ci].subnote=new SUBnote(kit[item].subpars,&ctl,notebasefreq,vel,portamento,note);

		    if ((kit[item].adpars!=NULL)|| (kit[item].subpars!=NULL)) {
			partnote[pos].itemsplaying++;
			if ( ((kit[item].Padenabled!=0)||(kit[item].Psubenabled!=0))
			   && (Pkitmode==2) ) break;
		    };
		};
	    };
	};
    };
    
    //this only relase the keys if there is maximum number of keys allowed
    setkeylimit(Pkeylimit);
};

/*
 * Note Off Messages
 */
void Part::NoteOff(unsigned char note){//relase the key
    int i;    
    for (i=POLIPHONY-1;i>=0;i--){ //first note in, is first out if there are same note multiple times
	if ((partnote[i].status==KEY_PLAYING)&&(partnote[i].note==note)) {
	    if (ctl.sustain.sustain==0){ //the sustain pedal is not pushed
		RelaseNotePos(i);
		break;
	    } else {//the sustain pedal is pushed
		partnote[i].status=KEY_RELASED_AND_SUSTAINED;
	    };
	};
    };
};

/*
 * Controllers
 */
void Part::SetController(unsigned int type,int par){
    switch (type){
	case C_pitchwheel:ctl.setpitchwheel(par);
			  break;
	case C_expression:ctl.setexpression(par);
			  setPvolume(Pvolume);//update the volume
			  break;
	case C_portamento:ctl.setportamento(par);
			  break;
	case C_panning:ctl.setpanning(par);
			  setPpanning(Ppanning);//update the panning
			  break;
	case C_filtercutoff:ctl.setfiltercutoff(par);
			  break;
	case C_filterq:ctl.setfilterq(par);
			  break;
	case C_bandwidth:ctl.setbandwidth(par);
			  break;
	case C_modwheel:ctl.setmodwheel(par);
			  break;
	case C_fmamp:ctl.setfmamp(par);
			  break;
	case C_volume:ctl.setvolume(par);
   		      if (ctl.volume.receive!=0) volume=ctl.volume.volume;
		        else setPvolume(Pvolume);
			  break;
	case C_sustain:ctl.setsustain(par);
			  if (ctl.sustain.sustain==0) RelaseSustainedKeys();
			  break;
	case C_allsoundsoff:AllNotesOff();//Panic
			  break;
	case C_resetallcontrollers:
			  ctl.resetall();
			  RelaseSustainedKeys();
   		          if (ctl.volume.receive!=0) volume=ctl.volume.volume;
		        	else setPvolume(Pvolume);
			  setPvolume(Pvolume);//update the volume
			  setPpanning(Ppanning);//update the panning
			  
			  ADPartParameters->GlobalPar.Reson->
			    sendcontroller(C_resonance_center,1.0);
			    
			  ADPartParameters->GlobalPar.Reson->
			    sendcontroller(C_resonance_bandwidth,1.0);
			  //more update to add here if I add controllers
			  break;
	case C_allnotesoff:RelaseAllKeys();
			  break;
	case C_resonance_center:
			ctl.setresonancecenter(par);
			ADPartParameters->GlobalPar.Reson->
			    sendcontroller(C_resonance_center,ctl.resonancecenter.relcenter);
			  break;
	case C_resonance_bandwidth:
			ctl.setresonancebw(par);
			ADPartParameters->GlobalPar.Reson->
			    sendcontroller(C_resonance_bandwidth,ctl.resonancebandwidth.relbw);
			  break;
    };
};
/*
 * Relase the sustained keys
 */

void Part::RelaseSustainedKeys(){
    for (int i=0;i<POLIPHONY;i++)
	if (partnote[i].status==KEY_RELASED_AND_SUSTAINED)  RelaseNotePos(i);
};

/*
 * Relase all keys
 */

void Part::RelaseAllKeys(){
    for (int i=0;i<POLIPHONY;i++){
	if ((partnote[i].status!=KEY_RELASED)&&
	    (partnote[i].status!=KEY_OFF)) //thanks to Frank Neumann
	    RelaseNotePos(i);
    };
};

/*
 * Release note at position
 */
void Part::RelaseNotePos(int pos){

    for (int j=0;j<NUM_KIT_ITEMS;j++){

     if (partnote[pos].kititem[j].adnote!=NULL) 
        if (partnote[pos].kititem[j].adnote) 
	  partnote[pos].kititem[j].adnote->relasekey();

     if (partnote[pos].kititem[j].subnote!=NULL) 
        if (partnote[pos].kititem[j].subnote!=NULL) 
	  partnote[pos].kititem[j].subnote->relasekey();	    
    };
    partnote[pos].status=KEY_RELASED;
}; 
 
 
/*
 * Kill note at position
 */
void Part::KillNotePos(int pos){
    partnote[pos].status=KEY_OFF;
    partnote[pos].note=-1;
    partnote[pos].time=0;
    partnote[pos].itemsplaying=0;

    for (int j=0;j<NUM_KIT_ITEMS;j++){
     if (partnote[pos].kititem[j].adnote!=NULL) {
	    delete(partnote[pos].kititem[j].adnote);
	    partnote[pos].kititem[j].adnote=NULL;
     };
     if (partnote[pos].kititem[j].subnote!=NULL) {
	    delete(partnote[pos].kititem[j].subnote);
	    partnote[pos].kititem[j].subnote=NULL;
     };
    };
    if (pos==ctl.portamento.noteusing) {
	ctl.portamento.noteusing=-1;
	ctl.portamento.used=0;
    };
};


/*
 * Set Part's key limit
 */
void Part::setkeylimit(unsigned char Pkeylimit){
    this->Pkeylimit=Pkeylimit;
    int keylimit=Pkeylimit;
    if (keylimit==0) keylimit=POLIPHONY-5;

    //release old keys if the number of notes>keylimit
    if (Ppolymode!=0){
	int notecount=0;
	for (int i=0;i<POLIPHONY;i++){
	    if ((partnote[i].status==KEY_PLAYING)||(partnote[i].status==KEY_RELASED_AND_SUSTAINED))
	        notecount++;
	};
	int oldestnotepos=-1,maxtime=0;
	if (notecount>keylimit){//find out the oldest note
	    for (int i=0;i<POLIPHONY;i++){
		if ( ((partnote[i].status==KEY_PLAYING)||(partnote[i].status==KEY_RELASED_AND_SUSTAINED))
		   && (partnote[i].time>maxtime)){
		      maxtime=partnote[i].time;
		      oldestnotepos=i;
		    };
	    };
	};
	if (oldestnotepos!=-1) RelaseNotePos(oldestnotepos);
    };
};


/*
 * Prepare all notes to be turned off
 */
void Part::AllNotesOff(){
   killallnotes=1;    
};


/*
 * Compute Part samples and store them in the partoutl[] and partoutr[]
 */
void Part::ComputePartSmps(){
    int i,k;
    int noteplay;//0 if there is nothing activated
    for (int nefx=0;nefx<NUM_PART_EFX+1;nefx++){
	for (i=0;i<SOUND_BUFFER_SIZE;i++){
	    partfxinputl[nefx][i]=0.0;
	    partfxinputr[nefx][i]=0.0;
	};
    };
    
    for (k=0;k<POLIPHONY;k++){
    	if (partnote[k].status==KEY_OFF) continue;
	noteplay=0;	
	partnote[k].time++;
	//get the sampledata of the note and kill it if it's finished

        for (int item=0;item<partnote[k].itemsplaying;item++){

	    int sendcurrenttofx=partnote[k].kititem[item].sendtoparteffect;
	    
	    ADnote *adnote=partnote[k].kititem[item].adnote;
	    SUBnote *subnote=partnote[k].kititem[item].subnote;
	 //get from the ADnote
            if (adnote!=NULL) {
    		noteplay++;
		if (adnote->ready!=0) adnote->noteout(&tmpoutl[0],&tmpoutr[0]);
            	    else for (i=0;i<SOUND_BUFFER_SIZE;i++){tmpoutl[i]=0.0;tmpoutr[i]=0.0;};
		if (adnote->finished()!=0){
		    delete (adnote);
		    partnote[k].kititem[item].adnote=NULL;
		};
	    for (i=0;i<SOUND_BUFFER_SIZE;i++){//add the ADnote to part(mix)
		partfxinputl[sendcurrenttofx][i]+=tmpoutl[i];
		partfxinputr[sendcurrenttofx][i]+=tmpoutr[i];
	    };
	};
	    //get from the SUBnote
    	    if (subnote!=NULL) {
    		noteplay++;
		if (subnote->ready!=0) subnote->noteout(&tmpoutl[0],&tmpoutr[0]);
            	    else for (i=0;i<SOUND_BUFFER_SIZE;i++){tmpoutl[i]=0.0;tmpoutr[i]=0.0;};

	        for (i=0;i<SOUND_BUFFER_SIZE;i++){//add the SUBnote to part(mix)
		    partfxinputl[sendcurrenttofx][i]+=tmpoutl[i];
		    partfxinputr[sendcurrenttofx][i]+=tmpoutr[i];
		};
		if (subnote->finished()!=0){
		    delete (subnote);
		    partnote[k].kititem[item].subnote=NULL;
		};
	    };	

	};
	//Kill note if there is no synth on that note
	if (noteplay==0) KillNotePos(k);
    };


    //Apply part's effects and mix them
    for (int nefx=0;nefx<NUM_PART_EFX;nefx++) {
    	partefx[nefx]->out(partfxinputl[nefx],partfxinputr[nefx]);
	int routeto=(Pefxroute[nefx]==0 ? nefx+1 : NUM_PART_EFX);
	for (i=0;i<SOUND_BUFFER_SIZE;i++){
	    partfxinputl[routeto][i]+=partfxinputl[nefx][i];
	    partfxinputr[routeto][i]+=partfxinputr[nefx][i];
	};
    };
    for (i=0;i<SOUND_BUFFER_SIZE;i++){
    	partoutl[i]=partfxinputl[NUM_PART_EFX][i];
	partoutr[i]=partfxinputr[NUM_PART_EFX][i];
    };

    //Kill All Notes if killallnotes!=0
    if (killallnotes!=0) {
	for (i=0;i<SOUND_BUFFER_SIZE;i++) {
	    REALTYPE tmp=(SOUND_BUFFER_SIZE-i)/(REALTYPE) SOUND_BUFFER_SIZE;
	    partoutl[i]*=tmp;
	    partoutr[i]*=tmp;
	    tmpoutl[i]=0.0;
	    tmpoutr[i]=0.0;
	};
	for (int k=0;k<POLIPHONY;k++) KillNotePos(k);
	killallnotes=0;
	for (int nefx=0;nefx<NUM_PART_EFX;nefx++) {
    	    partefx[nefx]->cleanup();
	};
    };
    ctl.updateportamento();
};

/*
 * Parameter control
 */
void Part::setPvolume(char Pvolume_){
    Pvolume=Pvolume_;
    volume=dB2rap((Pvolume-96.0)/96.0*40.0)*ctl.expression.relvolume;
};

void Part::setPpanning(char Ppanning_){
    Ppanning=Ppanning_;
    panning=Ppanning/127.0+ctl.panning.pan;
    if (panning<0.0) panning=0.0;else if (panning>1.0) panning=1.0;

};

/*
 * Enable or disable a kit item
 */
void Part::setkititemstatus(int kititem,int Penabled_){
    if ((kititem==0)&&(kititem>=NUM_KIT_ITEMS)) return;//nonexistent kit item and the first kit item is always enabled
    kit[kititem].Penabled=Penabled_;
    if (Penabled_==0){
	if (kit[kititem].adpars!=NULL) delete (kit[kititem].adpars);
	if (kit[kititem].subpars!=NULL) delete (kit[kititem].subpars);
	kit[kititem].adpars=NULL;kit[kititem].subpars=NULL;
	kit[kititem].Pname[0]='\0';
    } else {
	if (kit[kititem].adpars==NULL) kit[kititem].adpars=new ADnoteParameters(fft);
	if (kit[kititem].subpars==NULL) kit[kititem].subpars=new SUBnoteParameters();
    };
};


/*
 * Swap the item with other or copy the item to another item
 */
void Part::swapcopyitem(int item1, int item2, int mode){
    if (item1==item2) return;
    if ((item1>=NUM_KIT_ITEMS) || (item2>=NUM_KIT_ITEMS)) return;
    
    int e1=kit[item1].Penabled;
    int e2=kit[item2].Penabled;
    
    if ((e1==0) && (e2==0)) return;//both items are disabled

    kit[0].Padenabled=PADnoteenabled;
    kit[0].Psubenabled=PSUBnoteenabled;
    
    if ((e1==0)&&(mode==0)) {//copy a null item to a existent item
	setkititemstatus (item2,0);//delete item 2
    };
    
    if (e1==0) setkititemstatus(item1,1);
    if (e2==0) setkititemstatus(item2,1);

    Buffer tmpbuf;
    if (mode!=0){//swap
	tmpbuf.changemode(1);
	tmpbuf.changeminimal(0);
	saveloadbufkititem(&tmpbuf,item2,1);
    };

    slbuf.changemode(1);//write to buffer
    slbuf.changeminimal(0);
    saveloadbufkititem(&slbuf,item1,1);

    slbuf.changemode(0);//read from buffer
    saveloadbufkititem(&slbuf,item2,1);

    if (mode!=0){//swap
	tmpbuf.changemode(0);
	saveloadbufkititem(&tmpbuf,item1,1);
    };

    PADnoteenabled=kit[0].Padenabled;
    PSUBnoteenabled=kit[0].Psubenabled;
};


void Part::saveloadbufkititem(Buffer *buf,unsigned char item,int saveitem0){
    unsigned char npar,n,tmp;
    int fmon,min,fmexton;//fmon is 0 if there is no need to save the FM parameters

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n\n( Part paramete kit item %d) \n",item);    
#endif

    tmp=0xfe;
    buf->rwbyte(&tmp);//if tmp!=0xfe error

    
    if (item>=NUM_KIT_ITEMS){//too big kit item
	buf->skipbranch();
	return;
    };
    
    for (n=0x80;n<0xF0;n++){
	if (buf->getmode()==0) {
	    buf->rwbyte(&npar);
	    n=0;//force a loop until the end of parameters (0xff)
	} else npar=n;
	if (npar==0xff) break;

	switch(npar){
	    case 0x81:	buf->rwbytepar(n,&kit[item].Pmuted);
			break;
	    case 0x82:	buf->rwbytepar(n,&kit[item].Pminkey);
			break;
	    case 0x83:	buf->rwbytepar(n,&kit[item].Pmaxkey);
			break;
	    case 0x84:	buf->rwbytepar(n,&kit[item].Psendtoparteffect);
			break;
	    case 0x90:	buf->rwbytepar(n,&kit[item].Padenabled);
			break;
	    case 0x91:	buf->rwbytepar(n,&kit[item].Psubenabled);
			break;
            case 0xA0:	if (buf->getmode()!=0){//saving
			    if ((buf->getminimal()!=0)&&(kit[item].Padenabled==0)) break;
			    if ((item==0)&&(saveitem0==0)) break;//the first item parameters are already saved (as ADPartParameters)
			    if (kit[item].adpars==NULL) break;
			    buf->rwbyte(&npar);
			};
			kit[item].adpars->saveloadbuf(buf);
			break;
            case 0xA1:	if (buf->getmode()!=0){//saving
			    if ((buf->getminimal()!=0)&&(kit[item].Psubenabled==0)) break;
			    if ((item==0)&&(saveitem0==0)) break;//the first item parameters are already saved (as SUBPartParameters)
			    if (kit[item].subpars==NULL) break;
			    buf->rwbyte(&npar);
			};
			kit[item].subpars->saveloadbuf(buf);
			break;
	    case 0xB0:  if (buf->getmode()!=0) {
			    buf->rwbyte(&npar);
			    int k=strlen( (char *) kit[item].Pname);
			    if (k>PART_MAX_NAME_LEN-2) {
			        k=PART_MAX_NAME_LEN-2;
			    };
			    kit[item].Pname[k]='\0';
			    for (int i=0;i<k+1;i++){
			        unsigned char tmp=kit[item].Pname[i];
			        buf->rwbyte(&tmp);
			    };			   
			} else {
			    unsigned char k=0,tmp=1;
			    while ((tmp!=0)&&(k<=PART_MAX_NAME_LEN-2)){
				buf->rwbyte(&tmp);
				kit[item].Pname[k++]=tmp;
			    };
			    kit[item].Pname[k]='\0';
			};
			
	};
    };
    
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };
    
};

/*
 * Save or load the parameters to/from the buffer
 */
void Part::saveloadbuf(Buffer *buf,int instrumentonly){
    unsigned char npar,n,tmp;

#ifdef DEBUG_BUFFER
    fprintf(stderr,"\n( Part parameters) \n");
#endif    

    tmp=0xfe;
    buf->rwbyte(&tmp);//if tmp!=0xfe error

    if ((buf->getmode()==0)&&(disablekitloading==0)){//clears all kit items
	for (int item=0;item<NUM_KIT_ITEMS;item++) {
	    if (item>0) setkititemstatus(item,0);
	    kit[item].Pmuted=0;
    	    kit[item].Pminkey=0;kit[item].Pmaxkey=127;
	    kit[item].Padenabled=0;kit[item].Psubenabled=0;
	    kit[item].Psendtoparteffect=0;
	    for (int i=0;i<PART_MAX_NAME_LEN;i++) kit[item].Pname[i]='\0';
	};
    };
    
    for (n=0x80;n<0xf0;n++){
	if (buf->getmode()==0) {
	    buf->rwbyte(&npar);
	    n=0;//force a loop until the end of parameters (0xff)
	} else npar=n;
	if (npar==0xff) break;
	
	switch(npar){
	    //Part parameters
	    case 0x80:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Penabled);
			if (buf->getmode()==0) cleanup();
			break;
	    case 0x81:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Pvolume);
			if (buf->getmode()==0) setPvolume(Pvolume);
			break;
	    case 0x82:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Ppanning);
			if (buf->getmode()==0) setPpanning(Ppanning);
			break;
	    case 0x83:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Pvelsns);
			break;
	    case 0x84:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Pveloffs);
			break;
	    case 0x85:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Pkeyshift);
			break;
	    case 0x86:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Prcvchn);
			break;
	    case 0x87:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Pnoteon);
			break;
	    case 0x88:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Pminkey);
			break;
	    case 0x89:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Pmaxkey);
			break;
	    case 0x8A:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Ppolymode);
			break;
	    case 0x8B:	if (instrumentonly!=0) break;
			buf->rwbytepar(n,&Pkeylimit);
			break;
	    //Instrument data
	    case 0xB0:	buf->rwbytepar(n,&PADnoteenabled);
			break;
	    case 0xB1:	buf->rwbytepar(n,&PSUBnoteenabled);
			break;
	    case 0xB2:	tmp=0;
			if ((disablekitloading!=0)&&(buf->getmode()==0))
			     buf->rwbytepar(n,&tmp);
			    else  buf->rwbytepar(n,&Pkitmode);
			break;
	    case 0xB3:	buf->rwbytepar(n,&Pdrummode);
			break;
	    case 0xB8:	buf->rwbytepar(n,&info.Ptype);
			break;
	    case 0xB9:  if (buf->getmode()!=0) {
			    buf->rwbyte(&npar);
			    int k=strlen( (char *) info.Pauthor);
			    if (k>MAX_INFO_TEXT_SIZE-1) {
			        k=MAX_INFO_TEXT_SIZE-1;
			    };
			    info.Pauthor[k]='\0';
			    for (int i=0;i<k+1;i++){
			        unsigned char tmp=info.Pauthor[i];
				if (tmp>127) tmp=127;
			        buf->rwbyte(&tmp);
			    };			   
			} else {
			    unsigned char k=0,tmp=1;
			    while ((tmp!=0)&&(k<=MAX_INFO_TEXT_SIZE-1)){
				buf->rwbyte(&tmp);
				info.Pauthor[k++]=tmp;
			    };
			    info.Pauthor[k]='\0';
			};
			break;
	    case 0xBA:  if (buf->getmode()!=0) {
			    buf->rwbyte(&npar);
			    int k=strlen( (char *) info.Pcomments);
			    if (k>MAX_INFO_TEXT_SIZE-1) {
			        k=MAX_INFO_TEXT_SIZE-1;
			    };
			    info.Pcomments[k]='\0';
			    for (int i=0;i<k+1;i++){
			        unsigned char tmp=info.Pcomments[i];
				if (tmp>127) tmp=127;
			        buf->rwbyte(&tmp);
			    };			   
			} else {
			    unsigned char k=0,tmp=1;
			    while ((tmp!=0)&&(k<=MAX_INFO_TEXT_SIZE-1)){
				buf->rwbyte(&tmp);
				info.Pcomments[k++]=tmp;
			    };
			    info.Pcomments[k]='\0';
			};
			break;
	    case 0xC0:	if ((buf->getminimal()!=0) && (buf->getmode()!=0) 
			    && (PADnoteenabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			ADPartParameters->saveloadbuf(buf);
			break;
	    case 0xC1:	if ((buf->getminimal()!=0) && (buf->getmode()!=0) 
			    && (PSUBnoteenabled==0)) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			SUBPartParameters->saveloadbuf(buf);
			break;
	    case 0xD0:  if (buf->getmode()!=0) {
			    if (instrumentonly!=2){
				buf->rwbyte(&npar);
				int k=strlen( (char *) Pname);
				if (k>PART_MAX_NAME_LEN-2) {
				    k=PART_MAX_NAME_LEN-2;
				};
				Pname[k]='\0';
				for (int i=0;i<k+1;i++){
				    unsigned char tmp=Pname[i];
				    buf->rwbyte(&tmp);
				};			   
			    }; 
			} else {
			    unsigned char k=0,tmp=1;
			    while ((tmp!=0)&&(k<=PART_MAX_NAME_LEN-2)){
				buf->rwbyte(&tmp);
				Pname[k++]=tmp;
			    };
			    Pname[k]='\0';
			};
			break;
	    case 0xE0:	if (instrumentonly!=0) break;
			if (buf->getmode()!=0) buf->rwbyte(&npar);
			ctl.saveloadbuf(buf);
			break;
	    case 0xE1:	if (buf->getmode()!=0) {//PART INSERTION EFFECTS
			    for (unsigned char neffect=0;neffect<NUM_PART_EFX;neffect++){
				buf->rwbyte(&npar);
				buf->rwbyte(&neffect);
				partefx[neffect]->saveloadbuf(buf);
			    };
			} else {
			    unsigned char neffect;
			    buf->rwbyte(&neffect);
			    if (neffect<NUM_PART_EFX) partefx[neffect]->saveloadbuf(buf);
			    else buf->skipbranch();
			};
			break;
	    //Kit Parameters
	    case 0xE2:	if (buf->getmode()!=0) {//save
			    if (Pkitmode==0) break;
			    for (unsigned char item=0;item<NUM_KIT_ITEMS;item++){
				if (kit[item].Penabled==0) continue;
				buf->rwbyte(&npar);
				buf->rwbyte(&item);//Write the item's number
				saveloadbufkititem(buf,item,0);
			    };
			} else {//load
			    if (disablekitloading!=0) {
				buf->skipbranch();
				break;
			    };
			    unsigned char item;
			    buf->rwbyte(&item);
			    if (item<NUM_KIT_ITEMS){
				setkititemstatus(item,1);
				saveloadbufkititem(buf,item,0);
			    } else buf->skipbranch();
			};
			break;
	    case 0xE3:	if (buf->getmode()!=0) {//part insertion effect routing
			    for (unsigned char neffect=0;neffect<NUM_PART_EFX;neffect++){
				buf->rwbyte(&npar);
				buf->rwbyte(&neffect);
				buf->rwbyte(&Pefxroute[neffect]);
			    };
			} else {
			    unsigned char neffect;
			    buf->rwbyte(&neffect);
			    if (neffect<NUM_PART_EFX) buf->rwbyte(&Pefxroute[neffect]);
			    else buf->rwbyte(&neffect);//this line just throw away the byte
			};
			break;
	};
    };

    
    if (buf->getmode()!=0) {
	unsigned char tmp=0xff;
	buf->rwbyte(&tmp);
    };

    kit[0].Padenabled=PADnoteenabled;
    kit[0].Psubenabled=PSUBnoteenabled;

};

/*
      unsigned char Pkitmode;//if the kitmode is enabled
      unsigned char Pdrummode;//if all keys are mapped and the system is 12tET (used for drums)

*/

void Part::add2XMLinstrument(XMLwrapper *xml){
    xml->beginbranch("INSTRUMENTINFO");
	xml->addparstr("name",(char *)Pname);
	xml->addparstr("author",(char *)info.Pauthor);
	xml->addparstr("comments",(char *)info.Pcomments);
	xml->addpar("type",info.Ptype);
    xml->endbranch();
    
    xml->addpar("kitmode",Pkitmode);
    xml->addparbool("drummode",Pdrummode);
    
    for (int i=0;i<NUM_KIT_ITEMS;i++){
	xml->beginbranch("INSTRUMENTKITITEM",i);
	    xml->addparbool("enabled",kit[i].Penabled);
	    if (kit[i].Penabled!=0) {
		xml->addparstr("name",(char *)kit[i].Pname);

		xml->addpar("muted",kit[i].Pmuted);
		xml->addpar("minkey",kit[i].Pminkey);
		xml->addpar("maxkey",kit[i].Pmaxkey);
	    
		xml->addpar("Psendtoparteefect",kit[i].Psendtoparteffect);

		xml->addpar("adenabled",kit[i].Padenabled);
		if ((kit[i].Padenabled!=0)&&(kit[i].adpars!=NULL)){
		    xml->beginbranch("ADNOTEPARAMETERS");
			kit[i].adpars->add2XML(xml);
		    xml->endbranch();
		};

		xml->addpar("subenabled",kit[i].Psubenabled);
		if ((kit[i].Psubenabled!=0)&&(kit[i].subpars!=NULL)){
		    xml->beginbranch("SUBNOTEPARAMETERS");
		    kit[i].subpars->add2XML(xml);
		    xml->endbranch();
		};
		
	    };
	xml->endbranch();
    };
        
};


void Part::add2XML(XMLwrapper *xml){
    //parameters
    xml->addparbool("enabled",Penabled);
    if (Penabled==0) return;

    xml->addpar("volume",Pvolume);
    xml->addpar("panning",Ppanning);

    xml->addpar("minkey",Pminkey);
    xml->addpar("maxkey",Pmaxkey);
    xml->addpar("keyshift",Pkeyshift);
    xml->addpar("rcvchn",Prcvchn);

    xml->addpar("velsns",Pvelsns);
    xml->addpar("veloffs",Pveloffs);

    xml->addparbool("noteon",Pnoteon);
    xml->addparbool("polymode",Ppolymode);
    xml->addpar("keylimit",Pkeylimit);

    xml->beginbranch("INSTRUMENTKIT");
	add2XMLinstrument(xml);
    xml->endbranch();
    
    xml->beginbranch("CONTROLLER");
	ctl.add2XML(xml);
    xml->endbranch();

    for (int nefx=0;nefx<NUM_PART_EFX;nefx++){
	xml->beginbranch("PARTEFFECT",nefx);
	    xml->beginbranch("EFFECT");
		partefx[nefx]->add2XML(xml);
	    xml->endbranch();

	    xml->addpar("route",Pefxroute[nefx]);
	xml->endbranch();
    };
};


/*
  ZynAddSubFX - a software synthesizer
 
  PresetsStore.C - Presets and Clipboard store
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
#include <stdlib.h>
#include <string.h>

#include "PresetsStore.h"
#include "../Misc/Util.h"

PresetsStore presetsstore;

PresetsStore::PresetsStore(){
    clipboard.data=NULL;
    clipboard.type[0]=0;
    
    for (int i=0;i<MAX_PRESETS;i++){
	presets[i].file=NULL;
	presets[i].name=NULL;
    };
    
};

PresetsStore::~PresetsStore(){
    if (clipboard.data!=NULL) delete (clipboard.data);
    clearpresets();
};

//Clipboard management

void PresetsStore::copyclipboard(XMLwrapper *xml,char *type){
    strcpy(clipboard.type,type);
    if (clipboard.data!=NULL) delete (clipboard.data);
    clipboard.data=xml->getXMLdata();
};

bool PresetsStore::pasteclipboard(XMLwrapper *xml){
    if (clipboard.data!=NULL) xml->putXMLdata(clipboard.data);
	else return(false);
    return(true);    
};

bool PresetsStore::checkclipboardtype(char *type){
    //makes LFO's compatible
    if ((strstr(type,"Plfo")!=NULL)&&(strstr(clipboard.type,"Plfo")!=NULL)) return(true);
    return(strcmp(type,clipboard.type)==0);
};

//Presets management
void PresetsStore::clearpresets(){
    for (int i=0;i<MAX_PRESETS;i++){
	if (presets[i].file!=NULL) {
	    delete(presets[i].file);
	    presets[i].file=NULL;
	};
	if (presets[i].name!=NULL) {
	    delete(presets[i].name);
	    presets[i].name=NULL;
	};
    };
    
};

void PresetsStore::rescanforpresets(char *type){
    clearpresets();
//    int presetk=0;
    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++){
	if (config.cfg.presetsDirList[i]==NULL) continue;
	//de continuat aici
    };
};

void PresetsStore::copypreset(XMLwrapper *xml,char *type, const char *name){
    char filename[MAX_STRING_SIZE],tmpfilename[MAX_STRING_SIZE];
    
    if (config.cfg.presetsDirList[0]==NULL) return;
    
    snprintf(tmpfilename,MAX_STRING_SIZE,"%s",name);

    //make the filenames legal
    for (int i=0;i<(int) strlen(tmpfilename);i++) {
	char c=tmpfilename[i];
	if ((c>='0')&&(c<='9')) continue;
	if ((c>='A')&&(c<='Z')) continue;
	if ((c>='a')&&(c<='z')) continue;
	if ((c=='-')||(c==' ')) continue;
	tmpfilename[i]='_';
    };
    
    snprintf(filename,MAX_STRING_SIZE,"%s%s.%s.xpz",config.cfg.presetsDirList[0],name,type);
    
    xml->saveXMLfile(filename);
};



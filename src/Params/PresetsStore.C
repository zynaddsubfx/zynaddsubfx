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

PresetsStore presetsstore;

PresetsStore::PresetsStore(){
    clipboard.data=NULL;
    clipboard.type[0]=0;;
};

PresetsStore::~PresetsStore(){
    if (clipboard.data!=NULL) delete (clipboard.data);
};

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
    return(strcmp(type,clipboard.type)==0);
};




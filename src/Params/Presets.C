/*
  ZynAddSubFX - a software synthesizer
 
  Presets.C - Presets and Clipboard management
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

#include "Presets.h"
#include <string.h>


Presets::Presets(){
    type[0]=0;;
};

Presets::~Presets(){
};

void Presets::setpresettype(char *type){
    strcpy(this->type,type);
};

void Presets::copyclipboard(){
    XMLwrapper *xml=new XMLwrapper();

    xml->beginbranch(type);
    add2XML(xml);
    xml->endbranch();

    presetsstore.copyclipboard(xml,type);
    
    delete(xml);
};

void Presets::pasteclipboard(){
    if (!checkclipboardtype()) return;
    XMLwrapper *xml=new XMLwrapper();
    if (!presetsstore.pasteclipboard(xml)) {
	delete(xml);
	return;
    };
    
    if (xml->enterbranch(type)==0) return;
	getfromXML(xml);
    xml->exitbranch();
    
    delete(xml);

};

bool Presets::checkclipboardtype(){
    return(presetsstore.checkclipboardtype(type));
};



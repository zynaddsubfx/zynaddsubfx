/*
  ZynAddSubFX - a software synthesizer
 
  XMLwrapper.C - XML wrapper
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

#include "XMLwrapper.h"
#include <stdio.h>

int XMLwrapper_whitespace_callback(mxml_node_t *node,int where){
    const char *name=node->value.element.name;

    if ((where==MXML_WS_BEFORE_OPEN)&&(!strcmp(name,"?xml"))) return(0);
    if ((where==MXML_WS_BEFORE_OPEN)||(where==MXML_WS_BEFORE_CLOSE)) return('\n');

    return(0);
};


XMLwrapper::XMLwrapper(){
    memset(&parentstack,0,sizeof(parentstack));
    stackpos=0;

    tree=mxmlNewElement(MXML_NO_PARENT,"?xml");
    mxmlElementSetAttr(tree,"version","1.0");
    mxmlElementSetAttr(tree,"encoding","UTF-8");

    
    mxml_node_t *doctype=mxmlNewElement(tree,"!DOCTYPE");
    mxmlElementSetAttr(doctype,"zynaddsubfx-data",NULL);

    node=root=mxmlNewElement(tree,"zynaddsubfx-data");
    
//    mxml_node_t *tmp=mxmlNewElement(root,"test1");
//    tmp=mxmlNewElement(tmp,"PARAM11");
//    tmp=mxmlNewElement(tmp,"test3");
};

XMLwrapper::~XMLwrapper(){
    mxmlDelete(tree);
};



int XMLwrapper::saveXMLfile(char *filename,int compression){
    FILE *file;
    file=fopen(filename,"w");
    if (file==NULL) return(-1);
    mxmlSaveFile(tree,file,XMLwrapper_whitespace_callback);
    
    fclose(file);
    
    //test
    printf("\n\n");
    mxmlSaveFile(tree,stdout,XMLwrapper_whitespace_callback);
    printf("\n\n");
    
    return(0);
};


void XMLwrapper::addpar(char *name,int val){
    addparams2("PARAM","name",name,"value",int2str(val));
};

void XMLwrapper::addparbool(char *name,int val){
    if (val!=0) addparams2("BOOLPARAM","name",name,"value","yes");
	else addparams2("BOOLPARAM","name",name,"value","no");
};

void XMLwrapper::addparstr(char *name,char *val){
    addparams2("STRPARAM","name",name,"value",val);
};


void XMLwrapper::beginbranch(char *name){
    push(node);
    node=addparams0(name);
};

void XMLwrapper::beginbranch(char *name,char *par,int val){
    push(node);
    node=addparams1(name,par,int2str(val));
};

void XMLwrapper::endbranch(){
    node=pop();
};



/** Private members **/

char *XMLwrapper::int2str(int x){
    snprintf(tmpstr,TMPSTR_SIZE,"%d",x);
    return(tmpstr);
};

mxml_node_t *XMLwrapper::addparams0(char *name){
    mxml_node_t *element=mxmlNewElement(node,name);
    return(element);
};

mxml_node_t *XMLwrapper::addparams1(char *name,char *par1,char *val1){
    mxml_node_t *element=mxmlNewElement(node,name);
    mxmlElementSetAttr(element,par1,val1);
    return(element);
};

mxml_node_t *XMLwrapper::addparams2(char *name,char *par1,char *val1,char *par2, char *val2){
    mxml_node_t *element=mxmlNewElement(node,name);
    mxmlElementSetAttr(element,par1,val1);
    mxmlElementSetAttr(element,par2,val2);
    return(element);
};




void XMLwrapper::push(mxml_node_t *node){
    if (stackpos>=STACKSIZE-1) {
	printf("BUG!: XMLwrapper::push() - full parentstack\n");
	return;//nu mai este loc in stiva
    };
    stackpos++;
    parentstack[stackpos]=node;
    
};
mxml_node_t *XMLwrapper::pop(){
    if (stackpos<=0) {
	printf("BUG!: XMLwrapper::pop() - empty parentstack\n");
	return (root);
    };
    mxml_node_t *node=parentstack[stackpos];
    parentstack[stackpos]=NULL;
    stackpos--;
    return(node);
};




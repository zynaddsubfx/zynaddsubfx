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
#include <stdlib.h>

#include "../globals.h"

int XMLwrapper_whitespace_callback(mxml_node_t *node,int where){
    const char *name=node->value.element.name;

    if ((where==MXML_WS_BEFORE_OPEN)&&(!strcmp(name,"?xml"))) return(0);
    if ((where==MXML_WS_BEFORE_CLOSE)&&(!strcmp(name,"string"))) return(0);
    if ((where==MXML_WS_BEFORE_OPEN)||(where==MXML_WS_BEFORE_CLOSE)) return('\n');
    
    return(0);
};


XMLwrapper::XMLwrapper(){
    memset(&parentstack,0,sizeof(parentstack));
    memset(&values,0,sizeof(values));

    stackpos=0;
    
    tree=mxmlNewElement(MXML_NO_PARENT,"?xml");
    mxmlElementSetAttr(tree,"version","1.0");
    mxmlElementSetAttr(tree,"encoding","UTF-8");

    
    mxml_node_t *doctype=mxmlNewElement(tree,"!DOCTYPE");
    mxmlElementSetAttr(doctype,"ZynAddSubFX-data",NULL);

    node=root=mxmlNewElement(tree,"ZynAddSubFX-data");
    
    mxmlElementSetAttr(root,"version-major","0");
    mxmlElementSetAttr(root,"version-minor","1");
    
    //save zynaddsubfx specifications
    beginbranch("BASE_PARAMETERS");
	addpar("max_midi_parts",NUM_MIDI_PARTS);
	addpar("max_kit_items_per_instrument",NUM_KIT_ITEMS);

	addpar("max_system_effects",NUM_SYS_EFX);
	addpar("max_insertion_effects",NUM_INS_EFX);
	addpar("max_instrument_effects",NUM_PART_EFX);

	addpar("max_addsynth_voices",NUM_VOICES);
    endbranch();
    
};

XMLwrapper::~XMLwrapper(){
    if (tree!=NULL) mxmlDelete(tree);
};


/* SAVE XML members */

int XMLwrapper::saveXMLfile(char *filename,int compression){
    FILE *file;
    file=fopen(filename,"w");
    if (file==NULL) return(-1);
    mxmlSaveFile(tree,file,XMLwrapper_whitespace_callback);
    
    fclose(file);
    
/*    //test
    printf("\n\n");
    mxmlSaveFile(tree,stdout,XMLwrapper_whitespace_callback);
    printf("\n\n");
*/    
    return(0);
};


void XMLwrapper::addpar(char *name,int val){
    addparams2("par","name",name,"value",int2str(val));
};

void XMLwrapper::addparreal(char *name,REALTYPE val){
    addparams2("par_real","name",name,"value",real2str(val));
};

void XMLwrapper::addparbool(char *name,int val){
    if (val!=0) addparams2("par_bool","name",name,"value","yes");
	else addparams2("par_bool","name",name,"value","no");
};

void XMLwrapper::addparstr(char *name,char *val){
    mxml_node_t *element=mxmlNewElement(node,"string");
    mxmlElementSetAttr(element,"name",name);
    mxmlNewText(element,0,val);
};


void XMLwrapper::beginbranch(char *name){
    push(node);
    node=addparams0(name);
};

void XMLwrapper::beginbranch(char *name,int id){
    push(node);
    node=addparams1(name,"id",int2str(id));
};

void XMLwrapper::endbranch(){
    node=pop();
};



/* LOAD XML members */

int XMLwrapper::loadXMLfile(char *filename){
    if (tree!=NULL) mxmlDelete(tree);
    tree=NULL;
    
    FILE *file=fopen(filename,"r");
    if (file==NULL) return(-1);
    
    root=tree=mxmlLoadFile(NULL,file,MXML_NO_CALLBACK);
    fclose(file);
    
    if (tree==NULL) return(-1);//this is not XML
    
//    if (strcmp(tree->value.element.name,"?xml")!=0) return(-2);
    node=root=mxmlFindElement(tree,tree,"ZynAddSubFX-data",NULL,NULL,MXML_DESCEND);
    if (root==NULL) return(-2);//the XML doesnt embbed zynaddsubfx data
        
    push(root);

    values.xml_version.major=str2int(mxmlElementGetAttr(root,"version-major"));
    values.xml_version.minor=str2int(mxmlElementGetAttr(root,"version-minor"));

    
//    node=mxmlFindElement(node,node,NULL,"name","volume",MXML_DESCEND);
    
    
    
//    if (node!=NULL) printf("%s\n",node->value.element.name);
//        else printf("NULL node\n");
    
    return(0);
};

int XMLwrapper::enterbranch(char *name){
    node=mxmlFindElement(peek(),peek(),name,NULL,NULL,MXML_DESCEND_FIRST);
    if (node==NULL) return(0);
    push(node);
    
    return(1);
};

void XMLwrapper::exitbranch(){
    pop();
};


int XMLwrapper::getbranchid(int min, int max){
    int id=str2int(mxmlElementGetAttr(node,"id"));
    if ((min==0)&&(max==0)) return(id);
    
    if (id<min) id=min;
	else if (id>max) id=max;

    return(id);
};

int XMLwrapper::getpar(char *name,int defaultpar,int min,int max){
    node=mxmlFindElement(peek(),peek(),"par","name",name,MXML_DESCEND_FIRST);
    if (node==NULL) return(defaultpar);

    const char *strval=mxmlElementGetAttr(node,"value");
    if (strval==NULL) return(defaultpar);
    
    int val=str2int(strval);
    if (val<min) val=min;
	else if (val>max) val=max;
    
    return(val);
};

int XMLwrapper::getpar127(char *name,int defaultpar){
    return(getpar(name,defaultpar,0,127));
};

int XMLwrapper::getparbool(char *name,int defaultpar){
    node=mxmlFindElement(peek(),peek(),"par_bool","name",name,MXML_DESCEND_FIRST);
    if (node==NULL) return(defaultpar);

    const char *strval=mxmlElementGetAttr(node,"value");
    if (strval==NULL) return(defaultpar);
    
    if ((strval[0]=='Y')||(strval[0]=='y')) return(1);
	else return(0);
};



/** Private members **/

char *XMLwrapper::int2str(int x){
    snprintf(tmpstr,TMPSTR_SIZE,"%d",x);
    return(tmpstr);
};

char *XMLwrapper::real2str(REALTYPE x){
    snprintf(tmpstr,TMPSTR_SIZE,"%g",x);
    return(tmpstr);
};

int XMLwrapper::str2int(const char *str){
    if (str==NULL) return(0);
    int result=strtol(str,NULL,10);
    return(result);
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
	return;
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

mxml_node_t *XMLwrapper::peek(){
    if (stackpos<=0) {
	printf("BUG!: XMLwrapper::peek() - empty parentstack\n");
	return (root);
    };
    return(parentstack[stackpos]);
};




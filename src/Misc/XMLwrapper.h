/*
  ZynAddSubFX - a software synthesizer
 
  XML.h - XML wrapper
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

#include <mxml.h>

#ifndef XML_WRAPPER_H
#define XML_WRAPPER_H

#define TMPSTR_SIZE 20

//the maxim tree depth
#define STACKSIZE 100

class XMLwrapper{
    public:
	XMLwrapper();
	~XMLwrapper();
	
	//returns 0 if ok or -1 if the file cannot be saved
	int saveXMLfile(char *filename, int compression);

	//add simple parameter (name and value)
	void addpar(char *name,int val);
	
	//add boolean parameter (name and boolean value)
	//if the value is 0 => "yes", else "no"
	void addparbool(char *name,int val);

	//add string parameter (name and string)
	void addparstr(char *name,char *val);

	//add a branch
	void beginbranch(char *name);
	void beginbranch(char *name, char *par, int val);

	//this must be called after each branch (nodes that contains child nodes)
	void endbranch();
	
	
	
    private:
	mxml_node_t *tree;//all xml data
	mxml_node_t *root;//xml data used by zynaddsubfx
	mxml_node_t *node;//current node
	
	//adds params like this:
	// <name>
	//returns the node
	mxml_node_t *addparams0(char *name);

	//adds params like this:
	// <name par1="val1">
	//returns the node
	mxml_node_t *addparams1(char *name,char *par1,char *val1);

	//adds params like this:
	// <name par1="val1" par2="val2">
	//returns the node
	mxml_node_t *addparams2(char *name,char *par1,char *val1,char *par2, char *val2);
	
	char *int2str(int x);
	
	char tmpstr[TMPSTR_SIZE];	
	
	
	//this is used to store the parents
	mxml_node_t *parentstack[STACKSIZE];
	int stackpos;
	
	void push(mxml_node_t *node);
	mxml_node_t *pop();
	
	
};

#endif

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
#ifndef REALTYPE
#define REALTYPE float
#endif

#ifndef XML_WRAPPER_H
#define XML_WRAPPER_H

#define TMPSTR_SIZE 50

//the maxim tree depth
#define STACKSIZE 100

class XMLwrapper{
    public:
	XMLwrapper();
	~XMLwrapper();
	
	/********************************/
	/*         SAVE to XML          */
	/********************************/

	//returns 0 if ok or -1 if the file cannot be saved
	int saveXMLfile(char *filename, int compression);

	//add simple parameter (name and value)
	void addpar(char *name,int val);
	void addparreal(char *name,REALTYPE val);
	
	//add boolean parameter (name and boolean value)
	//if the value is 0 => "yes", else "no"
	void addparbool(char *name,int val);

	//add string parameter (name and string)
	void addparstr(char *name,char *val);

	//add a branch
	void beginbranch(char *name);
	void beginbranch(char *name, int id);

	//this must be called after each branch (nodes that contains child nodes)
	void endbranch();

	/********************************/
	/*        LOAD from XML         */
	/********************************/
	
	//returns 0 if ok or -1 if the file cannot be loaded
	int loadXMLfile(const char *filename);

	//enter into the branch
	//returns 1 if is ok, or 0 otherwise
	int enterbranch(char *name);

		
	//enter into the branch with id
	//returns 1 if is ok, or 0 otherwise
	int enterbranch(char *name, int id);

	//exits from a branch
	void exitbranch();
	
	//get the the branch_id and limits it between the min and max
	//if min==max==0, it will not limit it
	//if there isn't any id, will return min
	//this must be called only imediately after enterbranch()
	int getbranchid(int min, int max);

	//it returns the parameter and limits it between min and max
	//if min==max==0, it will not limit it
	//if no parameter will be here, the defaultpar will be returned
	int getpar(char *name,int defaultpar,int min,int max);

	//the same as getpar, but the limits are 0 and 127
	int getpar127(char *name,int defaultpar);
	
	int getparbool(char *name,int defaultpar);

	void getparstr(char *name,char *par,int maxstrlen);
	REALTYPE getparreal(char *name,REALTYPE defaultpar);
	REALTYPE getparreal(char *name,REALTYPE defaultpar,REALTYPE min,REALTYPE max);


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
	char *real2str(REALTYPE x);
	
	int str2int(const char *str);
	REALTYPE str2real(const char *str);
	
	char tmpstr[TMPSTR_SIZE];	
	
	
	//this is used to store the parents
	mxml_node_t *parentstack[STACKSIZE];
	int stackpos;

	
	void push(mxml_node_t *node);
	mxml_node_t *pop();
	mxml_node_t *peek();

	//theese are used to store the values
	struct{
	    struct {
		int major,minor;
	    }xml_version;
	}values;
	
};

#endif

/*
  ZynAddSubFX - a software synthesizer

  XML.h - XML wrapper
  Copyright (C) 2003-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <mxml.h>
#include <string>
#ifndef REALTYPE
#define REALTYPE float
#endif

#ifndef XML_WRAPPER_H
#define XML_WRAPPER_H

#define TMPSTR_SIZE 50

//the maxim tree depth
#define STACKSIZE 100

class XMLwrapper
{
public:
    XMLwrapper();
    ~XMLwrapper();

    /********************************/
    /*         SAVE to XML          */
    /********************************/

    //returns 0 if ok or -1 if the file cannot be saved
    int saveXMLfile(const std::string &filename);

    //returns the new allocated string that contains the XML data (used for clipboard)
    //the string is NULL terminated
    char *getXMLdata();

    //add simple parameter (name and value)
    void addpar(const std::string &name,int val);
    void addparreal(const std::string &name,REALTYPE val);

    //add boolean parameter (name and boolean value)
    //if the value is 0 => "yes", else "no"
    void addparbool(const std::string &name,int val);

    //add string parameter (name and string)
    void addparstr(const std::string &name,const std::string &val);

    //add a branch
    void beginbranch(const std::string &name);
    void beginbranch(const std::string &name, int id);

    //this must be called after each branch (nodes that contains child nodes)
    void endbranch();

    /********************************/
    /*        LOAD from XML         */
    /********************************/

    //returns 0 if ok or -1 if the file cannot be loaded
    int loadXMLfile(const std::string &filename);

    //used by the clipboard
    bool putXMLdata(const char *xmldata);

    //enter into the branch
    //returns 1 if is ok, or 0 otherwise
    int enterbranch(const std::string &name);


    //enter into the branch with id
    //returns 1 if is ok, or 0 otherwise
    int enterbranch(const std::string &name, int id);

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
    int getpar(const std::string &name,int defaultpar,int min,int max);

    //the same as getpar, but the limits are 0 and 127
    int getpar127(const std::string &name,int defaultpar);

    int getparbool(const std::string &name,int defaultpar);

    void getparstr(const std::string &name,char *par,int maxstrlen);
    REALTYPE getparreal(const char *name,REALTYPE defaultpar);
    REALTYPE getparreal(const char *name,REALTYPE defaultpar,REALTYPE min,REALTYPE max);

    bool minimal;//false if all parameters will be stored (used only for clipboard)

    struct {
        bool PADsynth_used;
    }information;

    //opens a file and parse only the "information" data on it
    //returns "true" if all went ok or "false" on errors
    bool checkfileinformation(const char *filename);

private:

    int dosavefile(const char *filename,int compression,const char *xmldata);
    char *doloadfile(const std::string &filename);


    mxml_node_t *tree;//all xml data
    mxml_node_t *root;//xml data used by zynaddsubfx
    mxml_node_t *node;//current node
    mxml_node_t *info;//this node is used to store the information about the data

    //adds params like this:
    // <name>
    //returns the node
    mxml_node_t *addparams0(const char *name);

    //adds params like this:
    // <name par1="val1">
    //returns the node
    mxml_node_t *addparams1(const char *name,const char *par1,const char *val1);

    //adds params like this:
    // <name par1="val1" par2="val2">
    //returns the node
    mxml_node_t *addparams2(const char *name,const char *par1,const char *val1,const char *par2, const char *val2);

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
    struct {
        struct {
            int major,minor;
        }xml_version;
    }values;

};

#endif

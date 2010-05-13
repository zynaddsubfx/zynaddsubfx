/*
  ZynAddSubFX - a software synthesizer

  Bank.h - Instrument Bank
  Copyright (C) 2002-2005 Nasca Octavian Paul
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

#include "Bank.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "Config.h"

#define INSTRUMENT_EXTENSION ".xiz"

//if this file exists into a directory, this make the directory to be considered as a bank, even if it not contains a instrument file
#define FORCE_BANK_DIR_FILE ".bankdir"

using namespace std;

Bank::Bank()
{
    defaultinsname = " ";

    for(int i = 0; i < BANK_SIZE; i++) {
        ins[i].used     = false;
        ins[i].info.PADsynth_used = false;
    }
    clearbank();

    bankfiletitle = dirname;

    loadbank(config.cfg.currentBankDir);
}

Bank::~Bank()
{
    clearbank();
}

/*
 * Get the name of an instrument from the bank
 */
string Bank::getname(unsigned int ninstrument)
{
    if(emptyslot(ninstrument))
        return defaultinsname;
    return ins[ninstrument].name;
}

/*
 * Get the numbered name of an instrument from the bank
 */
string Bank::getnamenumbered(unsigned int ninstrument)
{
    if(emptyslot(ninstrument))
        return defaultinsname;
    snprintf(tmpinsname[ninstrument],
             PART_MAX_NAME_LEN + 15,
             "%d. %s",
             ninstrument + 1,
             getname(ninstrument).c_str());
    return tmpinsname[ninstrument];
}

/*
 * Changes the name of an instrument (and the filename)
 */
void Bank::setname(unsigned int ninstrument, string newname, int newslot)
{
    if(emptyslot(ninstrument))
        return;

    string newfilename;
    char tmpfilename[100 + 1];

    if(newslot >= 0)
        snprintf(tmpfilename, 100, "%4d-%s", newslot + 1, newname.c_str());
    else
        snprintf(tmpfilename, 100, "%4d-%s", ninstrument + 1, newname.c_str());

    //add the zeroes at the start of filename
    for(int i = 0; i < 4; i++)
        if(tmpfilename[i] == ' ')
            tmpfilename[i] = '0';

    //make the filenames legal
    for(int i = 0; i < (int) strlen(tmpfilename); i++) {
        char c = tmpfilename[i];
        if((c >= '0') && (c <= '9'))
            continue;
        if((c >= 'A') && (c <= 'Z'))
            continue;
        if((c >= 'a') && (c <= 'z'))
            continue;
        if((c == '-') || (c == ' '))
            continue;

        tmpfilename[i] = '_';
    }

    newfilename = dirname + '/' + tmpfilename + ".xiz";

    rename(ins[ninstrument].filename.c_str(), newfilename.c_str());

    ins[ninstrument].filename = newfilename;
    ins[ninstrument].name = tmpfilename; //TODO limit name to PART_MAX_NAME_LEN
}

/*
 * Check if there is no instrument on a slot from the bank
 */
int Bank::emptyslot(unsigned int ninstrument)
{
    if(ninstrument >= BANK_SIZE)
        return 1;
    if(ins[ninstrument].filename.empty())
        return 1;

    if(ins[ninstrument].used)
        return 0;
    else
        return 1;
}

/*
 * Removes the instrument from the bank
 */
void Bank::clearslot(unsigned int ninstrument)
{
    if(emptyslot(ninstrument))
        return;

    remove(ins[ninstrument].filename.c_str());
    deletefrombank(ninstrument);
}

/*
 * Save the instrument to a slot
 */
void Bank::savetoslot(unsigned int ninstrument, Part *part)
{
    clearslot(ninstrument);

    const int maxfilename = 200;
    char      tmpfilename[maxfilename + 20];
    ZERO(tmpfilename, maxfilename + 20);

    snprintf(tmpfilename,
             maxfilename,
             "%4d-%s",
             ninstrument + 1,
             (char *)part->Pname);

    //add the zeroes at the start of filename
    for(int i = 0; i < 4; i++)
        if(tmpfilename[i] == ' ')
            tmpfilename[i] = '0';

    //make the filenames legal
    for(int i = 0; i < (int)strlen(tmpfilename); i++) {
        char c = tmpfilename[i];
        if((c >= '0') && (c <= '9'))
            continue;
        if((c >= 'A') && (c <= 'Z'))
            continue;
        if((c >= 'a') && (c <= 'z'))
            continue;
        if((c == '-') || (c == ' '))
            continue;

        tmpfilename[i] = '_';
    }

    strncat(tmpfilename, ".xiz", maxfilename + 10);

    string filename = dirname + '/' + tmpfilename;

    remove(filename.c_str());
    part->saveXML(filename.c_str());
    addtobank(ninstrument, tmpfilename, (char *) part->Pname);
}

/*
 * Loads the instrument from the bank
 */
void Bank::loadfromslot(unsigned int ninstrument, Part *part)
{
    if(emptyslot(ninstrument))
        return;

    part->defaultsinstrument();

    part->loadXMLinstrument(ins[ninstrument].filename.c_str());
}


/*
 * Makes current a bank directory
 */
int Bank::loadbank(string bankdirname)
{
    DIR *dir = opendir(bankdirname.c_str());
    clearbank();

    if(dir == NULL)
        return -1;

    dirname = bankdirname;

    bankfiletitle = dirname;

    struct dirent *fn;

    while((fn = readdir(dir))) {
        const char *filename = fn->d_name;

        //sa verific daca e si extensia dorita
        if(strstr(filename, INSTRUMENT_EXTENSION) == NULL)
            continue;

        //verify if the name is like this NNNN-name (where N is a digit)
        int no = 0;
        unsigned int startname = 0;

        for(unsigned int i = 0; i < 4; i++) {
            if(strlen(filename) <= i)
                break;

            if((filename[i] >= '0') && (filename[i] <= '9')) {
                no = no * 10 + (filename[i] - '0');
                startname++;
            }
        }


        if((startname + 1) < strlen(filename))
            startname++;                                //to take out the "-"

        string name = filename;

        //remove the file extension
        for(int i = name.size() - 1; i >= 2; i--) {
            if(name[i] == '.') {
                name = name.substr(0,i);
                break;
            }
        }

        if(no != 0) //the instrument position in the bank is found
            addtobank(no - 1, filename, name.substr(startname));
        else
            addtobank(-1, filename, name);
    }


    closedir(dir);

    if(!dirname.empty())
        config.cfg.currentBankDir = dirname;

    return 0;
}

/*
 * Makes a new bank, put it on a file and makes it current bank
 */
int Bank::newbank(string newbankdirname)
{
    int  result;
    string bankdir;
    bankdir = config.cfg.bankRootDirList[0];

    if(((bankdir[bankdir.size() - 1]) != '/')
       && ((bankdir[bankdir.size() - 1]) != '\\'))
        bankdir += "/";

    bankdir += newbankdirname;
#if OS_WINDOWS
    result = mkdir(bankdir.c_str());
#elif OS_LINUX || OS_CYGWIN
    result = mkdir(bankdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
#warning Undefined OS
#endif
    if(result < 0)
        return -1;

    string tmpfilename = bankdir + '/' + FORCE_BANK_DIR_FILE;

    FILE *tmpfile = fopen(tmpfilename.c_str(), "w+");
    fclose(tmpfile);

    return loadbank(bankdir);
}

/*
 * Check if the bank is locked (i.e. the file opened was readonly)
 */
int Bank::locked()
{
    return dirname.empty();
}

/*
 * Swaps a slot with another
 */
void Bank::swapslot(unsigned int n1, unsigned int n2)
{
    //TODO use std::swap
    if((n1 == n2) || (locked()))
        return;
    if(emptyslot(n1) && (emptyslot(n2)))
        return;
    if(emptyslot(n1)) { //change n1 to n2 in order to make
        int tmp = n2;
        n2 = n1;
        n1 = tmp;
    }

    if(emptyslot(n2)) { //this is just a movement from slot1 to slot2
        setname(n1, getname(n1), n2);
        ins[n2] = ins[n1];
        ins[n1].used     = false;
        ins[n1].name.clear();
        ins[n1].filename.clear();
        ins[n1].info.PADsynth_used = 0;
    }
    else {  //if both slots are used
        if(ins[n1].name == ins[n2].name) //change the name of the second instrument if the name are equal
            ins[n2].name += "2";

        setname(n1, getname(n1), n2);
        setname(n2, getname(n2), n1);
        ins_t tmp;
        tmp.used = true;
        tmp.name = ins[n2].name;
        string tmpfilename  = ins[n2].filename;
        bool  padsynth_used = ins[n2].info.PADsynth_used;

        ins[n2] = ins[n1];
        ins[n1].name = tmp.name;
        ins[n1].filename = tmpfilename;
        ins[n1].info.PADsynth_used = padsynth_used;
    }
}


bool Bank::bankstruct::operator<(const bankstruct &b) const
{
    return name < b.name;
}

/*
 * Re-scan for directories containing instrument banks
 */

void Bank::rescanforbanks()
{
    for(int i = 0; i < MAX_NUM_BANKS; i++) {
        banks[i].dir.clear();
        banks[i].name.clear();
    }

    for(int i = 0; i < MAX_BANK_ROOT_DIRS; i++)
        if(!config.cfg.bankRootDirList[i].empty())
            scanrootdir(config.cfg.bankRootDirList[i]);

    //sort the banks
    sort(banks.begin(), banks.end());

    //remove duplicate bank names
    int dupl = 0;
    for(int j = 0; j < banks.size() - 1; j++) {
        for(int i = j + 1; i < banks.size(); i++) {
            if(banks[i].name.empty() || banks[j].name.empty())
                continue;
            if(banks[i].name == banks[j].name) { //add a [1] to the first bankname and [n] to others
                char *tmpname = new char[strlen(tmpname) + 100];
                sprintf(tmpname, "%s[%d]", tmpname, dupl + 2);
                banks[i].name = tmpname;
                delete[] tmpname;

                if(dupl == 0)
                    banks[i].name += "[1]";

                dupl++;
            }
            else
                dupl = 0;
        }
    }
}



// private stuff

void Bank::scanrootdir(string rootdir)
{
    DIR *dir = opendir(rootdir.c_str());
    if(dir == NULL)
        return;

    const int maxdirsize = 1000;
    bankstruct bank;

    const char *separator = "/";
    if(rootdir.size()) {
        char tmp = rootdir[rootdir.size() - 1];
        if((tmp == '/') || (tmp == '\\'))
            separator = "";
    }

    struct dirent *fn;
    while((fn = readdir(dir))) {
        const char *dirname = fn->d_name;
        if(dirname[0] == '.')
            continue;

        bank.dir =  rootdir + separator + dirname + '/';
        bank.name = dirname;
        //find out if the directory contains at least 1 instrument
        bool isbank = false;

        DIR *d      = opendir(bank.dir.c_str());
        if(d == NULL)
            continue;

        struct dirent *fname;

        while((fname = readdir(d))) {
            if((strstr(fname->d_name, INSTRUMENT_EXTENSION) != NULL)
               || (strstr(fname->d_name, FORCE_BANK_DIR_FILE) != NULL)) {
                isbank = true;
                break; //aici as putea pune in loc de break un update la un counter care imi arata nr. de instrumente din bank
            }
        }

        closedir(d);

        if(isbank) {
            int pos = -1;
            for(int i = 1; i < MAX_NUM_BANKS; i++) { //banks[0] e liber intotdeauna
                if(banks[i].name.empty()) {
                    pos = i;
                    break;
                }
            }

            if(pos >= 0) {
                banks[pos].name = bank.name;
                banks[pos].dir  = bank.dir;
            }
        }
    }

    closedir(dir);
}

void Bank::clearbank()
{
    banks.clear();

    bankfiletitle.clear();
    dirname.clear();
}

int Bank::addtobank(int pos, string filename, string name)
{
    if((pos >= 0) && (pos < BANK_SIZE)) {
        if(ins[pos].used)
            pos = -1;             //force it to find a new free position
    }
    else
    if(pos >= BANK_SIZE)
        pos = -1;


    if(pos < 0) { //find a free position
        for(int i = BANK_SIZE - 1; i >= 0; i--)
            if(!ins[i].used) {
                pos = i;
                break;
            }
        ;
    }

    if(pos < 0)
        return -1;         //the bank is full

    // printf("%s   %d\n",filename,pos);

    deletefrombank(pos);

    ins[pos].used = true;
    ins[pos].name = name;

    snprintf(tmpinsname[pos], PART_MAX_NAME_LEN + 10, " ");

    ins[pos].filename = dirname + '/' + filename;

    //see if PADsynth is used
    if(config.cfg.CheckPADsynth) {
        XMLwrapper xml;
        xml.loadXMLfile(ins[pos].filename);

        ins[pos].info.PADsynth_used = xml.hasPadSynth();
    }
    else
        ins[pos].info.PADsynth_used = false;

    return 0;
}

bool Bank::isPADsynth_used(unsigned int ninstrument)
{
    if(config.cfg.CheckPADsynth == 0)
        return 0;
    else
        return ins[ninstrument].info.PADsynth_used;
}


void Bank::deletefrombank(int pos)
{
    if((pos < 0) || (pos >= banks.size()))
        return;
    ins[pos].used = false;
    ins[pos].name.clear();
    ins[pos].filename.clear();

    ZERO(tmpinsname[pos], PART_MAX_NAME_LEN + 20);
}


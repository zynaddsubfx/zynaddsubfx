/*
  ZynAddSubFX - a software synthesizer

  Bank.cpp - Instrument Bank
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2010-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Bank.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "Config.h"
#include "Util.h"
#include "Part.h"
#include "BankDb.h"
#ifdef WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <dlfcn.h>

static const char* my_dylib_path () {
    static Dl_info info;
    if (dladdr((const void*) &my_dylib_path, &info)) {
        return info.dli_fname;
    }
    return NULL;
}
#endif

using namespace std;

namespace zyn {

static const char* INSTRUMENT_EXTENSION = ".xiz";

//if this file exists into a directory, this make the directory to be considered as a bank, even if it not contains a instrument file
const char* FORCE_BANK_DIR_FILE = ".bankdir";

Bank::Bank(Config *config)
    :bankpos(0), defaultinsname(" "), config(config),
    db(new BankDb), bank_msb(0), bank_lsb(0)
{
    clearbank();
    bankfiletitle = dirname;
    rescanforbanks();
    loadbank(config->cfg.currentBankDir);

    for(unsigned i=0; i<banks.size(); ++i) {
        if(banks[i].dir == config->cfg.currentBankDir) {
            bankpos = i;
            break;
        }
    }
}

Bank::~Bank()
{
    clearbank();
    delete db;
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

    return stringFrom(ninstrument + 1) + ". " + getname(ninstrument);
}

/*
 * Changes the name of an instrument (and the filename)
 */
int Bank::setname(unsigned int ninstrument, const string &newname, int newslot)
{
    if(emptyslot(ninstrument))
        return 0;

    string newfilename;
    char   tmpfilename[100 + 1];
    tmpfilename[100] = 0;

    if(newslot >= 0)
        snprintf(tmpfilename, 100, "%4d-%s", newslot + 1, newname.c_str());
    else
        snprintf(tmpfilename, 100, "%4d-%s", ninstrument + 1, newname.c_str());

    //add the zeroes at the start of filename
    for(int i = 0; i < 4; ++i)
        if(tmpfilename[i] == ' ')
            tmpfilename[i] = '0';

    newfilename = dirname + legalizeFilename(tmpfilename) + ".xiz";

    int err = rename(ins[ninstrument].filename.c_str(), newfilename.c_str());
    if(err)
        return err;

    ins[ninstrument].filename = newfilename;
    ins[ninstrument].name     = newname;
    return err;
}

/*
 * Check if there is no instrument on a slot from the bank
 */
bool Bank::emptyslot(unsigned int ninstrument)
{
    if(ninstrument >= BANK_SIZE)
        return true;
    if(ins[ninstrument].filename.empty())
        return true;

    return false;
}

/*
 * Removes the instrument from the bank
 */
int Bank::clearslot(unsigned int ninstrument)
{
    if(emptyslot(ninstrument))
        return 0;

    //no error when no file
    FILE *f = fopen(ins[ninstrument].filename.c_str(), "r");
    if(!f)
        return 0;
    fclose(f);

    int err = remove(ins[ninstrument].filename.c_str());
    if(!err)
        deletefrombank(ninstrument);
    return err;
}

/*
 * Save the instrument to a slot
 */
int Bank::savetoslot(unsigned int ninstrument, Part *part)
{
    int err = clearslot(ninstrument);
    if(err)
        return err;

    const int maxfilename = 200;
    char      tmpfilename[maxfilename + 20];
    ZERO(tmpfilename, maxfilename + 20);

    snprintf(tmpfilename,
             maxfilename,
             "%04d-%s",
             ninstrument + 1,
             (char *)part->Pname);

    string filename = dirname + '/' + legalizeFilename(tmpfilename) + ".xiz";

    FILE *f = fopen(filename.c_str(), "r");
    if(f) {
        fclose(f);

        err = remove(filename.c_str());
        if(err)
            return err;
    }

    err = part->saveXML(filename.c_str());
    if(err)
        return err;
    addtobank(ninstrument, legalizeFilename(tmpfilename) + ".xiz",
              (char *) part->Pname);
    //Since we've changed the contents of one of the banks, rescan the
    //database to keep it updated.
    db->scanBanks();
    return 0;
}

/*
 * Loads the instrument from the bank
 */
int Bank::loadfromslot(unsigned int ninstrument, Part *part)
{
    if(emptyslot(ninstrument))
        return 0;

    part->AllNotesOff();
    part->defaultsinstrument();

    part->loadXMLinstrument(ins[ninstrument].filename.c_str());
    return 0;
}

/*
 * Makes current a bank directory
 */
int Bank::loadbank(string bankdirname)
{
    normalizedirsuffix(bankdirname);
    DIR *dir = opendir(bankdirname.c_str());
    clearbank();

    if(dir == NULL)
        return -1;

    //set msb when possible
    bank_msb = 0;
    for(unsigned i=0; i<banks.size(); i++)
        if(banks[i].dir == bankdirname)
            bank_msb = i;

    dirname = bankdirname;

    bankfiletitle = dirname;

    struct dirent *fn;

    while((fn = readdir(dir))) {
        const char *filename = fn->d_name;

        //check for extension
        if(strstr(filename, INSTRUMENT_EXTENSION) == NULL)
            continue;

        //verify if the name is like this NNNN-name (where N is a digit)
        int no = 0;
        unsigned int startname = 0;

        for(unsigned int i = 0; i < 4; ++i) {
            if(strlen(filename) <= i)
                break;

            if((filename[i] >= '0') && (filename[i] <= '9')) {
                no = no * 10 + (filename[i] - '0');
                startname++;
            }
        }

        if((startname + 1) < strlen(filename))
            startname++;  //to take out the "-"

        string name = filename;

        //remove the file extension
        for(int i = name.size() - 1; i >= 2; i--)
            if(name[i] == '.') {
                name = name.substr(0, i);
                break;
            }

        if(no != 0) //the instrument position in the bank is found
            addtobank(no - 1, filename, name.substr(startname));
        else
            addtobank(-1, filename, name);
    }

    closedir(dir);

    if(!dirname.empty())
        config->cfg.currentBankDir = dirname;

    return 0;
}

/*
 * Makes a new bank, put it on a file and makes it current bank
 */
int Bank::newbank(string newbankdirname)
{
    string bankdir;
    bankdir = config->cfg.bankRootDirList[0];

    expanddirname(bankdir);

    bankdir += newbankdirname;
#ifdef _WIN32
    if(mkdir(bankdir.c_str()) < 0)
#else
    if(mkdir(bankdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0)
#endif
        return -1;

    const string tmpfilename = bankdir + '/' + FORCE_BANK_DIR_FILE;

    FILE *tmpfile = fopen(tmpfilename.c_str(), "w+");
    fclose(tmpfile);

    return loadbank(bankdir);
}

/*
 * Check if the bank is locked (i.e. the file opened was readonly)
 */
int Bank::locked()
{
    //XXX Fixme
    return dirname.empty();
}

/*
 * Swaps a slot with another
 */
int Bank::swapslot(unsigned int n1, unsigned int n2)
{
    int err = 0;
    if((n1 == n2) || (locked()))
        return 0;
    if(emptyslot(n1) && (emptyslot(n2)))
        return 0;
    if(emptyslot(n1)) //change n1 to n2 in order to make
        swap(n1, n2);

    if(emptyslot(n2)) { //this is just a movement from slot1 to slot2
        err |= setname(n1, getname(n1), n2);
        if(err)
            return err;
        ins[n2] = ins[n1];
        ins[n1] = ins_t();
    }
    else {  //if both slots are used
        if(ins[n1].name == ins[n2].name) //change the name of the second instrument if the name are equal
            ins[n2].name += "2";

        err |= setname(n1, getname(n1), n2);
        err |= setname(n2, getname(n2), n1);
        if(err)
            return err;
        swap(ins[n2], ins[n1]);
    }
    return err;
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
    db->clear();
    //remove old banks
    banks.clear();

    for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i)
        if(!config->cfg.bankRootDirList[i].empty())
            scanrootdir(config->cfg.bankRootDirList[i]);
#ifdef WIN32
    {
        //Search the VST Directory for banks/preset/etc
        char path[1024];
        GetModuleFileName(GetModuleHandle("ZynAddSubFX.dll"), path, sizeof(path));
        if(strstr(path, "ZynAddSubFX.dll")) {
            strstr(path, "ZynAddSubFX.dll")[0] = 0;
            strcat(path, "banks");
            scanrootdir(path);
        }
    }
#endif
#ifdef __APPLE__
   {
        const char* path = my_dylib_path ();
        if (path && strstr(path, "ZynAddSubFX.dylib") && strlen (path) < 1000) {
            char tmp[1024];
            strcpy (tmp, path);
            strstr (tmp, "ZynAddSubFX.dylib")[0] = 0; // LV2
            strcat (tmp, "banks");
            scanrootdir(tmp);
            strcpy (tmp, path);
            strstr (tmp, "ZynAddSubFX.dylib")[0] = 0;
            strcat (tmp, "../Resources/banks"); // VST
            scanrootdir(tmp);
        }
   }
#endif

    //sort the banks
    sort(banks.begin(), banks.end());

    for(int i = 0; i < (int) banks.size(); ++i)
        db->addBankDir(banks[i].dir);

    //remove duplicate bank names
    for(int j = 0; j < (int) banks.size() - 1; ++j) {
        int dupl = 0;
        for(int i = j + 1; i < (int) banks.size(); ++i) {
            if(banks[i].name == banks[j].name) {
                //add a [1] to the first bankname and [n] to others
                banks[i].name = banks[i].name + '['
                                + stringFrom(dupl + 2) + ']';
                dupl++;
            }
        }
        if(dupl != 0)
            banks[j].name += "[1]";
        if(dupl)
            j += dupl;
    }
    db->scanBanks();
}

void Bank::setMsb(uint8_t msb)
{
    if(msb < banks.size() && banks[msb].dir != bankfiletitle)
        loadbank(banks[msb].dir);
}

void Bank::setLsb(uint8_t lsb)
{
    //should only involve values of 0/1 for the time being...
    bank_lsb = limit<uint8_t>(lsb,0,1);
}


// private stuff

void Bank::scanrootdir(string rootdir)
{
    expanddirname(rootdir);

    DIR *dir = opendir(rootdir.c_str());
    if(dir == NULL)
        return;

    bankstruct bank;
    struct dirent *fn;
    while((fn = readdir(dir))) {
        const char *dirname = fn->d_name;
        if(dirname[0] == '.')
            continue;

        bank.dir  = rootdir + dirname + '/';
        bank.name = dirname;
        //find out if the directory contains at least 1 instrument
        bool isbank = false;

        DIR *d = opendir(bank.dir.c_str());
        if(d == NULL)
            continue;

        struct dirent *fname;

        while((fname = readdir(d))) {
            if((strstr(fname->d_name, INSTRUMENT_EXTENSION) != NULL)
               || (strstr(fname->d_name, FORCE_BANK_DIR_FILE) != NULL)) {
                isbank = true;
                break; //could put a #instrument counter here instead
            }
        }

        if(isbank)
            banks.push_back(bank);

        closedir(d);
    }

    closedir(dir);
}

void Bank::clearbank()
{
    for(int i = 0; i < BANK_SIZE; ++i)
        ins[i] = ins_t();

    bankfiletitle.clear();
    dirname.clear();
}

std::vector<std::string> Bank::search(std::string s) const
{
    std::vector<std::string> out;
    auto vec = db->search(s);
    for(auto e:vec) {
        out.push_back(e.name);
        out.push_back(e.bank+e.file);
    }
    return out;
}

std::vector<std::string> Bank::blist(std::string s)
{
    std::vector<std::string> out;
    loadbank(s);
    for(int i=0; i<128; ++i) {
        if(ins[i].filename.empty())
            out.push_back("Empty Preset");
        else
            out.push_back(ins[i].name);
        out.push_back(to_s(i));
    }
    return out;
}

int Bank::addtobank(int pos, string filename, string name)
{
    if((pos >= 0) && (pos < BANK_SIZE)) {
        if(!ins[pos].filename.empty())
            pos = -1;  //force it to find a new free position
    }
    else
    if(pos >= BANK_SIZE)
        pos = -1;


    if(pos < 0)   //find a free position
        for(int i = BANK_SIZE - 1; i >= 0; i--)
            if(ins[i].filename.empty()) {
                pos = i;
                break;
            }

    if(pos < 0)
        return -1;  //the bank is full

    deletefrombank(pos);

    ins[pos].name     = name;
    ins[pos].filename = dirname + filename;
    return 0;
}

void Bank::deletefrombank(int pos)
{
    if((pos < 0) || (pos >= BANK_SIZE))
        return;
    ins[pos] = ins_t();
}

Bank::ins_t::ins_t()
    :name(""), filename("")
{}

}

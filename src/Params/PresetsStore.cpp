/*
  ZynAddSubFX - a software synthesizer

  PresetsStore.cpp - Presets and Clipboard store
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include <iostream>
#include <algorithm>
#include <cctype>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "PresetsStore.h"
#include "../Misc/XMLwrapper.h"
#include "../Misc/Util.h"
#include "../Misc/Config.h"

using namespace std;

namespace zyn {

//XXX to remove
//PresetsStore presetsstore;

PresetsStore::PresetsStore(const Config& config) : config(config)
{
}

PresetsStore::~PresetsStore()
{
}

//Clipboard management

void PresetsStore::copyclipboard(XMLwrapper &xml, char *type)
{
    clipboard.type = type;
    const char *tmp = xml.getXMLdata();
    clipboard.data = tmp;
    free((void*)tmp);
}

bool PresetsStore::pasteclipboard(XMLwrapper &xml)
{
    if(clipboard.data.empty())
        return false;
    xml.putXMLdata(clipboard.data.c_str());
    return true;
}

bool PresetsStore::checkclipboardtype(const char *type)
{
    //makes LFO's compatible
    if(strstr(type, "Plfo") && strstr(clipboard.type.c_str(), "Plfo"))
        return true;
    return type == clipboard.type;
}

//Presets management
void PresetsStore::clearpresets()
{
    presets.clear();
}

//a helper function that compares 2 presets[]
bool PresetsStore::presetstruct::operator<(const presetstruct &b) const
{
    return name < b.name;
}

void PresetsStore::scanforpresets()
{
    clearpresets();
    string ftype = ".xpz";

    for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i) {
        if(config.cfg.presetsDirList[i].empty())
            continue;

        //open directory
        string dirname = config.cfg.presetsDirList[i];
        expanddirname(dirname);
        DIR   *dir     = opendir(dirname.c_str());
        if(dir == NULL)
            continue;
        struct dirent *fn;

        //check all files in directory
        while((fn = readdir(dir))) {
            string filename = fn->d_name;
            if(filename.find(ftype) == string::npos)
                continue;

            string location = dirname + filename;

            //trim file type off of name
            string name_type = filename.substr(0, filename.find(ftype));

            size_t tmp  = name_type.find_last_of(".");
            if(tmp == string::npos)
                continue;
            string type = name_type.substr(tmp+1);
            string name = name_type.substr(0, tmp);

            //put on list
            presets.push_back(presetstruct{location, name, type});
        }

        closedir(dir);
    }

    //sort the presets
    sort(presets.begin(), presets.end());
}


void PresetsStore::copypreset(XMLwrapper &xml, char *type, string name)
{
    if(config.cfg.presetsDirList[0].empty())
        return;

    //make the filenames legal
    name = legalizeFilename(name);

    //make path legal
    string dirname = config.cfg.presetsDirList[0];
    expanddirname(dirname);

    string filename("" + dirname + name + "." + &type[1] + ".xpz");

    xml.saveXMLfile(filename, config.cfg.GzipCompression);
}

bool PresetsStore::pastepreset(XMLwrapper &xml, unsigned int npreset)
{
    npreset--;
    if(npreset >= presets.size())
        return false;
    string filename = presets[npreset].file;
    if(filename.empty())
        return false;
    return xml.loadXMLfile(filename) >= 0;
}

void PresetsStore::deletepreset(unsigned int npreset)
{
    npreset--;
    if(npreset >= presets.size())
        return;
    string filename = presets[npreset].file;
    if(filename.empty())
        return;
    remove(filename.c_str());
}

void PresetsStore::deletepreset(std::string filename)
{
    for(int i=0; i<(int)presets.size(); ++i) {
        if(presets[i].file == filename) {
            presets.erase(presets.begin()+i);
            remove(filename.c_str());
            return;
        }
    }
}

}

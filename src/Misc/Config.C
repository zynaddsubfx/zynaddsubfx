/*
  ZynAddSubFX - a software synthesizer
 
  Config.C - Configuration file functions
  Copyright (C) 2003-2004 Nasca Octavian Paul
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
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef OS_WINDOWS
#include <windows.h>
#include <mmsystem.h>
#endif

#include "Config.h"
#include "XMLwrapper.h"

Config::Config(){
    maxstringsize=MAX_STRING_SIZE;//for ui
    //defaults
    cfg.SampleRate=44100;
    cfg.SoundBufferSize=256;
    cfg.OscilSize=512;
    cfg.SwapStereo=0;
    
    cfg.LinuxOSSWaveOutDev=new char[MAX_STRING_SIZE];
    snprintf(cfg.LinuxOSSWaveOutDev,MAX_STRING_SIZE,"/dev/dsp");
    cfg.LinuxOSSSeqInDev=new char[MAX_STRING_SIZE];
    snprintf(cfg.LinuxOSSSeqInDev,MAX_STRING_SIZE,"/dev/sequencer");

    cfg.DumpFile=new char[MAX_STRING_SIZE];
    snprintf(cfg.DumpFile,MAX_STRING_SIZE,"zynaddsubfx_dump.txt");

    cfg.WindowsWaveOutId=0;
    cfg.WindowsMidiInId=0;
    
    cfg.BankUIAutoClose=0;
    cfg.DumpNotesToFile=0;
    cfg.DumpAppend=1;

    cfg.GzipCompression=3;

    cfg.Interpolation=0;

    winwavemax=1;winmidimax=1;
//try to find out how many input midi devices are there
#ifdef WINMIDIIN
    winmidimax=midiInGetNumDevs();
    if (winmidimax==0) winmidimax=1;
#endif
    winmididevices=new winmidionedevice[winmidimax];
    for (int i=0;i<winmidimax;i++) {
	winmididevices[i].name=new char[MAX_STRING_SIZE];
	for (int j=0;j<MAX_STRING_SIZE;j++) winmididevices[i].name[j]='\0';
    };

//get the midi input devices name
#ifdef WINMIDIIN
    MIDIINCAPS midiincaps;
    for (int i=0;i<winmidimax;i++){
	if (! midiInGetDevCaps(i,&midiincaps,sizeof(MIDIINCAPS)))
	    snprintf(winmididevices[i].name,MAX_STRING_SIZE,"%s",midiincaps.szPname);
    };
#endif
    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) cfg.bankRootDirList[i]=NULL;
    cfg.currentBankDir=new char[MAX_STRING_SIZE];
    sprintf(cfg.currentBankDir,"./testbnk");
    
    
    char filename[MAX_STRING_SIZE];
    getConfigFileName(filename,MAX_STRING_SIZE);
    readConfig(filename);
    if (cfg.bankRootDirList[0]==NULL){
#if defined(OS_LINUX)
        cfg.bankRootDirList[0]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[0],"~/banks");

	cfg.bankRootDirList[1]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[1],"./");

        cfg.bankRootDirList[2]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[2],"/usr/share/zynaddsubfx/banks");

	cfg.bankRootDirList[3]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[3],"/usr/local/share/zynaddsubfx/banks");

	cfg.bankRootDirList[4]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[4],"../banks");

	cfg.bankRootDirList[5]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[5],"banks");
#else
	cfg.bankRootDirList[0]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[0],"./");

	cfg.bankRootDirList[1]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[1],"../banks");

	cfg.bankRootDirList[2]=new char[MAX_STRING_SIZE];
	sprintf(cfg.bankRootDirList[2],"banks");
#endif
    };
};

Config::~Config(){
    char filename[MAX_STRING_SIZE];
    getConfigFileName(filename,MAX_STRING_SIZE);
    saveConfig(filename);

    delete(cfg.LinuxOSSWaveOutDev);
    delete(cfg.LinuxOSSSeqInDev);
    delete(cfg.DumpFile);

    for (int i=0;i<winmidimax;i++) delete (winmididevices[i].name);
    delete(winmididevices);
};

void Config::clearbankrootdirlist(){
    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) {
	if (cfg.bankRootDirList[i]==NULL) delete(cfg.bankRootDirList[i]);
	cfg.bankRootDirList[i]=NULL;
    };
};


void Config::readConfig(char *filename){
    XMLwrapper *xmlcfg=new XMLwrapper();
    if (xmlcfg->loadXMLfile(filename)<0) return;
    if (xmlcfg->enterbranch("CONFIGURATION")){
	cfg.SampleRate=xmlcfg->getpar("sample_rate",cfg.SampleRate,4000,1024000);
	cfg.SoundBufferSize=xmlcfg->getpar("sound_buffer_size",cfg.SoundBufferSize,2,8192);
	cfg.OscilSize=xmlcfg->getpar("oscil_size",cfg.OscilSize,MAX_AD_HARMONICS*2,131072);
	cfg.SwapStereo=xmlcfg->getpar("swap_stereo",cfg.SwapStereo,0,1);
	cfg.BankUIAutoClose=xmlcfg->getpar("bank_window_auto_close",cfg.BankUIAutoClose,0,1);

	cfg.DumpNotesToFile=xmlcfg->getpar("dump_notes_to_file",cfg.DumpNotesToFile,0,1);
	cfg.DumpAppend=xmlcfg->getpar("dump_append",cfg.DumpAppend,0,1);
	xmlcfg->getparstr("dump_file",cfg.DumpFile,MAX_STRING_SIZE);

	cfg.GzipCompression=xmlcfg->getpar("gzip_compression",cfg.GzipCompression,0,9);

	xmlcfg->getparstr("bank_current",cfg.currentBankDir,MAX_STRING_SIZE);
	cfg.Interpolation=xmlcfg->getpar("interpolation",cfg.Interpolation,0,1);

	//get bankroot dirs
	for (int i=0;i<MAX_BANK_ROOT_DIRS;i++){
	    if (xmlcfg->enterbranch("BANKROOT",i)){
	      cfg.bankRootDirList[i]=new char[MAX_STRING_SIZE];
	      xmlcfg->getparstr("bank_root",cfg.bankRootDirList[i],MAX_STRING_SIZE);
	     xmlcfg->exitbranch();
	    };
	//xmlcfg->getparstr("bank_root_list",cfg.bankRootDirList,MAX_STRING_SIZE);
	};

	//linux stuff
	xmlcfg->getparstr("linux_oss_wave_out_dev",cfg.LinuxOSSWaveOutDev,MAX_STRING_SIZE);
	xmlcfg->getparstr("linux_oss_seq_in_dev",cfg.LinuxOSSSeqInDev,MAX_STRING_SIZE);
	
	//windows stuff
	cfg.WindowsWaveOutId=xmlcfg->getpar("windows_wave_out_id",cfg.WindowsWaveOutId,0,winwavemax);
	cfg.WindowsMidiInId=xmlcfg->getpar("windows_midi_in_id",cfg.WindowsMidiInId,0,winmidimax);



      xmlcfg->exitbranch();
    };
    delete(xmlcfg);

    cfg.OscilSize=(int) pow(2,ceil(log (cfg.OscilSize-1.0)/log(2.0)));

};

void Config::saveConfig(char *filename){
    XMLwrapper *xmlcfg=new XMLwrapper();
    
    xmlcfg->beginbranch("CONFIGURATION");    
    
	xmlcfg->addpar("sample_rate",cfg.SampleRate);
	xmlcfg->addpar("sound_buffer_size",cfg.SoundBufferSize);
	xmlcfg->addpar("oscil_size",cfg.OscilSize);
	xmlcfg->addpar("swap_stereo",cfg.SwapStereo);
	xmlcfg->addpar("bank_window_auto_close",cfg.BankUIAutoClose);

	xmlcfg->addpar("dump_notes_to_file",cfg.DumpNotesToFile);
	xmlcfg->addpar("dump_append",cfg.DumpAppend);
	xmlcfg->addparstr("dump_file",cfg.DumpFile);

	xmlcfg->addpar("gzip_compression",cfg.GzipCompression);

	xmlcfg->addparstr("bank_current",cfg.currentBankDir);

	for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) if (cfg.bankRootDirList[i]!=NULL) {
	    xmlcfg->beginbranch("BANKROOT",i);
	     xmlcfg->addparstr("bank_root",cfg.bankRootDirList[i]);
	    xmlcfg->endbranch();
	};

	xmlcfg->addpar("interpolation",cfg.Interpolation);

	//linux stuff
	xmlcfg->addparstr("linux_oss_wave_out_dev",cfg.LinuxOSSWaveOutDev);
	xmlcfg->addparstr("linux_oss_seq_in_dev",cfg.LinuxOSSSeqInDev);
	
	//windows stuff
	xmlcfg->addpar("windows_wave_out_id",cfg.WindowsWaveOutId);
	xmlcfg->addpar("windows_midi_in_id",cfg.WindowsMidiInId);

    xmlcfg->endbranch();
    
    int tmp=cfg.GzipCompression;
    cfg.GzipCompression=0;
    xmlcfg->saveXMLfile(filename);
    cfg.GzipCompression=tmp;
    
    delete(xmlcfg);
};

void Config::getConfigFileName(char *name, int namesize){
    name[0]=0;
#ifdef OS_LINUX
    snprintf(name,namesize,"%s%s",getenv("HOME"),"/.zynaddsubfxXML.cfg");
#else
    snprintf(name,namesize,"%s","zynaddsubfxXML.cfg");
#endif

};


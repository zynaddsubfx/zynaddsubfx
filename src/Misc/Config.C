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

    cfg.bankRootDirList=new char[MAX_STRING_SIZE];
#if defined(OS_LINUX)
    sprintf(cfg.bankRootDirList,"./\n/usr/share/zynaddsubfx/banks\n/usr/local/share/zynaddsubfx/banks");
#else
    sprintf(cfg.bankRootDirList,"./");
#endif
    cfg.currentBankDir=new char[MAX_STRING_SIZE];
    sprintf(cfg.currentBankDir,"./testbnk");
    
    
    char filename[MAX_STRING_SIZE];
    getConfigFileName(filename,MAX_STRING_SIZE);
    readConfig(filename);
    
    ui.showinstrumentinfo=0;

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

void Config::readConfig(char *filename){
    FILE *file=fopen(filename,"r");
    if (file==NULL) return;
    while (!feof(file)){
	//read a line
	char line[MAX_STRING_SIZE+1];line[MAX_STRING_SIZE]=0;
	if (fgets(line,MAX_STRING_SIZE,file)==NULL) continue;
	if (strlen(line)<2) continue;

	//find out if the line begins with "#" (comment)
	char firstchar=0;
	for (int i=0;i<strlen(line);i++) 
	    if (line[i]>33) {
		firstchar=line[i];
		break;
	    };
	if (firstchar=='#') continue;
	//get the parameter and the value
	char par[MAX_STRING_SIZE],val[MAX_STRING_SIZE];
	par[0]=0;val[0]=0;
	if (sscanf(line,"%s = %s",par,val)!=2) continue;//bad line
	if ((strlen(par)<1)|| (strlen(val)<1)) continue;//empty parameter/value
	
//	printf("%s\n",val);


	int intvalue=0;
	char *tmp;
	intvalue=strtoul(val,&tmp,10);

	//Check the parameter
	if (strstr(par,"SAMPLE_RATE")!=NULL){
	    cfg.SampleRate=intvalue;
	};
	if (strstr(par,"SOUND_BUFFER_SIZE")!=NULL){
	    cfg.SoundBufferSize=intvalue;
	};
	if (strstr(par,"OSCIL_SIZE")!=NULL){
	    cfg.OscilSize=intvalue;
	};
	if (strstr(par,"SWAP_STEREO")!=NULL){
	    cfg.SwapStereo=intvalue;
	};
	if (strstr(par,"BANK_WINDOW_AUTO_CLOSE")!=NULL){
	    cfg.BankUIAutoClose=intvalue;
	};
	if (strstr(par,"LINUX_OSS_WAVE_OUT_DEV")!=NULL){
	    if (strlen(val)<2) continue;
	    snprintf(cfg.LinuxOSSWaveOutDev,MAX_STRING_SIZE,val);
	};
	if (strstr(par,"LINUX_OSS_SEQ_IN_DEV")!=NULL){
	    if (strlen(val)<2) continue;
	    snprintf(cfg.LinuxOSSSeqInDev,MAX_STRING_SIZE,val);
	};
	
	if (strstr(par,"WINDOWS_WAVE_OUT_ID")!=NULL){
	    cfg.WindowsWaveOutId=intvalue;
	};

	if (strstr(par,"WINDOWS_MIDI_IN_ID")!=NULL){
	    cfg.WindowsMidiInId=intvalue;
	};

	if (strstr(par,"DUMP_NOTES_TO_FILE")!=NULL){
	    cfg.DumpNotesToFile=intvalue;
	};
	if (strstr(par,"DUMP_APPEND")!=NULL){
	    cfg.DumpAppend=intvalue;
	};
	if (strstr(par,"DUMP_FILE")!=NULL){
	    if (strlen(val)<2) continue;
	    snprintf(cfg.DumpFile,MAX_STRING_SIZE,val);
	};

	if (strstr(par,"GZIP_COMPRESSION")!=NULL){
	    cfg.GzipCompression=intvalue;
	};
};    
    fclose(file);    
    
    //now, correct the bogus (incorrect) parameters 
    if (cfg.SampleRate<4000) cfg.SampleRate=4000;
    if (cfg.SoundBufferSize<2) cfg.SoundBufferSize=2;
    
    if (cfg.OscilSize<MAX_AD_HARMONICS*2) cfg.OscilSize=MAX_AD_HARMONICS*2;
    cfg.OscilSize=(int) pow(2,ceil(log (cfg.OscilSize-1.0)/log(2.0)));
    
    cfg.SwapStereo=(cfg.SwapStereo!=0?1:0);
#ifdef OS_WINDOWS
    if (cfg.WindowsWaveOutId>=winwavemax) cfg.WindowsWaveOutId=0;
    if (cfg.WindowsMidiInId>=winmidimax) cfg.WindowsMidiInId=0;
#endif
    if (cfg.GzipCompression<0) cfg.GzipCompression=0;
	else if (cfg.GzipCompression>9) cfg.GzipCompression=9;

};

void Config::saveConfig(char *filename){
    FILE *file=fopen(filename,"w");
    if (file==NULL) return;
    fprintf(file,"%s","#ZynAddSubFX configuration file\n");
    fprintf(file,"SAMPLE_RATE = %d\n",cfg.SampleRate);
    fprintf(file,"SOUND_BUFFER_SIZE = %d\n",cfg.SoundBufferSize);
    fprintf(file,"OSCIL_SIZE = %d\n",cfg.OscilSize);
    fprintf(file,"SWAP_STEREO = %d\n",cfg.SwapStereo);

    fprintf(file,"BANK_WINDOW_AUTO_CLOSE = %d\n",cfg.BankUIAutoClose);

    fprintf(file,"DUMP_NOTES_TO_FILE = %d\n",cfg.DumpNotesToFile);
    fprintf(file,"DUMP_APPEND = %d\n",cfg.DumpAppend);
    fprintf(file,"DUMP_FILE = %s\n",cfg.DumpFile);
    
    fprintf(file,"GZIP_COMPRESSION = %d\n",cfg.GzipCompression);

    fprintf(file,"#Linux\n");
    fprintf(file,"LINUX_OSS_WAVE_OUT_DEV = %s\n",cfg.LinuxOSSWaveOutDev);
    fprintf(file,"LINUX_OSS_SEQ_IN_DEV = %s\n",cfg.LinuxOSSSeqInDev);

    fprintf(file,"#Windows\n");
    fprintf(file,"WINDOWS_WAVE_OUT_ID = %d\n",cfg.WindowsWaveOutId);
    fprintf(file,"WINDOWS_MIDI_IN_ID = %d\n",cfg.WindowsMidiInId);
    
    fclose(file);    
};

void Config::getConfigFileName(char *name, int namesize){
    name[0]=0;
#ifdef OS_LINUX
    snprintf(name,namesize,"%s%s",getenv("HOME"),"/.zynaddsubfx.cfg");
#else
    snprintf(name,namesize,"%s","zynaddsubfx.cfg");
#endif

};


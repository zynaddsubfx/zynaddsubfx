/*
  ZynAddSubFX - a software synthesizer

  Config.h - Configuration file functions
  Copyright (C) 2003-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#define MAX_STRING_SIZE 4000
#define MAX_BANK_ROOT_DIRS 100

namespace rtosc
{
    struct Ports;
}

namespace zyn {

class oss_devs_t
{
    public:
        char *linux_wave_out, *linux_seq_in;
};

/**Configuration file functions*/
class Config
{
    public:
        Config();
        Config(const Config& ) = delete;
        ~Config();

        struct {
            oss_devs_t oss_devs;
            int   SampleRate, SoundBufferSize, OscilSize, SwapStereo;
            bool  AudioOutputCompressor;
            int   WindowsWaveOutId, WindowsMidiInId;
            int   BankUIAutoClose;
            int   GzipCompression;
            int   Interpolation;
            int   SaveFullXml; // when saving to a file save entire tree including disabled parts (Zynmuse)
            std::string bankRootDirList[MAX_BANK_ROOT_DIRS], currentBankDir;
            std::string presetsDirList[MAX_BANK_ROOT_DIRS];
            std::string favoriteList[MAX_BANK_ROOT_DIRS];
            int CheckPADsynth;
            int IgnoreProgramChange;
            int UserInterfaceMode;
            int VirKeybLayout;
            std::string LinuxALSAaudioDev;
            std::string nameTag;
        } cfg;
        int winwavemax, winmidimax; //number of wave/midi devices on Windows
        int maxstringsize;

        struct winmidionedevice {
            char *name;
        };
        winmidionedevice *winmididevices;

        void clearbankrootdirlist();
        void clearpresetsdirlist();
        void init();
        void save() const;

        static const rtosc::Ports &ports;
    private:
        void readConfig(const char *filename);
        void saveConfig(const char *filename) const;
        void getConfigFileName(char *name, int namesize) const;
};

}

#endif

/*
  ZynAddSubFX - a software synthesizer

  Master.h - It sends Midi Messages to Parts, receives samples from parts,
             process them with system/insertion effects and mix them
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef MASTER_H
#define MASTER_H
#include "../globals.h"
#include "Microtonal.h"
#include <rtosc/automations.h>
#include <rtosc/savefile.h>

#include "Time.h"
#include "Bank.h"
#include "Recorder.h"

#include "../Params/Controller.h"
#include "../Synth/WatchPoint.h"

namespace zyn {

class Allocator;

struct vuData {
    vuData(void);
    float outpeakl, outpeakr, maxoutpeakl, maxoutpeakr,
          rmspeakl, rmspeakr;
    int clipped;
};


/** It sends Midi Messages to Parts, receives samples from parts,
 *  process them with system/insertion effects and mix them */
class Master
{
    public:
        Master(const Master& other) = delete;
        Master(Master&& other) = delete;

        /** Constructor TODO make private*/
        Master(const SYNTH_T &synth, class Config *config);
        /** Destructor*/
        ~Master();

        char last_xmz[XMZ_PATH_MAX];

        //applyOscEvent overlays
        bool applyOscEvent(const char *event, float *outl, float *outr,
                           bool offline, bool nio = true, int msg_id = -1);
        bool applyOscEvent(const char *event, bool nio = true, int msg_id = -1);

        /**Saves all settings to a XML file
         * @return 0 for ok or <0 if there is an error*/
        int saveXML(const char *filename);

        /**This adds the parameters to the XML data*/
        void add2XML(XMLwrapper& xml);

        static void saveAutomation(XMLwrapper &xml, const rtosc::AutomationMgr &midi);
        static void loadAutomation(XMLwrapper &xml,       rtosc::AutomationMgr &midi);

        void defaults();

        /**loads all settings from a XML file
         * @return 0 for ok or -1 if there is an error*/
        int loadXML(const char *filename);

        /**Save all settings to an OSC file (as specified by RT OSC)
         * When the function returned, the OSC file has been either saved or
         * an error occured.
         * @param filename File to save to or NULL (useful for testing)
         * @param dispatcher Message dispatcher and modifier
         * @param master2 An empty master dummy where the savefile will be
         *                loaded to and compared with the current master
         * @return 0 for ok or <0 if there is an error*/
        int saveOSC(const char *filename,
                    class master_dispatcher_t* dispatcher,
                    Master* master2);
        /**loads all settings from an OSC file (as specified by RT OSC)
         * @param dispatcher Message dispatcher and modifier
         * @return 0 for ok or <0 if there is an error*/
        int loadOSC(const char *filename,
                    rtosc::savefile_dispatcher_t* dispatcher);

        /**Regenerate PADsynth and other non-RT parameters
         * It is NOT SAFE to call this from a RT context*/
        void applyparameters(void) NONREALTIME;

        //This must be called prior-to/at-the-time-of RT insertion
        void initialize_rt(void) REALTIME;

        void getfromXML(XMLwrapper& xml);

        /**get all data to a newly allocated array (used for plugin)
         * @return the datasize*/
        int getalldata(char **data) NONREALTIME;
        /**put all data from the *data array to zynaddsubfx parameters (used for plugin)*/
        void putalldata(const char *data);

        //Midi IN
        void noteOn(char chan, char note, char velocity);
        void noteOff(char chan, char note);
        void polyphonicAftertouch(char chan, char note, char velocity);
        void setController(char chan, int type, int par);
        //void NRPN...


        void ShutUp();
        int shutup;

        void vuUpdate(const float *outl, const float *outr);

        //Process a set of OSC events in the bToU buffer
        bool runOSC(float *outl, float *outr, bool offline=false);

        /**Audio Output*/
        bool AudioOut(float *outl, float *outr) REALTIME;
        /**Audio Output (for callback mode).
         * This allows the program to be controled by an external program*/
        void GetAudioOutSamples(size_t nsamples,
                                unsigned samplerate,
                                float *outl,
                                float *outr) REALTIME;


        void partonoff(int npart, int what);

        //Set callback to run when master changes
        void setMasterChangedCallback(void(*cb)(void*,Master*),void *ptr);
        //Copy callback to other master
        void copyMasterCbTo(Master* dest);
        bool hasMasterCb() const;

        /**parts \todo see if this can be made to be dynamic*/
        class Part * part[NUM_MIDI_PARTS];

        //parameters

        unsigned char Pvolume;
        unsigned char Pkeyshift;
        unsigned char Psysefxvol[NUM_SYS_EFX][NUM_MIDI_PARTS];
        unsigned char Psysefxsend[NUM_SYS_EFX][NUM_SYS_EFX];

        //parameters control
        void setPvolume(char Pvolume_);
        void setPkeyshift(char Pkeyshift_);
        void setPsysefxvol(int Ppart, int Pefx, char Pvol);
        void setPsysefxsend(int Pefxfrom, int Pefxto, char Pvol);

        //effects
        class EffectMgr * sysefx[NUM_SYS_EFX]; //system
        class EffectMgr * insefx[NUM_INS_EFX]; //insertion
//      void swapcopyeffects(int what,int type,int neff1,int neff2);

        //HDD recorder
        Recorder HDDRecorder;

        //part that's apply the insertion effect; -1 to disable
        short int Pinsparts[NUM_INS_EFX];


        //peaks for VU-meter
        void vuresetpeaks();

        //peaks for part VU-meters
        float vuoutpeakpart[NUM_MIDI_PARTS];
        unsigned char fakepeakpart[NUM_MIDI_PARTS]; //this is used to compute the "peak" when the part is disabled

        AbsTime  time;
        Controller ctl;
        bool       swaplr; //if L and R are swapped

        //other objects
        Microtonal microtonal;

        //Strictly Non-RT instrument bank object
        Bank bank;

        class FFTwrapper * fft;

        static const rtosc::Ports &ports;
        float  volume;

        //Statistics on output levels
        vuData vu;

        //Display info on midi notes
        bool activeNotes[128];

        //Other watchers
        WatchManager watcher;

        //Midi Learn
        rtosc::AutomationMgr automate;

        bool   frozenState;//read-only parameters for threadsafe actions
        Allocator *memory;
        rtosc::ThreadLink *bToU;
        rtosc::ThreadLink *uToB;
        bool pendingMemory;
        const SYNTH_T &synth;
        const int& gzip_compression; //!< value from config

        //Heartbeat for identifying plugin offline modes
        //in units of 10 ms (done s.t. overflow is in 497 days)
        uint32_t last_beat = 0;
        uint32_t last_ack = 0;

        //Buffer to contain the OSC path to the last GUI element
        //on which a drag and drop operation ended
        constexpr static std::size_t dnd_buffer_size = 1024;
        char dnd_buffer[dnd_buffer_size] = {0};

    private:
        float  sysefxvol[NUM_SYS_EFX][NUM_MIDI_PARTS];
        float  sysefxsend[NUM_SYS_EFX][NUM_SYS_EFX];
        int    keyshift;

        //information relevent to generating plugin audio samples
        float *bufl;
        float *bufr;
        off_t  off;
        size_t smps;

        //Callback When Master changes
        void(*mastercb)(void*,Master*);
        void* mastercb_ptr;

        //Return XML data as string. Must be freed.
        char* getXMLData();
        //Used by loadOSC and saveOSC
        int loadOSCFromStr(const char *file_content,
                           rtosc::savefile_dispatcher_t* dispatcher);
        //applyOscEvent with a DataObj parameter
        bool applyOscEvent(const char *event, float *outl, float *outr,
                           bool offline, bool nio,
                           class DataObj& d, int msg_id = -1);
};

class master_dispatcher_t : public rtosc::savefile_dispatcher_t
{
    virtual void vUpdateMaster(Master* m) = 0;
public:
    void updateMaster(Master* m) { vUpdateMaster(m); }
};

}

#endif

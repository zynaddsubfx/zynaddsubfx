/*
  ZynAddSubFX - a software synthesizer

  main.cpp  -  Main file of the synthesizer
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

#include <iostream>
#include <cmath>
#include <cctype>
#include <algorithm>

#include <unistd.h>
#include <pthread.h>

#if OS_LINUX || OS_CYGWIN
#include <getopt.h>
#elif OS_WINDOWS
#include <winbase.h>
#include <windows.h>
#endif

#include "Misc/Master.h"
#include "Misc/Util.h"
#include "Misc/Dump.h"
extern Dump dump;

//Nio System
#include "Nio/MidiIn.h"
#include "Nio/AudioOut.h"
#include "Nio/OutMgr.h"
#include "Nio/InMgr.h"
#include "Nio/EngineMgr.h"

#ifndef DISABLE_GUI
#ifdef QT_GUI

#include <QApplication>
#include "masterui.h"
QApplication *app;

#elif defined FLTK_GUI

#include "UI/MasterUI.h"
#endif // FLTK_GUI

MasterUI *ui;

#endif //DISABLE_GUI

using namespace std;

pthread_t thr3, thr4;
Master   *master;
int  swaplr    = 0; //1 for left-right swapping

#ifdef USE_LASH
#include "Misc/LASHClient.h"
LASHClient *lash;
#endif

int     Pexitprogram = 0; //if the UI set this to 1, the program will exit

/*
 * User Interface thread
 */
void *thread3(void *)
{
#ifndef DISABLE_GUI

#ifdef FLTK_GUI

    ui = new MasterUI(master, &Pexitprogram);
    ui->showUI();

    while(Pexitprogram == 0) {
#ifdef USE_LASH
        string filename;
        switch(lash->checkevents(filename)) {
        case LASHClient::Save:
            ui->do_save_master(filename.c_str());
            lash->confirmevent(LASHClient::Save);
            break;
        case LASHClient::Restore:
            ui->do_load_master(filename.c_str());
            lash->confirmevent(LASHClient::Restore);
            break;
        case LASHClient::Quit:
            Pexitprogram = 1;
        default:
            break;
        }
#endif //USE_LASH
        Fl::wait();
    }

#elif defined QT_GUI
    app = new QApplication(0, 0);
    ui  = new MasterUI(master, 0);
    ui->show();
    app->exec();
#endif //defined QT_GUI

#endif //DISABLE_GUI
    return 0;
}

//this code is disabled for Nio testing
//it should get moved out of here into the nio system soon
#if 0
/*
 * Sequencer thread (test)
 */
void *thread4(void *arg)
{
    while(Pexitprogram == 0) {
        int type, par1, par2, again, midichan;
        for(int ntrack = 0; ntrack < NUM_MIDI_TRACKS; ntrack++) {
            if(master->seq.play == 0)
                break;
            do {
                again = master->seq.getevent(ntrack,
                                             &midichan,
                                             &type,
                                             &par1,
                                             &par2);
//		printf("ntrack=%d again=%d\n",ntrack,again);
                if(type > 0) {
//	    printf("%d %d  %d %d %d\n",type,midichan,chan,par1,par2);

//	if (cmdtype==MidiController) master->SetController(cmdchan,cmdparams[0],cmdparams[1]);



                    pthread_mutex_lock(&master->mutex);
                    if(type == 1) { //note_on or note_off
                        if(par2 != 0)
                            master->NoteOn(midichan, par1, par2);
                        else
                            master->NoteOff(midichan, par1);
                    }
                    pthread_mutex_unlock(&master->mutex);
                }
            } while(again > 0);
        }
//if (!realtime player) atunci fac asta
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		os_sleep(1000);
    }

    return 0;
}
#endif

/*
 * Program initialisation
 */
void initprogram()
{
    cerr.precision(1);
    cerr << std::fixed;
    cerr << "\nSample Rate = \t\t" << SAMPLE_RATE << endl;
    cerr << "Sound Buffer Size = \t" << SOUND_BUFFER_SIZE << " samples" << endl;
    cerr << "Internal latency = \t" << SOUND_BUFFER_SIZE * 1000.0
    / SAMPLE_RATE << " ms" << endl;
    cerr << "ADsynth Oscil.Size = \t" << OSCIL_SIZE << " samples" << endl;

    srand(time(NULL));
    denormalkillbuf = new REALTYPE [SOUND_BUFFER_SIZE];
    for(int i = 0; i < SOUND_BUFFER_SIZE; i++)
        denormalkillbuf[i] = (RND - 0.5) * 1e-16;

    master = new Master();
    master->swaplr = swaplr;

    //Nio Initialization

    //Enable input wrapper
    sysIn     = new InMgr(master);

    //Initialize the Output Systems
    sysOut    = new OutMgr(master);

    //Initialize The Engines
    sysEngine = new EngineMgr();
}

/*
 * Program exit
 */
void exitprogram()
{
    pthread_mutex_lock(&master->mutex);
    pthread_mutex_unlock(&master->mutex);
    sysEngine->stop();
    delete sysOut;
    delete sysIn;
    delete sysEngine;

#ifndef DISABLE_GUI
    delete ui;
#endif
    delete master;

#ifdef USE_LASH
    delete lash;
#endif

    delete [] denormalkillbuf;
}

#if OS_WINDOWS
#define ARGSIZE 100
char winoptarguments[ARGSIZE];
char getopt(int argc, char *argv[], const char *shortopts, int *index)
{
    winoptarguments[0] = 0;
    char result = 0;

    if(*index >= argc)
        return -1;

    if(strlen(argv[*index]) == 2)
        if(argv[*index][0] == '-') {
            result = argv[*index][1];
            if(*index + 1 < argc)
                snprintf(winoptarguments, ARGSIZE, "%s", argv[*index + 1]);
        }
    (*index)++;
    return result;
}
int opterr = 0;
#undef ARGSIZE

#endif

#ifndef VSTAUDIOOUT
int main(int argc, char *argv[])
{
#ifdef USE_LASH
    lash = new LASHClient(&argc, &argv);
#endif

    config.init();
    dump.startnow();
    int noui = 0;
    cerr
    << "\nZynAddSubFX - Copyright (c) 2002-2009 Nasca Octavian Paul and others"
    << endl;
    cerr << "Compiled: " << __DATE__ << " " << __TIME__ << endl;
    cerr << "This program is free software (GNU GPL v.2 or later) and \n";
    cerr << "it comes with ABSOLUTELY NO WARRANTY.\n" << endl;
	if(argc == 1)
#if OS_LINUX || OS_CYGWIN
    cerr << "Try 'zynaddsubfx --help' for command-line options." << endl;
#else //assuming windows
    cerr << "Try 'zynaddsubfx -h' for command-line options.\n" << endl;
#endif

    /* Get the settings from the Config*/
    SAMPLE_RATE = config.cfg.SampleRate;
    SOUND_BUFFER_SIZE = config.cfg.SoundBufferSize;
    OSCIL_SIZE  = config.cfg.OscilSize;
    swaplr      = config.cfg.SwapStereo;


    /* Parse command-line options */
#if OS_LINUX || OS_CYGWIN
    struct option opts[] = {
        {"load", 2, NULL, 'l'},
        {"load-instrument", 2, NULL, 'L'},
        {"sample-rate", 2, NULL, 'r'},
        {"buffer-size", 2, NULL, 'b'},
        {"oscil-size", 2, NULL, 'o'},
        {"dump", 2, NULL, 'D'},
        {"swap", 2, NULL, 'S'},
        {"no-gui", 2, NULL, 'U'},
        {"dummy", 2, NULL, 'Y'},
        {"help", 2, NULL, 'h'},
        {"output", 1, NULL, 'O'},
        {"input", 1, NULL, 'I'},
        {0, 0, 0, 0}
    };
#endif
    opterr = 0;
    int option_index = 0, opt, exitwithhelp = 0;

    string loadfile, loadinstrument, input, output;

    while(1) {
        /**\todo check this process for a small memory leak*/
#if OS_LINUX || OS_CYGWIN
        opt = getopt_long(argc, argv, "l:L:r:b:o:I:O:hSDUY", opts, &option_index);
        char *optarguments = optarg;
#elif OS_WINDOWS
        opt = getopt(argc, argv, "l:L:r:b:o:I:O:hSDUY", &option_index);
        char *optarguments = &winoptarguments[0];
#else
#error Undefined OS
#endif

#define GETOP(x) if(optarguments) x = optarguments
#define GETOPNUM(x) if(optarguments) x = atoi(optarguments)


        if(opt == -1)
            break;

        switch(opt) {
        case 'h':
            exitwithhelp = 1;
            break;
        case 'Y':/* this command a dummy command (has NO effect)
                and is used because I need for NSIS installer
            (NSIS sometimes forces a command line for a
            program, even if I don't need that; eg. when
            I want to add a icon to a shortcut.
              */
            break;
        case 'U':
            noui = 1;
            break;
        case 'l':
            GETOP(loadfile);
            break;
        case 'L':
            GETOP(loadinstrument);
            break;
        case 'r':
            GETOPNUM(SAMPLE_RATE);
            if(SAMPLE_RATE <  4000) {
                cerr << "ERROR:Incorrect sample rate: " << optarguments << endl;
                exit(1);
            }
            break;
        case 'b':
            GETOPNUM(SOUND_BUFFER_SIZE);
            if(SOUND_BUFFER_SIZE < 2) {
                cerr << "ERROR:Incorrect buffer size: " << optarguments << endl;
                exit(1);
            }
            break;
        case 'o':
            int tmp;
            if(optarguments)
                OSCIL_SIZE = tmp = atoi(optarguments);
            if(OSCIL_SIZE < MAX_AD_HARMONICS * 2)
                OSCIL_SIZE = MAX_AD_HARMONICS * 2;
            OSCIL_SIZE = (int) pow(2, ceil(log(OSCIL_SIZE - 1.0) / log(2.0)));
            if(tmp != OSCIL_SIZE) {
                cerr << "OSCIL_SIZE is wrong (must be 2^n) or too small. Adjusting to "
                     <<  OSCIL_SIZE << "." << endl;
            }
            break;
        case 'S':
            swaplr = 1;
            break;
        case 'D':
            dump.startnow();
            break;
        case 'I':
            GETOP(input);
            break;
        case 'O':
            GETOP(output);
            break;
        case '?':
            cerr << "ERROR:Bad option or parameter.\n" << endl;
            exitwithhelp = 1;
            break;
        }
    }

    if(exitwithhelp != 0) {
        cout << "Usage: zynaddsubfx [OPTION]\n\n"
             << "  -h , --help \t\t\t\t Display command-line help and exit\n"
             << "  -l file, --load=FILE\t\t\t Loads a .xmz file\n"
             << "  -L file, --load-instrument=FILE\t Loads a .xiz file\n"
             << "  -r SR, --sample-rate=SR\t\t Set the sample rate SR\n"
             << "  -b BS, --buffer-size=SR\t\t Set the buffer size (granularity)\n"
             << "  -o OS, --oscil-size=OS\t\t Set the ADsynth oscil. size\n"
             << "  -S , --swap\t\t\t\t Swap Left <--> Right\n"
             << "  -D , --dump\t\t\t\t Dumps midi note ON/OFF commands\n"
             << "  -U , --no-gui\t\t\t\t Run ZynAddSubFX without user interface\n"
             << "  -O , --output\t\t\t\t Set Output Engine\n"
             << "  -I , --input\t\t\t\t Set Input Engine" << endl;

#if OS_WINDOWS
        cout << "nWARNING: On Windows systems, only short comandline parameters works.\n"
             << "  eg. instead '--buffer-size=512' use '-b 512'" << endl;
#endif
        return 0;
    }

    initprogram();

#if 0 //TODO update this code
#ifdef USE_LASH
#ifdef ALSAMIDIIN
    ALSAMidiIn *alsamidi = dynamic_cast<ALSAMidiIn *>(Midi);
    if(alsamidi)
        lash->setalsaid(alsamidi->getalsaid());
#endif
#ifdef JACKAUDIOOUT
    lash->setjackname(JACKgetname());
#endif
#endif
#endif


    if(!loadfile.empty()) {
        int tmp = master->loadXML(loadfile.c_str());
        if(tmp < 0) {
            cerr << "ERROR: Could not load master file " << loadfile
                 << "." << endl;
            exit(1);
        }
        else {
            master->applyparameters();
            cout << "Master file loaded." << endl;
        }
    }

    if(!loadinstrument.empty()) {
        int loadtopart = 0;
        int tmp = master->part[loadtopart]->loadXMLinstrument(loadinstrument.c_str());
        if(tmp < 0) {
            cerr << "ERROR: Could not load instrument file "
                 << loadinstrument << '.' << endl;
            exit(1);
        }
        else {
            master->part[loadtopart]->applyparameters();
            cout << "Instrument file loaded." << endl;
        }
    }


    if(!input.empty()) {
        if(!sysEngine->setInDefault(input)) {
            cerr << "There is no input for " << input << endl;
            exit(1);
        }
        cout << input << " selected." << endl;
    }
    if(!output.empty()) {
        if(!sysEngine->setOutDefault(output)) {
            cerr << "There is no output for " << output << endl;
            exit(1);
        }
        cout << output << " selected." << endl;
    }

    //Run the Nio system
    sysEngine->start(); //Drivers start your engines!

#warning remove welcome message when system is out of beta
    cout << "\nThanks for using the Nio system :)" << endl;

#ifndef DISABLE_GUI
    if(noui == 0) {
        ui = new MasterUI(master, &Pexitprogram);
        pthread_create(&thr3, NULL, thread3, NULL);
    }
#endif

//    pthread_create(&thr4, NULL, thread4, NULL);
#ifdef WINMIDIIN
    InitWinMidi(master);
#endif

    //TODO look into a conditional variable here, it seems to match usage
    while(Pexitprogram == 0) {
#ifdef OS_LINUX
        usleep(100000);
#elif OS_WINDOWS
        Sleep(100);
#endif
    }

#ifdef WINMIDIIN
    StopWinMidi();
#endif

    exitprogram();
    return 0;
}


#else

#include "Output/VSTaudiooutput.h"

#define main main_plugin
extern "C" __declspec(dllexport) AEffect * main_plugin(
    audioMasterCallback audioMaster);

int instances = -1;

AEffect *main(audioMasterCallback audioMaster)
{
//    if (audioMaster(0,audioMasterVersion,0,0,0,0)!=0) {
//	return(0);
//    };

    if(instances == -1) {
        Midi = new NULLMidiIn();
        denormalkillbuf = new REALTYPE [SOUND_BUFFER_SIZE];
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++)
            denormalkillbuf[i] = (RND - 0.5) * 1e-16;
        instances = 0;
    }

    if(instances != 0)
        return 0;               //don't allow multiple instances

    AudioEffect *sintetizator = new VSTSynth(audioMaster);

    return sintetizator->getAeffect();
}

void *hInstance;
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
    hInstance = hInst;
    return 1;
}

void *thread(void *arg)
{
    VSTSynth *vs = (VSTSynth *) arg;

    /*    FILE *a=fopen("aaaa1","a");
        fprintf(a,"%lx %lx %lx -i=%d\n",vs,0,vs->vmaster,instances);
        fflush(a);fclose(a);
    */

    vs->ui = new MasterUI(vs->vmaster, &vs->Pexitprogram);

    /*    a=fopen("aaaa1","a");
        fprintf(a,"%lx %lx %lx\n",vs,vs->ui->master,vs->vmaster);
        fflush(a);fclose(a);
    */

    vs->ui->showUI();

    /*    a=fopen("aaaa1","a");
        fprintf(a,"%lx %lx %lx\n",vs,vs->ui,vs->vmaster);
        fflush(a);fclose(a);
    */

    while(vs->Pexitprogram == 0)
        Fl::wait(0.01);

    delete (vs->ui);
    Fl::wait(0.01);

    /*    a=fopen("aaaa1","a");
        fprintf(a,"EXIT\n");
        fflush(a);fclose(a);
    */


    pthread_exit(0);
    return 0;
}

//Parts of the VSTSynth class
VSTSynth::VSTSynth(audioMasterCallback audioMaster):AudioEffectX(audioMaster, 1,
                                                                 0)
{
    instances++;

    if(audioMaster) {
        setNumInputs(0);
        setNumOutputs(2);
        setUniqueID('ZASF');
        canProcessReplacing();
//    hasVu(false);
//    hasClip(false);

        isSynth(true);

        programsAreChunks(true);
    }


    SAMPLE_RATE = config.cfg.SampleRate;
    SOUND_BUFFER_SIZE = config.cfg.SoundBufferSize;
    OSCIL_SIZE  = config.cfg.OscilSize;
    swaplr      = config.cfg.SwapStereo;
    this->Pexitprogram    = 0;

    this->vmaster         = new Master();
    this->vmaster->swaplr = swaplr;


//    FILE *a=fopen("aaaa0","a");
//    fprintf(a,"%lx %lx %lx\n",this,this->ui,this->ui->masterwindow);
//    fflush(a);fclose(a);

    pthread_create(&this->thr, NULL, thread, this);

//    suspend();
}



VSTSynth::~VSTSynth()
{
    this->Pexitprogram = 1;

    Sleep(200); //wait the thread to finish

//    pthread_mutex_lock(&vmaster->mutex);


    delete (this->vmaster);

    instances--;
}

long VSTSynth::processEvents(VstEvents *events)
{
    for(int i = 0; i < events->numEvents; i++) {
        //debug stuff
//      FILE *a=fopen("events","a");
//      fprintf(a,"%lx\n",events->events[i]->type);
//      fflush(a);fclose(a);

        if((events->events[i])->type != kVstMidiType)
            continue;
        VstMidiEvent  *ev   = (VstMidiEvent *) events->events[i];
        unsigned char *data = (unsigned char *)ev->midiData;
        int status  = data[0] / 16;
        int cmdchan = data[0] & 0x0f;
        int cntl;

        pthread_mutex_lock(&vmaster->mutex);
        switch(status) {
        case 0x8:
            vmaster->NoteOff(cmdchan, data[1] & 0x7f);
            break;
        case 0x9:
            if(data[2] == 0)
                vmaster->NoteOff(cmdchan, data[1] & 0x7f);
            else
                vmaster->NoteOn(cmdchan, data[1] & 0x7f, data[2] & 0x7f);
            break;
        case 0xB:
            cntl = Midi->getcontroller(data[1] & 0x7f);
            vmaster->SetController(cmdchan, cntl, data[2] & 0x7f);
            break;
        case 0xE:
            vmaster->SetController(cmdchan, C_pitchwheel, data[1] + data[2]
                                   * (long int) 128 - 8192);
            break;
        }
        pthread_mutex_unlock(&vmaster->mutex);
    }

    return 1;
}

long VSTSynth::getChunk(void **data, bool isPreset)
{
    int size = 0;
    size = vmaster->getalldata((char **)data);
    return (long)size;
}

long VSTSynth::setChunk(void *data, long size, bool isPreset)
{
    vmaster->putalldata((char *)data, size);
    return 0;
}
#endif


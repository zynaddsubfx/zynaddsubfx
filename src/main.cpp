/*
  ZynAddSubFX - a software synthesizer

  main.cpp  -  Main file of the synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2012-2019 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/


#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#include <cctype>
#include <ctime>
#include <algorithm>
#include <signal.h>

#ifndef WIN32
#include <err.h>
#endif
#include <unistd.h>

#include <getopt.h>

#include <sys/types.h>
#include <signal.h>

#include <rtosc/rtosc.h>
#include <rtosc/ports.h>
#include "Params/PADnoteParameters.h"

#include "DSP/FFTwrapper.h"
#include "Misc/MemLocker.h"
#include "Misc/PresetExtractor.h"
#include "Misc/Master.h"
#include "Misc/Part.h"
#include "Misc/Util.h"
#include "zyn-config.h"
#include "zyn-version.h"

//Nio System
#include "Nio/Nio.h"
#include "Nio/InMgr.h"

//GUI System
#include "UI/Connection.h"
GUI::ui_handle_t gui;

#ifdef ZEST_GUI
#ifndef WIN32
#include <sys/wait.h>
#endif
#endif

//Glue Layer
#include "Misc/MiddleWare.h"

using namespace std;
using namespace zyn;

MiddleWare *middleware;

Master   *master;
int       swaplr = 0; //1 for left-right swapping

// forward declarations of namespace zyn
namespace zyn
{
    extern int Pexitprogram;     //if the UI set this to 1, the program will exit
    void dump_json(std::ostream &o,
                   const rtosc::Ports &p);
}

#if LASH
#include "Misc/LASHClient.h"
LASHClient *lash = NULL;
#endif

#if USE_NSM
#include "UI/NSM.H"

NSM_Client *nsm = 0;
#endif

char *instance_name = 0;

void exitprogram(const Config &config);


//cleanup on signaled exit
void sigterm_exit(int /*sig*/)
{
    if(Pexitprogram)
        exit(1);
    Pexitprogram = 1;
}

/*
 * Program initialisation
 */
void initprogram(SYNTH_T synth, Config* config, int preferred_port)
{
    middleware = new MiddleWare(std::move(synth), config, preferred_port);
    master = middleware->spawnMaster();
    master->swaplr = swaplr;

    signal(SIGINT, sigterm_exit);
    signal(SIGTERM, sigterm_exit);
    Nio::init(master->synth, config->cfg.oss_devs, master);
}

/*
 * Program exit
 */
void exitprogram(const Config& config)
{
    Nio::stop();
    config.save();
    middleware->removeAutoSave();

    GUI::destroyUi(gui);
    delete middleware;
#if LASH
    if(lash)
        delete lash;
#endif
#if USE_NSM
    if(nsm)
        delete nsm;
#endif

    FFT_cleanup();
}

//Windows MIDI OH WHAT A HACK...
#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
namespace zyn{
extern InMgr  *in;
}
HMIDIIN winmidiinhandle = 0;

void CALLBACK WinMidiInProc(HMIDIIN hMidiIn,UINT wMsg,DWORD dwInstance,
                            DWORD dwParam1,DWORD dwParam2)
{
    int midicommand=0;
    if (wMsg==MIM_DATA) {
        int cmd,par1,par2;
        cmd=dwParam1&0xff;
        if (cmd==0xfe) return;
        par1=(dwParam1>>8)&0xff;
        par2=dwParam1>>16;
        int cmdchan=cmd&0x0f;
        int cmdtype=(cmd>>4)&0x0f;

        int tmp=0;
        MidiEvent ev;
        switch (cmdtype) {
            case(0x8)://noteon
                ev.type = 1;
                ev.num = par1;
                ev.channel = cmdchan;
                ev.value = 0;
                in->putEvent(ev);
                break;
            case(0x9)://noteoff
                ev.type = 1;
                ev.num = par1;
                ev.channel = cmdchan;
                ev.value = par2&0xff;
                in->putEvent(ev);
                break;
            case(0xb)://controller
                ev.type = 2;
                ev.num = par1;
                ev.channel = cmdchan;
                ev.value = par2&0xff;
                in->putEvent(ev);
                break;
            case(0xe)://pitch wheel
                //tmp=(par1+par2*(long int) 128)-8192;
                //winmaster->SetController(cmdchan,C_pitchwheel,tmp);
                break;
            default:
                break;
        };

    };
};

void InitWinMidi(int midi)
{
(void)midi;
    for(int i=0; i<10; ++i) {
        long int res=midiInOpen(&winmidiinhandle,i,(DWORD_PTR)(void*)WinMidiInProc,0,CALLBACK_FUNCTION);
        if(res == MMSYSERR_NOERROR) {
            res=midiInStart(winmidiinhandle);
            printf("[INFO] Starting Windows MIDI At %d with code %d(noerror=%d)\n", i, res, MMSYSERR_NOERROR);
            if(res == 0)
                return;
        } else
            printf("[INFO] No Windows MIDI Device At id %d\n", i);
    }
};

//void StopWinMidi()
//{
//    midiInStop(winmidiinhandle);
//    midiInClose(winmidiinhandle);
//};
#else
void InitWinMidi(int) {}
#endif


int main(int argc, char *argv[])
{
    SYNTH_T synth;
    Config config;
    int noui = 0;
    cerr
    << "\nZynAddSubFX - Copyright (c) 2002-2013 Nasca Octavian Paul and others"
    << endl;
    cerr
    << "                Copyright (c) 2009-2019 Mark McCurry [active maintainer]"
    << endl;
    cerr << "This program is free software (GNU GPL v2 or later) and \n";
    cerr << "it comes with ABSOLUTELY NO WARRANTY.\n" << endl;
    if(argc == 1)
        cerr << "Try 'zynaddsubfx --help' for command-line options." << endl;

    /* Get the settings from the Config*/
    synth.samplerate = config.cfg.SampleRate;
    synth.buffersize = config.cfg.SoundBufferSize;
    synth.oscilsize  = config.cfg.OscilSize;
    swaplr = config.cfg.SwapStereo;

    Nio::preferredSampleRate(synth.samplerate);

    synth.alias(); //build aliases

    sprng(time(NULL));

    // for option entries with the 3rd member (flag) pointing here,
    // getopt_long*() will return 0 and set this flag to the 4th member (val)
    int getopt_flag;

    /* Parse command-line options */
    struct option opts[] = {
        // options with single char equivalents
        {
            "load", 2, NULL, 'l'
        },
        {
            "load-instrument", 2, NULL, 'L'
        },
        {
            "midi-learn", 2, NULL, 'M'
        },
        {
            "sample-rate", 2, NULL, 'r'
        },
        {
            "buffer-size", 2, NULL, 'b'
        },
        {
            "oscil-size", 2, NULL, 'o'
        },
        {
            "swap", 2, NULL, 'S'
        },
        {
            "no-gui", 0, NULL, 'U'
        },
        {
            "dummy", 2, NULL, 'Y'
        },
        {
            "help", 2, NULL, 'h'
        },
        {
            "version", 2, NULL, 'v'
        },
        {
            "named", 1, NULL, 'N'
        },
        {
            "auto-connect", 0, NULL, 'a'
        },
        {
            "auto-save", 0, NULL, 'A'
        },
        {
            "pid-in-client-name", 0, NULL, 'p'
        },
        {
            "preferred-port", 1, NULL, 'P',
        },
        {
            "output", 1, NULL, 'O'
        },
        {
            "input", 1, NULL, 'I'
        },
        {
            "exec-after-init", 1, NULL, 'e'
        },
        {
            "dump-oscdoc", 2, NULL, 'd'
        },
        {
            "dump-json-schema", 2, NULL, 'D'
        },
        // options without single char equivalents ("getopt_flag" compulsory)
        {
            "list-inputs", no_argument, &getopt_flag, 'i'
        },
        {
            "list-outputs", no_argument, &getopt_flag, 'o'
        },
        {
            0, 0, 0, 0
        }
    };
    opterr = 0;
    int option_index = 0, opt;
    enum class exit_with_t
    {
        dont_exit,
        help,
        version,
        list_inputs,
        list_outputs
    };
    exit_with_t exit_with = exit_with_t::dont_exit;
    int preferred_port = -1;
    int auto_save_interval = 0;
    int wmidi = -1;

    string loadfile, loadinstrument, execAfterInit, loadmidilearn;

    while(1) {
        int tmp = 0;

        /**\todo check this process for a small memory leak*/
        opt = getopt_long(argc,
                          argv,
                          "l:L:M:r:b:o:I:O:N:e:P:A:d:D:hvapSDUYZ",
                          opts,
                          &option_index);
        char *optarguments = optarg;

#define GETOP(x) if(optarguments) \
        x = optarguments
#define GETOPNUM(x) if(optarguments) \
        x = atoi(optarguments)


        if(opt == -1)
            break;

        switch(opt) {
            case 'h':
                exit_with = exit_with_t::help;
                break;
            case 'v':
                exit_with = exit_with_t::version;
                break;
            case 'Y': /* this command a dummy command (has NO effect)
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
            case 'M':
                GETOP(loadmidilearn);
                break;
            case 'r':
                GETOPNUM(synth.samplerate);
                if(synth.samplerate < 4000) {
                    cerr << "ERROR:Incorrect sample rate: " << optarguments
                         << endl;
                    exit(1);
                }
                break;
            case 'b':
                GETOPNUM(synth.buffersize);
                if(synth.buffersize < 2) {
                    cerr << "ERROR:Incorrect buffer size: " << optarguments
                         << endl;
                    exit(1);
                }
                break;
            case 'o':
                if(optarguments)
                    synth.oscilsize = tmp = atoi(optarguments);
                if(synth.oscilsize < MAX_AD_HARMONICS * 2)
                    synth.oscilsize = MAX_AD_HARMONICS * 2;
                synth.oscilsize =
                    (int) powf(2,
                               ceil(logf(synth.oscilsize - 1.0f) / logf(2.0f)));
                if(tmp != synth.oscilsize)
                    cerr
                    <<
                    "synth.oscilsize is wrong (must be 2^n) or too small. Adjusting to "
                    << synth.oscilsize << "." << endl;
                break;
            case 'S':
                swaplr = 1;
                break;
            case 'N':
                Nio::setPostfix(optarguments);
                break;
            case 'I':
                if(optarguments)
                    Nio::setDefaultSource(optarguments);
                break;
            case 'O':
                if(optarguments)
                    Nio::setDefaultSink(optarguments);
                break;
            case 'a':
                Nio::autoConnect = true;
                break;
            case 'p':
                Nio::pidInClientName = true;
                break;
            case 'P':
                if(optarguments)
                    preferred_port = atoi(optarguments);
                break;
            case 'A':
                if(optarguments)
                    auto_save_interval = atoi(optarguments);
                break;
            case 'e':
                GETOP(execAfterInit);
                break;
            case 'd':
                if(optarguments)
                {
                    rtosc::OscDocFormatter s;
                    ofstream outfile(optarguments);
                    s.prog_name    = "ZynAddSubFX";
                    s.p            = &MiddleWare::getAllPorts();
                    s.uri          = "http://example.com/fake/";
                    s.doc_origin   = "http://example.com/fake/url.xml";
                    s.author_first = "Mark";
                    s.author_last  = "McCurry";
                    outfile << s;
                }
                break;
            case 'D':
                if(optarguments)
                {
                    ofstream outfile(optarguments);
                    dump_json(outfile, MiddleWare::getAllPorts());
                }
                break;
            case 'Z':
                if(optarguments)
                    wmidi = atoi(optarguments);
                break;
            case 0: // catch options without single char equivalent
                switch(getopt_flag)
                {
                    case 'i':
                        exit_with = exit_with_t::list_inputs;
                        break;
                    case 'o':
                        exit_with = exit_with_t::list_outputs;
                        break;
                }
                break;
            case '?':
                cerr << "ERROR:Bad option or parameter.\n" << endl;
                exit_with = exit_with_t::help;
                break;
        }
    }

    synth.alias();

    switch (exit_with)
    {
        case exit_with_t::version:
            cout << "Version: " << version << endl;
            break;
        case exit_with_t::help:
            cout << "Usage: zynaddsubfx [OPTION]\n\n"
                 << "  -h , --help \t\t\t\t Display command-line help and exit\n"
                 << "  -v , --version \t\t\t Display version and exit\n"
                 << "  -l file, --load=FILE\t\t\t Loads a .xmz file\n"
                 << "  -L file, --load-instrument=FILE\t Loads a .xiz file\n"
                 << "  -M file, --midi-learn=FILE\t\t Loads a .xlz file\n"
                 << "  -r SR, --sample-rate=SR\t\t Set the sample rate SR\n"
                 <<
            "  -b BS, --buffer-size=SR\t\t Set the buffer size (granularity)\n"
                 << "  -o OS, --oscil-size=OS\t\t Set the ADsynth oscil. size\n"
                 << "  -S , --swap\t\t\t\t Swap Left <--> Right\n"
                 <<
            "  -U , --no-gui\t\t\t\t Run ZynAddSubFX without user interface\n"
                 << "  -N , --named\t\t\t\t Postfix IO Name when possible\n"
                 << "  -a , --auto-connect\t\t\t AutoConnect when using JACK\n"
                 << "  -A , --auto-save=INTERVAL\t\t Automatically save at interval\n"
                 << "\t\t\t\t\t (disabled with 0 interval)\n"
                 << "  -p , --pid-in-client-name\t\t Append PID to (JACK) "
                    "client name\n"
                 << "  -P , --preferred-port\t\t\t Preferred OSC Port\n"
                 << "  -O , --output\t\t\t\t Set Output Engine\n"
                 << "  -I , --input\t\t\t\t Set Input Engine\n"
                 << "  -e , --exec-after-init\t\t Run post-initialization script\n"
                 << "  -d , --dump-oscdoc=FILE\t\t Dump oscdoc xml to file\n"
                 << "  -D , --dump-json-schema=FILE\t\t Dump osc schema (.json) to file\n"
                 << endl;
            break;
        case exit_with_t::list_inputs:
        case exit_with_t::list_outputs:
        {
            Nio::init(synth, config.cfg.oss_devs, nullptr);
            auto get_func = (getopt_flag == 'i')
                    ? &Nio::getSources
                    : &Nio::getSinks;
            std::set<std::string> engines = (*get_func)();
            for(std::string engine : engines)
            {
                std::transform(engine.begin(), engine.end(), engine.begin(),
                               ::tolower);
                cout << engine << endl;
            }
            break;
        }
        default:
            break;
    }
    if(exit_with != exit_with_t::dont_exit)
        return 0;

    cerr.precision(1);
    cerr << std::fixed;
    cerr << "\nSample Rate = \t\t" << synth.samplerate << endl;
    cerr << "Sound Buffer Size = \t" << synth.buffersize << " samples" << endl;
    cerr << "Internal latency = \t" << synth.dt() * 1000.0f << " ms" << endl;
    cerr << "ADsynth Oscil.Size = \t" << synth.oscilsize << " samples" << endl;

    initprogram(std::move(synth), &config, preferred_port);

    bool altered_master = false;
    if(!loadfile.empty()) {
        altered_master = true;
        const char *filename = loadfile.c_str();
        int tmp = master->loadXML(filename);
        if(tmp < 0) {
            cerr << "ERROR: Could not load master file " << loadfile
                 << "." << endl;
            exit(1);
        }
        else {
            fast_strcpy(master->last_xmz, filename, XMZ_PATH_MAX);
            master->last_xmz[XMZ_PATH_MAX-1] = 0;
            master->applyparameters();
            cout << "Master file loaded." << endl;
        }
    }

    if(!loadinstrument.empty()) {
        altered_master = true;
        int loadtopart = 0;
        int tmp = master->part[loadtopart]->loadXMLinstrument(
            loadinstrument.c_str());
        if(tmp < 0) {
            cerr << "ERROR: Could not load instrument file "
                 << loadinstrument << '.' << endl;
            exit(1);
        }
        else {
            master->part[loadtopart]->applyparameters();
            master->part[loadtopart]->initialize_rt();
            cout << "Instrument file loaded." << endl;
        }
    }

    if(!loadmidilearn.empty()) {
        char msg[1024];
        rtosc_message(msg, sizeof(msg), "/load_xlz",
                "s", loadmidilearn.c_str());
        middleware->transmitMsg(msg);
    }

    if(altered_master)
        middleware->updateResources(master);

    //Run the Nio system
    printf("[INFO] Nio::start()\n");
    bool ioGood = Nio::start();

    printf("[INFO] exec-after-init\n");
    if(!execAfterInit.empty()) {
        cout << "Executing user supplied command: " << execAfterInit << endl;
        if(system(execAfterInit.c_str()) == -1)
            cerr << "Command Failed..." << endl;
    }

    InitWinMidi(wmidi);


    gui = NULL;

    //Capture Startup Responses
    printf("[INFO] startup OSC\n");
    typedef std::vector<const char *> wait_t;
    wait_t msg_waitlist;
    middleware->setUiCallback([](void*v,const char*msg) {
            wait_t &wait = *(wait_t*)v;
            size_t len = rtosc_message_length(msg, -1);
            char *copy = new char[len];
            memcpy(copy, msg, len);
            wait.push_back(copy);
            }, &msg_waitlist);

    printf("[INFO] UI calbacks\n");
    if(!noui)
        gui = GUI::createUi(middleware->spawnUiApi(), &Pexitprogram);
    middleware->setUiCallback(GUI::raiseUi, gui);
    middleware->setIdleCallback([](void*){GUI::tickUi(gui);}, NULL);

    //Replay Startup Responses
    printf("[INFO] OSC replay\n");
    for(auto msg:msg_waitlist) {
        GUI::raiseUi(gui, msg);
        delete [] msg;
    }

    if(!noui)
    {
        GUI::raiseUi(gui, "/show",  "i", config.cfg.UserInterfaceMode);
        if(!ioGood)
            GUI::raiseUi(gui, "/alert", "s",
                    "Default IO did not initialize.\nDefaulting to NULL backend.");
    }

    printf("[INFO] auto_save setup\n");
    if(auto_save_interval > 0) {
        int old_save = middleware->checkAutoSave();
        if(old_save > 0)
            GUI::raiseUi(gui, "/alert-reload", "i", old_save);
        middleware->enableAutoSave(auto_save_interval);
    }

    //TODO move this stuff into Cmake
#if USE_NSM && defined(WIN32)
#undef USE_NSM
#define USE_NSM 0
#endif

#if LASH && defined(WIN32)
#undef LASH
#define LASH 0
#endif

#if USE_NSM
    printf("[INFO] NSM Stuff\n");
    char *nsm_url = getenv("NSM_URL");

    if(nsm_url) {
        nsm = new NSM_Client(middleware);

        if(!nsm->init(nsm_url))
            nsm->announce("ZynAddSubFX", ":switch:", argv[0]);
        else {
            delete nsm;
            nsm = NULL;
        }
    }
#endif

#if USE_NSM
    printf("[INFO] LASH Stuff\n");
    if(!nsm)
#endif
    {
#if LASH
        lash = new LASHClient(&argc, &argv);
        GUI::raiseUi(gui, "/session-type", "s", "LASH");
#endif
    }

#ifdef ZEST_GUI
#ifndef WIN32
    pid_t gui_pid = 0;
#endif
    if(!noui) {
        printf("[INFO] Launching Zyn-Fusion...\n");
        const char *addr = middleware->getServerAddress();
#ifndef WIN32
        gui_pid = fork();
        if(gui_pid == 0) {
            auto exec_fusion = [&addr](const char* path) {
                execlp(path, "zyn-fusion", addr, "--builtin", "--no-hotload",  0); };
            if(fusion_dir && *fusion_dir)
            {
                std::string fusion = fusion_dir;
                fusion += "/zest";
                if(access(fusion.c_str(), X_OK))
                    fputs("Warning: CMake's ZynFusionDir does not contain a"
                          "\"zest\" binary - ignoring.", stderr);
                else {
                    const char* cur = getenv("LD_LIBRARY_PATH");
                    std::string ld_library_path;
                    if(cur) {
                        ld_library_path += cur;
                        ld_library_path += ":";
                    }
                    ld_library_path += fusion_dir;
                    setenv("LD_LIBRARY_PATH", ld_library_path.c_str(), 1);
                    exec_fusion(fusion.c_str());
                }
            }
            exec_fusion("./zyn-fusion");
            exec_fusion("/opt/zyn-fusion/zyn-fusion");
            exec_fusion("zyn-fusion");
            err(1,"Failed to launch Zyn-Fusion");
        }
#else
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        memset(&pi, 0, sizeof(pi));
        char *why_windows = strrchr(addr, ':');
        char *seriously_why = why_windows + 1;
        char start_line[256] = {};
        if(why_windows)
            snprintf(start_line, sizeof(start_line), "zyn-fusion.exe osc.udp://127.0.0.1:%s", seriously_why);
        else {
            printf("COULD NOT PARSE <%s>\n", addr);
            exit(1);
        }
        printf("[INFO] starting subprocess via <%s>\n", start_line);
        if(!CreateProcess(NULL, start_line,
        NULL, NULL, 0, 0, NULL, NULL, &si, &pi)) {
            printf("Failed to launch Zyn-Fusion...\n");
            exit(1);
        }
#endif
    }
#endif

    MemLocker mem_locker;
    mem_locker.lock();

    printf("[INFO] Main Loop...\n");
    bool already_exited = false;
    while(Pexitprogram == 0) {
#ifndef WIN32
#if USE_NSM
        if(nsm) {
            nsm->check();
            goto done;
        }
#endif
#if LASH
        {
            string filename;
            switch(lash->checkevents(filename)) {
                case LASHClient::Save:
                    GUI::raiseUi(gui, "/save-master", "s", filename.c_str());
                    lash->confirmevent(LASHClient::Save);
                    break;
                case LASHClient::Restore:
                    GUI::raiseUi(gui, "/load-master", "s", filename.c_str());
                    lash->confirmevent(LASHClient::Restore);
                    break;
                case LASHClient::Quit:
                    Pexitprogram = 1;
                default:
                    break;
            }
        }
#endif //LASH

#if USE_NSM
done:
#endif
        GUI::tickUi(gui);
#endif // !WIN32
        middleware->tick();
#ifdef WIN32
        Sleep(1);
#endif

#ifdef ZEST_GUI
#ifndef WIN32
        if(!noui) {
            int status = 0;
            int ret = waitpid(gui_pid, &status, WNOHANG);
            if(ret == gui_pid) {
                Pexitprogram = 1;
                already_exited = true;
            }
        }
#endif
#endif
    } // while !Pexitprogram

    mem_locker.unlock();

#ifdef ZEST_GUI
#ifndef WIN32
    if(!already_exited) {
        int ret = kill(gui_pid, SIGHUP);
        if (ret == -1) {
            err(1, "Failed to terminate Zyn-Fusion...\n");
        }
    }
#else
    (void)already_exited;
#endif
#else
    (void)already_exited;
#endif
    exitprogram(config);
    return 0;
}

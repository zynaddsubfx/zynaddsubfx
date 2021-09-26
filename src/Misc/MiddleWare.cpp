/*
  ZynAddSubFX - a software synthesizer

  MiddleWare.cpp - Glue Logic And Home Of Non-RT Operations
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "MiddleWare.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <mutex>

#include <rtosc/undo-history.h>
#include <rtosc/thread-link.h>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <lo/lo.h>

#include <unistd.h>

#include "../UI/Connection.h"
#include "../UI/Fl_Osc_Interface.h"

#include <map>
#include <queue>

#include "Util.h"
#include "CallbackRepeater.h"
#include "Master.h"
#include "MsgParsing.h"
#include "Part.h"
#include "PresetExtractor.h"
#include "../Containers/MultiPseudoStack.h"
#include "../Params/PresetsStore.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/SUBnoteParameters.h"
#include "../Params/PADnoteParameters.h"
#include "../DSP/FFTwrapper.h"
#include "../Synth/OscilGen.h"
#include "../Nio/Nio.h"

#include <string>
#include <future>
#include <atomic>
#include <list>

#define errx(...) {}
#define warnx(...) {}
#ifndef errx
#include <err.h>
#endif

namespace zyn {

using std::string;
int Pexitprogram = 0;

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

/* work around missing clock_gettime on OSX */
static void monotonic_clock_gettime(struct timespec *ts) {
#ifdef __APPLE__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_MONOTONIC, ts);
#endif
}

/******************************************************************************
 *                        LIBLO And Reflection Code                           *
 *                                                                            *
 * All messages that are handled are handled in a serial fashion.             *
 * Thus, changes in the current interface sending messages can be encoded     *
 * into the stream via events which simply echo back the active interface     *
 ******************************************************************************/
static void liblo_error_cb(int i, const char *m, const char *loc)
{
    fprintf(stderr, "liblo :-( %d-%s@%s\n",i,m,loc);
}

// we need to access this before the definitions
// bad style?
static const rtosc::Ports& getNonRtParamPorts();

static int handler_function(const char *path, const char *types, lo_arg **argv,
        int argc, lo_message msg, void *user_data)
{
    (void) types;
    (void) argv;
    (void) argc;
    MiddleWare *mw = (MiddleWare*)user_data;
    lo_address addr = lo_message_get_source(msg);
    if(addr) {
        const char *tmp = lo_address_get_url(addr);
        if(tmp != mw->activeUrl()) {
            mw->transmitMsg("/echo", "ss", "OSC_URL", tmp);
            mw->activeUrl(tmp);
        }
        free((void*)tmp);
    }

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    size_t size = 2048;
    lo_message_serialise(msg, path, buffer, &size);

    if(!strcmp(buffer, "/path-search") &&
       (!strcmp("ss",  rtosc_argument_string(buffer)) ||
        !strcmp("ssT", rtosc_argument_string(buffer)) ) )
    {
        constexpr bool debug_path_search = false;
        if(debug_path_search) {
            fprintf(stderr, "MW: path-search: %s, %s\n",
                   rtosc_argument(buffer, 0).s, rtosc_argument(buffer, 1).s);
        }
        bool reply_with_query = rtosc_narguments(buffer) == 3;

        char reply_buffer[1024*20];
        std::size_t length =
            rtosc::path_search(MiddleWare::getAllPorts(), buffer, 128,
                               reply_buffer, sizeof(reply_buffer),
                               rtosc::path_search_opts::sorted_and_unique_prefix,
                               reply_with_query);
        if(length) {
            lo_message msg  = lo_message_deserialise((void*)reply_buffer,
                                                     length, NULL);
            if(debug_path_search) {
                fprintf(stderr, "  reply:\n");
                lo_message_pp(msg);
            }
            lo_address addr = lo_address_new_from_url(mw->activeUrl().c_str());
            if(addr)
                lo_send_message(addr, reply_buffer, msg);
            lo_address_free(addr);
            lo_message_free(msg);
        }
        else {
            if(debug_path_search)
                fprintf(stderr, "  -> no reply!\n");
        }
    }
    else if(buffer[0]=='/' && strrchr(buffer, '/')[1])
    {
        mw->transmitMsg(rtosc::Ports::collapsePath(buffer));
    }

    return 0;
}

typedef void(*cb_t)(void*,const char*);

//utility method (should be moved to a better location)
template <class T, class V>
std::vector<T> keys(const std::map<T,V> &m)
{
    std::vector<T> vec;
    for(auto &kv: m)
        vec.push_back(kv.first);
    return vec;
}


/*****************************************************************************
 *                    Memory Deallocation                                    *
 *****************************************************************************/
void deallocate(const char *str, void *v)
{
    //printf("deallocating a '%s' at '%p'\n", str, v);
    if(!strcmp(str, "Part"))
        delete (Part*)v;
    else if(!strcmp(str, "Master"))
        delete (Master*)v;
    else if(!strcmp(str, "fft_t"))
        delete[] (fft_t*)v;
    else if(!strcmp(str, "KbmInfo"))
        delete (KbmInfo*)v;
    else if(!strcmp(str, "SclInfo"))
        delete (SclInfo*)v;
    else if(!strcmp(str, "Microtonal"))
        delete (Microtonal*)v;
    else if(!strcmp(str, "PADsample"))
        delete[] (float*)v;
    else
        fprintf(stderr, "Unknown type '%s', leaking pointer %p!!\n", str, v);
}


/*****************************************************************************
 *                    PadSynth Setup                                         *
 *****************************************************************************/

// This lets MiddleWare compute non-realtime PAD synth data and send it to the backend
void preparePadSynth(string path, PADnoteParameters *p, rtosc::RtData &d)
{
    //printf("preparing padsynth parameters\n");
    assert(!path.empty());
    path += "sample";

#ifdef WIN32
    unsigned num = p->sampleGenerator([&path,&d]
                       (unsigned N, PADnoteParameters::Sample &&s)
                       {
                           //printf("sending info to '%s'\n",
                           //       (path+to_s(N)).c_str());
                           d.chain((path+to_s(N)).c_str(), "ifb",
                                   s.size, s.basefreq, sizeof(float*), &s.smp);
                       }, []{return false;}, 1);
#else
    std::mutex rtdata_mutex;
    unsigned num = p->sampleGenerator([&rtdata_mutex, &path,&d]
                       (unsigned N, PADnoteParameters::Sample&& s)
                       {
                           //printf("sending info to '%s'\n",
                           //       (path+to_s(N)).c_str());
                           rtdata_mutex.lock();
                           // send non-realtime computed data to PADnoteParameters
                           d.chain((path+to_s(N)).c_str(), "ifb",
                                   s.size, s.basefreq, sizeof(float*), &s.smp);
                           rtdata_mutex.unlock();
                       }, []{return false;});
#endif

    //clear out unused samples
    for(unsigned i = num; i < PAD_MAX_SAMPLES; ++i) {
        d.chain((path+to_s(i)).c_str(), "ifb",
                0, 440.0f, sizeof(float*), NULL);
    }
}

/******************************************************************************
 *                      MIDI Serialization                                    *
 *                                                                            *
 ******************************************************************************/
void saveMidiLearn(XMLwrapper &xml, const rtosc::MidiMappernRT &midi)
{
    xml.beginbranch("midi-learn");
    for(auto value:midi.inv_map) {
        XmlNode binding("midi-binding");
        auto biject = std::get<3>(value.second);
        binding["osc-path"]  = value.first;
        binding["coarse-CC"] = to_s(std::get<1>(value.second));
        binding["fine-CC"]   = to_s(std::get<2>(value.second));
        binding["type"]      = "i";
        binding["minimum"]   = to_s(biject.min);
        binding["maximum"]   = to_s(biject.max);
        xml.add(binding);
    }
    xml.endbranch();
}

void loadMidiLearn(XMLwrapper &xml, rtosc::MidiMappernRT &midi)
{
    using rtosc::Port;
    if(xml.enterbranch("midi-learn")) {
        auto nodes = xml.getBranch();

        //TODO clear mapper

        for(auto node:nodes) {
            if(node.name != "midi-binding" ||
                    !node.has("osc-path") ||
                    !node.has("coarse-CC"))
                continue;
            const string path = node["osc-path"];
            const int    CC   = atoi(node["coarse-CC"].c_str());
            const Port  *p    = Master::ports.apropos(path.c_str());
            if(p) {
                printf("loading midi port...\n");
                midi.addNewMapper(CC, *p, path);
            } else {
                printf("unknown midi bindable <%s>\n", path.c_str());
            }
        }
        xml.exitbranch();
    } else
        printf("cannot find 'midi-learn' branch...\n");
}

void connectMidiLearn(int par, int chan, bool isNrpn, string path, rtosc::MidiMappernRT &midi)
{
    const rtosc::Port *p = Master::ports.apropos(path.c_str());
    if(p) {
        if(isNrpn)
            printf("mapping midi NRPN: %d, CH: %d to Port: %s\n", par, chan, path.c_str());
        else
            printf("mapping midi CC: %d, CH: %d to Port: %s\n", par, chan, path.c_str());
            
        if(chan<1) chan=1;
        int ID = (isNrpn<<18) + (((chan-1)&0x0f)<<14) + par;
        //~ printf("ID = %d\n", ID);

        midi.addNewMapper(ID, *p, path);
    } else {
        printf("unknown port to midi bind <%s>\n", path.c_str());
    }
}
/******************************************************************************
 *                      Non-RealTime Object Store                             *
 *                                                                            *
 *                                                                            *
 * Storage For Objects which need to be interfaced with outside the realtime  *
 * thread (aka they have long lived operations which can be done out-of-band) *
 *                                                                            *
 * - OscilGen instances as prepare() cannot be done in realtime and PAD       *
 *   depends on these instances                                               *
 * - PADnoteParameter instances as applyparameters() cannot be done in        *
 *   realtime                                                                 *
 *                                                                            *
 * These instances are collected on every part change and kit change          *
 ******************************************************************************/
struct NonRtObjStore
{
    std::map<std::string, void*> objmap;

    void extractMaster(Master *master)
    {
        for(int i=0; i < NUM_MIDI_PARTS; ++i) {
            extractPart(master->part[i], i);
        }
    }

    void extractPart(Part *part, int i)
    {
        for(int j=0; j < NUM_KIT_ITEMS; ++j) {
            auto &obj = part->kit[j];
            extractAD(obj.adpars, i, j);
            extractPAD(obj.padpars, i, j);
        }
    }

    void extractAD(ADnoteParameters *adpars, int i, int j)
    {
        std::string base = "/part"+to_s(i)+"/kit"+to_s(j)+"/";
        for(int k=0; k<NUM_VOICES; ++k) {
            std::string nbase = base+"adpars/VoicePar"+to_s(k)+"/";
            if(adpars) {
                auto &nobj = adpars->VoicePar[k];
                objmap[nbase+"OscilSmp/"] = nobj.OscilGn;
                objmap[nbase+"FMSmp/"]    = nobj.FmGn;
            } else {
                objmap[nbase+"OscilSmp/"] = nullptr;
                objmap[nbase+"FMSmp/"]    = nullptr;
            }
        }
    }

    void extractPAD(PADnoteParameters *padpars, int i, int j)
    {
        std::string base = "/part"+to_s(i)+"/kit"+to_s(j)+"/";
        for(int k=0; k<NUM_VOICES; ++k) {
            if(padpars) {
                objmap[base+"padpars/"]       = padpars;
                objmap[base+"padpars/oscilgen/"] = padpars->oscilgen;
            } else {
                objmap[base+"padpars/"]       = nullptr;
                objmap[base+"padpars/oscilgen/"] = nullptr;
            }
        }
    }

    void clear(void)
    {
        objmap.clear();
    }

    bool has(std::string loc)
    {
        return objmap.find(loc) != objmap.end();
    }

    void *get(std::string loc)
    {
        return objmap[loc];
    }

    void handleOscil(const char *msg, rtosc::RtData &d) {
        string obj_rl(d.message, msg);
        assert(d.message);
        assert(msg);
        assert(msg >= d.message);
        assert(msg - d.message < 256);
        void *osc = get(obj_rl);
        if(osc)
        {
            strcpy(d.loc, obj_rl.c_str());
            d.obj = osc;
            OscilGen::non_realtime_ports.dispatch(msg, d);
        }
        else {
            // print warning, except in rtosc::walk_ports
            if(!strstr(d.message, "/pointer"))
            {
                fprintf(stderr, "Warning: trying to access oscil object \"%s\","
                                "which does not exist\n", obj_rl.c_str());
            }
            d.obj = nullptr; // tell walk_ports that there's nothing to recurse here...
        }
    }
    void handlePad(const char *msg, rtosc::RtData &d) {
        string obj_rl(d.message, msg);
        void *pad = get(obj_rl);
        if(!strcmp(msg, "prepare")) {
            preparePadSynth(obj_rl, (PADnoteParameters*)pad, d);
            d.matches++;
            d.reply((obj_rl+"needPrepare").c_str(), "F");
        } else {
            if(pad)
            {
                strcpy(d.loc, obj_rl.c_str());
                d.obj = pad;
                PADnoteParameters::non_realtime_ports.dispatch(msg, d);
                if(d.matches && rtosc_narguments(msg)) {
                    if(!strcmp(msg, "oscilgen/prepare"))
                        ; //ignore
                    else {
                        d.reply((obj_rl+"needPrepare").c_str(), "T");
                    }
                }
            }
            else {
                // print warning, except in rtosc::walk_ports
                if(!strstr(d.message, "/pointer"))
                {
                    fprintf(stderr, "Warning: trying to access pad synth object "
                                    "\"%s\", which does not exist\n",
                            obj_rl.c_str());
                }
                d.obj = nullptr; // tell walk_ports that there's nothing to recurse here...
            }
        }
    }
};

/******************************************************************************
 *                      Realtime Parameter Store                              *
 *                                                                            *
 * Storage for AD/PAD/SUB parameters which are allocated as needed by kits.   *
 * Two classes of events affect this:                                         *
 * 1. When a message to enable a kit is observed, then the kit is allocated   *
 *    and sent prior to the enable message.                                   *
 * 2. When a part is allocated all part information is rebuilt                *
 *                                                                            *
 * (NOTE pointers aren't really needed here, just booleans on whether it has  *
 * been  allocated)                                                           *
 * This may be later utilized for copy/paste support                          *
 ******************************************************************************/
struct ParamStore
{
    ParamStore(void)
    {
        memset(add, 0, sizeof(add));
        memset(pad, 0, sizeof(pad));
        memset(sub, 0, sizeof(sub));
    }

    void extractPart(Part *part, int i)
    {
        for(int j=0; j < NUM_KIT_ITEMS; ++j) {
            auto kit = part->kit[j];
            add[i][j] = kit.adpars;
            sub[i][j] = kit.subpars;
            pad[i][j] = kit.padpars;
        }
    }

    ADnoteParameters  *add[NUM_MIDI_PARTS][NUM_KIT_ITEMS];
    SUBnoteParameters *sub[NUM_MIDI_PARTS][NUM_KIT_ITEMS];
    PADnoteParameters *pad[NUM_MIDI_PARTS][NUM_KIT_ITEMS];
};

//XXX perhaps move this to Nio
//(there needs to be some standard Nio stub file for this sort of stuff)
namespace Nio
{
    using std::get;
    rtosc::Ports ports = {
        {"sink-list:", 0, 0, [](const char *, rtosc::RtData &d) {
                auto list = Nio::getSinks();
                char *ret = rtosc_splat(d.loc, list);
                d.reply(ret);
                delete [] ret;
            }},
        {"source-list:", 0, 0, [](const char *, rtosc::RtData &d) {
                auto list = Nio::getSources();
                char *ret = rtosc_splat(d.loc, list);
                d.reply(ret);
                delete [] ret;
            }},
        {"source::s", 0, 0, [](const char *msg, rtosc::RtData &d) {
                if(rtosc_narguments(msg) == 0)
                    d.reply(d.loc, "s", Nio::getSource().c_str());
                else
                    Nio::setSource(rtosc_argument(msg,0).s);}},
        {"sink::s", 0, 0, [](const char *msg, rtosc::RtData &d) {
                if(rtosc_narguments(msg) == 0)
                    d.reply(d.loc, "s", Nio::getSink().c_str());
                else
                    Nio::setSink(rtosc_argument(msg,0).s);}},
        {"audio-compressor::T:F", 0, 0, [](const char *msg, rtosc::RtData &d) {
                if(rtosc_narguments(msg) == 0)
                    d.reply(d.loc, Nio::getAudioCompressor() ? "T" : "F");
                else
                    Nio::setAudioCompressor(rtosc_argument(msg,0).T);}},
    };
}


/* Implementation */

class mw_dispatcher_t : public master_dispatcher_t
{
    MiddleWare* mw;
    bool do_dispatch(const char *msg) override
    {
        mw->transmitMsg(msg);
        return true; // we cannot yet say if the port matched
                     // we will query the Master after everything will be done
    }
    void vUpdateMaster(Master* m) { mw->switchMaster(m); }

public:
    mw_dispatcher_t(MiddleWare* mw) : mw(mw) {}
};

class MiddleWareImpl
{
    // messages chained with MwDataObj::chain
    // must yet be handled after a previous handleMsg
    std::queue<std::vector<char>> msgsToHandle;
public:
    MiddleWare *parent;
    Config* const config;
    MiddleWareImpl(MiddleWare *mw, SYNTH_T synth, Config* config,
                   int preferred_port);
    ~MiddleWareImpl(void);
    void recreateMinimalMaster();

    //Check offline vs online mode in plugins
    void heartBeat(Master *m);
    int64_t start_time_sec;
    int64_t start_time_nsec;
    bool offline;

    //Apply function while parameters are write locked
    void doReadOnlyOp(std::function<void()> read_only_fn);
    void doReadOnlyOpPlugin(std::function<void()> read_only_fn);
    bool doReadOnlyOpNormal(std::function<void()> read_only_fn, bool canfail=false);

    void savePart(int npart, const char *filename)
    {
        // Due to a possible bug in ThreadLink, filename may get trashed when
        // the read-only operation writes to the buffer again. Copy to string:
        std::string fname = filename;
        //printf("saving part(%d,'%s')\n", npart, filename);
        doReadOnlyOp([this,fname,npart](){
                int res = master->part[npart]->saveXML(fname.c_str());
                (void)res;
                /*printf("results: '%s' '%d'\n",fname.c_str(), res);*/});
    }

    void loadPendingBank(int par, Bank &bank)
    {
        if(((unsigned int)par < bank.banks.size())
           && (bank.banks[par].dir != bank.bankfiletitle))
            bank.loadbank(bank.banks[par].dir);
    }

    void loadPart(int npart, const char *filename, Master *master, rtosc::RtData &d)
    {
        actual_load[npart]++;

        if(actual_load[npart] != pending_load[npart])
            return;
        assert(actual_load[npart] <= pending_load[npart]);
        assert(filename);

        //load part in async fashion when possible
#ifndef WIN32
        auto alloc = std::async(std::launch::async,
                [master,filename,this,npart](){
                Part *p = new Part(*master->memory, synth,
                                   master->time,
                                   config->cfg.GzipCompression,
                                   config->cfg.Interpolation,
                                   &master->microtonal, master->fft, &master->watcher,
                                   ("/part"+to_s(npart)+"/").c_str());
                if(p->loadXMLinstrument(filename))
                    fprintf(stderr, "Warning: failed to load part<%s>!\n", filename);

                auto isLateLoad = [this,npart]{
                return actual_load[npart] != pending_load[npart];
                };

                p->applyparameters(isLateLoad);
                return p;});

        //Load the part
        if(idle) {
            while(alloc.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                idle(idle_ptr);
            }
        }

        Part *p = alloc.get();
#else
        Part *p = new Part(*master->memory, synth, master->time,
                config->cfg.GzipCompression,
                config->cfg.Interpolation,
                &master->microtonal, master->fft);

        if(p->loadXMLinstrument(filename))
            fprintf(stderr, "Warning: failed to load part<%s>!\n", filename);

        auto isLateLoad = [this,npart]{
            return actual_load[npart] != pending_load[npart];
        };

        p->applyparameters(isLateLoad);
#endif

        obj_store.extractPart(p, npart);
        kits.extractPart(p, npart);

        //Give it to the backend and wait for the old part to return for
        //deallocation
        parent->transmitMsg("/load-part", "ib", npart, sizeof(Part*), &p);
        d.broadcast("/damage", "s", ("/part"+to_s(npart)+"/").c_str());
    }

    //Load a new cleared Part instance
    void loadClearPart(int npart)
    {
        if(npart == -1)
            return;

        Part *p = new Part(*master->memory, synth,
                master->time,
                config->cfg.GzipCompression,
                config->cfg.Interpolation,
                &master->microtonal, master->fft);
        p->applyparameters();
        obj_store.extractPart(p, npart);
        kits.extractPart(p, npart);

        //Give it to the backend and wait for the old part to return for
        //deallocation
        parent->transmitMsg("/load-part", "ib", npart, sizeof(Part *), &p);
        GUI::raiseUi(ui, "/damage", "s", ("/part" + to_s(npart) + "/").c_str());
    }

    //Well, you don't get much crazier than changing out all of your RT
    //structures at once...
    int loadMaster(const char *filename, bool osc_format = false)
    {
        Master *m = new Master(synth, config);
        m->uToB = uToB;
        m->bToU = bToU;

        if(filename) {
            if(osc_format)
            {
                mw_dispatcher_t dispatcher(parent);
                if( m->loadOSC(filename, &dispatcher) < 0 ) {
                    delete m;
                    return -1;
                }
            }
            else
            {
                if ( m->loadXML(filename) ) {
                    delete m;
                    return -1;
                }
            }
            m->applyparameters();
        }

        //Update resource locator table
        updateResources(m);

        previous_master = master;
        master = m;

        //Give it to the backend and wait for the old part to return for
        //deallocation
        parent->transmitMsg("/load-master", "b", sizeof(Master*), &m);
        return 0;
    }

    // Save all possible parameters
    // In user language, this is called "saving a master", but we
    // are saving parameters owned by Master and by MiddleWare
    // Return 0 if OK, <0 if not
    int saveParams(const char *filename, bool osc_format = false)
    {
        int res;
        if(osc_format)
        {
            mw_dispatcher_t dispatcher(parent);

            // allocate an "empty" master
            // after the savefile will have been saved, it will be loaded into this
            // dummy master, and then the two masters will be compared
            zyn::Config config;
            zyn::SYNTH_T* synth = new zyn::SYNTH_T;
            synth->buffersize = master->synth.buffersize;
            synth->samplerate = master->synth.samplerate;
            synth->alias();
            zyn::Master master2(*synth, &config);
            master->copyMasterCbTo(&master2);
            master2.frozenState = true;

            std::string savefile;
            rtosc_version m_version =
            {
                (unsigned char) version.get_major(),
                (unsigned char) version.get_minor(),
                (unsigned char) version.get_revision()
            };
            savefile = rtosc::save_to_file(getNonRtParamPorts(), this, "ZynAddSubFX", m_version);
            savefile += '\n';

            doReadOnlyOp([this,filename,&dispatcher,&master2,&savefile,&res]()
            {
                savefile = master->saveOSC(savefile);
#if 1
                // load the savefile string into another master to compare the results
                // between the original and the savefile-loaded master
                // this requires a temporary master switch
                Master* old_master = master;
                dispatcher.updateMaster(&master2);

                res = master2.loadOSCFromStr(savefile.c_str(), &dispatcher);
                // TODO: compare MiddleWare, too?

                // The above call is done by this thread (i.e. the MiddleWare thread), but
                // it sends messages to master2 in order to load the values
                // We need to wait until savefile has been loaded into master2
                int i;
                for(i = 0; i < 20 && master2.uToB->hasNext(); ++i)
                    os_usleep(50000);
                if(i >= 20) // >= 1 second?
                {
                    // Master failed to fetch its messages
                    res = -1;
                }
                printf("Saved in less than %d ms.\n", 50*i);

                dispatcher.updateMaster(old_master);
#endif
                if(res < 0)
                {
                    std::cerr << "invalid savefile (or a backend error)!" << std::endl;
                    std::cerr << "complete savefile:" << std::endl;
                    std::cerr << savefile << std::endl;
                    std::cerr << "first entry that could not be parsed:" << std::endl;

                    for(int i = -res + 1; savefile[i]; ++i)
                    if(savefile[i] == '\n')
                    {
                        savefile.resize(i);
                        break;
                    }
                    std::cerr << (savefile.c_str() - res) << std::endl;

                    res = -1;
                }
                else
                {
                    char* xml = master->getXMLData(),
                        * xml2 = master2.getXMLData();
                    // TODO: below here can be moved out of read only op

                    res = strcmp(xml, xml2) ? -1 : 0;

                    if(res == 0)
                    {
                        if(filename && *filename)
                        {
                            std::ofstream ofs(filename);
                            ofs << savefile;
                        }
                        else {
                            std::cout << "The savefile content follows" << std::endl;
                            std::cout << "---->8----" << std::endl;
                            std::cout << savefile << std::endl;
                            std::cout << "---->8----" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << savefile << std::endl;
                        std::cerr << "Can not write OSC savefile!! (see tmp1.txt and tmp2.txt)"
                                  << std::endl;
                        std::ofstream tmp1("tmp1.txt"), tmp2("tmp2.txt");
                        tmp1 << xml;
                        tmp2 << xml2;
                        res = -1;
                    }

                    free(xml);
                    free(xml2);
                }
            });
        }
        else // xml format
        {
            doReadOnlyOp([this,filename,&res](){
                             res = master->saveXML(filename);});
        }
        return res;
    }

    void loadXsz(const char *filename, rtosc::RtData &d)
    {
        Microtonal *micro = new Microtonal(master->gzip_compression);
        int err = micro->loadXML(filename);
        if(err) {
            d.reply("/alert", "s", "Error: Could not load the xsz file.");
            delete micro;
        } else
            d.chain("/microtonal/paste", "b", sizeof(void*), &micro);
    }

    void saveXsz(const char *filename, rtosc::RtData &d)
    {
        int err = 0;
        doReadOnlyOp([this,filename,&err](){
                err = master->microtonal.saveXML(filename);});
        if(err)
            d.reply("/alert", "s", "Error: Could not save the xsz file.");
    }

    void loadScl(const char *filename, rtosc::RtData &d)
    {
        SclInfo *scl = new SclInfo;
        int err=Microtonal::loadscl(*scl, filename);
        if(err) {
            d.reply("/alert", "s", "Error: Could not load the scl file.");
            delete scl;
        } else
            d.chain("/microtonal/paste_scl", "b", sizeof(void*), &scl);
    }

    void loadKbm(const char *filename, rtosc::RtData &d)
    {
        KbmInfo *kbm = new KbmInfo;
        int err=Microtonal::loadkbm(*kbm, filename);
        if(err) {
            d.reply("/alert", "s", "Error: Could not load the kbm file.");
            delete kbm;
        } else
            d.chain("/microtonal/paste_kbm", "b", sizeof(void*), &kbm);
    }

    void updateResources(Master *m)
    {
        obj_store.clear();
        obj_store.extractMaster(m);
        for(int i=0; i<NUM_MIDI_PARTS; ++i)
            kits.extractPart(m->part[i], i);
    }

    //If currently broadcasting messages
    bool broadcast = false;
    //If message should be forwarded through snoop ports
    bool forward   = false;
    //if message is in order or out-of-order execution
    bool in_order  = false;
    //If accepting undo events as user driven
    bool recording_undo = true;
    void bToUhandle(const char *rtmsg);

    void tick(void)
    {
        if(server)
            while(lo_server_recv_noblock(server, 0));

        while(bToU->hasNext()) {
            const char *rtmsg = bToU->read();
            bToUhandle(rtmsg);
        }

        while(auto *m = multi_thread_source.read()) {
            handleMsg(m->memory);
            multi_thread_source.free(m);
        }

        autoSave.tick();

        heartBeat(master);

        if(offline)
        {
            //pass previous master in case it will have to be freed
            //similar to previous_master->runOSC(0,0,true)
            //but note that previous_master could have been freed already
            master->runOSC(0,0,true, previous_master);
        }
    }


    void kitEnable(const char *msg);
    void kitEnable(int part, int kit, int type);

    // Handle an event with special cases
    void handleMsg(const char *msg, bool msg_comes_from_realtime = false);

    // Add a message for handleMsg to a queue
    void queueMsg(const char* msg)
    {
        msgsToHandle.emplace(msg, msg+rtosc_message_length(msg, -1));
    }

    void write(const char *path, const char *args, ...);
    void write(const char *path, const char *args, va_list va);

    void currentUrl(string addr)
    {
        curr_url = addr;
        known_remotes.insert(addr);
    }

    // Send a message to a remote client
    void sendToRemote(const char *msg, std::string dest);
    // Send a message to the current remote client
    void sendToCurrentRemote(const char *msg)
    {
        sendToRemote(msg, in_order ? curr_url : last_url);
    }
    // Broadcast a message to all listening remote clients
    void broadcastToRemote(const char *msg);


    /*
     * Provides a mapping for non-RT objects stored inside the backend
     * - Oscilgen almost all parameters can be safely set
     * - Padnote can have anything set on its oscilgen and a very small set
     *   of general parameters
     */
    NonRtObjStore obj_store;

    //This code will own the pointer to master, be prepared for odd things if
    //this assumption is broken
    Master *master;

    //The master before the last load operation, if any
    //Only valid until freed
    Master *previous_master = nullptr;

    //The ONLY means that any chunk of UI code should have for interacting with the
    //backend
    Fl_Osc_Interface *osc;
    //Synth Engine Parameters
    ParamStore kits;

    //Callback When Waiting on async events
    void(*idle)(void*);
    void* idle_ptr;

    //General UI callback
    cb_t cb;
    //UI handle
    void *ui;

    std::atomic_int pending_load[NUM_MIDI_PARTS];
    std::atomic_int actual_load[NUM_MIDI_PARTS];

    //Undo/Redo
    rtosc::UndoHistory undo;

    //MIDI Learn
    rtosc::MidiMappernRT midi_mapper;

    //Link To the Realtime
    rtosc::ThreadLink *bToU;
    rtosc::ThreadLink *uToB;

    //Link to the unknown
    MultiQueue multi_thread_source;

    //LIBLO
    lo_server server;
    string last_url, curr_url;
    std::set<string> known_remotes;

    //Synthesis Rate Parameters
    SYNTH_T synth;

    PresetsStore presetsstore;

    CallbackRepeater autoSave;
};

/*****************************************************************************
 *                      Data Object for Non-RT Class Dispatch                *
 *****************************************************************************/

class MwDataObj:public rtosc::RtData
{
    public:
        MwDataObj(MiddleWareImpl *mwi_)
        {
            loc_size = 1024;
            loc = new char[loc_size];
            memset(loc, 0, loc_size);
            buffer = new char[4*4096];
            memset(buffer, 0, 4*4096);
            obj       = mwi_;
            mwi       = mwi_;
            forwarded = false;
        }

        ~MwDataObj(void)
        {
            delete[] loc;
            delete[] buffer;
        }

        //Replies and broadcasts go to the remote

        //Chain calls repeat the call into handle()

        //Forward calls send the message directly to the realtime
        virtual void reply(const char *path, const char *args, ...) override
        {
            //printf("reply building '%s'\n", path);
            va_list va;
            va_start(va,args);
            if(!strcmp(path, "/forward")) { //forward the information to the backend
                args++;
                path = va_arg(va, const char *);
                rtosc_vmessage(buffer,4*4096,path,args,va);
            } else {
                rtosc_vmessage(buffer,4*4096,path,args,va);
                reply(buffer);
            }
            va_end(va);
        }
        virtual void replyArray(const char *path, const char *args, rtosc_arg_t *argd) override
        {
            //printf("reply building '%s'\n", path);
            if(!strcmp(path, "/forward")) { //forward the information to the backend
                args++;
                rtosc_amessage(buffer,4*4096,path,args,argd);
            } else {
                rtosc_amessage(buffer,4*4096,path,args,argd);
                reply(buffer);
            }
        }
        //! In the case of MiddleWare, "reply" always means sending back to
        //! the front-end. If a message from the back-end gets "replied", this
        //! only means that its counterpart has been sent from the front-end via
        //! MiddleWare to the backend, so the reply has to go back to the
        //! front-end. The back-end itself usually doesn't ask things, so it
        //! will not get replies.
        virtual void reply(const char *msg) override{
            mwi->sendToCurrentRemote(msg);
        }

        virtual void broadcast(const char *msg) override {
            mwi->broadcastToRemote(msg);
        }

        virtual void chain(const char *msg) override
        {
            assert(msg);
            // printf("chain call on <%s>\n", msg);
            mwi->queueMsg(msg);
        }

        virtual void chain(const char *path, const char *args, ...) override
        {
            assert(path);
            va_list va;
            va_start(va,args);
            rtosc_vmessage(buffer,4*4096,path,args,va);
            chain(buffer);
            va_end(va);
        }

        virtual void forward(const char *) override
        {
            forwarded = true;
        }

        bool forwarded;
    private:
        char *buffer;
        MiddleWareImpl   *mwi;
};

static std::vector<std::string> getFiles(const char *folder, bool finddir)
{
    DIR *dir = opendir(folder);

    if(dir == NULL) {
        return {};
    }

    struct dirent *fn;
    std::vector<string> files;
    bool has_updir = false;

    while((fn = readdir(dir))) {
#ifndef WIN32
        bool is_dir = fn->d_type & DT_DIR;
        //it could still be a symbolic link
        if(!is_dir) {
            string path = string(folder) + "/" + fn->d_name;
            struct stat buf;
            memset((void*)&buf, 0, sizeof(buf));
            int err = stat(path.c_str(), &buf);
            if(err)
                printf("[Zyn:Error] stat cannot handle <%s>:%d\n", path.c_str(), err);
            if(S_ISDIR(buf.st_mode)) {
                is_dir = true;
            }
        }
#else
        std::string darn_windows = folder + std::string("/") + std::string(fn->d_name);
        //printf("attr on <%s> => %x\n", darn_windows.c_str(), GetFileAttributes(darn_windows.c_str()));
        //printf("desired mask =  %x\n", mask);
        //printf("error = %x\n", INVALID_FILE_ATTRIBUTES);
        bool is_dir = GetFileAttributes(darn_windows.c_str()) & FILE_ATTRIBUTE_DIRECTORY;
#endif
        if(finddir == is_dir && strcmp(".", fn->d_name))
            files.push_back(fn->d_name);

        if(!strcmp("..", fn->d_name))
            has_updir = true;
    }

    if(finddir == true && has_updir == false)
        files.push_back("..");

    closedir(dir);
    std::sort(begin(files), end(files));
    return files;
}



static int extractInt(const char *msg)
{
    const char *mm = msg;
    while(*mm && !isdigit(*mm)) ++mm;
    if(isdigit(*mm))
        return atoi(mm);
    return -1;
}

static const char *chomp(const char *msg)
{
    while(*msg && *msg!='/') ++msg;
    msg = *msg ? msg+1 : msg;
    return msg;
};

using rtosc::RtData;
#define rObject Bank
#define rBegin [](const char *msg, RtData &d) { (void)msg;(void)d;\
    rObject &impl = *((rObject*)d.obj);(void)impl;
#define rEnd }
/*****************************************************************************
 *                       Instrument Banks                                    *
 *                                                                           *
 * Banks and presets in general are not classed as realtime safe             *
 *                                                                           *
 * The supported operations are:                                             *
 * - Load Names                                                              *
 * - Load Bank                                                               *
 * - Refresh List of Banks                                                   *
 *****************************************************************************/
extern const rtosc::Ports bankPorts;
const rtosc::Ports bankPorts = {
    {"rescan:", 0, 0,
        rBegin;
        impl.bankpos = 0;
        impl.rescanforbanks();
        //Send updated banks
        int i = 0;
        for(auto &elm : impl.banks)
            d.reply("/bank/bank_select", "iss", i++, elm.name.c_str(), elm.dir.c_str());
        d.reply("/bank/bank_select", "i", impl.bankpos);
        if (i > 0) {
            impl.loadbank(impl.banks[0].dir);

            //Reload bank slots
            for(int i=0; i<BANK_SIZE; ++i) {
                d.reply("/bankview", "iss",
                    i, impl.ins[i].name.c_str(),
                    impl.ins[i].filename.c_str());
            }
        } else {
            //Clear all bank slots
            for(int i=0; i<BANK_SIZE; ++i) {
                d.reply("/bankview", "iss", i, "", "");
            }
        }
        d.broadcast("/damage", "s", "/bank/");
        rEnd},
    {"bank_list:", 0, 0,
        rBegin;
#define MAX_BANKS 256
        char        types[MAX_BANKS*2+1]={0};
        rtosc_arg_t args[MAX_BANKS*2];
        int i = 0;
        for(auto &elm : impl.banks) {
            types[i] = types [i + 1] = 's';
            args[i++].s = elm.name.c_str();
            args[i++].s = elm.dir.c_str();
        }
        d.replyArray("/bank/bank_list", types, args);
#undef MAX_BANKS
        rEnd},
    {"types:", 0, 0,
        rBegin;
        const char *types[17];
        types[ 0] = "None";
        types[ 1] = "Piano";
        types[ 2] = "Chromatic Percussion";
        types[ 3] = "Organ";
        types[ 4] = "Guitar";
        types[ 5] = "Bass";
        types[ 6] = "Solo Strings";
        types[ 7] = "Ensemble";
        types[ 8] = "Brass";
        types[ 9] = "Reed";
        types[10] = "Pipe";
        types[11] = "Synth Lead";
        types[12] = "Synth Pad";
        types[13] = "Synth Effects";
        types[14] = "Ethnic";
        types[15] = "Percussive";
        types[16] = "Sound Effects";
        char        t[17+1]={0};
        rtosc_arg_t args[17];
        for(int i=0; i<17; ++i) {
            t[i]      = 's';
            args[i].s = types[i];
        }
        d.replyArray("/bank/types", t, args);
        rEnd},
    {"tags:", 0, 0,
        rBegin;
        const char *types[8];
        types[ 0] = "fast";
        types[ 1] = "slow";
        types[ 2] = "saw";
        types[ 3] = "bell";
        types[ 4] = "lead";
        types[ 5] = "ambient";
        types[ 6] = "horn";
        types[ 7] = "alarm";
        char        t[8+1]={0};
        rtosc_arg_t args[8];
        for(int i=0; i<8; ++i) {
            t[i]      = 's';
            args[i].s = types[i];
        }
        d.replyArray(d.loc, t, args);
        rEnd},
    {"slot#1024:", 0, 0,
        rBegin;
        const int loc = extractInt(msg);
        if(loc >= BANK_SIZE)
            return;

        d.reply("/bankview", "iss",
                loc, impl.ins[loc].name.c_str(),
                impl.ins[loc].filename.c_str());
        rEnd},
    {"banks:", 0, 0,
        rBegin;
        int i = 0;
        for(auto &elm : impl.banks)
            d.reply("/bank/bank_select", "iss", i++, elm.name.c_str(), elm.dir.c_str());
        rEnd},
    {"bank_select::i", 0, 0,
        rBegin
           if(rtosc_narguments(msg)) {
               const int pos = rtosc_argument(msg, 0).i;
               d.reply(d.loc, "i", pos);
               if(impl.bankpos != pos) {
                   impl.bankpos = pos;
                   impl.loadbank(impl.banks[pos].dir);

                   //Reload bank slots
                   for(int i=0; i<BANK_SIZE; ++i)
                       d.reply("/bankview", "iss",
                                   i, impl.ins[i].name.c_str(),
                                   impl.ins[i].filename.c_str());
               }
            } else
                d.reply("/bank/bank_select", "i", impl.bankpos);
        rEnd},
    {"rename_slot:is", 0, 0,
        rBegin;
        const int   slot = rtosc_argument(msg, 0).i;
        const char *name = rtosc_argument(msg, 1).s;
        const int err = impl.setname(slot, name, -1);
        if(err) {
            d.reply("/alert", "s",
                    "Failed To Rename Bank Slot, please check file permissions");
        }
        rEnd},
    {"swap_slots:ii", 0, 0,
        rBegin;
        const int slota = rtosc_argument(msg, 0).i;
        const int slotb = rtosc_argument(msg, 1).i;
        const int err = impl.swapslot(slota, slotb);
        if(err)
            d.reply("/alert", "s",
                    "Failed To Swap Bank Slots, please check file permissions");
        rEnd},
    {"clear_slot:i", 0, 0,
        rBegin;
        const int slot = rtosc_argument(msg, 0).i;
        const int err  = impl.clearslot(slot);
        if(err)
            d.reply("/alert", "s",
                    "Failed To Clear Bank Slot, please check file permissions");
        rEnd},
    {"msb::i", 0, 0,
        rBegin;
        if(rtosc_narguments(msg))
            impl.setMsb(rtosc_argument(msg, 0).i);
        else
            d.reply(d.loc, "i", impl.bank_msb);
        rEnd},
    {"lsb::i", 0, 0,
        rBegin;
        if(rtosc_narguments(msg))
            impl.setLsb(rtosc_argument(msg, 0).i);
        else
            d.reply(d.loc, "i", impl.bank_lsb);
        rEnd},
    {"newbank:s", 0, 0,
        rBegin;
        int err = impl.newbank(rtosc_argument(msg, 0).s);
        if(err)
            d.reply("/alert", "s", "Error: Could not make a new bank (directory)..");
        rEnd},
    {"search:s", 0, 0,
        rBegin;
        auto res = impl.search(rtosc_argument(msg, 0).s);
#define MAX_SEARCH 300
        char res_type[MAX_SEARCH+1] = {};
        rtosc_arg_t res_dat[MAX_SEARCH] = {};
        for(unsigned i=0; i<res.size() && i<MAX_SEARCH; ++i) {
            res_type[i]  = 's';
            res_dat[i].s = res[i].c_str();
        }
        d.replyArray("/bank/search_results", res_type, res_dat);
#undef MAX_SEARCH
        rEnd},
    {"blist:s", 0, 0,
        rBegin;
        auto res = impl.blist(rtosc_argument(msg, 0).s);
#define MAX_SEARCH 300
        char res_type[MAX_SEARCH+1] = {};
        rtosc_arg_t res_dat[MAX_SEARCH] = {};
        for(unsigned i=0; i<res.size() && i<MAX_SEARCH; ++i) {
            res_type[i]  = 's';
            res_dat[i].s = res[i].c_str();
        }
        d.replyArray("/bank/search_results", res_type, res_dat);
#undef MAX_SEARCH
        rEnd},
    {"search_results:", 0, 0,
        rBegin;
        d.reply("/bank/search_results", "");
        rEnd},
};

/******************************************************************************
 *                       MiddleWare Snooping Ports                            *
 *                                                                            *
 * These ports handle:                                                        *
 * - Events going to the realtime thread which cannot be safely handled       *
 *   there                                                                    *
 * - Events generated by the realtime thread which are not destined for a     *
 *   user interface                                                           *
 ******************************************************************************/

#undef  rObject
#define rObject MiddleWareImpl

#ifndef STRINGIFY
#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)
#endif

/*
 * common snoop port callbacks
 */
template<bool osc_format>
void load_cb(const char *msg, RtData &d)
{
    MiddleWareImpl &impl = *((MiddleWareImpl*)d.obj);
    const char *file = rtosc_argument(msg, 0).s;
    uint64_t request_time = 0;
    if(rtosc_narguments(msg) > 1)
        request_time = rtosc_argument(msg, 1).t;

    if(!impl.loadMaster(file, osc_format)) { // return-value 0 <=> OK
        d.broadcast("/damage", "s", "/");
        d.broadcast(d.loc, "stT", file, request_time);
    }
    else
        d.broadcast(d.loc, "stF", file, request_time);
}

template<bool osc_format>
void save_cb(const char *msg, RtData &d)
{
    MiddleWareImpl &impl = *((MiddleWareImpl*)d.obj);
    // Due to a possible bug in ThreadLink, filename may get trashed when
    // the read-only operation writes to the buffer again. Copy to string:
    const string file = rtosc_argument(msg, 0).s;
    uint64_t request_time = 0;
    if(rtosc_narguments(msg) > 1)
        request_time = rtosc_argument(msg, 1).t;

    int res = impl.saveParams(file.c_str(), osc_format);
    d.broadcast(d.loc, (res == 0) ? "stT" : "stF",
                file.c_str(), request_time);
}

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#pragma GCC push_options
#pragma GCC optimize("O0")
#endif

void gcc_10_1_0_is_dumb(const std::vector<std::string> &files,
        const int N,
        char *types,
        rtosc_arg_t *args)
{
        types[N] = 0;
        for(int i=0; i<N; ++i) {
            args[i].s = files[i].c_str();
            types[i]  = 's';
        }
}

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#pragma GCC pop_options
#endif

/*
 * BASE/part#/kititem#
 * BASE/part#/kit#/adpars/voice#/oscil/\*
 * BASE/part#/kit#/adpars/voice#/mod-oscil/\*
 * BASE/part#/kit#/padpars/prepare
 * BASE/part#/kit#/padpars/oscil/\*
 */
static rtosc::Ports nonRtParamPorts = {
    {"part#" STRINGIFY(NUM_MIDI_PARTS)
        "/kit#" STRINGIFY(NUM_KIT_ITEMS) "/adpars/VoicePar#"
            STRINGIFY(NUM_VOICES) "/OscilSmp/", 0, &OscilGen::non_realtime_ports,
        rBegin;
        impl.obj_store.handleOscil(chomp(chomp(chomp(chomp(chomp(msg))))), d);
        rEnd},
    {"part#" STRINGIFY(NUM_MIDI_PARTS)
        "/kit#" STRINGIFY(NUM_KIT_ITEMS)
            "/adpars/VoicePar#" STRINGIFY(NUM_VOICES) "/FMSmp/", 0, &OscilGen::non_realtime_ports,
        rBegin
        impl.obj_store.handleOscil(chomp(chomp(chomp(chomp(chomp(msg))))), d);
        rEnd},
    {"part#" STRINGIFY(NUM_MIDI_PARTS)
        "/kit#" STRINGIFY(NUM_KIT_ITEMS) "/padpars/", 0, &PADnoteParameters::non_realtime_ports,
        rBegin
        impl.obj_store.handlePad(chomp(chomp(chomp(msg))), d);
        rEnd},
};

static rtosc::Ports middwareSnoopPortsWithoutNonRtParams = {
    {"bank/", 0, &bankPorts,
        rBegin;
        d.obj = &impl.master->bank;
        bankPorts.dispatch(chomp(msg),d);
        rEnd},
    {"bank/save_to_slot:ii", 0, 0,
        rBegin;
        const int part_id = rtosc_argument(msg, 0).i;
        const int slot    = rtosc_argument(msg, 1).i;

        int err = 0;
        impl.doReadOnlyOp([&impl,slot,part_id,&err](){
                err = impl.master->bank.savetoslot(slot, impl.master->part[part_id]);});
        if(err) {
            char buffer[1024];
            rtosc_message(buffer, 1024, "/alert", "s",
                    "Failed To Save To Bank Slot, please check file permissions");
            GUI::raiseUi(impl.ui, buffer);
        }
        else d.broadcast("/damage", "s", "/bank/search_results/");
        rEnd},
    {"config/", 0, &Config::ports,
        rBegin;
        d.obj = impl.config;
        Config::ports.dispatch(chomp(msg), d);
        rEnd},
    {"presets/", 0,  &real_preset_ports,          [](const char *msg, RtData &d) {
        MiddleWareImpl *obj = (MiddleWareImpl*)d.obj;
        d.obj = (void*)obj->parent;
        real_preset_ports.dispatch(chomp(msg), d);
        if(strstr(msg, "paste") && rtosc_argument_string(msg)[0] == 's')
            d.broadcast("/damage", "s", rtosc_argument(msg, 0).s);
        }},
    {"io/", 0, &Nio::ports,               [](const char *msg, RtData &d) {
        Nio::ports.dispatch(chomp(msg), d);}},
    {"part*/kit*/{Padenabled,Ppadenabled,Psubenabled}:T:F", 0, 0,
        rBegin;
        impl.kitEnable(msg);
        d.forward();
        rEnd},
    {"save_xcz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        XMLwrapper xml;
        saveMidiLearn(xml, impl.midi_mapper);
        xml.saveXMLfile(file, impl.master->gzip_compression);
        rEnd},
    {"load_xcz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        XMLwrapper xml;
        xml.loadXMLfile(file);
        loadMidiLearn(xml, impl.midi_mapper);
        rEnd},
    {"clear_xcz:", 0, 0,
        rBegin;
        impl.midi_mapper.clear();
        rEnd},
    {"midi-map-cc:is", rDoc("bind a midi CC on CH to an OSC path"), 0,
        rBegin;
        const int par = rtosc_argument(msg, 0).i;
        const string path = rtosc_argument(msg, 1).s;
        connectMidiLearn(par, 1, false, path, impl.midi_mapper);
        rEnd},
    {"midi-map-cc:iis", rDoc("bind a midi CC on CH to an OSC path"), 0,
        rBegin;
        const int par = rtosc_argument(msg, 0).i;
        const int ch = rtosc_argument(msg, 1).i;
        const string path = rtosc_argument(msg, 2).s;
        connectMidiLearn(par, ch, false, path, impl.midi_mapper);
        rEnd},
    {"midi-map-nrpn:iis", rDoc("bind nrpn on channel to an OSC path"), 0,
        rBegin;
        const int par = rtosc_argument(msg, 0).i;
        const int ch = rtosc_argument(msg, 1).i;
        const string path = rtosc_argument(msg, 2).s;
        connectMidiLearn(par, ch, true, path, impl.midi_mapper);
        rEnd},
    {"save_xlz:s", 0, 0,
        rBegin;
        impl.doReadOnlyOp([&]() {
                const char *file = rtosc_argument(msg, 0).s;
                XMLwrapper xml;
                Master::saveAutomation(xml, impl.master->automate);
                xml.saveXMLfile(file, impl.master->gzip_compression);
                });
        rEnd},
    {"load_xlz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        XMLwrapper xml;
        xml.loadXMLfile(file);
        rtosc::AutomationMgr *mgr = new rtosc::AutomationMgr(16,4,8);
        mgr->set_ports(Master::ports);
        Master::loadAutomation(xml, *mgr);
        d.chain("/automate/load-blob", "b", sizeof(void*), &mgr);
        rEnd},
    {"clear_xlz:", 0, 0,
        rBegin;
        d.chain("/automate/clear", "");
        rEnd},
    //scale file stuff
    {"load_xsz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        impl.loadXsz(file, d);
        rEnd},
    {"save_xsz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        impl.saveXsz(file, d);
        rEnd},
    {"load_scl:s", rDoc("Load a scale from a file"), 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        impl.loadScl(file, d);
        rEnd},
    {"load_kbm:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        impl.loadKbm(file, d);
        rEnd},
    {"save_xmz:s:st", 0, 0, save_cb<false>},
    {"save_osc:s:st", 0, 0, save_cb<true>},
    {"save_xiz:is", 0, 0,
        rBegin;
        const int   part_id = rtosc_argument(msg,0).i;
        const char *file    = rtosc_argument(msg,1).s;
        impl.savePart(part_id, file);
        rEnd},
    {"file_home_dir:", 0, 0,
        rBegin;
        const char *home = getenv("PWD");
        if(!home)
            home = getenv("HOME");
        if(!home)
            home = getenv("USERPROFILE");
        if(!home)
            home = getenv("HOMEPATH");
        if(!home)
            home = "/";

        string home_ = home;
#ifndef WIN32
        if(home_[home_.length()-1] != '/')
            home_ += '/';
#endif
        d.reply(d.loc, "s", home_.c_str());
        rEnd},
    {"file_list_files:s", 0, 0,
        rBegin;
        const char *folder = rtosc_argument(msg, 0).s;

        auto files = getFiles(folder, false);

        const int N = files.size();
        rtosc_arg_t *args  = new rtosc_arg_t[N];
        char        *types = new char[N+1];
        gcc_10_1_0_is_dumb(files, N, types, args);

        d.replyArray(d.loc, types, args);
        delete [] types;
        delete [] args;
        rEnd},
    {"file_list_dirs:s", 0, 0,
        rBegin;
        const char *folder = rtosc_argument(msg, 0).s;

        auto files = getFiles(folder, true);

        const int N = files.size();
        rtosc_arg_t *args  = new rtosc_arg_t[N];
        char        *types = new char[N+1];
        gcc_10_1_0_is_dumb(files, N, types, args);

        d.replyArray(d.loc, types, args);
        delete [] types;
        delete [] args;
        rEnd},
    {"reload_auto_save:i", 0, 0,
        rBegin
        const int save_id      = rtosc_argument(msg,0).i;
        const string save_dir  = string(getenv("HOME")) + "/.local";
        const string save_file = "zynaddsubfx-"+to_s(save_id)+"-autosave.xmz";
        const string save_loc  = save_dir + "/" + save_file;
        impl.loadMaster(save_loc.c_str());
        //XXX it would be better to remove the autosave after there is a new
        //autosave, but this method should work for non-immediate crashes :-|
        remove(save_loc.c_str());
        rEnd},
    {"delete_auto_save:i", 0, 0,
        rBegin
        const int save_id      = rtosc_argument(msg,0).i;
        const string save_dir  = string(getenv("HOME")) + "/.local";
        const string save_file = "zynaddsubfx-"+to_s(save_id)+"-autosave.xmz";
        const string save_loc  = save_dir + "/" + save_file;
        remove(save_loc.c_str());
        rEnd},
    {"load_xmz:s:st", 0, 0, load_cb<false>},
    {"load_osc:s:st", 0, 0, load_cb<true>},
    {"reset_master:", 0, 0,
        rBegin;
        impl.loadMaster(NULL);
        d.broadcast("/damage", "s", "/");
        rEnd},
    {"load_xiz:is", 0, 0,
        rBegin;
        const int part_id = rtosc_argument(msg,0).i;
        const char *file  = rtosc_argument(msg,1).s;
        impl.pending_load[part_id]++;
        impl.loadPart(part_id, file, impl.master, d);
        rEnd},
    {"load-part:is", 0, 0,
        rBegin;
        const int part_id = rtosc_argument(msg,0).i;
        const char *file  = rtosc_argument(msg,1).s;
        impl.pending_load[part_id]++;
        impl.loadPart(part_id, file, impl.master, d);
        rEnd},
    {"load-part:iss", 0, 0,
        rBegin;
        const int part_id = rtosc_argument(msg,0).i;
        const char *file  = rtosc_argument(msg,1).s;
        const char *name  = rtosc_argument(msg,2).s;
        impl.pending_load[part_id]++;
        impl.loadPart(part_id, file, impl.master, d);
        impl.uToB->write(("/part"+to_s(part_id)+"/Pname").c_str(), "s",
                name);
        rEnd},
    {"setprogram:i:c", 0, 0,
        rBegin;
        Bank &bank     = impl.master->bank;
        const int slot = rtosc_argument(msg, 0).i + 128*bank.bank_lsb;
        if(slot < BANK_SIZE) {
            impl.pending_load[0]++;
            impl.loadPart(0, impl.master->bank.ins[slot].filename.c_str(), impl.master, d);
            impl.uToB->write("/part0/Pname", "s", impl.master->bank.ins[slot].name.c_str());
        }
        rEnd},
    {"part#16/clear:", 0, 0,
        rBegin;
        int id = extractInt(msg);
        impl.loadClearPart(id);
        d.broadcast("/damage", "s", ("/part"+to_s(id)).c_str());
        rEnd},
    {"undo:", 0, 0,
        rBegin;
        impl.undo.seekHistory(-1);
        rEnd},
    {"redo:", 0, 0,
        rBegin;
        impl.undo.seekHistory(+1);
        rEnd},
    //port to observe the midi mappings
    {"mlearn-values:", 0, 0,
        rBegin;
        auto &midi  = impl.midi_mapper;
        auto  key   = keys(midi.inv_map);
        //cc-id, path, min, max
#define MAX_MIDI 32
        rtosc_arg_t args[MAX_MIDI*4];
        char        argt[MAX_MIDI*4+1] = {};
        int j=0;
        for(unsigned i=0; i<key.size() && i<MAX_MIDI; ++i) {
            auto par = midi.inv_map[key[i]];
            if(std::get<1>(par) == -1)
                continue;
            auto bounds = midi.getBounds(key[i].c_str());
            argt[4*j+0]   = 'i';
            args[4*j+0].i = std::get<1>(par);
            argt[4*j+1]   = 's';
            args[4*j+1].s = key[i].c_str();
            argt[4*j+2]   = 'f';
            args[4*j+2].f = std::get<0>(bounds);
            argt[4*j+3]   = 'f';
            args[4*j+3].f = std::get<1>(bounds);
            j++;

        }
        d.replyArray(d.loc, argt, args);
#undef  MAX_MIDI
        rEnd},
    {"mlearn:s", 0, 0,
        rBegin;
        string addr = rtosc_argument(msg, 0).s;
        auto &midi  = impl.midi_mapper;
        auto map    = midi.getMidiMappingStrings();
        if(map.find(addr) != map.end())
            midi.map(addr.c_str(), false);
        else
            midi.map(addr.c_str(), true);
        rEnd},
    {"munlearn:s", 0, 0,
        rBegin;
        string addr = rtosc_argument(msg, 0).s;
        auto &midi  = impl.midi_mapper;
        auto map    = midi.getMidiMappingStrings();
        midi.unMap(addr.c_str(), false);
        midi.unMap(addr.c_str(), true);
        rEnd},
    //drop this message into the abyss
    {"ui/title:", 0, 0, [](const char *, RtData &) {}},
    {"quit:", rDoc("Stops the Zynaddsubfx process"),
     0, [](const char *, RtData&) {Pexitprogram = 1;}},
    // may only be called when Master is not being run
    {"change-synth:iiit", 0, 0,
        rBegin
        // save all data, overwrite all params defining SYNTH,
        // restart the master and load all data back into it

        char* data = nullptr;
        impl.master->getalldata(&data);
        delete impl.master;

        impl.synth.samplerate = (unsigned)rtosc_argument(msg, 0).i;
        impl.synth.buffersize = rtosc_argument(msg, 1).i;
        impl.synth.oscilsize = rtosc_argument(msg, 2).i;
        impl.synth.alias();

        impl.recreateMinimalMaster();
        impl.master->defaults();
        impl.master->putalldata(data);
        impl.master->applyparameters();
        impl.master->initialize_rt();
        impl.updateResources(impl.master);

        d.broadcast("/change-synth", "t", rtosc_argument(msg, 3).t);
        rEnd
    }
};

static rtosc::MergePorts middwareSnoopPorts =
{
    &nonRtParamPorts,
    &middwareSnoopPortsWithoutNonRtParams
};

const rtosc::MergePorts allPorts =
{
    // order is important: params should be queried on Master first
    // (because MiddleWare often just redirects, hiding the metadata)
    &Master::ports,
    &middwareSnoopPorts
};
const rtosc::Ports& getNonRtParamPorts() { return nonRtParamPorts; }
const rtosc::MergePorts& MiddleWare::getAllPorts() { return allPorts; }

static rtosc::Ports middlewareReplyPorts = {
    {"echo:ss", 0, 0,
        rBegin;
        const char *type = rtosc_argument(msg, 0).s;
        const char *url  = rtosc_argument(msg, 1).s;
        if(!strcmp(type, "OSC_URL"))
            impl.currentUrl(url);
        rEnd},
    {"free:sb", 0, 0,
        rBegin;
        const char *type = rtosc_argument(msg, 0).s;
        void       *ptr  = *(void**)rtosc_argument(msg, 1).b.data;
        deallocate(type, ptr);
        rEnd},
    {"request-memory:", 0, 0,
        rBegin;
        //Generate out more memory for the RT memory pool
        //5MBi chunk
        size_t N  = 5*1024*1024;
        void *mem = malloc(N);
        impl.uToB->write("/add-rt-memory", "bi", sizeof(void*), &mem, N);
        rEnd},
    {"setprogram:cc:ii", 0, 0,
        rBegin;
        Bank &bank        = impl.master->bank;
        const int part    = rtosc_argument(msg, 0).i;
        const int program = rtosc_argument(msg, 1).i + 128*bank.bank_lsb;
        if (program >= BANK_SIZE) {
            fprintf(stderr, "bank:program number %d:%d is out of range.",
                    program >> 7, program & 0x7f);
            return;
        }
        const char *fn  = impl.master->bank.ins[program].filename.c_str();
        impl.loadPart(part, fn, impl.master, d);
        impl.uToB->write(("/part"+to_s(part)+"/Pname").c_str(), "s",
                         fn ? impl.master->bank.ins[program].name.c_str() : "");
        rEnd},
    {"setbank:c", 0, 0,
        rBegin;
        impl.loadPendingBank(rtosc_argument(msg,0).i, impl.master->bank);
        rEnd},
    {"undo_pause:", 0, 0, rBegin; impl.recording_undo = false; rEnd},
    {"undo_resume:", 0, 0, rBegin; impl.recording_undo = true; rEnd},
    {"undo_change", 0, 0,
        rBegin;
        if(impl.recording_undo)
            impl.undo.recordEvent(msg);
        rEnd},
    {"midi-use-CC:i", 0, 0,
        rBegin;
        impl.midi_mapper.useFreeID(rtosc_argument(msg, 0).i);
        rEnd},
    {"broadcast:", 0, 0, rBegin; impl.broadcast = true; rEnd},
    {"forward:", 0, 0, rBegin; impl.forward = true; rEnd},
};
#undef rBegin
#undef rEnd

/******************************************************************************
 *                         MiddleWare Implementation                          *
 ******************************************************************************/

MiddleWareImpl::MiddleWareImpl(MiddleWare *mw, SYNTH_T synth_,
    Config* config, int preferrred_port)
    :parent(mw), config(config), ui(nullptr), synth(std::move(synth_)),
    presetsstore(*config), autoSave(-1, [this]() {
            auto master = this->master;
            this->doReadOnlyOp([master](){
                std::string home = getenv("HOME");
                std::string save_file = home+"/.local/zynaddsubfx-"+to_s(getpid())+"-autosave.xmz";
                printf("doing an autosave <%s>...\n", save_file.c_str());
                int res = master->saveXML(save_file.c_str());
                (void)res;});})
{
    bToU = new rtosc::ThreadLink(4096*2*16,1024/16);
    uToB = new rtosc::ThreadLink(4096*2*16,1024/16);
    midi_mapper.base_ports = &Master::ports;
    midi_mapper.rt_cb      = [this](const char *msg){handleMsg(msg);};
    if(preferrred_port != -1)
        server = lo_server_new_with_proto(to_s(preferrred_port).c_str(),
                                          LO_UDP, liblo_error_cb);
    else
        server = lo_server_new_with_proto(NULL, LO_UDP, liblo_error_cb);

    if(server) {
        lo_server_add_method(server, NULL, NULL, handler_function, mw);
        fprintf(stderr, "lo server running on %d\n", lo_server_get_port(server));
    } else
        fprintf(stderr, "lo server could not be started :-/\n");


    //dummy callback for starters
    cb = [](void*, const char*){};
    idle = 0;
    idle_ptr = 0;

    recreateMinimalMaster();
    osc    = GUI::genOscInterface(mw);

    //Grab objects of interest from master
    updateResources(master);

    //Null out Load IDs
    for(int i=0; i < NUM_MIDI_PARTS; ++i) {
        pending_load[i] = 0;
        actual_load[i] = 0;
    }

    //Setup Undo
    undo.setCallback([this](const char *msg) {
           // printf("undo callback <%s>\n", msg);
            char buf[1024];
            rtosc_message(buf, 1024, "/undo_pause","");
            handleMsg(buf);
            handleMsg(msg);
            rtosc_message(buf, 1024, "/undo_resume","");
            handleMsg(buf);
            });

    //Setup starting time
    struct timespec time;
    monotonic_clock_gettime(&time);
    start_time_sec  = time.tv_sec;
    start_time_nsec = time.tv_nsec;

    offline = false;
}

MiddleWareImpl::~MiddleWareImpl(void)
{

    if(server)
        lo_server_free(server);

    delete master;
    delete osc;
    delete bToU;
    delete uToB;

}

void zyn::MiddleWareImpl::recreateMinimalMaster()
{
    master = new Master(synth, config);
    master->bToU = bToU;
    master->uToB = uToB;
}

/** Threading When Saving
 *  ----------------------
 *
 * Procedure Middleware:
 *   1) Middleware sends /freeze_state to backend
 *   2) Middleware waits on /state_frozen from backend
 *      All intervening commands are held for out of order execution
 *   3) Acquire memory
 *      At this time by the memory barrier we are guaranteed that all old
 *      writes are done and assuming the freezing logic is sound, then it is
 *      impossible for any other parameter to change at this time
 *   3) Middleware performs saving operation
 *   4) Middleware sends /thaw_state to backend
 *   5) Restore in order execution
 *
 * Procedure Backend:
 *   1) Observe /freeze_state and disable all mutating events (MIDI CC)
 *   2) Run a memory release to ensure that all writes are complete
 *   3) Send /state_frozen to Middleware
 *   time...
 *   4) Observe /thaw_state and resume normal processing
 */

void MiddleWareImpl::doReadOnlyOp(std::function<void()> read_only_fn)
{
    assert(uToB);
    uToB->write("/freeze_state","");

    std::list<const char *> fico;
    int tries = 0;
    while(tries++ < 10000) {
        if(!bToU->hasNext()) {
            os_usleep(500);
            continue;
        }
        const char *msg = bToU->read();
        if(!strcmp("/state_frozen", msg))
            break;
        size_t bytes = rtosc_message_length(msg, bToU->buffer_size());
        char *save_buf = new char[bytes];
        memcpy(save_buf, msg, bytes);
        fico.push_back(save_buf);
    }

    assert(tries < 10000);//if this happens, the backend must be dead

    std::atomic_thread_fence(std::memory_order_acquire);

    //Now it is safe to do any read only operation
    read_only_fn();

    //Now to resume normal operations
    uToB->write("/thaw_state","");
    for(auto x:fico) {
        uToB->raw_write(x);
        delete [] x;
    }
}

//Offline detection code:
// - Assume that the audio callback should be run at least once every 50ms
// - Atomically provide the number of ms since start to Master
// - Every time middleware ticks provide a heart beat
// - If when the heart beat is provided the backend is more than 200ms behind
//   the last heartbeat then it must be offline
// - When marked offline the backend doesn't receive another heartbeat until it
//   registers the current beat that it's behind on
void MiddleWareImpl::heartBeat(Master *master)
{
    //Current time
    //Last provided beat
    //Last acknowledged beat
    //Current offline status

    struct timespec time;
    monotonic_clock_gettime(&time);
    uint32_t now = (time.tv_sec-start_time_sec)*100 +
                   (time.tv_nsec-start_time_nsec)*1e-9*100;
    int32_t last_ack   = master->last_ack;
    int32_t last_beat  = master->last_beat;

    //everything is considered online for the first second
    if(now < 100)
        return;

    if(offline) {
        if(last_beat == last_ack) {
            //XXX INSERT MESSAGE HERE ABOUT TRANSITION TO ONLINE
            offline = false;

            //Send new heart beat
            master->last_beat = now;
        }
    } else {
        //it's unquestionably alive
        if(last_beat == last_ack) {

            //Send new heart beat
            master->last_beat = now;
            return;
        }

        //it's pretty likely dead
        if(last_beat-last_ack > 0 && now-last_beat > 20) {
            //The backend has had 200 ms to acquire a new beat
            //The backend instead has an older beat
            //XXX INSERT MESSAGE HERE ABOUT TRANSITION TO OFFLINE
            offline = true;
            return;
        }

        //who knows if it's alive or not here, give it a few ms to acquire or
        //not
    }

}

void MiddleWareImpl::doReadOnlyOpPlugin(std::function<void()> read_only_fn)
{
    assert(uToB);
    int offline = 0;
    if(offline) {
        std::atomic_thread_fence(std::memory_order_acquire);

        //Now it is safe to do any read only operation
        read_only_fn();
    } else if(!doReadOnlyOpNormal(read_only_fn, true)) {
        //check if we just transitioned to offline mode

        std::atomic_thread_fence(std::memory_order_acquire);

        //Now it is safe to do any read only operation
        read_only_fn();
    }
}

bool MiddleWareImpl::doReadOnlyOpNormal(std::function<void()> read_only_fn, bool canfail)
{
    assert(uToB);
    uToB->write("/freeze_state","");

    std::list<const char *> fico;
    int tries = 0;
    while(tries++ < 2000) {
        if(!bToU->hasNext()) {
            os_usleep(500);
            continue;
        }
        const char *msg = bToU->read();
        if(!strcmp("/state_frozen", msg))
            break;
        size_t bytes = rtosc_message_length(msg, bToU->buffer_size());
        char *save_buf = new char[bytes];
        memcpy(save_buf, msg, bytes);
        fico.push_back(save_buf);
    }

    if(canfail) {
        //Now to resume normal operations
        uToB->write("/thaw_state","");
        for(auto x:fico) {
            uToB->raw_write(x);
            delete [] x;
        }
        return false;
    }

    assert(tries < 10000);//if this happens, the backend must be dead

    std::atomic_thread_fence(std::memory_order_acquire);

    //Now it is safe to do any read only operation
    read_only_fn();

    //Now to resume normal operations
    uToB->write("/thaw_state","");
    for(auto x:fico) {
        uToB->raw_write(x);
        delete [] x;
    }
    return true;
}

void MiddleWareImpl::broadcastToRemote(const char *rtmsg)
{
    //Always send to the local UI
    sendToRemote(rtmsg, "GUI");

    //Send to remote UI if there's one listening
    for(auto rem:known_remotes)
        if(rem != "GUI")
            sendToRemote(rtmsg, rem);

    broadcast = false;
}

void MiddleWareImpl::sendToRemote(const char *rtmsg, std::string dest)
{
    if(!rtmsg || rtmsg[0] != '/' || !rtosc_message_length(rtmsg, -1)) {
        printf("[Warning] Invalid message in sendToRemote <%s>...\n", rtmsg);
        return;
    }

    //printf("sendToRemote(%s:%s,%s)\n", rtmsg, rtosc_argument_string(rtmsg),
    //        dest.c_str());
    if(dest == "GUI") {
        cb(ui, rtmsg);
    } else if(!dest.empty()) {
        lo_message msg  = lo_message_deserialise((void*)rtmsg,
                rtosc_message_length(rtmsg, bToU->buffer_size()), NULL);
        if(!msg) {
            printf("[ERROR] OSC to <%s> Failed To Parse In Liblo\n", rtmsg);
            return;
        }

        //Send to known url
        lo_address addr = lo_address_new_from_url(dest.c_str());
        if(addr)
            lo_send_message(addr, rtmsg, msg);
        lo_address_free(addr);
        lo_message_free(msg);
    }
}

/**
 * Handle all events coming from the backend
 *
 * This includes forwarded events which need to be retransmitted to the backend
 * after the snooping code inspects the message
 */
void MiddleWareImpl::bToUhandle(const char *rtmsg)
{
    //Verify Message isn't a known corruption bug
    assert(strcmp(rtmsg, "/part0/kit0/Ppadenableda"));
    assert(strcmp(rtmsg, "/ze_state"));

    //Dump Incoming Events For Debugging
    if(strcmp(rtmsg, "/vu-meter") && false) {
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 1 + 30, 0 + 40);
        fprintf(stdout, "frontend[%c]: '%s'<%s>\n", forward?'f':broadcast?'b':'N',
                rtmsg, rtosc_argument_string(rtmsg));
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    //Activity dot
    //printf(".");fflush(stdout);

    MwDataObj d(this);
    middlewareReplyPorts.dispatch(rtmsg, d, true);

    if(!rtmsg) {
        fprintf(stderr, "[ERROR] Unexpected Null OSC In Zyn\n");
        return;
    }

    in_order = true;
    //Normal message not captured by the ports
    if(d.matches == 0) {
        if(forward) {
            forward = false;
            handleMsg(rtmsg, true);
        } if(broadcast)
            broadcastToRemote(rtmsg);
        else
            sendToCurrentRemote(rtmsg);
    }
    in_order = false;

}

//Allocate kits on a as needed basis
void MiddleWareImpl::kitEnable(const char *msg)
{
    const string argv = rtosc_argument_string(msg);
    if(argv != "T")
        return;
    //Extract fields from:
    //BASE/part#/kit#/Pxxxenabled
    int type = -1;
    if(strstr(msg, "Padenabled"))
        type = 0;
    else if(strstr(msg, "Ppadenabled"))
        type = 1;
    else if(strstr(msg, "Psubenabled"))
        type = 2;
    else
        return;

    int part, kit;
    bool res = idsFromMsg(msg, &part, &kit, nullptr);
    assert(res);
    kitEnable(part, kit, type);
}

void MiddleWareImpl::kitEnable(int part, int kit, int type)
{
    //printf("attempting a kit enable<%d,%d,%d>\n", part, kit, type);
    string url = "/part"+to_s(part)+"/kit"+to_s(kit)+"/";
    void *ptr = NULL;
    if(type == 0 && kits.add[part][kit] == NULL) {
        ptr = kits.add[part][kit] = new ADnoteParameters(synth, master->fft,
                                                         &master->time);
        url += "adpars-data";
        obj_store.extractAD(kits.add[part][kit], part, kit);
    } else if(type == 1 && kits.pad[part][kit] == NULL) {
        ptr = kits.pad[part][kit] = new PADnoteParameters(synth, master->fft,
                                                          &master->time);
        url += "padpars-data";
        obj_store.extractPAD(kits.pad[part][kit], part, kit);
    } else if(type == 2 && kits.sub[part][kit] == NULL) {
        ptr = kits.sub[part][kit] = new SUBnoteParameters(&master->time);
        url += "subpars-data";
    }

    //Send the new memory
    if(ptr)
        uToB->write(url.c_str(), "b", sizeof(void*), &ptr);
}


/*
 * Handle all messages traveling to the realtime side.
 */
void MiddleWareImpl::handleMsg(const char *msg, bool msg_comes_from_realtime)
{
    //Check for known bugs
    assert(msg && *msg && strrchr(msg, '/')[1]);
    assert(strstr(msg,"free") == NULL || strstr(rtosc_argument_string(msg), "b") == NULL);
    assert(strcmp(msg, "/part0/Psysefxvol"));
    assert(strcmp(msg, "/Penabled"));
    assert(strcmp(msg, "part0/part0/Ppanning"));
    assert(strcmp(msg, "sysefx0sysefx0/preset"));
    assert(strcmp(msg, "/sysefx0preset"));
    assert(strcmp(msg, "Psysefxvol0/part0"));

    if(strcmp("/get-vu", msg) && false) {
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 6 + 30, 0 + 40);
        fprintf(stdout, "middleware: '%s':%s\n", msg, rtosc_argument_string(msg));
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    const char *last_path = strrchr(msg, '/');
    if(!last_path) {
        printf("Bad message in handleMsg() <%s>\n", msg);
        assert(false);
        return;
    }

    MwDataObj d(this);
    middwareSnoopPorts.dispatch(msg, d, true);

    //A message unmodified by snooping
    if(d.matches == 0 || d.forwarded) {
        if(msg_comes_from_realtime) {
            // don't reply the same msg to realtime - avoid cycles
            //printf("Message from RT will not be replied to RT: <%s:%s>...\n",
            //       msg, rtosc_argument_string(msg));
        } else {
            //if(strcmp("/get-vu", msg)) {
            //    printf("Message Continuing on<%s:%s>...\n", msg, rtosc_argument_string(msg));
            //}
            uToB->raw_write(msg);
        }
    } else {
        //printf("Message Handled<%s:%s>...\n", msg, rtosc_argument_string(msg));
    }

    // now handle all chained messages
    while(!msgsToHandle.empty())
    {
        std::vector<char> front = msgsToHandle.front();
        msgsToHandle.pop();
        handleMsg(front.data());
    }
}

void MiddleWareImpl::write(const char *path, const char *args, ...)
{
    //We have a free buffer in the threadlink, so use it
    va_list va;
    va_start(va, args);
    write(path, args, va);
    va_end(va);
}

void MiddleWareImpl::write(const char *path, const char *args, va_list va)
{
    //printf("is that a '%s' I see there?\n", path);
    char *buffer = uToB->buffer();
    unsigned len = uToB->buffer_size();
    bool success = rtosc_vmessage(buffer, len, path, args, va);
    //printf("working on '%s':'%s'\n",path, args);

    if(success)
        handleMsg(buffer);
    else
        warnx("Failed to write message to '%s'", path);
}

/******************************************************************************
 *                         MidleWare Forwarding Stubs                         *
 ******************************************************************************/
MiddleWare::MiddleWare(SYNTH_T synth, Config* config,
                       int preferred_port)
:impl(new MiddleWareImpl(this, std::move(synth), config, preferred_port))
{}

MiddleWare::~MiddleWare(void)
{
    delete impl;
}

void MiddleWare::updateResources(Master *m)
{
    impl->updateResources(m);
}

Master *MiddleWare::spawnMaster(void)
{
    assert(impl->master);
    assert(impl->master->uToB);
    return impl->master;
}

void MiddleWare::enableAutoSave(int interval_sec)
{
    impl->autoSave.dt = interval_sec;
}

int MiddleWare::checkAutoSave(void) const
{
    //save spec zynaddsubfx-PID-autosave.xmz
    const std::string home     = getenv("HOME");
    const std::string save_dir = home+"/.local/";

    DIR *dir = opendir(save_dir.c_str());

    if(dir == NULL)
        return -1;

    struct dirent *fn;
    int    reload_save = -1;

    while((fn = readdir(dir))) {
        const char *filename = fn->d_name;
        const char *prefix = "zynaddsubfx-";

        //check for manditory prefix
        if(strstr(filename, prefix) != filename)
            continue;

        int id = atoi(filename+strlen(prefix));

        bool in_use = false;

        std::string proc_file = "/proc/" + to_s(id) + "/comm";
        std::ifstream ifs(proc_file);
        if(ifs.good()) {
            std::string comm_name;
            ifs >> comm_name;
            in_use = (comm_name == "zynaddsubfx");
        }

        if(!in_use) {
            reload_save = id;
            break;
        }
    }

    closedir(dir);

    return reload_save;
}

void MiddleWare::removeAutoSave(void)
{
    std::string home = getenv("HOME");
    std::string save_file = home+"/.local/zynaddsubfx-"+to_s(getpid())+"-autosave.xmz";
    remove(save_file.c_str());
}

Fl_Osc_Interface *MiddleWare::spawnUiApi(void)
{
    return impl->osc;
}

void MiddleWare::tick(void)
{
    impl->tick();
}

void MiddleWare::doReadOnlyOp(std::function<void()> fn)
{
    impl->doReadOnlyOp(fn);
}

void MiddleWare::setUiCallback(void(*cb)(void*,const char *), void *ui)
{
    impl->cb = cb;
    impl->ui = ui;
}

void MiddleWare::setIdleCallback(void(*cb)(void*), void *ptr)
{
    impl->idle     = cb;
    impl->idle_ptr = ptr;
}

void MiddleWare::transmitMsg(const char *msg)
{
    impl->handleMsg(msg);
}

void MiddleWare::transmitMsg(const char *path, const char *args, ...)
{
    char buffer[1024];
    va_list va;
    va_start(va,args);
    if(rtosc_vmessage(buffer,1024,path,args,va))
        transmitMsg(buffer);
    else
        fprintf(stderr, "Error in transmitMsg(...)\n");
    va_end(va);
}

void MiddleWare::transmitMsg_va(const char *path, const char *args, va_list va)
{
    char buffer[1024];
    if(rtosc_vmessage(buffer, 1024, path, args, va))
        transmitMsg(buffer);
    else
        fprintf(stderr, "Error in transmitMsg(va)n");
}

void MiddleWare::messageAnywhere(const char *path, const char *args, ...)
{
    auto *mem = impl->multi_thread_source.alloc();
    if(!mem)
        fprintf(stderr, "Middleware::messageAnywhere memory pool out of memory...\n");

    va_list va;
    va_start(va,args);
    if(rtosc_vmessage(mem->memory,mem->size,path,args,va))
        impl->multi_thread_source.write(mem);
    else {
        fprintf(stderr, "Middleware::messageAnywhere message too big...\n");
        impl->multi_thread_source.free(mem);
    }
}


void MiddleWare::pendingSetBank(int bank)
{
    impl->bToU->write("/setbank", "c", bank);
}
void MiddleWare::pendingSetProgram(int part, int program)
{
    impl->pending_load[part]++;
    impl->bToU->write("/setprogram", "cc", part, program);
}

std::string zyn::MiddleWare::getProgramName(int program) const
{
    return impl->master->bank.ins[program].name;
}

std::string MiddleWare::activeUrl(void) const
{
    return impl->last_url;
}

void MiddleWare::activeUrl(std::string u)
{
    impl->last_url = u;
}

const SYNTH_T &MiddleWare::getSynth(void) const
{
    return impl->synth;
}

char* MiddleWare::getServerAddress(void) const
{
    if(impl->server)
        return lo_server_get_url(impl->server);
    else
        return nullptr;
}

char* MiddleWare::getServerPort(void) const
{
    char* addr = getServerAddress();
    char* result;
    if(addr)
    {
        result = lo_url_get_port(addr);
        free(addr);
    }
    else
        result = nullptr;
    return result;
}

const PresetsStore& MiddleWare::getPresetsStore() const
{
    return impl->presetsstore;
}

PresetsStore& MiddleWare::getPresetsStore()
{
    return impl->presetsstore;
}

void MiddleWare::switchMaster(Master* new_master)
{
    // this function is kept similar to loadMaster
    assert(impl->master->frozenState);

    new_master->uToB = impl->uToB;
    new_master->bToU = impl->bToU;
    impl->updateResources(new_master);
    impl->master = new_master;

    if(impl->master->hasMasterCb())
    {
        // inform the realtime thread about the switch
        // this will be done by calling the mastercb
        transmitMsg("/switch-master", "b", sizeof(Master*), &new_master);
    }
}

}

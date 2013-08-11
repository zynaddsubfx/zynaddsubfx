#include "MiddleWare.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <rtosc/thread-link.h>
#include <rtosc/ports.h>
#include <lo/lo.h>

#include <unistd.h>

#include "../UI/Fl_Osc_Interface.h"
#include "../UI/Fl_Osc_Widget.H"

#include <map>

#include "Master.h"
#include "Part.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/PADnoteParameters.h"

#include <string>

#include <err.h>

rtosc::ThreadLink *bToU = new rtosc::ThreadLink(4096*2,1024);
rtosc::ThreadLink *uToB = new rtosc::ThreadLink(4096*2,1024);

/**
 * General local static code/data
 */
static lo_server server;
static string last_url, curr_url;

static void liblo_error_cb(int i, const char *m, const char *loc)
{
    fprintf(stderr, "liblo :-( %d-%s@%s\n",i,m,loc);
}

void path_search(const char *m)
{
    using rtosc::Ports;
    using rtosc::Port;

    //assumed upper bound of 32 ports (may need to be resized)
    char         types[65];
    rtosc_arg_t  args[64];
    size_t       pos    = 0;
    const Ports *ports  = NULL;
    const char  *str    = rtosc_argument(m,0).s;
    const char  *needle = rtosc_argument(m,1).s;

    //zero out data
    memset(types, 0, sizeof(types));
    memset(args,  0, sizeof(args));

    if(!*str) {
        ports = &Master::ports;
    } else {
        const Port *port = Master::ports.apropos(rtosc_argument(m,0).s);
        if(port)
            ports = port->ports;
    }

    if(ports) {
        //RTness not confirmed here
        for(const Port &p:*ports) {
            if(strstr(p.name, needle)!=p.name)
                continue;
            printf("Reporting '%s'\n", p.name);
            types[pos]    = 's';
            args[pos++].s = p.name;
            types[pos]    = 'b';
            if(p.metadata && *p.metadata) {
                args[pos].b.data = (unsigned char*) p.metadata;
                auto tmp = rtosc::Port::MetaContainer(p.metadata);
                args[pos++].b.len  = tmp.length();
            } else {
                args[pos].b.data = (unsigned char*) NULL;
                args[pos++].b.len  = 0;
            }
        }
    }


    //Reply to requester
    char buffer[1024*4];
    size_t length = rtosc_amessage(buffer, sizeof(buffer), "/paths", types, args);
    printf("ARG COUNT = %d\n", rtosc_narguments(buffer));
    if(length) {
        lo_message msg  = lo_message_deserialise((void*)buffer, length, NULL);
        lo_address addr = lo_address_new_from_url(last_url.c_str());
        if(addr)
            lo_send_message(addr, buffer, msg);
    }
}

static int handler_function(const char *path, const char *types, lo_arg **argv,
        int argc, lo_message msg, void *user_data)
{
    (void) types;
    (void) argv;
    (void) argc;
    (void) user_data;
    lo_address addr = lo_message_get_source(msg);
    if(addr) {
        const char *tmp = lo_address_get_url(addr);
        if(tmp != last_url) {
            uToB->write("/echo", "ss", "OSC_URL", tmp);
            last_url = tmp;
        }

    }

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    size_t size = 2048;
    lo_message_serialise(msg, path, buffer, &size);
    if(!strcmp(buffer, "/path-search") && !strcmp("ss", rtosc_argument_string(buffer))) {
        path_search(buffer);
    } else
        uToB->raw_write(buffer);

    return 0;
}

typedef void(*cb_t)(void*,const char*);


void deallocate(const char *str, void *v)
{
    printf("deallocating a '%s' at '%p'\n", str, v);
    if(!strcmp(str, "Part"))
        delete (Part*)v;
    else if(!strcmp(str, "fft_t"))
        delete[] (fft_t*)v;
    else
        fprintf(stderr, "Unknown type '%s', leaking pointer %p!!\n", str, v);
}



/**
 * - Fetches liblo messages and forward them to the backend
 * - Grabs backend messages and distributes them to the frontends
 */
void osc_check(cb_t cb, void *ui)
{
    lo_server_recv_noblock(server, 0);
    while(bToU->hasNext()) {
        const char *rtmsg = bToU->read();
        //printf("return: got a '%s'\n", rtmsg);
        if(!strcmp(rtmsg, "/echo")
                && !strcmp(rtosc_argument_string(rtmsg),"ss")
                && !strcmp(rtosc_argument(rtmsg,0).s, "OSC_URL"))
            curr_url = rtosc_argument(rtmsg,1).s;
        else if(!strcmp(rtmsg, "/free")
                && !strcmp(rtosc_argument_string(rtmsg),"sb")) {
            deallocate(rtosc_argument(rtmsg, 0).s, *((void**)rtosc_argument(rtmsg, 1).b.data));
        } else if(curr_url == "GUI") {
            cb(ui, rtmsg); //GUI::raiseUi(gui, bToU->read());
        } else{
            lo_message msg  = lo_message_deserialise((void*)rtmsg,
                    rtosc_message_length(rtmsg, bToU->buffer_size()), NULL);

            //Send to known url
            if(!curr_url.empty()) {
                lo_address addr = lo_address_new_from_url(curr_url.c_str());
                lo_send_message(addr, rtmsg, msg);
            }
        }
    }
}



void preparePadSynth(string path, PADnoteParameters *p)
{
    printf("preparing padsynth parameters\n");
    assert(!path.empty());
    path += "sample";

    unsigned max = 0;
    p->sampleGenerator([&max,&path]
            (unsigned N, PADnoteParameters::Sample &s)
            {
            max = max<N ? N : max;
            printf("sending info to '%s'\n", (path+to_s(N)).c_str());
            uToB->write((path+to_s(N)).c_str(), "ifb",
                s.size, s.basefreq, sizeof(float*), &s.smp);
            });
    //clear out unused samples
    for(unsigned i = max+1; i < PAD_MAX_SAMPLES; ++i) {
        uToB->write((path+to_s(i)).c_str(), "ifb",
                0, 440.0f, sizeof(float*), NULL);
    }
}

void refreshBankView(const Bank &bank, unsigned loc, Fl_Osc_Interface *osc)
{
    if(loc >= BANK_SIZE)
        return;

    char response[2048];
    if(!rtosc_message(response, 1024, "/bankview", "iss",
                loc, bank.ins[loc].name.c_str(),
                bank.ins[loc].filename.c_str()))
        errx(1, "Failure to handle bank update properly...");


    osc->tryLink(response);
}

//
//static rtosc::Ports padPorts= {
//    {"prepare:", "::Prepares the padnote instance", 0, }
//};
//
//static rtosc::Ports oscilPorts = {
//    {"prepare:", "", 0, }
//};

class DummyDataObj:public rtosc::RtData
{
    public:
        DummyDataObj(char *loc_, size_t loc_size_, void *obj_, cb_t cb_, void *ui_,
                Fl_Osc_Interface *osc_)
        {
            memset(loc_, 0, sizeof(loc_size_));
            //XXX Possible bug in rtosc using this buffer
            buffer = new char[4*4096];
            memset(buffer, 0, 4*4096);
            loc      = loc_;
            loc_size = loc_size_;
            obj      = obj_;
            cb       = cb_;
            ui       = ui_;
            osc      = osc_;
        }
        ~DummyDataObj(void)
        {
            delete[] buffer;
        }

        virtual void reply(const char *path, const char *args, ...)
        {
            //printf("reply building '%s'\n", path);
            va_list va;
            va_start(va,args);
            if(!strcmp(path, "/forward")) { //forward the information to the backend
                args++;
                path = va_arg(va, const char *);
                fprintf(stderr, "forwarding information to the backend on '%s'<%s>\n",
                        path, args);
                rtosc_vmessage(buffer,4*4096,path,args,va);
                uToB->raw_write(buffer);
            } else {
                //printf("path = '%s' args = '%s'\n", path, args);
                //printf("buffer = '%p'\n", buffer);
                rtosc_vmessage(buffer,4*4096,path,args,va);
                //printf("buffer = '%s'\n", buffer);
                reply(buffer);
            }
        }
        virtual void reply(const char *msg)
        {
            //DEBUG
            //printf("reply used for '%s'\n", msg);
            osc->tryLink(msg);
            cb(ui, msg);
        }
        //virtual void broadcast(const char *path, const char *args, ...){(void)path;(void)args;};
        //virtual void broadcast(const char *msg){(void)msg;};
    private:
        char *buffer;
        cb_t cb;
        void *ui;
        Fl_Osc_Interface *osc;
};


/**
 * Forwarding logic
 * if(!dispatch(msg, data))
 *     forward(msg)
 */


static Fl_Osc_Interface *genOscInterface(struct MiddleWareImpl*);

/* Implementation */
struct MiddleWareImpl
{
    MiddleWareImpl(void)
    {
        server = lo_server_new_with_proto(NULL, LO_UDP, liblo_error_cb);
        lo_server_add_method(server, NULL, NULL, handler_function, NULL);
        fprintf(stderr, "lo server running on %d\n", lo_server_get_port(server));

        //dummy callback for starters
        cb = [](void*, const char*){};

        master = new Master();
        osc    = genOscInterface(this);

        //Grab objects of interest from master
        for(int i=0; i < NUM_MIDI_PARTS; ++i) {
            for(int j=0; j < NUM_KIT_ITEMS; ++j) {
                objmap["/part"+to_s(i)+"/kit"+to_s(j)+"/padpars/"] =
                    master->part[i]->kit[j].padpars;
                objmap["/part"+to_s(i)+"/kit"+to_s(j)+"/padpars/oscil/"] =
                    master->part[i]->kit[j].padpars->oscilgen;
                for(int k=0; k<NUM_VOICES; ++k) {
                    objmap["/part"+to_s(i)+"/kit"+to_s(j)+"/adpars/voice"+to_s(k)+"/oscil/"] =
                        master->part[i]->kit[j].adpars->VoicePar[k].OscilSmp;
                    objmap["/part"+to_s(i)+"/kit"+to_s(j)+"/adpars/voice"+to_s(k)+"/oscil-mod/"] =
                        master->part[i]->kit[j].adpars->VoicePar[k].FMSmp;
                }
            }
        }
    }

    ~MiddleWareImpl(void)
    {
        warnMemoryLeaks();

        delete master;
        delete osc;
    }

    void warnMemoryLeaks(void);

    void loadPart(const char *msg, Master *master)
    {
        fprintf(stderr, "loading a part!!\n");
        //Load the part
        Part *p = new Part(&master->microtonal, master->fft, &master->mutex);
        unsigned npart = rtosc_argument(msg, 0).i;
        fprintf(stderr, "Part is stored in '%s'\n", rtosc_argument(msg, 1).s);
        p->loadXMLinstrument(rtosc_argument(msg, 1).s);
        p->applyparameters();

        //Update the resource locators
        string base = "/part"+to_s(npart)+"/kit";
        for(int j=0; j < NUM_KIT_ITEMS; ++j) {
            objmap[base+to_s(j)+"/padpars/"] =
                p->kit[j].padpars;
            objmap[base+to_s(j)+"/padpars/oscil/"] =
                p->kit[j].padpars->oscilgen;
            for(int k=0; k<NUM_VOICES; ++k) {
                objmap[base+to_s(j)+"/adpars/voice"+to_s(k)+"/oscil/"] =
                    p->kit[j].adpars->VoicePar[k].OscilSmp;
                objmap[base+to_s(j)+"/adpars/voice"+to_s(k)+"/oscil-mod/"] =
                    p->kit[j].adpars->VoicePar[k].FMSmp;
            }
        }

        //Give it to the backend and wait for the old part to return for
        //deallocation
        //printf("writing something to the location called '%s'\n", msg);
        uToB->write("/load-part", "ib", npart, sizeof(Part*), &p);
    }

    void tick(void)
    {
        osc_check(cb, ui);
    }

    bool handlePAD(string path, const char *msg, void *v)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        DummyDataObj d(buffer, 1024, v, cb, ui, osc);
        strcpy(buffer, path.c_str());

        //for(auto &p:OscilGen::ports.ports) {
        //    if(strstr(p.name,msg) && strstr(p.metadata, "realtime") &&
        //            !strcmp("b", rtosc_argument_string(msg))) {
        //        printf("sending along packet '%s'...\n", msg);
        //        return false;
        //    }
        //}

        PADnoteParameters::ports.dispatch(msg, d);
        if(!d.matches) {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
            fprintf(stderr, "Unknown location '%s%s'<%s>\n",
                    path.c_str(), msg, rtosc_argument_string(msg));
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }

        return true;
    }

    bool handleOscil(string path, const char *msg, void *v)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        DummyDataObj d(buffer, 1024, v, cb, ui, osc);
        strcpy(buffer, path.c_str());

        for(auto &p:OscilGen::ports.ports) {
            if(strstr(p.name,msg) && strstr(p.metadata, "realtime") &&
                    !strcmp("b", rtosc_argument_string(msg))) {
                //printf("sending along packet '%s'...\n", msg);
                return false;
            }
        }

        OscilGen::ports.dispatch(msg, d);
        if(!d.matches) {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
            fprintf(stderr, "Unknown location '%s%s'<%s>\n",
                    path.c_str(), msg, rtosc_argument_string(msg));
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }

        return true;
    }

    /* Should handle the following paths specially
     * as they are all fundamentally non-realtime data
     *
     * BASE/part#/kititem#
     * BASE/part#/kit#/adpars/voice#/oscil/\*
     * BASE/part#/kit#/adpars/voice#/mod-oscil/\*
     * BASE/part#/kit#/padpars/prepare
     * BASE/part#/kit#/padpars/oscil/\*
     */
    void handleMsg(const char *msg)
    {
        assert(!strstr(msg,"free"));
        //fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 6 + 30, 0 + 40);
        //fprintf(stdout, "middleware: '%s'\n", msg);
        //fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        const char *last_path = rindex(msg, '/');
        if(!last_path)
            return;


        //printf("watching '%s' go by\n", msg);
        //Get the object resource locator
        string obj_rl(msg, last_path+1);
        //XXX Utilize object map for this as well
        //This dereference will become unsafe when loading new master instances
        //is supported
        if(!strcmp(msg, "/refresh_bank") && !strcmp(rtosc_argument_string(msg), "i"))
            refreshBankView(master->bank, rtosc_argument(msg,0).i, osc);
        else if(objmap.find(obj_rl) != objmap.end()) {
            //try some over simplified pattern matching
            if(strstr(msg, "oscil/")) {
                if(!handleOscil(obj_rl, last_path+1, objmap[obj_rl]))
                    uToB->raw_write(msg);
            //else if(strstr(obj_rl.c_str(), "kititem"))
            //    handleKitItem(obj_rl, objmap[obj_rl],atoi(rindex(msg,'m')+1),rtosc_argument(msg,0).T);
            } else if(strstr(msg, "padpars/prepare"))
                preparePadSynth(obj_rl,(PADnoteParameters *) objmap[obj_rl]);
            else if(strstr(msg, "padpars")) {
                if(!handlePAD(obj_rl, last_path+1, objmap[obj_rl]))
                    uToB->raw_write(msg);
            } else //just forward the message
                uToB->raw_write(msg);
        } else if(strstr(msg, "load-part"))
            loadPart(msg, master);
        else
            uToB->raw_write(msg);
    }


    void write(const char *path, const char *args, ...)
    {
        //We have a free buffer in the threadlink, so use it
        va_list va;
        va_start(va, args);
        write(path, args, va);
    }

    void write(const char *path, const char *args, va_list va)
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

    //Ports
    //oscil - base path, kit, voice, name
    //pad   - base path, kit

    //Actions that may be done on these objects
    //Oscilgen almost all parameters can be safely set
    //Padnote can have anything set on its oscilgen and a very small set of general
    //parameters

    //Provides a mapping for non-RT objects stored inside the backend
    /**
     * TODO These pointers may invalidate when part/master is loaded via xml
     */
    std::map<std::string, void*> objmap;

    //This code will own the pointer to master, be prepared for odd things if
    //this assumption is broken
    Master *master;

    //The ONLY means that any chunk of UI code should have for interacting with the
    //backend
    Fl_Osc_Interface *osc;

    cb_t cb;
    void *ui;
};

/**
 * Interface to the middleware layer via osc packets
 */
class UI_Interface:public Fl_Osc_Interface
{
    public:
        UI_Interface(MiddleWareImpl *impl_)
            :impl(impl_)
        {}

        void requestValue(string s) override
        {
            //Fl_Osc_Interface::requestValue(s);
            if(last_url != "GUI") {
                impl->write("/echo", "ss", "OSC_URL", "GUI");
                last_url = "GUI";
            }

            impl->write(s.c_str(),"");
        }

        void write(string s, const char *args, ...) override
        {
            va_list va;
            va_start(va, args);
            impl->write(s.c_str(), args, va);
        }

        void writeRaw(const char *msg) override
        {
            impl->handleMsg(msg);
        }

        void writeValue(string s, string ss) override
        {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            fprintf(stderr, "writevalue<string>(%s,%s)\n", s.c_str(),ss.c_str());
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            impl->write(s.c_str(), "s", ss.c_str());
        }

        void writeValue(string s, char c) override
        {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            fprintf(stderr, "writevalue<char>(%s,%d)\n", s.c_str(),c);
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            impl->write(s.c_str(), "c", c);
        }

        void writeValue(string s, float f) override
        {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            fprintf(stderr, "writevalue<float>(%s,%f)\n", s.c_str(),f);
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            impl->write(s.c_str(), "f", f);
        }

        void createLink(string s, class Fl_Osc_Widget*w) override
        {
            assert(s.length() != 0);
            Fl_Osc_Interface::createLink(s,w);
            map.insert(std::pair<string,Fl_Osc_Widget*>(s,w));
        }

        void renameLink(string old, string newer, Fl_Osc_Widget *w) override
        {
            fprintf(stderr, "renameLink('%s','%s',%p)\n",
                    old.c_str(), newer.c_str(), w);
            removeLink(old, w);
            createLink(newer, w);
        }

        void removeLink(string s, class Fl_Osc_Widget*w) override
        {
            for(auto i = map.begin(); i != map.end(); ++i) {
                if(i->first == s && i->second == w) {
                    map.erase(i);
                    break;
                }
            }
            printf("[%d] removing '%s' (%p)...\n", map.size(), s.c_str(), w);
        }

        virtual void removeLink(class Fl_Osc_Widget *w)
        {
            bool processing = true;
            while(processing)
            {
                //Verify Iterator invalidation sillyness
                processing = false;//Exit if no new elements are found
                for(auto i = map.begin(); i != map.end(); ++i) {
                    if(i->second == w) {
                        printf("[%d] removing '%s' (%p)...\n", map.size()-1,
                                i->first.c_str(), w);
                        map.erase(i);
                        processing = true;
                        break;
                    }
                }
            }
        }

        void tryLink(const char *msg) override
        {

            //DEBUG
            //if(strcmp(msg, "/vu-meter"))//Ignore repeated message
            //    printf("trying the link for a '%s'<%s>\n", msg, rtosc_argument_string(msg));
            const char *handle = rindex(msg,'/');
            if(handle)
                ++handle;

            int found_count = 0;

            for(auto pair:map) {
                if(pair.first == msg) {
                    found_count++;
                    const char *arg_str = rtosc_argument_string(msg);

                    //Always provide the raw message
                    pair.second->OSC_raw(msg);

                    if(!strcmp(arg_str, "b")) {
                        pair.second->OSC_value(rtosc_argument(msg,0).b.len,
                                rtosc_argument(msg,0).b.data,
                                handle);
                    } else if(!strcmp(arg_str, "c")) {
                        pair.second->OSC_value((char)rtosc_argument(msg,0).i,
                                handle);
                    } else if(!strcmp(arg_str, "i")) {
                        pair.second->OSC_value((int)rtosc_argument(msg,0).i,
                                handle);
                    } else if(!strcmp(arg_str, "f")) {
                        pair.second->OSC_value((float)rtosc_argument(msg,0).f,
                                handle);
                    } else if(!strcmp(arg_str, "T") || !strcmp(arg_str, "F")) {
                        pair.second->OSC_value((bool)rtosc_argument(msg,0).T, handle);
                    }
                }
            }

            if(found_count == 0 
                    && strcmp(msg, "/vu-meter")
                    && strcmp(msg, "undo_change")
                    && !strstr(msg, "parameter")) {
                fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
                fprintf(stderr, "Unknown widget '%s'\n", msg);
                fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            }
        };

        void dumpLookupTable(void)
        {
            for(auto i = map.begin(); i != map.end(); ++i) {
                printf("Known control  '%s' (%p)...\n", i->first.c_str(), i->second);
            }
        }


    private:
        std::multimap<string,Fl_Osc_Widget*> map;
        MiddleWareImpl *impl;
};
    
void MiddleWareImpl::warnMemoryLeaks(void)
{
    UI_Interface *o = (UI_Interface*)osc;
    o->dumpLookupTable();
}

Fl_Osc_Interface *genOscInterface(struct MiddleWareImpl *impl)
{
    return new UI_Interface(impl);
}

//stubs
MiddleWare::MiddleWare(void)
    :impl(new MiddleWareImpl())
{}
MiddleWare::~MiddleWare(void)
{
    delete impl;
}
Master *MiddleWare::spawnMaster(void)
{
    return impl->master;
}
Fl_Osc_Interface *MiddleWare::spawnUiApi(void)
{
    return impl->osc;
}
void MiddleWare::tick(void)
{
    impl->tick();
}
void MiddleWare::setUiCallback(void(*cb)(void*,const char *),void *ui)
{
    impl->cb = cb;
    impl->ui = ui;
}


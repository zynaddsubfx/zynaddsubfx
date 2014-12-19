#include "MiddleWare.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <rtosc/thread-link.h>
#include <rtosc/ports.h>
#include <lo/lo.h>

#include <unistd.h>
#include <dirent.h>

#include "../UI/Fl_Osc_Interface.h"
#include "../UI/Fl_Osc_Widget.H"

#include <map>

#include "Master.h"
#include "Part.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/SUBnoteParameters.h"
#include "../Params/PADnoteParameters.h"

#include <string>
#include <future>
#include <atomic>

#include <err.h>

rtosc::ThreadLink *bToU = new rtosc::ThreadLink(4096*2,1024);
rtosc::ThreadLink *uToB = new rtosc::ThreadLink(4096*2,1024);

/******************************************************************************
 *                        LIBLO And Reflection Code                           *
 *                                                                            *
 * All messages that are handled are handled in a serial fashion.             *
 * Thus, changes in the current interface sending messages can be encoded     *
 * into the stream via events which simply echo back the active interface     *
 ******************************************************************************/
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
    char         types[129];
    rtosc_arg_t  args[128];
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


    //Reply to requester [wow, these messages are getting huge...]
    char buffer[1024*20];
    size_t length = rtosc_amessage(buffer, sizeof(buffer), "/paths", types, args);
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


/*****************************************************************************
 *                    Memory Deallocation                                    *
 *****************************************************************************/
void deallocate(const char *str, void *v)
{
    printf("deallocating a '%s' at '%p'\n", str, v);
    if(!strcmp(str, "Part"))
        delete (Part*)v;
    else if(!strcmp(str, "Master"))
        delete (Master*)v;
    else if(!strcmp(str, "fft_t"))
        delete[] (fft_t*)v;
    else
        fprintf(stderr, "Unknown type '%s', leaking pointer %p!!\n", str, v);
}


/*****************************************************************************
 *                    PadSynth Setup                                         *
 *****************************************************************************/

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
            }, []{return false;});
    //clear out unused samples
    for(unsigned i = max+1; i < PAD_MAX_SAMPLES; ++i) {
        uToB->write((path+to_s(i)).c_str(), "ifb",
                0, 440.0f, sizeof(float*), NULL);
    }
}

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

void bankList(Bank &bank, Fl_Osc_Interface *osc)
{
    char response[2048];
    int i = 0;

    for(auto &elm : bank.banks) {
        if(!rtosc_message(response, 2048, "/bank-list", "iss",
                    i++, elm.name.c_str(), elm.dir.c_str()))
            errx(1, "Failure to handle bank update properly...");
        osc->tryLink(response);
    }
}

void rescanForBanks(Bank &bank, Fl_Osc_Interface *osc)
{
    bank.rescanforbanks();
    bankList(bank, osc);
}

void loadBank(Bank &bank, int pos, Fl_Osc_Interface *osc)
{
    if(bank.bankpos != pos) {
        bank.bankpos = pos;
        bank.loadbank(bank.banks[pos].dir);
        for(int i=0; i<BANK_SIZE; ++i)
            refreshBankView(bank, i, osc);
    }
}

void bankPos(Bank &bank, Fl_Osc_Interface *osc)
{
    char response[2048];

    if(!rtosc_message(response, 2048, "/loadbank", "i", bank.bankpos))
        errx(1, "Failure to handle bank update properly...");
    osc->tryLink(response);
}

/*****************************************************************************
 *                      Data Object for Non-RT Class Dispatch                *
 *****************************************************************************/

class DummyDataObj:public rtosc::RtData
{
    public:
        DummyDataObj(char *loc_, size_t loc_size_, void *obj_, cb_t cb_, void *ui_,
                Fl_Osc_Interface *osc_)
        {
            memset(loc_, 0, sizeof(loc_size_));
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
            auto &obj = part->kit[i];
            extractAD(obj.adpars, i, j);
            extractPAD(obj.padpars, i, j);
        }
    }

    void extractAD(ADnoteParameters *adpars, int i, int j)
    {
        std::string base = "/part"+to_s(i)+"/kit"+to_s(j)+"/";
        for(int k=0; k<NUM_VOICES; ++k) {
            std::string nbase = base+"adpars/voice"+to_s(k)+"/";
            if(adpars) {
                auto &nobj = adpars->VoicePar[k];
                objmap[nbase+"oscil/"]     = nobj.OscilSmp;
                objmap[nbase+"mod-oscil/"] = nobj.FMSmp;
            } else {
                objmap[nbase+"oscil/"]     = nullptr;
                objmap[nbase+"mod-oscil/"] = nullptr;
            }
        }
    }

    void extractPAD(PADnoteParameters *padpars, int i, int j)
    {
        std::string base = "/part"+to_s(i)+"/kit"+to_s(j)+"/";
        for(int k=0; k<NUM_VOICES; ++k) {
            std::string base = "/part"+to_s(i)+"/kit"+to_s(j)+"/";
            if(padpars) {
                objmap[base+"padpars/"]       = padpars;
                objmap[base+"padpars/oscil/"] = padpars->oscilgen;
            } else {
                objmap[base+"padpars/"]       = nullptr;
                objmap[base+"padpars/oscil/"] = nullptr;
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

static Fl_Osc_Interface *genOscInterface(class MiddleWareImpl*);

/* Implementation */
class MiddleWareImpl
{
    static constexpr const char* tmp_nam_prefix = "/tmp/zynaddsubfx_";

    //! returns file name to where UDP port is saved
    std::string get_tmp_nam() const
    {
         return tmp_nam_prefix + to_s(getpid());
    }

    void create_tmp_file(unsigned server_port)
    {
        std::string tmp_nam = get_tmp_nam();
        if(0 == access(tmp_nam.c_str(), F_OK)) {
            fprintf(stderr, "Error: Cannot overwrite file %s. "
                    "You should probably remove it.", tmp_nam.c_str());
            exit(EXIT_FAILURE);
        }
        FILE* tmp_fp = fopen(tmp_nam.c_str(), "w");
        if(!tmp_fp)
            fprintf(stderr, "Warning: could not create new file %s.\n",
                    tmp_nam.c_str());
        else
            fprintf(tmp_fp, "%u", server_port);
        fclose(tmp_fp);
    }

    void clean_up_tmp_nams() const
    {
        DIR *dir;
        struct dirent *entry;
        if ((dir = opendir ("/tmp/")) != nullptr)
        {
            while ((entry = readdir (dir)) != nullptr)
            {
                std::string name = std::string("/tmp/") + entry->d_name;
                if(!name.compare(0, strlen(tmp_nam_prefix),tmp_nam_prefix))
                {
                    std::string pid = name.substr(strlen(tmp_nam_prefix));
                    std::string proc_file = "/proc/" + std::move(pid) +
                                            "/comm";

                    std::ifstream ifs(proc_file);
                    bool remove = false;

                    if(!ifs.good())
                    {
                        fprintf(stderr, "Note: trying to remove %s - the "
                                        "process does not exist anymore.\n",
                                name.c_str());
                        remove = true;
                    }
                    else
                    {
                        std::string comm_name;
                        ifs >> comm_name;
                        if(comm_name == "zynaddsubfx")
                            fprintf(stderr, "Note: detected running "
                                            "zynaddsubfx with PID %s.\n",
                                    name.c_str() + strlen(tmp_nam_prefix));
                        else {
                            fprintf(stderr, "Note: trying to remove %s - the "
                                            "PID is owned by\n  another "
                                            "process: %s\n",
                                    name.c_str(), comm_name.c_str());
                            remove = true;
                        }
                    }


                    if(remove)
                    {
                        // make sure this file contains only one unsigned
                        unsigned udp_port;
                        std::ifstream ifs2(name);
                        if(!ifs2.good())
                            fprintf(stderr, "Warning: could not open %s.\n",
                                    name.c_str());
                        else
                        {
                            ifs2 >> udp_port;
                            if(ifs.good())
                                fprintf(stderr, "Warning: could not remove "
                                                "%s, \n  it has not been "
                                                "written by zynaddsubfx\n",
                                        name.c_str());
                            else
                            {
                                if(std::remove(name.c_str()) != 0)
                                    fprintf(stderr, "Warning: can not remove "
                                                    "%s.\n", name.c_str());
                            }
                        }
                    }

                    /* one might want to connect to zyn here,
                       but it is not necessary:
                    lo_address la = lo_address_new(nullptr, udp_port.c_str());
                    if(lo_send(la, "/echo", nullptr) <= 0)
                        fputs("Note: found crashed file %s\n", stderr);
                    lo_address_free(la);*/
                }
            }
            closedir (dir);
        } else {
            fputs("Warning: can not read /tmp.\n", stderr);
        }
    }

public:
    MiddleWareImpl(void);
    ~MiddleWareImpl(void);

    void warnMemoryLeaks(void);

    //Apply function while parameters are write locked
    void doReadOnlyOp(std::function<void()> read_only_fn);

    void saveMaster(const char *filename)
    {
        //Copy is needed as filename WILL get trashed during the rest of the run
        std::string fname = filename;
        printf("saving master('%s')\n", filename);
        doReadOnlyOp([this,fname](){
                int res = master->saveXML(fname.c_str());
                printf("results: '%s' '%d'\n",fname.c_str(), res);});
    }

    void savePart(int npart, const char *filename)
    {
        //Copy is needed as filename WILL get trashed during the rest of the run
        std::string fname = filename;
        printf("saving part(%d,'%s')\n", npart, filename);
        doReadOnlyOp([this,fname,npart](){
                int res = master->part[npart]->saveXML(fname.c_str());
                printf("results: '%s' '%d'\n",fname.c_str(), res);});
    }

    void loadPart(int npart, const char *filename, Master *master, Fl_Osc_Interface *osc)
    {
        actual_load[npart]++;

        if(actual_load[npart] != pending_load[npart])
            return;
        assert(actual_load[npart] <= pending_load[npart]);

        auto alloc = std::async(std::launch::async,
                [master,filename,this,npart](){
                Part *p = new Part(*master->memory, &master->microtonal, master->fft);
                if(p->loadXMLinstrument(filename))
                fprintf(stderr, "FAILED TO LOAD PART!!\n");

                auto isLateLoad = [this,npart]{
                return actual_load[npart] != pending_load[npart];
                };

                p->applyparameters(isLateLoad);
                return p;});

        //Load the part
        if(idle) {
            while(alloc.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                idle();
            }
        }

        Part *p = alloc.get();

        obj_store.extractPart(p, npart);
        kits.extractPart(p, npart);

        //Give it to the backend and wait for the old part to return for
        //deallocation
        uToB->write("/load-part", "ib", npart, sizeof(Part*), &p);
        osc->damage(("/part"+to_s(npart)+"/").c_str());
    }

    //Well, you don't get much crazier than changing out all of your RT
    //structures at once... TODO error handling
    void loadMaster(const char *filename)
    {
        Master *m = new Master();
        m->loadXML(filename);
        m->applyparameters();

        //Update resource locator table
        obj_store.clear();
        obj_store.extractMaster(m);

        master = m;

        //Give it to the backend and wait for the old part to return for
        //deallocation
        uToB->write("/load-master", "b", sizeof(Master*), &m);
    }

    //If currently broadcasting messages
    bool broadcast = false;
    void bToUhandle(const char *rtmsg);

    void tick(void)
    {
        lo_server_recv_noblock(server, 0);
        while(bToU->hasNext()) {
            const char *rtmsg = bToU->read();
            bToUhandle(rtmsg);
        }
    }

    bool handlePAD(string path, const char *msg, void *v)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        DummyDataObj d(buffer, 1024, v, cb, ui, osc);
        strcpy(buffer, path.c_str());

        PADnoteParameters::ports.dispatch(msg, d);
        if(!d.matches) {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
            fprintf(stderr, "Unknown location '%s%s'<%s>\n",
                    path.c_str(), msg, rtosc_argument_string(msg));
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }

        return true;
    }

    bool handleOscil(string path, const char *msg, void *v);

    void kitEnable(const char *msg);
    void kitEnable(int part, int kit, int type);

    // Handle an event with special cases
    void handleMsg(const char *msg);


    void write(const char *path, const char *args, ...);
    void write(const char *path, const char *args, va_list va);


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

    //The ONLY means that any chunk of UI code should have for interacting with the
    //backend
    Fl_Osc_Interface *osc;
    //Synth Engine Parameters
    ParamStore kits;

    //Callback When Waiting on async events
    void(*idle)(void);
    //General UI callback
    cb_t cb;
    //UI handle
    void *ui;

    std::atomic_int pending_load[NUM_MIDI_PARTS];
    std::atomic_int actual_load[NUM_MIDI_PARTS];
};

MiddleWareImpl::MiddleWareImpl(void)
{
    server = lo_server_new_with_proto(NULL, LO_UDP, liblo_error_cb);
    lo_server_add_method(server, NULL, NULL, handler_function, NULL);
    fprintf(stderr, "lo server running on %d\n", lo_server_get_port(server));

    clean_up_tmp_nams();
    create_tmp_file((unsigned)lo_server_get_port(server));

    //dummy callback for starters
    cb = [](void*, const char*){};
    idle = 0;

    master = new Master();
    osc    = genOscInterface(this);

    //Grab objects of interest from master
    obj_store.extractMaster(master);

    //Null out Load IDs
    for(int i=0; i < NUM_MIDI_PARTS; ++i) {
        pending_load[i] = 0;
        actual_load[i] = 0;
    }
}
MiddleWareImpl::~MiddleWareImpl(void)
{
    remove(get_tmp_nam().c_str());

    warnMemoryLeaks();

    delete master;
    delete osc;
}

/** Threading When Saving
 *  ----------------------
 *
 * Procedure Middleware:
 *   1) Middleware sends /freeze_state to backend
 *   2) Middleware waits on /state_frozen from backend
 *      All intervening commands are held for out of order execution
 *   3) Aquire memory
 *      At this time by the memory barrier we are guarenteed that all old
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
    //Copy is needed as filename WILL get trashed during the rest of the run
    uToB->write("/freeze_state","");

    std::list<const char *> fico;
    int tries = 0;
    while(tries++ < 10000) {
        if(!bToU->hasNext()) {
            usleep(500);
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

void MiddleWareImpl::bToUhandle(const char *rtmsg)
{
    assert(strcmp(rtmsg, "/part0/kit0/Ppadenableda"));
    //Dump Incomming Events For Debugging
    if(strcmp(rtmsg, "/vu-meter") && false) {
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 1 + 30, 0 + 40);
        fprintf(stdout, "frontend: '%s'<%s>\n", rtmsg,
                rtosc_argument_string(rtmsg));
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    //Activity dot
    printf(".");fflush(stdout);

    if(!strcmp(rtmsg, "/echo")
            && !strcmp(rtosc_argument_string(rtmsg),"ss")
            && !strcmp(rtosc_argument(rtmsg,0).s, "OSC_URL"))
        curr_url = rtosc_argument(rtmsg,1).s;
    else if(!strcmp(rtmsg, "/free")
            && !strcmp(rtosc_argument_string(rtmsg),"sb")) {
        deallocate(rtosc_argument(rtmsg, 0).s, *((void**)rtosc_argument(rtmsg, 1).b.data));
    } else if(!strcmp(rtmsg, "/request-memory")) {
        //Generate out more memory for the RT memory pool
        //5MBi chunk
        size_t N  = 5*1024*1024;
        void *mem = malloc(N);
        uToB->write("/add-rt-memory", "bi", sizeof(void*), &mem, N);
    } else if(!strcmp(rtmsg, "/setprogram")
            && !strcmp(rtosc_argument_string(rtmsg),"cc")) {
        loadPart(rtosc_argument(rtmsg,0).i, master->bank.ins[rtosc_argument(rtmsg,1).i].filename.c_str(), master, osc);
    } else if(!strcmp(rtmsg, "/broadcast")) {
        broadcast = true;
    } else if(broadcast) {
        broadcast = false;
        cb(ui, rtmsg);
        if(curr_url != "GUI") {
            lo_message msg  = lo_message_deserialise((void*)rtmsg,
                    rtosc_message_length(rtmsg, bToU->buffer_size()), NULL);

            //Send to known url
            if(!curr_url.empty()) {
                lo_address addr = lo_address_new_from_url(curr_url.c_str());
                lo_send_message(addr, rtmsg, msg);
            }
        }
    } else if(curr_url == "GUI" || !strcmp(rtmsg, "/close-ui")) {
        cb(ui, rtmsg);
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

bool MiddleWareImpl::handleOscil(string path, const char *msg, void *v)
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
        //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
        //fprintf(stderr, "Unknown location '%s%s'<%s>\n",
        //        path.c_str(), msg, rtosc_argument_string(msg));
        //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    return true;
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

    if(type == -1)
        return;

    const char *tmp = strstr(msg, "part");

    if(tmp == NULL)
        return;

    const int part = atoi(tmp+4);

    tmp = strstr(msg, "kit");

    if(tmp == NULL)
        return;

    const int kit = atoi(tmp+3);

    kitEnable(part, kit, type);
}

void MiddleWareImpl::kitEnable(int part, int kit, int type)
{
    printf("attempting a kit enable\n");
    string url = "/part"+to_s(part)+"/kit"+to_s(kit)+"/";
    void *ptr = NULL;
    if(type == 0 && kits.add[part][kit] == NULL) {
        ptr = kits.add[part][kit] = new ADnoteParameters(master->fft);
        url += "adpars-data";
        obj_store.extractAD(kits.add[part][kit], part, kit);
    } else if(type == 1 && kits.pad[part][kit] == NULL) {
        ptr = kits.pad[part][kit] = new PADnoteParameters(master->fft);
        url += "padpars-data";
        obj_store.extractPAD(kits.pad[part][kit], part, kit);
    } else if(type == 2 && kits.sub[part][kit] == NULL) {
        ptr = kits.sub[part][kit] = new SUBnoteParameters();
        url += "subpars-data";
    }

    //Send the new memory
    if(ptr)
        uToB->write(url.c_str(), "b", sizeof(void*), &ptr);
}

/* BASE/part#/kititem#
 * BASE/part#/kit#/adpars/voice#/oscil/\*
 * BASE/part#/kit#/adpars/voice#/mod-oscil/\*
 * BASE/part#/kit#/padpars/prepare
 * BASE/part#/kit#/padpars/oscil/\*
 */
void MiddleWareImpl::handleMsg(const char *msg)
{
    assert(!strstr(msg,"free"));
    assert(msg && *msg && rindex(msg, '/')[1]);
    //fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 6 + 30, 0 + 40);
    //fprintf(stdout, "middleware: '%s'\n", msg);
    //fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    const char *last_path = rindex(msg, '/');
    if(!last_path)
        return;


    //printf("watching '%s' go by\n", msg);
    //Get the object resource locator
    string obj_rl(msg, last_path+1);

    if(!strcmp(msg, "/refresh_bank") && !strcmp(rtosc_argument_string(msg), "i")) {
        refreshBankView(master->bank, rtosc_argument(msg,0).i, osc);
    } else if(!strcmp(msg, "/bank-list") && !strcmp(rtosc_argument_string(msg), "")) {
        bankList(master->bank, osc);
    } else if(!strcmp(msg, "/rescanforbanks") && !strcmp(rtosc_argument_string(msg), "")) {
        rescanForBanks(master->bank, osc);
    } else if(!strcmp(msg, "/loadbank") && !strcmp(rtosc_argument_string(msg), "i")) {
        loadBank(master->bank, rtosc_argument(msg, 0).i, osc);
    } else if(!strcmp(msg, "/loadbank") && !strcmp(rtosc_argument_string(msg), "")) {
        bankPos(master->bank, osc);
    } else if(obj_store.has(obj_rl)) {
        //try some over simplified pattern matching
        if(strstr(msg, "oscil/")) {
            if(!handleOscil(obj_rl, last_path+1, obj_store.get(obj_rl)))
                uToB->raw_write(msg);
            //else if(strstr(obj_rl.c_str(), "kititem"))
            //    handleKitItem(obj_rl, objmap[obj_rl],atoi(rindex(msg,'m')+1),rtosc_argument(msg,0).T);
        } else if(strstr(msg, "padpars/prepare"))
            preparePadSynth(obj_rl,(PADnoteParameters *) obj_store.get(obj_rl));
        else if(strstr(msg, "padpars")) {
            if(!handlePAD(obj_rl, last_path+1, obj_store.get(obj_rl)))
                uToB->raw_write(msg);
        } else //just forward the message
            uToB->raw_write(msg);
    } else if(strstr(msg, "/save_xmz") && !strcmp(rtosc_argument_string(msg), "s")) {
        saveMaster(rtosc_argument(msg,0).s);
    } else if(strstr(msg, "/save_xiz") && !strcmp(rtosc_argument_string(msg), "is")) {
        savePart(rtosc_argument(msg,0).i,rtosc_argument(msg,1).s);
    } else if(strstr(msg, "/load_xmz") && !strcmp(rtosc_argument_string(msg), "s")) {
        loadMaster(rtosc_argument(msg,0).s);
    } else if(!strcmp(msg, "/load_xiz") && !strcmp(rtosc_argument_string(msg), "is")) {
        pending_load[rtosc_argument(msg,0).i]++;
        loadPart(rtosc_argument(msg,0).i, rtosc_argument(msg,1).s, master, osc);
    } else if(strstr(msg, "load-part") && !strcmp(rtosc_argument_string(msg), "is")) {
        pending_load[rtosc_argument(msg,0).i]++;
        loadPart(rtosc_argument(msg,0).i, rtosc_argument(msg,1).s, master, osc);
    } else if(strstr(msg, "Padenabled") || strstr(msg, "Ppadenabled") || strstr(msg, "Psubenabled")) {
        kitEnable(msg);
        uToB->raw_write(msg);
    } else
        uToB->raw_write(msg);
}

void MiddleWareImpl::write(const char *path, const char *args, ...)
{
    //We have a free buffer in the threadlink, so use it
    va_list va;
    va_start(va, args);
    write(path, args, va);
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
 *    OSC Interface For User Interface                                        *
 *                                                                            *
 *    This is a largely out of date section of code                           *
 *    Most type specific write methods are no longer used                     *
 *    See UI/Fl_Osc_* to see what is actually used in this interface          *
 ******************************************************************************/
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
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            fprintf(stderr, ".");
            //fprintf(stderr, "write(%s)\n", msg);
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
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
            assert(!strstr(s.c_str(), "/part0/kit-1"));
            map.insert(std::pair<string,Fl_Osc_Widget*>(s,w));
        }

        void renameLink(string old, string newer, Fl_Osc_Widget *w) override
        {
            //fprintf(stdout, "renameLink('%s','%s',%p)\n",
            //        old.c_str(), newer.c_str(), w);
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
            //printf("[%d] removing '%s' (%p)...\n", map.size(), s.c_str(), w);
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
                        //printf("[%d] removing '%s' (%p)...\n", map.size()-1,
                        //        i->first.c_str(), w);
                        map.erase(i);
                        processing = true;
                        break;
                    }
                }
            }
        }

        //A very simplistic implementation of a UI agnostic refresh method
        virtual void damage(const char *path)
        {
            //printf("\n\nDamage(\"%s\")\n", path);
            for(auto pair:map) {
                if(strstr(pair.first.c_str(), path)) {
                    auto *tmp = dynamic_cast<Fl_Widget*>(pair.second);
                    //if(tmp)
                    //    printf("%x, %d %d [%s]\n", (int)pair.second, tmp->visible_r(), tmp->visible(), pair.first.c_str());
                    //else
                    //    printf("%x, (NULL)[%s]\n", (int)pair.second,pair.first.c_str());
                    if(!tmp || tmp->visible_r())
                        pair.second->update();
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

            auto range = map.equal_range(msg);
            for(auto itr = range.first; itr != range.second; ++itr) {
                auto widget = itr->second;
                found_count++;
                const char *arg_str = rtosc_argument_string(msg);

                //Always provide the raw message
                widget->OSC_raw(msg);

                if(!strcmp(arg_str, "b")) {
                    widget->OSC_value(rtosc_argument(msg,0).b.len,
                            rtosc_argument(msg,0).b.data,
                            handle);
                } else if(!strcmp(arg_str, "c")) {
                    widget->OSC_value((char)rtosc_argument(msg,0).i,
                            handle);
                } else if(!strcmp(arg_str, "s")) {
                    widget->OSC_value((const char*)rtosc_argument(msg,0).s,
                            handle);
                } else if(!strcmp(arg_str, "i")) {
                    widget->OSC_value((int)rtosc_argument(msg,0).i,
                            handle);
                } else if(!strcmp(arg_str, "f")) {
                    widget->OSC_value((float)rtosc_argument(msg,0).f,
                            handle);
                } else if(!strcmp(arg_str, "T") || !strcmp(arg_str, "F")) {
                    widget->OSC_value((bool)rtosc_argument(msg,0).T, handle);
                }
            }

            if(found_count == 0
                    && strcmp(msg, "/vu-meter")
                    && strcmp(msg, "undo_change")
                    && !strstr(msg, "parameter")
                    && !strstr(msg, "Prespoint")) {
                //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
                //fprintf(stderr, "Unknown widget '%s'\n", msg);
                //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            }
        };

        void dumpLookupTable(void)
        {
            if(map.size() != 0)
                printf("Leaked controls:\n");
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

Fl_Osc_Interface *genOscInterface(class MiddleWareImpl *impl)
{
    return new UI_Interface(impl);
}

/******************************************************************************
 *                         MidleWare Forwarding Stubs                         *
 ******************************************************************************/
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

void MiddleWare::doReadOnlyOp(std::function<void()> fn)
{
    impl->doReadOnlyOp(fn);
}

void MiddleWare::setUiCallback(void(*cb)(void*,const char *),void *ui)
{
    impl->cb = cb;
    impl->ui = ui;
}

void MiddleWare::setIdleCallback(void(*cb)(void))
{
    impl->idle = cb;
}
        
void MiddleWare::transmitMsg(const char *msg)
{
    impl->handleMsg(msg);
}

void MiddleWare::pendingSetProgram(int part)
{
    impl->pending_load[part]++;
}

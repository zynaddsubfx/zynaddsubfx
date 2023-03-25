#include <cassert>
#include <thread>
#include <mutex>
#include <iostream>
#include <ctime>
#include <unistd.h>
#include <rtosc/thread-link.h>
#include <rtosc/rtosc-time.h>

#include "../Misc/Master.h"
#include "../Misc/MiddleWare.h"
#include "../UI/NSM.H"

// for linking purposes only:
NSM_Client *nsm = 0;
zyn::MiddleWare *middleware = 0;

char *instance_name=(char*)"";

class Xiz2Xmz
{

        void _masterChangedCallback(zyn::Master* m)
        {
            master = m;
            master->setMasterChangedCallback(__masterChangedCallback, this);
            master->setUnknownAddressCallback(__unknownAddressCallback, this);
        }

        // TODO: eliminate static callbacks
        static void __masterChangedCallback(void* ptr, zyn::Master* m)
        {
            ((Xiz2Xmz*)ptr)->_masterChangedCallback(m);
        }

        std::atomic<unsigned> unknown_addresses_count = { 0 };

        void _unknownAddressCallback(bool, rtosc::msg_t)
        {
            ++unknown_addresses_count;
        }

        static void __unknownAddressCallback(void* ptr, bool offline, rtosc::msg_t msg)
        {
            ((Xiz2Xmz*)ptr)->_unknownAddressCallback(offline, msg);
        }

        void setUp() {
            // this might be set to true in the future
            // when saving will work better
            config.cfg.SaveFullXml = false;

            synth = new zyn::SYNTH_T;
            synth->buffersize = 256;
            synth->samplerate = 48000;
            synth->alias();

            mw = new zyn::MiddleWare(std::move(*synth), &config);
            mw->setUiCallback(_uiCallback, this);
            _masterChangedCallback(mw->spawnMaster());
            realtime = nullptr;
        }

        void tearDown() {
#ifdef SAVE_OSC_DEBUG
            printf("Master at the end: %p\n", master);
#endif
            delete mw;
            delete synth;
        }

        struct {
            std::string operation;
            std::string file;
            uint64_t stamp;
            bool status;
            std::string savefile_content;
            int msgnext = 0, msgmax;
        } recent;
        std::mutex cb_mutex;
        using mutex_guard = std::lock_guard<std::mutex>;

        bool timeOutOperation(const char* osc_path, const char* arg1, int tries)
        {
            clock_t begin = clock(); // just for statistics

            bool ok = false;
            rtosc_arg_val_t start_time;
            rtosc_arg_val_current_time(&start_time);

            mw->transmitMsgGui(osc_path, "stT", arg1, start_time.val.t);

            int attempt;
            for(attempt = 0; attempt < tries; ++attempt)
            {
                mutex_guard guard(cb_mutex);
                if(recent.stamp == start_time.val.t &&
                   recent.operation == osc_path &&
                   recent.file == arg1)
                {
                    ok = recent.status;
                    break;
                }
                usleep(1000);
            }

            // statistics:
            clock_t end = clock();
            double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

            fprintf(stderr, "Action %s finished after %lf ms,\n"
                            "    with a timeout of <%d ms (%s)\n",
                    osc_path, elapsed_secs,
                    attempt+1,
                    attempt == tries ? "timeout"
                                     : ok ? "success" : "failure");

            return ok && (attempt != tries);
        }

        void uiCallback(const char* msg)
        {
            if(!strcmp(msg, "/save_osc") || !strcmp(msg, "/load_xmz"))
            {
                mutex_guard guard(cb_mutex);
#ifdef SAVE_OSC_DEBUG
                fprintf(stderr, "Received message \"%s\".\n", msg);
#endif
                int args = rtosc_narguments(msg);

                if(args == 3)
                {
                    assert(!strcmp(rtosc_argument_string(msg), "stT"));
                    recent.operation = msg;
                    recent.file = rtosc_argument(msg, 0).s;
                    recent.stamp = rtosc_argument(msg, 1).t;
                    recent.status = rtosc_argument(msg, 2).T;
                    recent.savefile_content.clear();
                    recent.msgmax = recent.msgnext = 0;
                }
                else
                {
                    assert(!strcmp(rtosc_argument_string(msg), "stiis"));
                    assert(rtosc_argument(msg, 0).s == recent.file);
                    assert(rtosc_argument(msg, 1).t == recent.stamp);
                    if(recent.msgmax)
                        assert(recent.msgmax == rtosc_argument(msg, 3).i);
                    else
                        recent.msgmax = rtosc_argument(msg, 3).i;
                    assert(recent.msgnext == rtosc_argument(msg, 2).i);
                    recent.savefile_content += rtosc_argument(msg, 4).s;
                    ++recent.msgnext;
                }
            }
            else if(!strcmp(msg, "/damage"))
            {
                // (ignore)
            }
            else
            {
                // parameter update triggers message to UI
                // (ignore)
            }
        }

    public:
        Xiz2Xmz() { setUp(); }
        ~Xiz2Xmz() { tearDown(); }

        static void _uiCallback(void* ptr, const char* msg)
        {
            ((Xiz2Xmz*)ptr)->uiCallback(msg);
        }

        int run(int argc, char** argv)
        {
            // enable almost everything, in order to test all ports
            std::string outfile = argv[1];
            outfile += ".xmz";
            mw->transmitMsgGui("/load_xiz", "is", 0, argv[1]);
            mw->transmitMsgGui("/save_xmz", "s", outfile.c_str());

            int res = EXIT_SUCCESS;

            return res;
        }


        void start_realtime(void)
        {
            do_exit = false;

            realtime = new std::thread([this](){
                while(!do_exit)
                {
                    if(!master->uToB->hasNext()) {
                        if(do_exit)
                            break;

                        usleep(500);
                        continue;
                    }
                    const char *msg = master->uToB->read();
#ifdef SAVE_OSC_DEBUG
                    printf("Master %p: handling <%s>\n", master, msg);
#endif
                    master->applyOscEvent(msg, false);
                }});
        }

        void stop_realtime(void)
        {
            do_exit = true;

            realtime->join();
            delete realtime;
            realtime = NULL;
        }

    private:
        zyn::Config config;
        zyn::SYNTH_T* synth;
        zyn::Master* master = NULL;
        zyn::MiddleWare* mw;
        std::thread* realtime;
        bool do_exit;
};

int main(int argc, char** argv)
{
    Xiz2Xmz test;
    test.start_realtime();
    printf("argc: %d\n", argc);
    int res = test.run(argc, argv);
    test.stop_realtime();
    std::cerr << "Summary: " << ((res == EXIT_SUCCESS) ? "SUCCESS" : "FAILURE")
              << std::endl;
    return res;
}



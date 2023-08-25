#include <cassert>
#include <thread>
#include <mutex>
#include <iostream>
#include <ctime>
#include <unistd.h>
#include <rtosc/thread-link.h>
#include <rtosc/rtosc-time.h>
#include <rtosc/port-checker.h>
#include <liblo-server.h>  // from rtosc's test directory

#include "../Misc/Master.h"
#include "../Misc/MiddleWare.h"
#include "../UI/NSM.H"

// for linking purposes only:
NSM_Client *nsm = 0;
zyn::MiddleWare *middleware = 0;

char *instance_name=(char*)"";

class PortChecker
{
        void _masterChangedCallback(zyn::Master* m)
        {
            master = m;
            master->setMasterChangedCallback(__masterChangedCallback, this);
        }

        // TODO: eliminate static callbacks
        static void __masterChangedCallback(void* ptr, zyn::Master* m)
        {
            ((PortChecker*)ptr)->_masterChangedCallback(m);
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

        void uiCallback(const char* msg)
        {
            (void)msg;
        }

        bool exit_mw = false;
        void run_mw()
        {
            while(!exit_mw) {
                if(mw)
                    mw->tick();
                usleep(20000);
            }
        }

    public:
        PortChecker() { setUp(); }
        ~PortChecker() { tearDown(); }

        static void _uiCallback(void* ptr, const char* msg)
        {
            ((PortChecker*)ptr)->uiCallback(msg);
        }

        int run()
        {
            assert(mw);

            std::thread mwthread(&PortChecker::run_mw, this);

            bool ok;
            try {
                int timeout_msecs = 50;
                rtosc::liblo_server sender(timeout_msecs), other(timeout_msecs);
                rtosc::port_checker pc(&sender, &other);
                ok = pc(mw->getServerPort());

                if(!pc.print_sanity_checks())
                    ok = false;
                pc.print_evaluation();
                if(pc.errors_found())
                    ok = false;
                pc.print_not_affected();
                pc.print_skipped();
                pc.print_statistics();

            } catch(const std::exception& e) {
                std::cerr << "**Error caught**: " << e.what() << std::endl;
                ok = false;
            }

            int res = ok ? EXIT_SUCCESS : EXIT_FAILURE;
            exit_mw = true;
            mwthread.join();
            return res;
        }


        void start_realtime(void)
        {
            do_exit = false;

            realtime = new std::thread([this](){
                while(!do_exit)
                {
                    if(master->uToB->hasNext()) {
                        const char *msg = master->uToB->read();
#ifdef ZYN_PORT_CHECKER_DEBUG
                        printf("Master %p: handling <%s>\n", master, msg);
#endif
                        master->applyOscEvent(msg, false);
                    }
                    else {
                        // RT has no incoming message?
                        if(do_exit)
                            break;
                        usleep(500);
                    }
                    master->last_ack = master->last_beat;
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

int main()
{
    PortChecker test;
    test.start_realtime();
    int res = test.run();
    test.stop_realtime();
    std::cerr << "Summary: " << ((res == EXIT_SUCCESS) ? "SUCCESS" : "FAILURE")
              << std::endl;
    return res;
}



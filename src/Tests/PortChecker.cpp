#include <cassert>
#include <queue>
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
#include "rtosc/arg-val.h"

// for linking purposes only:
NSM_Client *nsm = 0;
zyn::MiddleWare *middleware = 0;

char *instance_name=(char*)"";

//#define DEBUG_PORT_CHECKER

//! non-network implementation of port_checker::server
//! @note This is all in one thread - thus, no mutexes etc
class direct_server : public rtosc::port_checker::server
{
    zyn::MiddleWare* const mw;
    const std::size_t gui_id;
    std::queue<std::vector<char>> received;  // inbox for MW's replies

public:
    //!< UI callback, will queue message in `received`
    void on_recv(const char* message)
    {
        int len = rtosc_message_length(message, -1);
        std::vector<char> buf(len);
        memcpy(buf.data(), message, len);
        received.push(std::move(buf));
    }

    //!< Fetch exactly 1 msg from `received`, may call server::handle_recv
    void fetch_received()
    {
        if(!received.empty())
        {
            const char* message = received.front().data();
            if(waiting && exp_paths_n_args[0].size())
            {
                _replied_path = 0;
                for(std::vector<const char*>* exp_strs = exp_paths_n_args; exp_strs->size();
                     ++exp_strs, ++_replied_path)
                    if(!strcmp((*exp_strs)[0], message))
                    {
#ifdef DEBUG_PORT_CHECKER
                        std::cout << "on_recv: match:" << std::endl;
#endif
                        int len = rtosc_message_length(message, -1);

                        *last_buffer = std::vector<char>(len);
                        memcpy(last_buffer->data(), message, len);

                        unsigned nargs = rtosc_narguments(message);
                        std::vector<char> types(nargs);
                        for (unsigned i = 0; i < nargs; ++i)
                            types[i] = rtosc_type(message, i);

                        server::handle_recv(nargs, types.data(), exp_strs);
                        break;
                    }
            }
            received.pop();
        }
    }

    //! Let port checker send a message to MW
    bool send_msg(const char* address,
                  size_t nargs, const rtosc_arg_val_t* args) override
    {
        char buf[8192];
        int len = rtosc_avmessage(buf, sizeof(buf), address, nargs, args);
        if(len <= 0)
            throw std::runtime_error("Could not serialize message");
        mw->transmitMsgGui(gui_id, buf);
        return true;
    }

    //! Let port checker wait until matching message was replied
    bool _wait_for_reply(std::vector<char>* buffer,
                         std::vector<rtosc_arg_val_t> * args,
                         int n0, int n1) override
    {
        (void)n0;
        // TODO: most of this function is common to liblo_server
        // => refactor into portchecker
        assert(n1);
        exp_paths_n_args[n1].clear();

        last_args = args;
        last_buffer = buffer;

        // allow up to 1000 recv calls = 1s
        // if there's no reply at all after 0.5 seconds, abort
        const int timeout_initial = timeout_msecs;
        int tries_left = 1000, timeout = timeout_initial;
        while(tries_left-->1 && timeout-->1 && waiting) // waiting is set in "on_recv"
        {
            mw->tick();
            usleep(1000);
            if(received.size()) {
                fetch_received();
                timeout = timeout_initial;
            }
            // message will be dispatched to the server's callback
            // server::handle_recv will unset `waiting` if a "good" message was found
        }
        waiting = true; // prepare for next round

        return tries_left && timeout;
    }

    void vinit(const char* ) override {}  // nothing to do here

    direct_server(std::size_t gui_id, int timeout_msecs, zyn::MiddleWare* mw)
        : server(timeout_msecs)
        , mw(mw)
        , gui_id(gui_id) {}
};

class PortChecker
{
        void _masterChangedCallback(zyn::Master* m)
        {
            master = m;
            master->setMasterChangedCallback(
                [](void* p, zyn::Master* m) {
                    ((PortChecker*)p)->_masterChangedCallback(m); },
                this);
        }

        void setUp() {
            // this might be set to true in the future
            // when saving will work better
            config.cfg.SaveFullXml = false;

            synth = new zyn::SYNTH_T;
            synth->buffersize = 256;
            synth->samplerate = 48000;
            synth->oscilsize = 256;
            synth->alias();

            mw = new zyn::MiddleWare(std::move(*synth), &config);
            mw->setUiCallback(0,
                [](void* p, const char* msg) {
                    ((PortChecker*)p)->uiCallback0(msg); },
                this);
            mw->setUiCallback(1,
                [](void* p, const char* msg) {
                    ((PortChecker*)p)->uiCallback1(msg); },
                this);
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

        void uiCallback0(const char* msg)
        {
            sender->on_recv(msg);
        }

        void uiCallback1(const char* msg)
        {
            other->on_recv(msg);
        }

    public:
        PortChecker() { setUp(); }
        ~PortChecker() { tearDown(); }

        int run()
        {
            assert(mw);

            bool ok;
            try {
                int timeout_msecs = 50;
                sender = new direct_server(0u, timeout_msecs, mw);
                other = new direct_server(1u, timeout_msecs, mw);
                rtosc::port_checker pc(sender, other);
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
        direct_server* sender, * other;
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



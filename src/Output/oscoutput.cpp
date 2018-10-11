/*
  ZynAddSubFX - a software synthesizer

  oscoutput.h - Audio output for OSC plugins
  Copyright (C) 2017-2018 Johannes Lorenz
  Author: Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cassert>
#include <cstdarg>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <set>
#include <string>
#include <vector>
#include <rtosc/thread-link.h>
#include <rtosc/rtosc.h>
#include <mutex>

#include <zyn-config.h>
#include "icon.h"
#include "oscoutput.h"
#include "../Misc/Master.h"
#include "../Misc/Util.h"

using std::set;
using std::string;
using std::vector;

//Dummy variables and functions for linking purposes
namespace zyn {
    const char *instance_name = nullptr;
    class WavFile;
    namespace Nio {
        bool start(void){return 1;};
        void stop(void){};
        void masterSwap(Master *){};
        void waveNew(WavFile *){}
        void waveStart(void){}
        void waveStop(void){}
        void waveEnd(void){}
        bool setSource(string){return true;}
        bool setSink(string){return true;}
        set<string> getSources(void){return set<string>();}
        set<string> getSinks(void){return set<string>();}
        string getSource(void){return "";}
        string getSink(void){return "";}
    }
}


void ZynOscPlugin::run(/*float* outl, float* outr, unsigned long sample_count*/)
{
    check_osc();
    master->GetAudioOutSamples(p_samplecount,
                               static_cast<unsigned>(p_samplerate),
                               p_out.left, p_out.right);
}

void ZynOscPlugin::ui_ext_show(bool show)
{
    if(show)
    {
        if(ui_pid)
        {
            // ui is already running
        }
        else
        {
            // copied from main.c - please keep in sync
            const char *addr = middleware->getServerAddress();
#ifndef WIN32
            ui_pid = fork();
            if(ui_pid == 0) {
                auto exec_fusion = [&addr](const char* path) {
                    execlp(path, "zyn-fusion", addr, "--builtin", "--no-hotload",  0); };
                if(zyn::fusion_dir && *zyn::fusion_dir)
                {
                    std::string fusion = zyn::fusion_dir;
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
                        ld_library_path += zyn::fusion_dir;
                        setenv("LD_LIBRARY_PATH", ld_library_path.c_str(), 1);
                        exec_fusion(fusion.c_str());
                    }
                }
                exec_fusion("./zyn-fusion");
                exec_fusion("zyn-fusion");
                fputs("Failed to launch Zyn-Fusion", stderr);
                exit(1);
            }
#else
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            memset(&si, 0, sizeof(si));
            memset(&pi, 0, sizeof(pi));
            char *why_windows = strrchr(addr, ':');
            char *seriously_why = why_windows + 1;
            char start_line[256] = {0};
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
    }
    else
    {
        hide_ui();
    }
}

const char** ZynOscDescriptor::xpm_load() const
{
    return get_icon();
}

/*// TODO: make ports with full zyn metadata
struct zyn_port
{
    bool linear; //!< vs log

};*/

spa::port_ref_base* port_from_osc_args(const char* args)
{
    if(!args)
        return nullptr;
    for(; *args == ':'; ++args) ;
    if(strchr(args, 'S'))
        return new spa::audio::control_in<int>; // TODO: separate port type
    else switch(*args)
    {
        case 'T':
        case 'F':
            return new spa::audio::control_in<bool>;
        case 'i': return new spa::audio::control_in<int>;
        case 'h': return new spa::audio::control_in<long long int>;
        case 'f': return new spa::audio::control_in<float>;
        case 'd': return new spa::audio::control_in<double>;
        default: return nullptr;
    }
}

struct set_min final : public spa::audio::visitor
{
    const char* min = nullptr;
    template<class T> using ci = spa::audio::control_in<T>;
    void visit(ci<int> &p) override;
    void visit(ci<long long int> &p) override;
    void visit(ci<float> &p) override;
    void visit(ci<double> &p) override;
};

void set_min::visit(ci<int> &p) { p.min = atoi(min); }
void set_min::visit(ci<long long int> &p) { p.min = atoll(min); }
void set_min::visit(ci<float> &p) {
    p.min = static_cast<float>(atof(min)); }
void set_min::visit(ci<double> &p) { p.min = atof(min); }

struct set_max final : public spa::audio::visitor
{
    const char* max = nullptr;
    template<class T> using ci = spa::audio::control_in<T>;
    void visit(ci<int> &p) override;
    void visit(ci<long long int> &p) override;
    void visit(ci<float> &p) override;
    void visit(ci<double> &p) override;
};

void set_max::visit(ci<int> &p) { p.max = atoi(max); }
void set_max::visit(ci<long long int> &p) { p.max = atoll(max); }
void set_max::visit(ci<float> &p) {
    p.max = static_cast<float>(atof(max)); }
void set_max::visit(ci<double> &p) { p.max = atof(max); }

struct set_step final : public spa::audio::visitor
{
    const char* step = nullptr;
    template<class T> using ci = spa::audio::control_in<T>;
    void visit(ci<int> &p) override;
    void visit(ci<long long int> &p) override;
    void visit(ci<float> &p) override;
    void visit(ci<double> &p) override;
};

void set_step::visit(ci<int> &p) { p.step = atoi(step); }
void set_step::visit(ci<long long> &p) { p.step = atoll(step); }
void set_step::visit(ci<float> &p) { p.step = static_cast<float>(atof(step)); }
void set_step::visit(ci<double> &p) { p.step = atof(step); }

struct is_bool_t final : public spa::audio::visitor
{
    bool res = false;
    template<class T> using ci = spa::audio::control_in<T>;
    void visit(ci<bool> &);
};

void is_bool_t::visit(ci<bool> &) { res = true; }

// TODO: bad design of control_in<T>
struct set_scale_type final : public spa::audio::visitor
{
    template<class T> using ci = spa::audio::control_in<T>;
    spa::audio::scale_type_t scale_type = spa::audio::scale_type_t::linear;
    void visit(ci<int> &p) override;
    void visit(ci<long long int> &p) override;
    void visit(ci<float> &p) override;
    void visit(ci<double> &p) override;
};

void set_scale_type::visit(ci<int> &p) { p.scale_type = scale_type; }
void set_scale_type::visit(ci<long long> &p) { p.scale_type = scale_type; }
void set_scale_type::visit(ci<float> &p) { p.scale_type = scale_type; }
void set_scale_type::visit(ci<double> &p) { p.scale_type = scale_type; }

//! class for capturing numerics (not pointers to string/blob memory involved)
class capture final : public rtosc::RtData
{
    rtosc_arg_val_t res;
    void replyArray(const char*, const char *args,
                    rtosc_arg_t *vals) override
    {
        assert(*args && args[1] == 0); // reply only one arg for numeric ports
        res.type = args[0];
        res.val = vals[0];
    }

    void reply_va(const char *args, va_list va)
    {
        rtosc_v2argvals(&res, 1, args, va);
    }

    void reply(const char *, const char *args, ...) override
    {
        va_list va;
        va_start(va,args);
        reply_va(args, va);
        va_end(va);
    }
public:
    const rtosc_arg_val_t& val() { return res; }
};

class set_init final : public spa::audio::visitor
{
    template<class T> using ci = spa::audio::control_in<T>;
    const rtosc_arg_val_t& av;
    void visit(ci<int> &p) override { assert(av.type == 'i'); p.def = av.val.i; }
    void visit(ci<long long int> &p) override { assert(av.type == 'h'); p.def = av.val.h; }
    void visit(ci<float> &p) override { assert(av.type == 'f'); p.def = av.val.f; }
    void visit(ci<double> &p) override { assert(av.type == 'd'); p.def = av.val.d; }
    void visit(ci<bool> &p) override { assert(av.type == 'T' || av.type == 'F'); p.def = av.val.T; }
public:
    set_init(const rtosc_arg_val_t& av) : av(av) {}
};

spa::port_ref_base* new_port(const char* metadata, const char* args)
{
    assert(args);
    rtosc::Port::MetaContainer meta(metadata);
    bool is_enumerated = false;
    (void)is_enumerated; // TODO
    bool is_parameter = false;
    set_min min_setter;
    set_max max_setter;
    set_step step_setter;
    set_scale_type scale_type_setter;

    for(const auto& x : meta)
    {
        if(!strcmp(x.title, "parameter"))
            is_parameter = true;
        else if(!strcmp(x.title, "enumerated"))
            is_enumerated = true;
        else if(!strcmp(x.title, "min"))
            min_setter.min = x.value;
        else if(!strcmp(x.title, "max"))
            max_setter.max = x.value;
        else if(!strcmp(x.title, "scale")) {
            if(!strcmp(x.value, "linear"))
            {}
            else if(!strcmp(x.value, "logarithmic"))
                scale_type_setter.scale_type =
                    spa::audio::scale_type_t::logartihmic;
            else throw std::runtime_error("unknown scale type");
        }

        /*else if(!strncmp(x.title, "map ", 4)) {
            ++mappings[atoi(x.title + 4)];
            ++mapping_values[x.value];
        }*/
    }
    if(is_parameter)
    {
        spa::port_ref_base* res = port_from_osc_args(args);
        is_bool_t is_bool;
        res->accept(is_bool);
        if(!is_bool.res)
        {
            if(min_setter.min) res->accept(min_setter); else if(!is_bool.res) throw std::runtime_error("Port has no minimum value");
            if(max_setter.max) res->accept(max_setter); else if(!is_bool.res) throw std::runtime_error("Port has no maximum value");
            if(scale_type_setter.scale_type != spa::audio::scale_type_t::logartihmic)
                res->accept(scale_type_setter);
        }
        return res;
    }
    else return nullptr;
}

spa::port_ref_base &ZynOscPlugin::port(const char *pname) {
    // TODO: add those to map?
    if(!strcmp(pname, "buffersize")) return p_buffersize; // TODO: use slashes?
    else if(!strcmp(pname, "osc")) return p_osc_in;
    else if(!strcmp(pname, "out")) return p_out;
    else if(!strcmp(pname, "samplecount")) return p_samplecount;
    else if(!strcmp(pname, "samplerate")) return p_samplerate;
    else
    {
        spa::port_ref_base* new_ref = nullptr;
        char types[2+1];
        rtosc_arg_t args[2];
        rtosc::path_search(zyn::Master::ports, pname, nullptr,
                           types, sizeof(types), args, sizeof(args));
        if(!strcmp(types, "sb"))
        {
            const char* metadata = reinterpret_cast<char*>(args[1].b.data);
            if(!metadata)
                metadata = "";
            new_ref = new_port(metadata, strchr(args[0].s, ':'));
        }

        if(new_ref) {
            capture cap;
            char loc[1024];
            cap.obj = master;
            cap.loc = loc;
            cap.loc_size = sizeof(loc);
            char msgbuf[1024];
            std::size_t length =
                rtosc_message(msgbuf, sizeof(msgbuf), pname, "");
            if(!length)
                throw std::runtime_error("Could not build rtosc message");
            zyn::Master::ports.dispatch(msgbuf, cap, true);
            if(cap.matches != 1)
                throw std::runtime_error("Could not find port"); // TODO...
            set_init init_setter(cap.val());
            new_ref->accept(init_setter);
            ports.emplace(pname, new_ref);
            return *new_ref;
        } else
            throw spa::port_not_found(pname);
    }
}

void ZynOscPlugin::check_osc()
{
    while(p_osc_in.read_msg())
    {
        const char* path = p_osc_in.path();
#if 0
        printf("received: %s %s\n", path, p_osc_in.types());
        if(p_osc_in.types()[0] == 'i')
        {
            printf("arg0: %d\n", +p_osc_in.arg(0).i);
        }
#endif
        master->uToB->raw_write(path);
        }
}

void ZynOscPlugin::hide_ui()
{
    if(ui_pid)
    {
        kill(ui_pid, SIGTERM);
        ui_pid = 0;
    }
    else
    {
        // zyn-fusion is not running
    }
}

unsigned ZynOscPlugin::net_port() const {
    const char* addr = middleware->getServerAddress();
    const char* port_str = strrchr(addr, ':');
    assert(port_str);
    return static_cast<unsigned>(atoi(port_str + 1));
}

bool ZynOscPlugin::save(const char *savefile, uint64_t ticket) {
    middleware->messageAnywhere("/save_xmz", "st", savefile, ticket);
    return true;
}

bool ZynOscPlugin::load(const char *savefile, uint64_t ticket) {
    middleware->messageAnywhere("/load_xmz", "st", savefile, ticket);
    return true;
}

void ZynOscPlugin::restore(uint64_t ticket)
{
    middleware->messageAnywhere("/change-synth", "iiit",
                                (int)p_samplerate, (int)p_buffersize,
                                middleware->getSynth().oscilsize, ticket);
}



bool ZynOscPlugin::save_check(const char *savefile, uint64_t ticket) {
    mutex_guard guard(cb_mutex);
    return recent_save.file == savefile &&
        recent_save.stamp == ticket &&
        recent_save.status; }

bool ZynOscPlugin::load_check(const char *savefile, uint64_t ticket) {
    mutex_guard guard(cb_mutex);
    return recent_load.file == savefile &&
        recent_load.stamp == ticket &&
        recent_load.status; }
bool ZynOscPlugin::restore_check(uint64_t ticket) {
    mutex_guard guard(cb_mutex);
    if(recent_restore.stamp == ticket) {
        masterChangedCallback(middleware->spawnMaster());
        return true;
    }
    else
        return false;
}

void ZynOscPlugin::masterChangedCallback(zyn::Master *m)
{
    master = m;
    master->setMasterChangedCallback(_masterChangedCallback, this);
}

void ZynOscPlugin::_masterChangedCallback(void *ptr, zyn::Master *m)
{
    (static_cast<ZynOscPlugin*>(ptr))->masterChangedCallback(m);
}

extern "C" {
//! the main entry point
const spa::descriptor* spa_descriptor(unsigned long )
{
    return new ZynOscDescriptor;
}
}

ZynOscPlugin::ZynOscPlugin()
    : p_osc_in(16384, 2048)
{
}

ZynOscPlugin::~ZynOscPlugin()
{
    // usually, this is not our job...
    hide_ui();

    auto *tmp = middleware;
    middleware = nullptr;
    middlewareThread->join();
    delete tmp;
    delete middlewareThread;
}

void ZynOscPlugin::init()
{
    zyn::SYNTH_T synth;
    synth.samplerate = p_samplerate;

    config.init();
    // disable compression for being conform with LMMS' format
    config.cfg.GzipCompression = 0;

    zyn::sprng(time(nullptr));

    synth.alias();
    middleware = new zyn::MiddleWare(std::move(synth), &config);
    middleware->setUiCallback(_uiCallback, this);
    masterChangedCallback(middleware->spawnMaster());

    middlewareThread = new std::thread([this]() {
        while(middleware) {
            middleware->tick();
            usleep(1000);
        }
    });
}

void ZynOscPlugin::uiCallback(const char *msg)
{
    //fprintf(stderr, "UI callback: \"%s\".\n", msg);
    if(!strcmp(msg, "/save_xmz") || !strcmp(msg, "/load_xmz") ||
        !strcmp(msg, "/change-synth"))
    {
        mutex_guard guard(cb_mutex);
#ifdef SAVE_OSC_DEBUG
        fprintf(stderr, "Received message \"%s\".\n", msg);
#endif
        if(msg[1] == 'c')
        {
            recent_restore.stamp = rtosc_argument(msg, 0).t;
            recent_restore.status = true;
        }
        else
        {
            recent_op_t& recent = (msg[1] == 'l')
                ? recent_load
                : recent_save;
            recent.file = rtosc_argument(msg, 0).s;
            recent.stamp = rtosc_argument(msg, 1).t;
            recent.status = rtosc_argument(msg, 2).T;
        }
    }
    else if(!strcmp(msg, "/damage"))
    {
        // (ignore)
    }
    else if(!strcmp(msg, "/connection-remove"))
    {
        // TODO: save this info somewhere
    }
//    else
//        fprintf(stderr, "OSC Plugin: Unknown message \"%s\" from "
//            "MiddleWare, ignoring...\n", msg);
}

spa::descriptor::hoster_t ZynOscDescriptor::hoster() const
{
    return hoster_t::github;
}

const char *ZynOscDescriptor::organization_url() const
{
    return nullptr; // none on SF
}

const char *ZynOscDescriptor::project_url() const
{
    return "zynaddsubfx";
}

const char* ZynOscDescriptor::label() const {
    return "ZASF"; /* conforming to zyn's DSSI plugin */ }

const char* ZynOscDescriptor::name() const { return "ZynAddSubFX"; }

const char* ZynOscDescriptor::authors() const {
    return "JohannesLorenz";
}

spa::descriptor::license_type ZynOscDescriptor::license() const {
    return license_type::gpl_2_0;
}

const char* ZynOscDescriptor::project() const { return "TODO"; }
ZynOscPlugin* ZynOscDescriptor::instantiate() const
{
    return new ZynOscPlugin;
}

spa::simple_vec<spa::simple_str> ZynOscDescriptor::port_names() const
{
    return { "out", "buffersize", "osc", "samplecount", "samplerate" };
}


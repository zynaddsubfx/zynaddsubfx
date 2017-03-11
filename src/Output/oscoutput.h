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

#ifndef OSC_AUDIO_OUTPUT_PLUGIN_H
#define OSC_AUDIO_OUTPUT_PLUGIN_H

#include <thread>
#include <mutex>
#include <map>

#include <spa/audio.h>

#include "../globals.h"
#include "../Misc/Config.h"
#include "../Misc/MiddleWare.h"

class ZynOscPlugin : public spa::plugin
{
    void check_osc();
public:
    //SPA_PLUGIN

    static const spa::descriptor* getOscDescriptor(unsigned long index);
    static const ZynOscPlugin* instantiate(const spa::descriptor*, unsigned long sampleRate);

    void run() override;

    void ui_ext_show(bool show) override;

    std::map<std::string, spa::port_ref_base*> ports;
    spa::port_ref_base& port(const char* pname) override;

    // TODO: handle UI callback?

    void masterChangedCallback(zyn::Master* m);
    static void _masterChangedCallback(void* ptr, zyn::Master* m);

    std::mutex cb_mutex;
    using mutex_guard = std::lock_guard<std::mutex>;

    // TODO: provide whole map instead, such that multiple loads can be started
    //       and queried later?
    struct recent_op_t {
        std::string file;
        //! this must be 64 bit in order to be compliant with OSC timestamps
        uint64_t stamp;
        bool status;
    } recent_load, recent_save;

public:	// FEATURE: make these private?
    //! The sole constructor.
    ZynOscPlugin();
    ~ZynOscPlugin();

    void init() override;

    static void _uiCallback(void* ptr, const char* msg)
    {
        ((ZynOscPlugin*)ptr)->uiCallback(msg);
    }

    void uiCallback(const char* msg);

    spa::audio::stereo::out p_out;
    spa::audio::stereo::in p_in;
    spa::audio::buffersize p_buffersize;
    spa::audio::osc_ringbuffer_in p_osc_in;
    spa::audio::samplecount p_samplecount;
    spa::audio::samplerate p_samplerate;

private:
    void hide_ui();

    pid_t ui_pid = 0;
    long sampleRate;
    zyn::MiddleWare *middleware;
    std::thread *middlewareThread;

    zyn::Config config;
    zyn::Master* master = nullptr;

    unsigned net_port() const override;

    //! Let the plugin dump a savefile. The success of the operation will
    //! need to be checked later by check_save()
    //! @param savefile The destination to dump the savefile to
    //! @param ticket A value that, combined with @p savefile, identifies
    //!   that operation, e.g. an increasing counter or an OSC timestamp
    //! @return true iff the saving request could be sent
    bool save(const char* savefile, uint64_t ticket) override;

    //! Let the plugin load a savefile. The success of the operation will
    //! need to be checked later by check_load()
    //! @param savefile The path to load the savefile from
    //! @param ticket A value that, combined with @p savefile, identifies
    //!   that operation, e.g. an increasing counter or an OSC timestamp
    //! @return true iff the loading request could be sent
    bool load(const char* savefile, uint64_t ticket) override;

    //! Check if a requested save operation succeeded
    bool save_check(const char* savefile, uint64_t ticket) override;

    //! Check if a requested load operation succeeded
    bool load_check(const char* savefile, uint64_t ticket) override;
};

class ZynOscDescriptor : public spa::descriptor
{
    SPA_DESCRIPTOR

    hoster_t hoster() const override;
    const char* organization_url() const override;
    const char* project_url() const override;
    const char* label() const override;
    const char* branch() const override { return "osc-plugin"; }

    const char* project() const override;
    const char* name() const override;
    const char* authors() const override;

    license_type license() const override;

    const char* description_line() const override { return "ZynAddSubFX"; }
    const char* description_full() const override {
        return "The ZynAddSubFX synthesizer"; }

    const char** xpm_load() const override;

    bool ui_ext() const override { return true; } // TODO...

//	int id() const;

    ZynOscPlugin* instantiate() const;
    bool save_has() const override { return true; }
    bool load_has() const override { return true; }
    // if you add more: comma separated, no spaces
    const char* save_formats() const override { return "xmz"; }

    spa::simple_vec<spa::simple_str> port_names() const override;
};

#endif

/*
  ZynAddSubFX - a software synthesizer

  Controller.cpp - (Midi) Controllers implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Controller.h"
#include "../Misc/Util.h"
#include "../Misc/Time.h"
#include "../Misc/XMLwrapper.h"
#include <cmath>
#include <cstdio>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
using namespace rtosc;

namespace zyn {

#define rObject Controller

#define rChangeCbBase if (obj->time) { obj->last_update_timestamp = obj->time->time(); }

#undef rChangeCb
#define rChangeCb rChangeCbBase
const rtosc::Ports Controller::ports = {
#undef rChangeCb
#define rChangeCb obj->setpanning(); rChangeCbBase
    rParamZyn(panning.depth,       rShort("pan.d"), rDefault(64),
        "Depth of Panning MIDI Control"),
#undef rChangeCb
#define rChangeCb obj->setfiltercutoff(); rChangeCbBase
    rParamZyn(filtercutoff.depth,  rShort("fc.d"), rDefault(64),
        "Depth of Filter Cutoff MIDI Control"),
#undef rChangeCb
#define rChangeCb obj->setfilterq(); rChangeCbBase
    rParamZyn(filterq.depth,       rShort("fq.d"), rDefault(64),
        "Depth of Filter Q MIDI Control"),
#undef rChangeCb
#define rChangeCb obj->setbandwidth(); rChangeCbBase
    rParamZyn(bandwidth.depth,     rShort("bw.d"), rDefault(64),
        "Depth of Bandwidth MIDI Control"),
    rToggle(bandwidth.exponential, rShort("bw.exp"), rDefault(false),
        "Bandwidth Exponential Mode"),
#undef rChangeCb
#define rChangeCb obj->setmodwheel(); rChangeCbBase
    rParamZyn(modwheel.depth,      rShort("mdw.d"), rDefault(80),
        "Depth of Modwheel MIDI Control"),
    rToggle(modwheel.exponential,  rShort("mdw.exp"), rDefault(false),
        "Modwheel Exponential Mode"),
#undef rChangeCb
#define rChangeCb obj->setpitchwheel(); rChangeCbBase
    rToggle(pitchwheel.is_split,   rDefault(false),
        "If PitchWheel has unified bendrange or not"),
    rParamI(pitchwheel.bendrange,  rShort("pch.d"), rDefault(200),
            rLinear(-6400, 6400), rUnit(% of semitone),
        "Range of MIDI Pitch Wheel"),
    rParamI(pitchwheel.bendrange_down, rDefault(0),
        "Lower Range of MIDI Pitch Wheel"),
#undef rChangeCb
#define rChangeCb obj->setexpression(); rChangeCbBase
    rToggle(expression.receive, rShort("exp.rcv"), rDefault(true),
        "Expression MIDI Receive"),
#undef rChangeCb
#define rChangeCb obj->setfmamp(); rChangeCbBase
    rToggle(fmamp.receive,      rShort("fma.rcv"), rDefault(true),
        "FM amplitude MIDI Receive"),
#undef rChangeCb
#define rChangeCb obj->setvolume(); rChangeCbBase
    rToggle(volume.receive,     rShort("vol.rcv"), rDefault(true),
        "Volume MIDI Receive"),
#undef rChangeCb
#define rChangeCb obj->setsustain(); rChangeCbBase
    rToggle(sustain.receive,    rShort("sus.rcv"), rDefault(true),
        "Sustain MIDI Receive"),
#undef rChangeCb
#define rChangeCb rChangeCbBase
    rToggle(portamento.receive, rShort("prt.rcv"), rDefault(true),
        "Portamento MIDI Receive"),
    rToggle(portamento.portamento, rDefault(false),
        "Portamento Enable"),
    rToggle(portamento.automode, rDefault(true),
        "Portamento Auto mode"),
    rParamZyn(portamento.time,          rShort("time"), rDefault(64),
        "Portamento Length"),
    rToggle(portamento.proportional,    rShort("propt."), rDefault(false),
            "Whether the portamento time is proportional"
            "to the size of the interval between two notes."),
    rParamZyn(portamento.propRate,      rShort("scale"), rDefault(80),
        "Portamento proportional scale"),
    rParamZyn(portamento.propDepth,     rShort("depth"), rDefault(90),
        "Portamento proportional depth"),
    rParamZyn(portamento.pitchthresh,   rShort("thresh"), rDefault(3),
        "Threshold for portamento"),
    rToggle(portamento.pitchthreshtype, rShort("tr.type"), rDefault(true),
        "Type of threshold"),
    rParamZyn(portamento.updowntimestretch, rShort("up/dwn"), rDefault(64),
        "Relative length of glide up vs glide down"),
#undef rChangeCb
#define rChangeCb obj->setresonancecenter(); rChangeCbBase
    rParamZyn(resonancecenter.depth,    rShort("rfc.d"), rDefault(64),
        "Resonance Center MIDI Depth"),
#undef rChangeCb
#define rChangeCb obj->setresonancebw(); rChangeCbBase
    rParamZyn(resonancebandwidth.depth, rShort("rbw.d"), rDefault(64),
        "Resonance Bandwidth MIDI Depth"),
#undef rChangeCb
#define rChangeCb rChangeCbBase
    rToggle(NRPN.receive, rDefault(true), "NRPN MIDI Enable"),
    rAction(defaults),
};
#undef rChangeCb

Controller::Controller(const SYNTH_T &synth_, const AbsTime *time_)
    :time(time_), last_update_timestamp(0), synth(synth_)
{
    defaults();
    resetall();
}

void Controller::defaults()
{
    pitchwheel.bendrange         = 200; //2 halftones
    pitchwheel.bendrange_down    = 0; //Unused by default
    pitchwheel.is_split          = false;
    expression.receive           = 1;
    panning.depth                = 64;
    filtercutoff.depth           = 64;
    filterq.depth                = 64;
    bandwidth.depth              = 64;
    bandwidth.exponential        = 0;
    modwheel.depth               = 80;
    modwheel.exponential         = 0;
    fmamp.receive                = 1;
    volume.receive               = 1;
    sustain.receive              = 1;
    NRPN.receive                 = 1;

    portamento.portamento        = 0;
    portamento.automode          = 1;
    portamento.proportional      = 0;
    portamento.propRate          = 80;
    portamento.propDepth         = 90;
    portamento.receive           = 1;
    portamento.time              = 64;
    portamento.updowntimestretch = 64;
    portamento.pitchthresh       = 3;
    portamento.pitchthreshtype   = 1;

    resonancecenter.depth        = 64;
    resonancebandwidth.depth     = 64;

    setportamento(0);
}

void Controller::resetall()
{
    setpitchwheel(0); //center
    setexpression(127);
    setpanning(64);
    setfiltercutoff(64);
    setfilterq(64);
    setbandwidth(64);
    setmodwheel(64);
    setfmamp(127);
    setvolume(127);
    setsustain(0);
    setresonancecenter(64);
    setresonancebw(64);

    //reset the NRPN
    NRPN.parhi = -1;
    NRPN.parlo = -1;
    NRPN.valhi = -1;
    NRPN.vallo = -1;
}

void Controller::setpitchwheel(int value)
{
    pitchwheel.data = value;
    setpitchwheel();
}

void Controller::setpitchwheel()
{
    int value = pitchwheel.data;
    float cents = value / 8192.0f;
    if(pitchwheel.is_split && cents < 0)
        cents *= pitchwheel.bendrange_down;
    else
        cents *= pitchwheel.bendrange;
    pitchwheel.relfreq = powf(2, cents / 1200.0f);
    //fprintf(stderr,"%ld %ld -> %.3f\n",pitchwheel.bendrange,pitchwheel.data,pitchwheel.relfreq);fflush(stderr);
}

void Controller::setexpression(int value)
{
    expression.data = value;
    setexpression();
}

void Controller::setexpression(void)
{
    int value = expression.data;
    if(expression.receive != 0)
    {
        assert( value <= 127 ); /* to protect what's left of JML's hearing */

        expression.relvolume = value / 127.0f;
    }
    else
        expression.relvolume = 1.0f;
}

void Controller::setpanning(int value)
{
    panning.data = value;
    setpanning();
}

void Controller::setpanning(void)
{
    int value = panning.data;
    panning.pan  = (value / 128.0f - 0.5f) * (panning.depth / 64.0f);
}

void Controller::setfiltercutoff(int value)
{
    filtercutoff.data  = value;
    setfiltercutoff();
}

void Controller::setfiltercutoff(void)
{
    int value = filtercutoff.data;
    filtercutoff.relfreq =
        (value - 64.0f) * filtercutoff.depth / 4096.0f * 3.321928f;         //3.3219f..=ln2(10)
}

void Controller::setfilterq(int value)
{
    filterq.data = value;
    setfilterq();
}

void Controller::setfilterq(void)
{
    int value = filterq.data;
    filterq.relq = powf(30.0f, (value - 64.0f) / 64.0f * (filterq.depth / 64.0f));
}

void Controller::setbandwidth(int value)
{
    bandwidth.data = value;
    setbandwidth();
}

void Controller::setbandwidth(void)
{
    int value = bandwidth.data;
    if(bandwidth.exponential == 0) {
        float tmp = powf(25.0f, powf(bandwidth.depth / 127.0f, 1.5f)) - 1.0f;
        if((value < 64) && (bandwidth.depth >= 64))
            tmp = 1.0f;
        bandwidth.relbw = (value / 64.0f - 1.0f) * tmp + 1.0f;
        if(bandwidth.relbw < 0.01f)
            bandwidth.relbw = 0.01f;
    }
    else
        bandwidth.relbw =
            powf(25.0f, (value - 64.0f) / 64.0f * (bandwidth.depth / 64.0f));
    ;
}

void Controller::setmodwheel(int value)
{
    modwheel.data = value;
    setmodwheel();
}

void Controller::setmodwheel(void)
{
    int value = modwheel.data;
    if(modwheel.exponential == 0) {
        float tmp =
            powf(25.0f, powf(modwheel.depth / 127.0f, 1.5f) * 2.0f) / 25.0f;
        if((value < 64) && (modwheel.depth >= 64))
            tmp = 1.0f;
        modwheel.relmod = (value / 64.0f - 1.0f) * tmp + 1.0f;
        if(modwheel.relmod < 0.0f)
            modwheel.relmod = 0.0f;
    }
    else
        modwheel.relmod =
            powf(25.0f, (value - 64.0f) / 64.0f * (modwheel.depth / 80.0f));
}

void Controller::setfmamp(int value)
{
    fmamp.data = value;
    setfmamp();
}

void Controller::setfmamp(void)
{
    int value = fmamp.data;
    fmamp.relamp = value / 127.0f;
    if(fmamp.receive != 0)
        fmamp.relamp = value / 127.0f;
    else
        fmamp.relamp = 1.0f;
}

void Controller::setvolume(int value)
{
    volume.data = value;
    setvolume();
}

void Controller::setvolume(void)
{
    int value = volume.data;
    if(volume.receive != 0)
    {
        /* volume.volume = powf(0.1f, (127 - value) / 127.0f * 2.0f); */
        /* rather than doing something fancy that results in a value
         * of 0 not completely muting the output, do the reasonable
         * thing and just give the value the user ordered. */
        assert( value <= 127 );

        volume.volume = value / 127.0f;
    }
    else
        volume.volume = 1.0f;
}

void Controller::setsustain(int value)
{
    sustain.data = value;
    setsustain();
}

void Controller::setsustain(void)
{
    int value = sustain.data;
    if(sustain.receive != 0)
        sustain.sustain = ((value < 64) ? 0 : 1);
    else
        sustain.sustain = 0;
}

void Controller::setportamento(int value)
{
    portamento.data = value;
    if(portamento.receive != 0)
        portamento.portamento = ((value < 64) ? 0 : 1);
}

void Controller::setresonancecenter(int value)
{
    resonancecenter.data = value;
    setresonancecenter();
}

void Controller::setresonancecenter(void)
{
    int value = resonancecenter.data;
    resonancecenter.relcenter =
        powf(3.0f, (value - 64.0f) / 64.0f * (resonancecenter.depth / 64.0f));
}

void Controller::setresonancebw(int value)
{
    resonancebandwidth.data  = value;
}

void Controller::setresonancebw(void)
{
    int value = resonancebandwidth.data;
    resonancebandwidth.relbw =
        powf(1.5f, (value - 64.0f) / 64.0f * (resonancebandwidth.depth / 127.0f));
}


//Returns 0 if there is NRPN or 1 if there is not
int Controller::getnrpn(int *parhi, int *parlo, int *valhi, int *vallo)
{
    if(NRPN.receive == 0)
        return 1;
    if((NRPN.parhi < 0) || (NRPN.parlo < 0) || (NRPN.valhi < 0)
       || (NRPN.vallo < 0))
        return 1;

    *parhi = NRPN.parhi;
    *parlo = NRPN.parlo;
    *valhi = NRPN.valhi;
    *vallo = NRPN.vallo;
    return 0;
}


void Controller::setparameternumber(unsigned int type, int value)
{
    switch(type) {
        case C_nrpnhi:
            NRPN.parhi = value;
            NRPN.valhi = -1;
            NRPN.vallo = -1; //clear the values
            break;
        case C_nrpnlo:
            NRPN.parlo = value;
            NRPN.valhi = -1;
            NRPN.vallo = -1; //clear the values
            break;
        case C_dataentryhi:
            if((NRPN.parhi >= 0) && (NRPN.parlo >= 0))
                NRPN.valhi = value;
            break;
        case C_dataentrylo:
            if((NRPN.parhi >= 0) && (NRPN.parlo >= 0))
                NRPN.vallo = value;
            break;
    }
}



void Controller::add2XML(XMLwrapper& xml)
{
    xml.addpar("pitchwheel_bendrange", pitchwheel.bendrange);
    xml.addpar("pitchwheel_bendrange_down", pitchwheel.bendrange_down);
    xml.addparbool("pitchwheel_split", pitchwheel.is_split);

    xml.addparbool("expression_receive", expression.receive);
    xml.addpar("panning_depth", panning.depth);
    xml.addpar("filter_cutoff_depth", filtercutoff.depth);
    xml.addpar("filter_q_depth", filterq.depth);
    xml.addpar("bandwidth_depth", bandwidth.depth);
    xml.addpar("mod_wheel_depth", modwheel.depth);
    xml.addparbool("mod_wheel_exponential", modwheel.exponential);
    xml.addparbool("fm_amp_receive", fmamp.receive);
    xml.addparbool("volume_receive", volume.receive);
    xml.addparbool("sustain_receive", sustain.receive);

    xml.addparbool("portamento_receive", portamento.receive);
    xml.addpar("portamento_time", portamento.time);
    xml.addpar("portamento_pitchthresh", portamento.pitchthresh);
    xml.addpar("portamento_pitchthreshtype", portamento.pitchthreshtype);
    xml.addpar("portamento_portamento", portamento.portamento);
    xml.addparbool("portamento_auto", portamento.automode);
    xml.addpar("portamento_updowntimestretch", portamento.updowntimestretch);
    xml.addpar("portamento_proportional", portamento.proportional);
    xml.addpar("portamento_proprate", portamento.propRate);
    xml.addpar("portamento_propdepth", portamento.propDepth);

    xml.addpar("resonance_center_depth", resonancecenter.depth);
    xml.addpar("resonance_bandwidth_depth", resonancebandwidth.depth);
}

void Controller::getfromXML(XMLwrapper& xml)
{
    pitchwheel.bendrange = xml.getpar("pitchwheel_bendrange",
                                       pitchwheel.bendrange,
                                       -6400,
                                       6400);
    pitchwheel.bendrange_down = xml.getpar("pitchwheel_bendrange_down",
                                       pitchwheel.bendrange_down,
                                       -6400,
                                       6400);
    pitchwheel.is_split = xml.getparbool("pitchwheel_split",
                                          pitchwheel.is_split);

    expression.receive = xml.getparbool("expression_receive",
                                         expression.receive);
    panning.depth      = xml.getpar127("panning_depth", panning.depth);
    filtercutoff.depth = xml.getpar127("filter_cutoff_depth",
                                        filtercutoff.depth);
    filterq.depth        = xml.getpar127("filter_q_depth", filterq.depth);
    bandwidth.depth      = xml.getpar127("bandwidth_depth", bandwidth.depth);
    modwheel.depth       = xml.getpar127("mod_wheel_depth", modwheel.depth);
    modwheel.exponential = xml.getparbool("mod_wheel_exponential",
                                           modwheel.exponential);
    fmamp.receive = xml.getparbool("fm_amp_receive",
                                    fmamp.receive);
    volume.receive = xml.getparbool("volume_receive",
                                     volume.receive);
    sustain.receive = xml.getparbool("sustain_receive",
                                      sustain.receive);

    portamento.receive = xml.getparbool("portamento_receive",
                                         portamento.receive);
    portamento.automode = xml.getparbool("portamento_auto",
                                         portamento.automode);
    portamento.time = xml.getpar127("portamento_time",
                                     portamento.time);
    portamento.pitchthresh = xml.getpar127("portamento_pitchthresh",
                                            portamento.pitchthresh);
    portamento.pitchthreshtype = xml.getpar127("portamento_pitchthreshtype",
                                                portamento.pitchthreshtype);
    portamento.portamento = xml.getpar127("portamento_portamento",
                                           portamento.portamento);
    portamento.updowntimestretch = xml.getpar127(
        "portamento_updowntimestretch",
        portamento.updowntimestretch);
    portamento.proportional = xml.getpar127("portamento_proportional",
                                             portamento.proportional);
    portamento.propRate = xml.getpar127("portamento_proprate",
                                         portamento.propRate);
    portamento.propDepth = xml.getpar127("portamento_propdepth",
                                          portamento.propDepth);


    resonancecenter.depth = xml.getpar127("resonance_center_depth",
                                           resonancecenter.depth);
    resonancebandwidth.depth = xml.getpar127("resonance_bandwidth_depth",
                                              resonancebandwidth.depth);
}

}

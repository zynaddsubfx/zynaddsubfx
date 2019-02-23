/*
  ZynAddSubFX - a software synthesizer

  MidiIn.C - This class is inherited by all the Midi input classes
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "MidiIn.h"
#include "../globals.h"
#include "InMgr.h"
#include <string.h>

namespace zyn {

MidiIn::MidiIn()
{
    sysex_offset = 0;
    memset(sysex_data, 0, sizeof(sysex_data));
}

uint8_t MidiIn::midiSysEx(unsigned char data)
{
    if (data & 0x80) {
        if (data == 0xF0) {
		sysex_offset = 0; /* begin */
        } else if (data == 0xF7) {
                return (2); /* end */
        } else {
                return (1); /* error */
        }
    } else if (sysex_offset >= sizeof(sysex_data)) {
        return (1); /* error */
    }
    sysex_data[sysex_offset++] = data;
    return (0);
}

void MidiIn::midiProcess(unsigned char head,
                         unsigned char num,
                         unsigned char value)
{
    MidiEvent     ev;
    unsigned char chan = head & 0x0f;

    /* SYSEX handling */
    if (head == 0xF0 || sysex_offset != 0) {
        uint8_t status = 0;

        status |= midiSysEx(head);
        status |= midiSysEx(num);
        status |= midiSysEx(value);

        if (status & 1) {
            /* error parsing SYSEX */
            sysex_offset = 0;
        } else if (status & 2) {
            /* message complete */

            if (sysex_offset >= 10 &&
                sysex_data[1] == 0x0A &&
                sysex_data[2] == 0x55) {
                ev.type = M_FLOAT_NOTE;
                ev.channel = sysex_data[3] & 0x0F;
                ev.num = sysex_data[4];
                ev.value = sysex_data[5];
                ev.log2_freq = (sysex_data[6] +
                  (sysex_data[7] / (128.0f)) +
                  (sysex_data[8] / (128.0f * 128.0f)) +
                  (sysex_data[9] / (128.0f * 128.0f * 128.0f))
                  ) / 12.0f;
                InMgr::getInstance().putEvent(ev);
            }
            return; /* message complete */
        } else {
	    return; /* wait for more data */
        }
    }
    switch(head & 0xf0) {
        case 0x80: //Note Off
            ev.type    = M_NOTE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = 0;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0x90: //Note On
            ev.type    = M_NOTE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = value;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0xA0: /* pressure, aftertouch */
            ev.type    = M_PRESSURE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = value;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0xb0: //Controller
            ev.type    = M_CONTROLLER;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = value;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0xc0: //Program Change
            ev.type    = M_PGMCHANGE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = 0;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0xe0: //Pitch Wheel
            ev.type    = M_CONTROLLER;
            ev.channel = chan;
            ev.num     = C_pitchwheel;
            ev.value   = (num + value * (int) 128) - 8192;
            InMgr::getInstance().putEvent(ev);
            break;
    }
}

}

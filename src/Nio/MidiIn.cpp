/*
    MusicIO.cpp

    Copyright 2009, Alan Calvert

    This file is part of yoshimi, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    yoshimi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <cstring>

using namespace std;

#include "../Misc/Master.h"
#include "MidiIn.h"

MidiIn::MidiIn()
{}

int MidiIn::getMidiController(unsigned char b)
{
    int ctl = C_NULL;
    switch (b)
    {
	    case 1:
            ctl = C_modwheel;            // Modulation Wheel
            break;
	    case 7:
            ctl=C_volume;                // Volume
    		break;
	    case 10:
            ctl = C_panning;             // Panning
            break;
	    case 11:
            ctl = C_expression;          // Expression
            break;
	    case 64:
            ctl = C_sustain;             // Sustain pedal
	        break;
	    case 65:
            ctl = C_portamento;          // Portamento
	        break;
	    case 71:
            ctl = C_filterq;             // Filter Q (Sound Timbre)
            break;
	    case 74:
            ctl = C_filtercutoff;        // Filter Cutoff (Brightness)
	        break;
	    case 75:
            ctl = C_bandwidth;           // BandWidth
	        break;
	    case 76:
            ctl = C_fmamp;               // FM amplitude
	        break;
	    case 77:
            ctl = C_resonance_center;    // Resonance Center Frequency
	        break;
	    case 78:
            ctl = C_resonance_bandwidth; // Resonance Bandwith
	        break;
	    case 120:
            ctl = C_allsoundsoff;        // All Sounds OFF
	        break;
	    case 121:
            ctl = C_resetallcontrollers; // Reset All Controllers
	        break;
	    case 123:
            ctl = C_allnotesoff;         // All Notes OFF
	        break;
	    // RPN and NRPN
	    case 0x06:
            ctl = C_dataentryhi;         // Data Entry (Coarse)
	         break;
	    case 0x26:
            ctl = C_dataentrylo;         // Data Entry (Fine)
	         break;
	    case 99:
            ctl = C_nrpnhi;              // NRPN (Coarse)
	         break;
	    case 98:
            ctl = C_nrpnlo;              // NRPN (Fine)
	        break;
	    default:
            ctl = C_NULL;                // an unrecognised controller!
            break;
	}
    return ctl;
}


void MidiIn::setMidiController(unsigned char ch, unsigned int ctrl,
                               int param)
{
    master->mutexLock(MUTEX_LOCK);
    master->SetController(ch, ctrl, param);
    master->mutexLock(MUTEX_UNLOCK);
}


void MidiIn::setMidiNote(unsigned char channel, unsigned char note,
                         unsigned char velocity)
{
    master->mutexLock(MUTEX_LOCK);
    master->NoteOn(channel, note, velocity);
    master->mutexLock(MUTEX_UNLOCK);
}


void MidiIn::setMidiNote(unsigned char channel, unsigned char note)
{
    master->mutexLock(MUTEX_LOCK);
    master->NoteOff(channel, note);
    master->mutexLock(MUTEX_UNLOCK);
}


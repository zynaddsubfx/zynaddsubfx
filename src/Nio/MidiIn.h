/*
    MusicIO.h

    Copyright 2009, Alan Calvert
    Copyright 2009, James Morris

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

#ifndef MIDI_IN_H
#define MIDI_IN_H

class MidiIn
{
    public:
        MidiIn();
        virtual ~MusicIO() {};

        int getMidiController(unsigned char b);
        void setMidiController(unsigned char ch, unsigned int ctrl, int param);
        void setMidiNote(unsigned char chan, unsigned char note);
        void setMidiNote(unsigned char chan, unsigned char note,
                         unsigned char velocity);
};

#endif

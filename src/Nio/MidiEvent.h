
#ifndef MIDI_EVENT
#define MIDI_EVENT

#include <iostream>

/**A class to generalize midi events*/
struct MidiEvent
{
    MidiEvent(){};
};

struct MidiNote : public MidiEvent
{
    MidiNote(char _note, char _channel, char _velocity = 0);

    char note;
    char channel;
    char velocity;
};

std::ostream &operator<<(std::ostream &out, const MidiNote &midi);

struct MidiCtl : public MidiEvent
{
    MidiCtl(char _controller, char _channel, char _value);
    
    char controller;
    char channel;
    char value;
};

std::ostream &operator<<(std::ostream &out, const MidiCtl &ctl);
#endif

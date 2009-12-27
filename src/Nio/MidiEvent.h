
#ifndef MIDI_EVENT
#define MIDI_EVENT

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

struct MidiCtl : public MidiEvent
{
    MidiCtl(char _controller, char _channel, char _value);
    
    char controller;
    char channel;
    char value;
};

#endif

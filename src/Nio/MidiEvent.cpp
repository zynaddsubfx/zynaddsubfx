#include "MidiEvent.h"

MidiNote::MidiNote(char _note, char _channel, char _velocity)
    :note(_note), channel(_channel), velocity(_velocity)
{};

MidiCtl::MidiCtl(char _controller, char _channel, char _value)
    :controller(_controller), channel(_channel), value(_value)
{};
    

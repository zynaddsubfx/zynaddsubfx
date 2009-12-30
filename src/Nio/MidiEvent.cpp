#include "MidiEvent.h"

using namespace std;

MidiNote::MidiNote(char _note, char _channel, char _velocity)
    :note(_note), channel(_channel), velocity(_velocity)
{};

ostream &operator<<(ostream &out, const MidiNote& note)
{
    out << "MidiNote: note("     << (int)note.note     << ")\n"
        << "          channel("  << (int)note.channel  << ")\n"
        << "          velocity(" << (int)note.velocity << ")";
    return out;
}

MidiCtl::MidiCtl(char _controller, char _channel, char _value)
    :controller(_controller), channel(_channel), value(_value)
{};
    
ostream &operator<<(ostream &out, const MidiCtl &ctl)
{
    out << "MidiCtl: controller(" << (int)ctl.controller << ")\n"
        << "         channel("    << (int)ctl.channel    << ")\n"
        << "         value("      << (int)ctl.value      << ")";
    return out;
}

#pragma once
class PADnoteParameters;
//Link between realtime and non-realtime layers
namespace MiddleWare
{
static void preparePadSynth(const char *path, PADnoteParameters *p);
};

//XXX Odd Odd compiler behavior has made this hack necessary (darn you linker)
#include "MiddleWare.cpp"

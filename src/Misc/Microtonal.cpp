/*
  ZynAddSubFX - a software synthesizer

  Microtonal.cpp - Tuning settings and microtonal capabilities
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2016      Mark McCurry
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cassert>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>


#include "XMLwrapper.h"
#include "Util.h"
#include "Microtonal.h"

using namespace rtosc;

#define MAX_LINE_SIZE 80

namespace zyn {

#define rObject Microtonal

/**
 * TODO
 * Consider how much of this should really exist on the rt side of things.
 * All the rt side needs is a function to map notes at various keyshifts to
 * frequencies, which does not require this many parameters...
 *
 * A good lookup table should be a good finalization of this
 */
const rtosc::Ports Microtonal::ports = {
    rToggle(Pinvertupdown, rShort("inv."), rDefault(false),
        "key mapping inverse"),
    rParamZyn(Pinvertupdowncenter, rShort("center"), rDefault(60),
        "center of the inversion"),
    rToggle(Penabled, rShort("enable"), rDefault(false),
        "Enable for microtonal mode"),
    rParamZyn(PAnote, rShort("1/1 midi note"), rDefault(69),
        "The note for 'A'"),
    rParamF(PAfreq,   rShort("ref freq"), rDefault(440.0f),
         rLog(1.0,20000.0),
        "Frequency of the 'A' note"),
    rParamZyn(Pscaleshift, rShort("shift"), rDefault(64),
        "UNDOCUMENTED"),
    rParamZyn(Pfirstkey, rShort("first key"), rDefault(0),
        "First key to retune"),
    rParamZyn(Plastkey,  rShort("last key"), rDefault(127),
        "Last key to retune"),
    rParamZyn(Pmiddlenote, rShort("middle"), rDefault(60),
        "Scale degree 0 note"),

    //TODO check to see if this should be exposed
    rParamZyn(Pmapsize, rDefault(12), "Size of key map"),
    rToggle(Pmappingenabled, rDefault(false), "Mapping Enable"),

    rParams(Pmapping, 128, rDefault([0 1 ...]), "Mapping of keys"),
    rParamZyn(Pglobalfinedetune, rShort("fine"), rDefault(64),
        "Fine detune for all notes"),

    rString(Pname, MICROTONAL_MAX_NAME_LEN,    rShort("name"),
        rDefault("12tET"), "Microtonal Name"),
    rString(Pcomment, MICROTONAL_MAX_NAME_LEN, rShort("comment"),
        rDefault("Equal Temperament 12 notes per octave"), "Microtonal comments"),

    {"octavesize:", rDoc("Get octave size"), 0, [](const char*, RtData &d)
        {
            Microtonal &m = *(Microtonal*)d.obj;
            d.reply(d.loc, "i", m.getoctavesize());
        }},
    {"mapping::s", rDoc("Get user editable tunings"), 0, [](const char *msg, RtData &d)
        {
            char buf[100*MAX_OCTAVE_SIZE] = {};
            char tmpbuf[100] = {};
            Microtonal &m = *(Microtonal*)d.obj;
            if(rtosc_narguments(msg) == 1) {
                m.texttomapping(rtosc_argument(msg,0).s);
            } else {
                for (int i=0;i<m.Pmapsize;i++){
                    if (i!=0)
                        strncat(buf, "\n", sizeof(buf)-1);
                    if (m.Pmapping[i]==-1)
                        snprintf(tmpbuf,100,"x");
                    else
                        snprintf(tmpbuf,100,"%d",m.Pmapping[i]);
                    strncat(buf, tmpbuf, sizeof(buf)-1);
                };
                d.reply(d.loc, "s", buf);
            }
            }},
    {"tunings::s", rDoc("Get user editable tunings"), 0, [](const char *msg, RtData &d)
        {
            char buf[100*MAX_OCTAVE_SIZE] = {};
            char tmpbuf[100] = {};
            Microtonal &m = *(Microtonal*)d.obj;
            if(rtosc_narguments(msg) == 1) {
                int err = m.texttotunings(rtosc_argument(msg,0).s);
                if (err>=0)
                    d.reply("/alert", "s",
                            "Parse Error: The input may contain only numbers (like 232.59)\n"
                            "or divisions (like 121/64).");
                if (err==-2)
                    d.reply("/alert", "s", "Parse Error: The input is empty.");
            } else {
                for (int i=0;i<m.getoctavesize();i++){
                    if (i!=0)
                        strncat(buf, "\n", sizeof(buf)-1);
                    m.tuningtoline(i,tmpbuf,100);
                    strncat(buf, tmpbuf, sizeof(buf)-1);
                };
                d.reply(d.loc, "s", buf);
            }
        }},

#define COPY(x) self.x = other->x;
    {"paste:b", rProp(internal) rDoc("Clone Input Microtonal Object"), 0,
        [](const char *msg, RtData &d)
        {
            rtosc_blob_t b = rtosc_argument(msg, 0).b;
            assert(b.len == sizeof(void*));
            Microtonal *other = *(Microtonal**)b.data;
            Microtonal &self  = *(Microtonal*)d.obj;
            //oh how I wish there was some darn reflection for this...

            COPY(Pinvertupdown);
            COPY(Pinvertupdowncenter);
            COPY(Penabled);
            COPY(PAnote);
            COPY(PAfreq);
            COPY(Pscaleshift);
            COPY(Pfirstkey);
            COPY(Plastkey);
            COPY(Pmiddlenote);
            COPY(Pmapsize);
            COPY(Pmappingenabled);
            for(int i=0; i<self.octavesize; ++i)
                self.octave[i] = other->octave[i];
            COPY(Pglobalfinedetune);

            memcpy(self.Pname,    other->Pname,    sizeof(self.Pname));
            memcpy(self.Pcomment, other->Pcomment, sizeof(self.Pcomment));
            COPY(octavesize);

            for(int i=0; i<self.octavesize; ++i)
                self.octave[i] = other->octave[i];
            d.reply("/free", "sb", "Microtonal", b.len, b.data);
        }},
    {"paste_scl:b", rProp(internal) rDoc("Clone Input scl Object"), 0,
        [](const char *msg, RtData &d)
        {
            rtosc_blob_t b = rtosc_argument(msg, 0).b;
            assert(b.len == sizeof(void*));
            SclInfo *other = *(SclInfo**)b.data;
            Microtonal &self  = *(Microtonal*)d.obj;
            memcpy(self.Pname,    other->Pname,    sizeof(self.Pname));
            memcpy(self.Pcomment, other->Pcomment, sizeof(self.Pcomment));
            COPY(octavesize);

            for(int i=0; i<self.octavesize; ++i)
                self.octave[i] = other->octave[i];
            d.reply("/free", "sb", "SclInfo", b.len, b.data);
        }},
    {"paste_kbm:b", rProp(internal) rDoc("Clone Input kbm Object"), 0,
        [](const char *msg, RtData &d)
        {
            rtosc_blob_t b = rtosc_argument(msg, 0).b;
            assert(b.len == sizeof(void*));
            KbmInfo *other = *(KbmInfo**)b.data;
            Microtonal &self  = *(Microtonal*)d.obj;
            COPY(Pmapsize);
            COPY(Pfirstkey);
            COPY(Plastkey);
            COPY(Pmiddlenote);
            COPY(PAnote);
            COPY(PAfreq);
            COPY(Pmappingenabled);

            for(int i=0; i<128; ++i)
                self.Pmapping[i] = other->Pmapping[i];
            d.reply("/free", "sb", "KbmInfo", b.len, b.data);
        }},
#undef COPY
};


Microtonal::Microtonal(const int &gzip_compression)
    : gzip_compression(gzip_compression)
{
    defaults();
}

void Microtonal::defaults()
{
    Pinvertupdown = 0;
    Pinvertupdowncenter = 60;
    octavesize  = 12;
    Penabled    = 0;
    PAnote      = 69;
    PAfreq      = 440.0f;
    Pscaleshift = 64;

    Pfirstkey       = 0;
    Plastkey        = 127;
    Pmiddlenote     = 60;
    Pmapsize        = 12;
    Pmappingenabled = 0;

    for(int i = 0; i < 128; ++i)
        Pmapping[i] = i;

    for(int i = 0; i < MAX_OCTAVE_SIZE; ++i) {
        octave[i].tuning_log2 = (i % octavesize + 1) / 12.0f;
        octave[i].type =  1;
        octave[i].x1   = (i % octavesize + 1) * 100;
        octave[i].x2   =  0;
    }
    octave[11].type = 2;
    octave[11].x1   = 2;
    octave[11].x2   = 1;
    for(int i = 0; i < MICROTONAL_MAX_NAME_LEN; ++i) {
        Pname[i]    = '\0';
        Pcomment[i] = '\0';
    }
    snprintf((char *) Pname, MICROTONAL_MAX_NAME_LEN, "12tET");
    snprintf((char *) Pcomment,
             MICROTONAL_MAX_NAME_LEN,
             "Equal Temperament 12 notes per octave");
    Pglobalfinedetune = 64;
}

Microtonal::~Microtonal()
{}

/*
 * Get the size of the octave
 */
unsigned char Microtonal::getoctavesize() const
{
    if(Penabled != 0)
        return octavesize;
    else
        return 12;
}

/*
 * Update the logarithmic power of two frequency according the note number
 */
bool Microtonal::updatenotefreq_log2(float &note_log2_freq, int keyshift) const
{
    note_t note = roundf(12.0f * note_log2_freq);
    float freq_log2 = note_log2_freq;

    // in this function will appears many times things like this:
    // var=(a+b*100)%b
    // I had written this way because if I use var=a%b gives unwanted results when a<0
    // This is the same with divisions.

    if((Pinvertupdown != 0) && ((Pmappingenabled == 0) || (Penabled == 0))) {
        note = (int) Pinvertupdowncenter * 2 - note;
        freq_log2 = Pinvertupdowncenter * (2.0f / 12.0f) - freq_log2;
    }

    /* compute global fine detune, -64.0f .. 63.0f cents */
    const float globalfinedetunerap_log2 = (Pglobalfinedetune - 64.0f) / 1200.0f;

    if(Penabled == 0) { /* 12tET */
        freq_log2 += (keyshift - PAnote) / 12.0f;
    }
    else { /* Microtonal */
        const int scaleshift =
            ((int)Pscaleshift - 64 + (int) octavesize * 100) % octavesize;

        /* compute the keyshift */
        float rap_keyshift_log2;
        if(keyshift != 0) {
            const int kskey = (keyshift + (int)octavesize * 100) % octavesize;
            const int ksoct = (keyshift + (int)octavesize * 100) / octavesize - 100;

            rap_keyshift_log2 =
                ((kskey == 0) ? 0.0f : octave[kskey - 1].tuning_log2) +
                (octave[octavesize - 1].tuning_log2 * ksoct);
        }
        else {
            rap_keyshift_log2 = 0.0f;
        }

        /* if the mapping is enabled */
        if(Pmappingenabled) {
            if((note < Pfirstkey) || (note > Plastkey))
                goto failure;

            /*
             * Compute how many mapped keys are from middle note to reference note
             * and find out the proportion between the freq. of middle note and "A" note
             */
            int tmp = PAnote - Pmiddlenote;
            const bool minus = (tmp < 0);
            if(minus)
                tmp = -tmp;

            int deltanote = 0;
            for(int i = 0; i < tmp; ++i)
                if(Pmapping[i % Pmapsize] >= 0)
                    deltanote++;

            float rap_anote_middlenote_log2;
            if(deltanote == 0) {
                rap_anote_middlenote_log2 = 0.0f;
            }
            else {
                rap_anote_middlenote_log2 =
                    octave[(deltanote - 1) % octavesize].tuning_log2 +
                    octave[octavesize - 1].tuning_log2 * ((deltanote - 1) / octavesize);
            }
            if(minus)
                rap_anote_middlenote_log2 = -rap_anote_middlenote_log2;

            /* Convert from note (midi) to degree (note from the tuning) */
            int degoct =
                (note - (int)Pmiddlenote + (int) Pmapsize
                 * 200) / (int)Pmapsize - 200;
            int degkey = (note - Pmiddlenote + (int)Pmapsize * 100) % Pmapsize;
            degkey = Pmapping[degkey];

            /* check if key is not mapped */
            if(degkey < 0)
                goto failure;

            /*
             * Invert the keyboard upside-down if it is asked for
             * TODO: do the right way by using Pinvertupdowncenter
             */
            if(Pinvertupdown != 0) {
                degkey = octavesize - degkey - 1;
                degoct = -degoct;
            }

            degkey  = degkey + scaleshift;
            degoct += degkey / octavesize;
            degkey %= octavesize;

            /* compute the logrithmic frequency of the note */
            freq_log2 =
                ((degkey == 0) ? 0.0f : octave[degkey - 1].tuning_log2) +
                (octave[octavesize - 1].tuning_log2 * degoct) -
                rap_anote_middlenote_log2;
        }
        else {  /* if the mapping is disabled */
            const int nt    = note - PAnote + scaleshift;
            const int ntkey = (nt + (int)octavesize * 100) % octavesize;
            const int ntoct = (nt - ntkey) / octavesize;

            freq_log2 =
                octave[(ntkey + octavesize - 1) % octavesize].tuning_log2 +
                octave[octavesize - 1].tuning_log2 * (ntkey ? ntoct : (ntoct - 1));
        }
        if(scaleshift)
            freq_log2 -= octave[scaleshift - 1].tuning_log2;
        freq_log2 += rap_keyshift_log2;
    }

    /* common part */
    freq_log2 += log2f(PAfreq);
    freq_log2 += globalfinedetunerap_log2;

    /* update value */
    note_log2_freq = freq_log2;
    return true;

failure:
    return false;
}

/*
 * Get the note frequency in Hz, -1.0f if invalid.
 */
float Microtonal::getnotefreq(float note_log2_freq, int keyshift) const
{
    if (updatenotefreq_log2(note_log2_freq, keyshift))
        return powf(2.0f, note_log2_freq);
    else
        return -1.0f;
}

bool Microtonal::operator==(const Microtonal &micro) const
{
    return !(*this != micro);
}

bool Microtonal::operator!=(const Microtonal &micro) const
{
    //A simple macro to test equality MiCRotonal EQuals (not the perfect
    //approach, but good enough)
#define MCREQ(x) if(x != micro.x) \
        return true

    //for floats
#define FMCREQ(x) if(!((x < micro.x + 0.0001f) && (x > micro.x - 0.0001f))) \
        return true

    MCREQ(Pinvertupdown);
    MCREQ(Pinvertupdowncenter);
    MCREQ(octavesize);
    MCREQ(Penabled);
    MCREQ(PAnote);
    FMCREQ(PAfreq);
    MCREQ(Pscaleshift);

    MCREQ(Pfirstkey);
    MCREQ(Plastkey);
    MCREQ(Pmiddlenote);
    MCREQ(Pmapsize);
    MCREQ(Pmappingenabled);

    for(int i = 0; i < 128; ++i)
        MCREQ(Pmapping[i]);

    for(int i = 0; i < octavesize; ++i) {
        FMCREQ(octave[i].tuning_log2);
        MCREQ(octave[i].type);
        MCREQ(octave[i].x1);
        MCREQ(octave[i].x2);
    }
    if(strcmp((const char *)this->Pname, (const char *)micro.Pname))
        return true;
    if(strcmp((const char *)this->Pcomment, (const char *)micro.Pcomment))
        return true;
    MCREQ(Pglobalfinedetune);
    return false;

    //undefine macros, as they are no longer needed
#undef MCREQ
#undef FMCREQ
}


/*
 * Convert a line to tunings; returns -1 if it ok
 */
int Microtonal::linetotunings(OctaveTuning &octave, const char *line)
{
    int x1 = -1, x2 = -1;
    int type;
    float tmp;
    float x = -1.0f;
    float tuning_log2 = 0.0f;
    if(strstr(line, "/") == NULL) {
        if(strstr(line, ".") == NULL) { // M case (M=M/1)
            sscanf(line, "%d", &x1);
            x2   = 1;
            type = 2; //division
        }
        else {  // float number case
            sscanf(line, "%f", &x);
            if(x < 0.000001f)
                return 1;
            type = 1; //float type(cents)
        }
    }
    else {  // M/N case
        sscanf(line, "%d/%d", &x1, &x2);
        if((x1 < 0) || (x2 < 0))
            return 1;
        if(x2 == 0)
            x2 = 1;
        type = 2; //division
    }

    if(x1 <= 0)
        x1 = 1;     //do not allow zero frequency sounds (consider 0 as 1)

    //convert to float if the number are too big
    if((type == 2)
       && ((x1 > (128 * 128 * 128 - 1)) || (x2 > (128 * 128 * 128 - 1)))) {
        type = 1;
        x    = ((float) x1) / x2;
    }
    switch(type) {
        case 1:
            x1     = (int) floor(x);
            tmp    = fmod(x, 1.0f);
            x2     = (int) floor(tmp * 1e6);
            tuning_log2 = x / 1200.0f;
            break;
        case 2:
            x      = ((float)x1) / x2;
            tuning_log2 = log2f(x);
            break;
        default:
            return 1;
    }

    octave.tuning_log2 = tuning_log2;
    octave.type   = type;
    octave.x1     = x1;
    octave.x2     = x2;

    return -1; //ok
}

/*
 * Convert the text to tunnings
 */
int Microtonal::texttotunings(const char *text)
{
    unsigned int k = 0, nl = 0;
    char *lin = new char[MAX_LINE_SIZE + 1];
    OctaveTuning tmpoctave[MAX_OCTAVE_SIZE];
    while(k < strlen(text)) {
        int i;
        for(i = 0; i < MAX_LINE_SIZE; ++i) {
            lin[i] = text[k++];
            if(lin[i] < 0x20)
                break;
        }
        lin[i] = '\0';
        if(strlen(lin) == 0)
            continue;
        int err = linetotunings(tmpoctave[nl], lin);
        if(err != -1) {
            delete [] lin;
            return nl; //Parse error
        }
        nl++;
    }
    delete [] lin;
    if(nl > MAX_OCTAVE_SIZE)
        nl = MAX_OCTAVE_SIZE;
    if(nl == 0)
        return -2;        //the input is empty
    octavesize = nl;
    for(int i = 0; i < octavesize; ++i) {
        octave[i].tuning_log2 = tmpoctave[i].tuning_log2;
        octave[i].type   = tmpoctave[i].type;
        octave[i].x1     = tmpoctave[i].x1;
        octave[i].x2     = tmpoctave[i].x2;
    }
    return -1; //ok
}

/*
 * Convert the text to mapping
 */
void Microtonal::texttomapping(const char *text)
{
    unsigned int i, k = 0;
    char *lin;
    lin = new char[MAX_LINE_SIZE + 1];
    for(i = 0; i < 128; ++i)
        Pmapping[i] = -1;
    int tx = 0;
    while(k < strlen(text)) {
        for(i = 0; i < MAX_LINE_SIZE; ++i) {
            lin[i] = text[k++];
            if(lin[i] < 0x20)
                break;
        }
        lin[i] = '\0';
        if(strlen(lin) == 0)
            continue;

        int tmp = 0;
        if(sscanf(lin, "%d", &tmp) == 0)
            tmp = -1;
        if(tmp < -1)
            tmp = -1;
        Pmapping[tx] = tmp;

        if((tx++) > 127)
            break;
    }
    delete [] lin;

    if(tx == 0)
        tx = 1;
    Pmapsize = tx;
}

/*
 * Convert tunning to text line
 */
void Microtonal::tuningtoline(int n, char *line, int maxn)
{
    if((n > octavesize) || (n > MAX_OCTAVE_SIZE)) {
        line[0] = '\0';
        return;
    }
    if(octave[n].type == 1)
        snprintf(line, maxn, "%d.%06d", octave[n].x1, octave[n].x2);
    if(octave[n].type == 2)
        snprintf(line, maxn, "%d/%d", octave[n].x1, octave[n].x2);
}


int Microtonal::loadline(FILE *file, char *line)
{
    memset(line, 0, 500);
    do {
        if(fgets(line, 500, file) == 0)
            return 1;
    } while(line[0] == '!');
    return 0;
}


/*
 * Loads the tunnings from a scl file
 */
int Microtonal::loadscl(SclInfo &scl, const char *filename)
{
    FILE *file = fopen(filename, "r");
    char  tmp[500];
    OctaveTuning tmpoctave[MAX_OCTAVE_SIZE];

    if(!file)
        return 2;

    fseek(file, 0, SEEK_SET);

    //loads the short description
    if(loadline(file, &tmp[0]) != 0)
        return 2;

    for(int i = 0; i < 500; ++i)
        if(tmp[i] < 32)
            tmp[i] = 0;

    strncpy(scl.Pname,    tmp, MICROTONAL_MAX_NAME_LEN);
    strncpy(scl.Pcomment, tmp, MICROTONAL_MAX_NAME_LEN);
    scl.Pname[MICROTONAL_MAX_NAME_LEN-1] = 0;
    scl.Pcomment[MICROTONAL_MAX_NAME_LEN-1] = 0;

    //loads the number of the notes
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    int nnotes = MAX_OCTAVE_SIZE;
    sscanf(&tmp[0], "%d", &nnotes);
    if(nnotes > MAX_OCTAVE_SIZE)
        return 2;

    //load the tunnings
    for(int nline = 0; nline < nnotes; ++nline) {
        if(loadline(file, &tmp[0]) != 0)
            return 2;
        linetotunings(tmpoctave[nline], tmp);
    }
    fclose(file);

    scl.octavesize = nnotes;
    for(int i = 0; i < scl.octavesize; ++i) {
        scl.octave[i].tuning_log2 = tmpoctave[i].tuning_log2;
        scl.octave[i].type   = tmpoctave[i].type;
        scl.octave[i].x1     = tmpoctave[i].x1;
        scl.octave[i].x2     = tmpoctave[i].x2;
    }

    return 0;
}


/*
 * Loads the mapping from a kbm file
 */
int Microtonal::loadkbm(KbmInfo &kbm, const char *filename)
{
    FILE *file = fopen(filename, "r");
    int   x;
    float tmpPAfreq = 440.0f;
    char  tmp[500];

    if(!file)
        return 2;

    fseek(file, 0, SEEK_SET);
    //loads the mapsize
    if(loadline(file, tmp) != 0 || sscanf(tmp, "%d", &x) == 0)
        return 2;
    kbm.Pmapsize = limit(x, 0, 127);

    //loads first MIDI note to retune
    if(loadline(file, tmp) != 0 || sscanf(tmp, "%d", &x) == 0)
        return 2;
    kbm.Pfirstkey = limit(x, 0, 127);

    //loads last MIDI note to retune
    if(loadline(file, tmp) != 0 || sscanf(tmp, "%d", &x) == 0)
        return 2;
    kbm.Plastkey = limit(x, 0, 127);

    //loads last the middle note where scale from scale degree=0
    if(loadline(file, tmp) != 0 || sscanf(tmp, "%d", &x) == 0)
        return 2;
    kbm.Pmiddlenote = limit(x, 0, 127);

    //loads the reference note
    if(loadline(file, tmp) != 0 || sscanf(tmp, "%d", &x) == 0)
        return 2;
    kbm.PAnote = limit(x,0,127);

    //loads the reference freq.
    if(loadline(file, tmp) != 0 || sscanf(tmp, "%f", &tmpPAfreq) == 0)
        return 2;
    kbm.PAfreq = tmpPAfreq;

    //the scale degree(which is the octave) is not loaded,
    //it is obtained by the tunnings with getoctavesize() method
    if(loadline(file, &tmp[0]) != 0)
        return 2;

    //load the mappings
    if(kbm.Pmapsize != 0) {
        for(int nline = 0; nline < kbm.Pmapsize; ++nline) {
            if(loadline(file, tmp) != 0)
                return 2;
            if(sscanf(tmp, "%d", &x) == 0)
                x = -1;
            kbm.Pmapping[nline] = x;
        }
        kbm.Pmappingenabled = 1;
    }
    else {
        kbm.Pmappingenabled = 0;
        kbm.Pmapping[0]     = 0;
        kbm.Pmapsize = 1;
    }
    fclose(file);

    return 0;
}



void Microtonal::add2XML(XMLwrapper& xml) const
{
    xml.addparstr("name", (char *) Pname);
    xml.addparstr("comment", (char *) Pcomment);

    xml.addparbool("invert_up_down", Pinvertupdown);
    xml.addpar("invert_up_down_center", Pinvertupdowncenter);

    xml.addparbool("enabled", Penabled);
    xml.addpar("global_fine_detune", Pglobalfinedetune);

    xml.addpar("a_note", PAnote);
    xml.addparreal("a_freq", PAfreq);

    if((Penabled == 0) && (xml.minimal))
        return;

    xml.beginbranch("SCALE");
    xml.addpar("scale_shift", Pscaleshift);
    xml.addpar("first_key", Pfirstkey);
    xml.addpar("last_key", Plastkey);
    xml.addpar("middle_note", Pmiddlenote);

    xml.beginbranch("OCTAVE");
    xml.addpar("octave_size", octavesize);
    for(int i = 0; i < octavesize; ++i) {
        xml.beginbranch("DEGREE", i);
        if(octave[i].type == 1)
            xml.addparreal("cents", powf(2.0f, octave[i].tuning_log2));

        if(octave[i].type == 2) {
            xml.addpar("numerator", octave[i].x1);
            xml.addpar("denominator", octave[i].x2);
        }
        xml.endbranch();
    }
    xml.endbranch();

    xml.beginbranch("KEYBOARD_MAPPING");
    xml.addpar("map_size", Pmapsize);
    xml.addpar("mapping_enabled", Pmappingenabled);
    for(int i = 0; i < Pmapsize; ++i) {
        xml.beginbranch("KEYMAP", i);
        xml.addpar("degree", Pmapping[i]);
        xml.endbranch();
    }

    xml.endbranch();
    xml.endbranch();
}

void Microtonal::getfromXML(XMLwrapper& xml)
{
    xml.getparstr("name", (char *) Pname, MICROTONAL_MAX_NAME_LEN);
    xml.getparstr("comment", (char *) Pcomment, MICROTONAL_MAX_NAME_LEN);

    Pinvertupdown = xml.getparbool("invert_up_down", Pinvertupdown);
    Pinvertupdowncenter = xml.getpar127("invert_up_down_center",
                                         Pinvertupdowncenter);

    Penabled = xml.getparbool("enabled", Penabled);
    Pglobalfinedetune = xml.getpar127("global_fine_detune", Pglobalfinedetune);

    PAnote = xml.getpar127("a_note", PAnote);
    PAfreq = xml.getparreal("a_freq", PAfreq, 1.0f, 10000.0f);

    if(xml.enterbranch("SCALE")) {
        Pscaleshift = xml.getpar127("scale_shift", Pscaleshift);
        Pfirstkey   = xml.getpar127("first_key", Pfirstkey);
        Plastkey    = xml.getpar127("last_key", Plastkey);
        Pmiddlenote = xml.getpar127("middle_note", Pmiddlenote);

        if(xml.enterbranch("OCTAVE")) {
            octavesize = xml.getpar127("octave_size", octavesize);
            for(int i = 0; i < octavesize; ++i) {
                if(xml.enterbranch("DEGREE", i) == 0)
                    continue;
                octave[i].x2     = 0;
                octave[i].tuning_log2 = log2f(xml.getparreal("cents",
                    powf(2.0f, octave[i].tuning_log2))) / log2(2.0f);
                octave[i].x1     = xml.getpar("numerator", octave[i].x1, 0, 65535);
                octave[i].x2     = xml.getpar("denominator", octave[i].x2, 0, 65535);

                if(octave[i].x2 != 0)
                    octave[i].type = 2;
                else {
                    octave[i].type = 1;
                    /* populate fields for display */
                    const float x = octave[i].tuning_log2 * 1200.0f;
                    octave[i].x1 = (int) floorf(x);
                    octave[i].x2 = (int) (floorf((x-octave[i].x1) * 1.0e6));
                }


                xml.exitbranch();
            }
            xml.exitbranch();
        }

        if(xml.enterbranch("KEYBOARD_MAPPING")) {
            Pmapsize = xml.getpar127("map_size", Pmapsize);
            Pmappingenabled = xml.getpar127("mapping_enabled", Pmappingenabled);
            for(int i = 0; i < Pmapsize; ++i) {
                if(xml.enterbranch("KEYMAP", i) == 0)
                    continue;
                Pmapping[i] = xml.getpar127("degree", Pmapping[i]);
                xml.exitbranch();
            }
            xml.exitbranch();
        }
        xml.exitbranch();
    }
    apply();
}



int Microtonal::saveXML(const char *filename) const
{
    XMLwrapper xml;

    xml.beginbranch("MICROTONAL");
    add2XML(xml);
    xml.endbranch();

    return xml.saveXMLfile(filename, gzip_compression);
}

int Microtonal::loadXML(const char *filename)
{
    XMLwrapper xml;
    if(xml.loadXMLfile(filename) < 0) {
        return -1;
    }

    if(xml.enterbranch("MICROTONAL") == 0)
        return -10;
    getfromXML(xml);
    xml.exitbranch();

    return 0;
}

//roundabout function, but it works
void Microtonal::apply(void)
{
    {
        char buf[100*MAX_OCTAVE_SIZE] = {};
        char tmpbuf[100] = {};
        for (int i=0;i<Pmapsize;i++) {
            if (i!=0)
                strncat(buf, "\n", sizeof(buf)-1);
            if (Pmapping[i]==-1)
                snprintf(tmpbuf,100,"x");
            else
                snprintf(tmpbuf,100,"%d",Pmapping[i]);
            strncat(buf, tmpbuf, sizeof(buf)-1);
        }
        texttomapping(buf);
    }

    {
        char buf[100*MAX_OCTAVE_SIZE] = {};
        char tmpbuf[100] = {};
        for (int i=0;i<octavesize;i++){
            if (i!=0)
                strncat(buf, "\n", sizeof(buf)-1);
            tuningtoline(i,tmpbuf,100);
            strncat(buf, tmpbuf, sizeof(buf)-1);
        }
        int err = texttotunings(buf);
        (void) err;
    }
}

}

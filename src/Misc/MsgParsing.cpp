/*
  ZynAddSubFX - a software synthesizer

  MsgParsing.cpp - Parsing messages int <-> string
  Copyright (C) 2020-2020 Johannes Lorenz

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <cstring>
#include <string>

#include "MsgParsing.h"

namespace zyn {

std::string buildVoiceParMsg(const int* part, const int* kit, const int* voice,
                             const bool* isFm)
{
    std::string res;
    if(!part || !kit) { return std::string(); }
    res = std::string("/part") + std::to_string(*part)
        + std::string("/kit") + std::to_string(*kit);
    if(voice)
    {
        res += std::string("/adpars/VoicePar") + std::to_string(*voice);
        if(isFm)
            res += (*isFm) ? "/FMSmp" : "/OscilSmp";
    }
    return res;
}

std::size_t idsFromMsg(const char* const msg, int* part, int* kit, int* voice, bool* isFm)
{
    auto msg_match = [](const char* msg, const char* match) -> bool {
        return !strncmp(msg, match, strlen(match));
    };

    const char *end = msg;
    char *newend;

    if(*end == '/')
        ++end;

    if(!msg_match(end, "part")) return 0;
    end += 4;
    *part = static_cast<int>(strtol(end, &newend, 10));
    if(newend == end) return 0; // number read?
    end = newend;

    if(!msg_match(end, "/kit")) return 0;
    end += 4;
    *kit = static_cast<int>(strtol(end, &newend, 10));
    if(newend == end) return 0;
    end = newend;

    if(voice)
    {
        if(!msg_match(end, "/adpars/VoicePar")) return 0;
        end += 16;
        *voice = static_cast<int>(strtol(end, &newend, 10));
        if(newend == end) return 0;
        end = newend;

        if(isFm)
        {
            if(msg_match(end, "/OscilSmp")) {
                end += 9;
                *isFm = false;
            }
            else if(msg_match(end, "/FMSmp")) {
                end += 6;
                *isFm = true;
            }
            else { return 0; }
        }
    }
    return end - msg;
}

}

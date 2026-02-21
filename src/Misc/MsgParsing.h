/*
  ZynAddSubFX - a software synthesizer

  MsgParsing.h - Message Parsing declarations
  Copyright (C) 2020-2020 Johannes Lorenz
  Author: Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef MSGPARSING_H
#define MSGPARSING_H

#include <string>

/*
 * Build/parse messages from/to part/kit/voice IDs
 */

namespace zyn {

std::string buildVoiceParMsg(const int *part, const int *kit,
                             const int *voice = nullptr, const bool *isFm = nullptr);
std::size_t idsFromMsg(const char* msg, int* part, int* kit,
                       int* voice = nullptr, bool* isFm = nullptr);

}

#endif // MSGPARSING_H

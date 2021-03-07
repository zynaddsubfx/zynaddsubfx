/*
  ZynAddSubFX - a software synthesizer

  MemLocker.cpp - Memory page locker
  Copyright (C) 2019-2019 Johannes Lorenz

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

#include "MemLocker.h"

#ifndef WIN32
#include <cstdio>
#include <sys/mman.h>
#endif

namespace zyn {

void MemLocker::lock()
{
#ifndef WIN32
    if (!isLocked)
    {
        if (mlockall (MCL_CURRENT | MCL_FUTURE)) {
            perror ("Warning: Can not lock memory");
        } else {
            isLocked = true;
        }
    }
#endif
}

void MemLocker::unlock()
{
#ifndef WIN32
    if (isLocked)
    {
        if (munlockall ()) {
            perror ("Warning: Can not unlock memory");
        } else {
            isLocked = false;
        }
    }
#endif
}

}

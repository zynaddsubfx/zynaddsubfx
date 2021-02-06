/*
  ZynAddSubFX - a software synthesizer

  MemLocker.h - Memory page locker
  Copyright (C) 2019-2019 Johannes Lorenz
  Author: Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

namespace zyn {

//! Class to lock all pages in memory
class MemLocker
{
    bool isLocked = false;
public:
    MemLocker() = default;
    ~MemLocker() { unlock(); }

    //! try to lock all current and future pages, if not already locked
    void lock();
    //! try to unlock all pages, if locked
    void unlock();
};

}

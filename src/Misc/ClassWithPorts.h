/*
  ZynAddSubFX - a software synthesizer

  ClassWithPorts.h - Abstract base class for class that contain Ports
  Copyright (C) 2020-2020 Johannes Lorenz
  Author: Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef CLASSWITHPORTS_H
#define CLASSWITHPORTS_H

namespace rtosc
{
    struct Ports;
}

namespace zyn {

class ClassWithPorts
{
public:
    virtual const rtosc::Ports* getPorts() const = 0;
    virtual void* getClass() = 0; //!< this may not be "this" (multiple inheritance!)
    virtual ~ClassWithPorts() = default;
};

}

#endif // CLASSWITHPORTS_H

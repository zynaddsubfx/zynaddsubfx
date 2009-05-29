/*
  ZynAddSubFX - a software synthesizer
 
  Sample.h - Object for storing information on samples

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
#ifndef STEREOSAMPLE_H
#define STEREOSAMPLE_H

#include <iostream>
#include "Sample.h"
#include "MonoSample.h"

class StereoSample:public Sample{
    public:
        StereoSample(int length);
        StereoSample(const MonoSample &left_,const MonoSample &right_);
        ~StereoSample(){std::cout<<"~StereoSample\n";};
        void clear();
        int size() const;

        float &operator[](const int &index);
        StereoSample &operator=(const StereoSample &smp);
        MonoSample &left(){return leftSample;}; 
        MonoSample &right(){return rightSample;}; 
    private:
        MonoSample leftSample;
        MonoSample rightSample;

};
#endif


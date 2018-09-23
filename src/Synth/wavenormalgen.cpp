/*
  ZynAddSubFX - a software synthesizer

  wavenormalgen.cpp - precompute C-code with wavetable normal values
  Copyright (C) 2016-2018 Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <iomanip>
#include <iostream>
#include "basefunctions.h"

int main()
{
    const std::size_t
        values = 128 << 2,
        num_basefunctions = 15; // number of non-sine basefunctions

    // 4 leading digits,
    // floatfield set to 6 digits fixed
    std::cout.width(4);
    std::cout.precision(6);
    std::cout.setf( std::ios::fixed, std:: ios::floatfield );

    std::cout << "float wavenormals["
              << num_basefunctions
              << "][" << values << "] = " << std::endl
              << "{" << std::endl;
    for(std::size_t func_no = 1; // sine has no parameter modulation
        func_no <= num_basefunctions;
        ++func_no)
    {
        std::cout << "    {" << std::endl << "        ";
        for(std::size_t par = 0;
            par < values;
            ++par)
        {
            std::cout << std::setprecision(6) << std::setw(4)
                      << zyn::rmsNormalOfBaseFunction(func_no, 1024, par>>2)
                      << 'f';
            if(par != values-1)
                std::cout << ", ";
            if(par % 5 == 4)
            {
                std::cout << std::endl << "        ";
            }
        }
        std::cout << std::endl << "    }";
        if(func_no != num_basefunctions)
            std::cout << ',';
        std::cout << std::endl;
    }
    std::cout << "};";

    std::cout << std::endl << std::endl;
    std::cout << "const float (*getWavenormals())["
              << values
              << "] {" << std::endl
              << "    return wavenormals;" << std::endl
              << "}" << std::endl;
    return 0;
}


#pragma once
#include <stdio.h>
#include <string>
using std::string;

class Fl_Osc_Interface
{
    public:
        //It is assumed that you want to have a registry for all of these
        //elements
        virtual void createLink(string s, class Fl_Osc_Widget*){printf("linking %s...\n", s.c_str());};
        virtual void renameLink(string,string,class Fl_Osc_Widget*){};
        virtual void removeLink(string,class Fl_Osc_Widget*){};

        //Communication link
        virtual void requestValue(string){};
        virtual void writeValue(string s, float f){printf("%s -> %f\n",s.c_str(), f); };
        virtual void writeValue(string, int){};
        virtual void writeValue(string, bool){};
        virtual void writeValue(string, string){};
};

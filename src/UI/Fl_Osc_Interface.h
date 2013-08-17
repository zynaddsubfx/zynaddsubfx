#pragma once
#include <stdio.h>
#include <string>
using std::string;

class Fl_Osc_Interface
{
    public:
        virtual ~Fl_Osc_Interface(void){};
        //It is assumed that you want to have a registry for all of these
        //elements
        virtual void createLink(string, class Fl_Osc_Widget *) {};
        virtual void renameLink(string,string,class Fl_Osc_Widget*){};
        virtual void removeLink(string,class Fl_Osc_Widget*){};
        virtual void removeLink(class Fl_Osc_Widget*){};

        //and to be able to give them events
        virtual void tryLink(const char *){};

        //Damage the values of a collection of widgets
        virtual void damage(const char*){};

        //Communication link
        virtual void requestValue(string s) { printf("request: '%s'...\n", s.c_str()); };
        virtual void writeValue(string s, float f){printf("%s -> %f\n",s.c_str(), f); };
        virtual void writeValue(string s, char c){printf("%s->%d\n", s.c_str(), c);};
        virtual void writeValue(string, int){};
        virtual void writeValue(string, bool){};
        virtual void writeValue(string, string){};
        virtual void write(string s) {write(s, "");};//{printf("write: '%s'\n", s.c_str());};
        virtual void write(string, const char *, ...) {};//{printf("write: '%s'\n", s.c_str());};
        virtual void writeRaw(const char *) {}
};

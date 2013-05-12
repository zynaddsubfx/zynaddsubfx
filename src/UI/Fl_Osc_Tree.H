#pragma once
#include <rtosc/ports.h>
#include "Fl_Osc_Interface.h"
#include <FL/Fl_Tree.H>

class Fl_Osc_Tree: public Fl_Tree
{
    public:
        Fl_Osc_Tree(int X, int Y, int W, int H, const char *L=0)
            :Fl_Tree(X,Y,W,H,L)
        {
            root_label("");
            add("nil");
            add("/nil/nil");
            close(first());
            callback(Fl_Osc_Tree::cb, NULL);
        }

        static void cb(Fl_Widget *w, void*)
        {
            using namespace rtosc;
            Fl_Osc_Tree *t=(Fl_Osc_Tree*)w;
            int reason = t->callback_reason();

            char pathname[1024];
            t->item_pathname(pathname, sizeof(pathname), t->callback_item());


            if(reason==1) {
                char *colon = index(pathname, ':');
                if(colon) {
                    *colon = 0;
                    t->osc->writeValue("/learn", string(pathname));
                }
            }

            if(reason==3) //Populate fields
            {
                const Ports &p = *Fl_Osc_Tree::subtree_lookup(t->root_ports,pathname+1);
                if(auto *i = t->find_item((std::string(pathname)+"/"+"nil").c_str()))
                    t->remove(i);
                for(const Port &port : p) {
                    bool subnodes = index(port.name, '/');
                    string path = std::string(pathname)+"/"+port.name;
                    if(subnodes) {
                        if(!t->find_item(path.c_str()))
                            t->add(path.c_str());
                        if(!t->find_item((path+"nil").c_str()))
                            t->add((path+"nil").c_str());
                        t->close(path.c_str());
                    } else {
                        if(!t->find_item(path.c_str()))
                            t->add(path.c_str());
                    }
                }

            }
        }

        static const rtosc::Ports *subtree_lookup(const rtosc::Ports *p, std::string s)
        {
            using namespace rtosc;
            if(s=="")
                return p;

            if(s[s.length()-1] != '/')
                s += '/';

            for(const Port &port : *p) {
                const char *name  = port.name;
                if(!index(name, '/'))//only accept objects that will have subports
                    continue;
                if(s.substr(0,strlen(name)) == name) {
                    return subtree_lookup(port.ports, s.substr(strlen(name)));
                }
            }

            //TODO else case
            return p;
        }

        rtosc::Ports *root_ports;
        Fl_Osc_Interface *osc;
};

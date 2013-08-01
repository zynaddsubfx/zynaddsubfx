#include <FL/Fl_Counter.H>

class Fl_Osc_Counter: public Fl_Counter
{
    public:
        Fl_Osc_Counter(int x, int y, int w, int h, const char *label=0);
        void update(void);
};

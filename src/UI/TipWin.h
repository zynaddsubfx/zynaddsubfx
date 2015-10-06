using namespace std;

class TipWin:public Fl_Menu_Window
{
    public:
        TipWin();
        void draw();
        void showValue(float f);
        void setText(const char *c);
        void showText();
    private:
        void redraw();
        const char *getStr() const;
        string tip;
        string text;
        bool   textmode;
};

#include "Connection.h"
#include "Fl_Osc_Interface.h"
#include "../globals.h"

#include <rtosc/rtosc.h>
#include <rtosc/ports.h>

#include <FL/Fl.H>
#include "Fl_Osc_Tree.H"
#include "common.H"
#include "MasterUI.h"

#ifdef NTK_GUI
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/Fl_Dial.H>
#endif // NTK_GUI

using namespace GUI;
MasterUI *ui;

Fl_Osc_Interface *osc;//TODO: the scope of this should be narrowed

#ifdef NTK_GUI
static Fl_Tiled_Image *module_backdrop;
#endif

void
set_module_parameters ( Fl_Widget *o )
{
#ifdef NTK_GUI
    o->box( FL_DOWN_FRAME );
    o->align( o->align() | FL_ALIGN_IMAGE_BACKDROP );
    o->color( FL_BLACK );
    o->image( module_backdrop );
    o->labeltype( FL_SHADOW_LABEL );
#else
    o->box( FL_PLASTIC_UP_BOX );
    o->color( FL_CYAN );
    o->labeltype( FL_EMBOSSED_LABEL );
#endif
}

ui_handle_t GUI::createUi(Fl_Osc_Interface *osc, void *exit)
{
    ::osc = osc;
#ifdef NTK_GUI
    fl_register_images();

    Fl_Dial::default_style(Fl_Dial::PIXMAP_DIAL);

    if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/knob.png"))
        Fl_Dial::default_image(img);
    else
        Fl_Dial::default_image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/knob.png"));

    if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/window_backdrop.png"))
        Fl::scheme_bg(new Fl_Tiled_Image(img));
    else
        Fl::scheme_bg(new Fl_Tiled_Image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/window_backdrop.png")));

    if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/module_backdrop.png"))
        module_backdrop = new Fl_Tiled_Image(img);
    else
        module_backdrop = new Fl_Tiled_Image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/module_backdrop.png"));

    Fl::background(50,  50,  50);
    Fl::background2(70, 70,  70);
    Fl::foreground(255, 255, 255);
#endif

    Fl_Window *midi_win = new Fl_Double_Window(400, 400, "Midi connections");
    Fl_Osc_Tree *tree   = new Fl_Osc_Tree(0,0,400,400);
    midi_win->resizable(tree);
    tree->root_ports    = &Master::ports;
    tree->osc           = osc;
    midi_win->show();

    return (void*) (ui = new MasterUI((int*)exit, osc));
}
void GUI::destroyUi(ui_handle_t ui)
{
    delete static_cast<MasterUI*>(ui);
}

#define BEGIN(x) {x,"",NULL,[](const char *m, rtosc::RtData d){ \
    MasterUI *ui   = static_cast<MasterUI*>(d.obj); \
    rtosc_arg_t a0 = rtosc_argument(m,0);
    //rtosc_arg_t a1 = rtosc_argument(m,1); \
    //rtosc_arg_t a2 = rtosc_argument(m,2); \
    //rtosc_arg_t a3 = rtosc_argument(m,3);
#define END }},

//DSL based ports
static rtosc::Ports ports = {
    BEGIN("show:T") {
        ui->showUI();
    } END
    BEGIN("alert:s") {
        fl_alert(a0.s);
    } END
    BEGIN("session-type:s") {
        if(strcmp(a0.s,"LASH"))
            return;
        ui->sm_indicator1->value(1);
        ui->sm_indicator2->value(1);
        ui->sm_indicator1->tooltip("LASH");
        ui->sm_indicator2->tooltip("LASH");
    } END
    BEGIN("save-master:s") {
        ui->do_save_master(a0.s);
    } END
    BEGIN("load-master:s") {
        ui->do_load_master(a0.s);
    } END
    BEGIN("vu-meter:bb") {
        rtosc_arg_t a1 = rtosc_argument(m,1);
        if(a0.b.len == sizeof(vuData) &&
                a1.b.len == sizeof(float)*NUM_MIDI_PARTS) {
            //Refresh the primary VU meters
            ui->simplemastervu->update((vuData*)a0.b.data);
            ui->mastervu->update((vuData*)a0.b.data);

            float *partvu = (float*)a1.b.data;
            for(int i=0; i<NUM_MIDI_PARTS; ++i)
                ui->panellistitem[i]->partvu->update(partvu[i]);
        }
    } END
};

void GUI::raiseUi(ui_handle_t gui, const char *message)
{
    MasterUI *mui = (MasterUI*)gui;
    mui->osc->tryLink(message);
    //printf("got message for UI '%s'\n", message);
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    rtosc::RtData d;
    d.loc = buffer;
    d.loc_size = 1024;
    d.obj = gui;
    ports.dispatch(message+1, d);
}

void GUI::raiseUi(ui_handle_t gui, const char *dest, const char *args, ...)
{
    char buffer[1024];
    va_list va;
    va_start(va,args);
    if(rtosc_vmessage(buffer,1024,dest,args,va))
        raiseUi(gui, buffer);
}

void GUI::tickUi(ui_handle_t)
{
    Fl::wait(0.02f);
}

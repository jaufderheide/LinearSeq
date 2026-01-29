#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>

// Unicode values for FontAwesome Solid
#define ICON_PLAY     "\uf04b"
#define ICON_STOP     "\uf04d"
#define ICON_RECORD   "\uf111"
#define ICON_REWIND   "\uf048"

void setup_transport_icons() {
    // 1. Load the font file into FLTK's font table
    // Returns the index of the new font
    Fl::set_font(FL_FREE_FONT, "Font Awesome 6 Free Solid");
    
    // 2. Create the buttons
    Fl_Button* btn_play = new Fl_Button(10, 10, 40, 40, ICON_PLAY);
    Fl_Button* btn_stop = new Fl_Button(55, 10, 40, 40, ICON_STOP);
    Fl_Button* btn_rec  = new Fl_Button(100, 10, 40, 40, ICON_RECORD);

    // 3. Apply the font and styling
    btn_play->labelfont(FL_FREE_FONT);
    btn_play->labelsize(20);
    btn_play->labelcolor(FL_GREEN);

    btn_stop->labelfont(FL_FREE_FONT);
    btn_stop->labelsize(20);

    btn_rec->labelfont(FL_FREE_FONT);
    btn_rec->labelsize(20);
    btn_rec->labelcolor(FL_RED);
}
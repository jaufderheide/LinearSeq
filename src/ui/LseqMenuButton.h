#ifndef LSEQ_MENU_BUTTON_H
#define LSEQ_MENU_BUTTON_H

#include <FL/Fl_Menu_Button.H>

namespace linearseq {

class LseqMenuButton : public Fl_Menu_Button {
public:
    LseqMenuButton(int X, int Y, int W, int H, const char* L = 0);
protected:
    void draw() override;
};

} // namespace linearseq

#endif
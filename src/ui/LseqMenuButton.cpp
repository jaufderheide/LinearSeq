#include "ui/LseqMenuButton.h"
#include <FL/fl_draw.H>

namespace linearseq {

LseqMenuButton::LseqMenuButton(int X, int Y, int W, int H, const char* L)
    : Fl_Menu_Button(X, Y, W, H, L) {}

void LseqMenuButton::draw() {
    draw_box(value() ? (down_box() ? down_box() : FL_DOWN_BOX) : box(), color());
    draw_label();
    // Your custom drawing code here...
}
} // namespace linearseq

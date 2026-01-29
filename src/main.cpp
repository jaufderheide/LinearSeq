#include <FL/Fl.H>
#include "ui/MainWindow.h"

int main(int argc, char** argv) {
    linearseq::MainWindow window(1024, 640, "LinearSeq");
    window.show(argc, argv);
    return Fl::run();
}

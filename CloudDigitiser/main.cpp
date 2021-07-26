#include "MainWindow.h"
//#include "IrrlichtWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

//    IrrlichtWindow w;
//    w.resize(800, 600);
//    w.setShowDemo(true);
//    w.initializeIrrlicht();
//    w.show();
//    w.renderLater();

    return a.exec();
}


#include "Source\cameraMainWindow.h"
#include <QApplication>
#include "Source\Version.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    a.setApplicationName("Vimba JILA Viewer");
    a.setApplicationVersion(VIMBAVIEWER_VERSION);

    cameraMainWindow Application;
    Application.setWindowTitle(QString("Vimba JILA Viewer ") + QString(VIMBAVIEWER_VERSION));
    
    //qApp->screens()[0]->geometry().height();

    Application.showMaximized();
    return a.exec();
}

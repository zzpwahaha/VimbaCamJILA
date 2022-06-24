
#include "Source\cameraMainWindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    a.setApplicationName("Vimba JILA Viewer");
    //a.setApplicationVersion(VIMBAVIEWER_VERSION);

    cameraMainWindow Application;
    Application.setWindowTitle(QString("Chimera Vimba Viewer "));
    

    qRegisterMetaType< QVector<double> >("QVector<double>");
    qRegisterMetaType<size_t>();
    qRegisterMetaType<MakoSettings*>();
    qRegisterMetaType<CameraInfo>();

    //qApp->screens()[0]->geometry().height();

    Application.showMaximized();
    return a.exec();
}

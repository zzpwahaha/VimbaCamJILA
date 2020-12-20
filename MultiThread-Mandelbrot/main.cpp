#include "mandelbrotwidgets.h"

#include <QApplication>

//! [0]
int main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    MandelbrotWidget widget;
    widget.show();
    return app.exec();
}
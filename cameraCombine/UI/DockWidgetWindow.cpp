
#include "DockWidgetWindow.h"


DockWidgetWindow::DockWidgetWindow ( const QString &sTitle, QWidget *parent ) : 
    QDockWidget( sTitle, parent )
{
    //this->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea|Qt::TopDockWidgetArea|Qt::BottomDockWidgetArea);
    QFont font;
    font.setFamily(QString::fromUtf8("Verdana"));
    this->setFont(font);
}

DockWidgetWindow::~DockWidgetWindow ( )
{

}
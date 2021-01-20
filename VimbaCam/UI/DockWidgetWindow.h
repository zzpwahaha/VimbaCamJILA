
#ifndef DOCKWIDGETWINDOW_H
#define DOCKWIDGETWINDOW_H

#include <QDockWidget>
#include "Helper.h"


class DockWidgetWindow : public QDockWidget
{
    Q_OBJECT
    public: 

    protected:


    private:

    public:
            DockWidgetWindow ( const QString &sTitle, QWidget *parent = 0 );
           ~DockWidgetWindow ( void );

    protected:
        

    private:

    private slots:
        

    signals:
        
};

#endif
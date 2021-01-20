//Description: Main MDI Window for loggingand event viewer

#ifndef MAININFORMATIONWINDOW_H
#define MAININFORMATIONWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <VimbaCPP/Include/VimbaSystem.h>
#include "MdiChild.h"

const unsigned int MAX_LOGGING = 100;

using AVT::VmbAPI::CameraPtr;

class MdiChild;

class MainInformationWindow : public QMainWindow
{
    Q_OBJECT
    public: 
            
    protected:

    private:
            QAction                *m_OpenLoggerAct;
            QAction                *m_OpenEventViewerAct;
            QAction                *m_CloseAllAct;
            QAction                *m_TileAct;
            QAction                *m_CascadeAct;
            QMenu                  *m_WindowMenu;

            QMdiArea               *m_MDIArea;
            QList< MdiChild *>      m_MDIChildList;
            CameraPtr               m_pCam;

            int                     m_nEventViewerPosition;
            bool                    m_bIsLogging;
            bool                    m_bIsEventViewerOpen;

            unsigned int            m_NumberOfLogging;
    public:
            MainInformationWindow ( QWidget *parent = 0, Qt::WindowFlags flags = 0, CameraPtr pCam = CameraPtr()  );
           ~MainInformationWindow ( void );
            
            void setEventMessage ( const QStringList &sMessage );
            void openLoggingWindow ( void );
            void feedLogger ( const QString &sWhatWindow, const QString &sInfo, const VimbaViewerLogCategory &logCategory );

    protected:

    private:
            void createAction ( void );
            void createMenu ( void );
            MdiChild *activeMdiChild ( void );
            void createMdiChild ( const QString &sWhatWindow );
            void updateEventViewerPosition ( void );

    private slots:
            void openLogger ( void );
            void openEventViewer ( void );
            void updateMenus ( void );
            void onDestroyed ( QObject *obj );
};

#endif
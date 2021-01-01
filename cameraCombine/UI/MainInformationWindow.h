/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        MainInformationWindow.h

  Description: Main MDI Window for logging and event viewer

-------------------------------------------------------------------------------

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
  NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

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
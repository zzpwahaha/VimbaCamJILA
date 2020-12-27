/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        MainInformationWindow.cpp

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


#include "MainInformationWindow.h"
#include <QToolBar>


MainInformationWindow::MainInformationWindow ( QWidget *parent , Qt::WindowFlags flags, CameraPtr pCam ) : QMainWindow( parent, flags ),
m_WindowMenu( NULL ), m_OpenLoggerAct( NULL ), m_OpenEventViewerAct( NULL ), m_CloseAllAct( NULL ), m_TileAct( NULL ),
m_CascadeAct( NULL ), m_MDIArea( NULL )      , m_nEventViewerPosition (-1) , m_bIsLogging ( false ), m_bIsEventViewerOpen ( false ),
m_NumberOfLogging ( 0 )    
{
    m_pCam = pCam;
    /* make sure to treat this as a widget :) */
    this->setWindowFlags(Qt::Widget); 
    QToolBar* toolBarInformation = new QToolBar(this);
    toolBarInformation->setObjectName(QString::fromUtf8("toolBarInformation"));
    toolBarInformation->setWindowTitle("Information ToolBar");
	toolBarInformation->setStyleSheet(QString::fromUtf8("QToolBar{background-color: transparent;} QToolTip {""}"));
    this->addToolBar(toolBarInformation);

    /* create MDI area */
    m_MDIArea = new QMdiArea;
    setCentralWidget(m_MDIArea);

    createAction();
    toolBarInformation->addAction(m_OpenLoggerAct);
    toolBarInformation->addAction(m_OpenEventViewerAct);
    toolBarInformation->addSeparator();
    toolBarInformation->addAction(m_CascadeAct);
    toolBarInformation->addAction(m_TileAct);
    toolBarInformation->addAction(m_CloseAllAct);
    m_OpenLoggerAct->setCheckable(true);
    m_OpenEventViewerAct->setCheckable(true);
    createMenu();
    connect(m_MDIArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(updateMenus()));

    updateMenus();
}

MainInformationWindow::~MainInformationWindow ( )
{
    
}

void MainInformationWindow::createAction ( void )
{
    m_OpenLoggerAct      = new QAction(QIcon(":/VimbaViewer/Images/logger.png"), tr("&Open logging window"), this );
    m_OpenEventViewerAct = new QAction(QIcon(":/VimbaViewer/Images/open_eventviewer.png"), tr("&Open event viewer"), this );
    m_CascadeAct         = new QAction(QIcon(":/VimbaViewer/Images/cascade.png"),tr("&Cascade"), this);
    m_TileAct            = new QAction(QIcon(":/VimbaViewer/Images/tile.png"),tr("&Tile "), this);
    m_CloseAllAct        = new QAction(QIcon(":/VimbaViewer/Images/close.png"),tr("&Close all"), this);
    
    m_OpenLoggerAct->setPriority(QAction::LowPriority);
    m_OpenEventViewerAct->setPriority(QAction::LowPriority);
    m_CascadeAct->setPriority(QAction::LowPriority);
    m_TileAct->setPriority(QAction::LowPriority);
    m_CloseAllAct->setPriority(QAction::LowPriority);

    connect(m_OpenLoggerAct, SIGNAL(triggered()), this, SLOT(openLogger()));
    connect(m_OpenEventViewerAct, SIGNAL(triggered()), this, SLOT(openEventViewer()));
    connect(m_CloseAllAct, SIGNAL(triggered()), m_MDIArea, SLOT(closeAllSubWindows()));
    connect(m_CascadeAct, SIGNAL(triggered()), m_MDIArea, SLOT(cascadeSubWindows()));
    connect(m_TileAct, SIGNAL(triggered()), m_MDIArea, SLOT(tileSubWindows()));
}

void MainInformationWindow::createMenu ( void )
{
    m_WindowMenu = menuBar()->addMenu(tr("&Window"));    
    m_WindowMenu->addAction(m_OpenLoggerAct);
    m_WindowMenu->addAction(m_OpenEventViewerAct);
    m_WindowMenu->addSeparator();
    m_WindowMenu->addAction(m_TileAct);
    m_WindowMenu->addAction(m_CascadeAct);
    m_WindowMenu->addSeparator();
    m_WindowMenu->addAction(m_CloseAllAct);
}

void MainInformationWindow::feedLogger ( const QString &sWhatWindow, const QString &sInfo, const VimbaViewerLogCategory &logCategory )
{
    QStringList sInfoList;
    sInfoList << sInfo;

    for(int i=0; i< m_MDIChildList.size(); i++)
    {
        if( 0 == m_MDIChildList.at(i)->getName().compare(sWhatWindow) ) 
            m_MDIChildList.at(i)->setLogger(sInfoList, logCategory);
    }
}

void MainInformationWindow::openLoggingWindow ( void )
{
    m_OpenLoggerAct->setChecked(true);
    openLogger();
}

void MainInformationWindow::openLogger ( void )
{
    createMdiChild("Logging");

    /* getting info from the camera */
    std::string sInfo;

    if(VmbErrorSuccess == m_pCam->GetName(sInfo))
        feedLogger("Logging", QString("Name: "+ QString::fromStdString(sInfo)), VimbaViewerLogCategory_INFO);
    sInfo.clear();

    if(VmbErrorSuccess == m_pCam->GetModel(sInfo))
        feedLogger("Logging", QString("Model: "+ QString::fromStdString(sInfo)), VimbaViewerLogCategory_INFO);
    sInfo.clear();

    if(VmbErrorSuccess == m_pCam->GetSerialNumber(sInfo))
        feedLogger("Logging", QString("S/N: "+ QString::fromStdString(sInfo)), VimbaViewerLogCategory_INFO);
    sInfo.clear();

    if(VmbErrorSuccess == m_pCam->GetID(sInfo))
        feedLogger("Logging", QString("ID: "+ QString::fromStdString(sInfo)), VimbaViewerLogCategory_INFO);
    sInfo.clear();

    if(VmbErrorSuccess == m_pCam->GetInterfaceID(sInfo))
        feedLogger("Logging", QString("Interface ID: "+ QString::fromStdString(sInfo)), VimbaViewerLogCategory_INFO);    
}

void MainInformationWindow::openEventViewer ( void )
{
    createMdiChild("Event Viewer");
}

void MainInformationWindow::onDestroyed ( QObject *obj )
{
    QString s = obj->objectName();

    if(0 == s.compare("Logging"))
    {
        m_OpenLoggerAct->setChecked(false);
    }

    if(0 == s.compare("Event Viewer"))
    {
        m_bIsLogging = false;
        m_OpenEventViewerAct->setChecked(false);
    }

    /* just make sure to clean it up */
    for(int i=0; i< m_MDIChildList.size(); i++)
    {
        if( 0 == m_MDIChildList.at(i)->getName().compare(s) )
        {
            if(0 == s.compare("Event Viewer"))
            {
                /* make sure to stop logging before delete the object */
                m_bIsLogging = false;
                m_MDIChildList.at(i)->stopLogger();
            }

            m_MDIChildList.removeAt(i);
            updateEventViewerPosition();
        }
    }
}

void MainInformationWindow::updateEventViewerPosition ( void )
{
    for(int i=0; i< m_MDIChildList.size(); i++)
    {
        if( 0 == m_MDIChildList.at(i)->getName().compare("Event Viewer"))
        {
            m_nEventViewerPosition = i;
            m_bIsEventViewerOpen = true;
            return;
        }
    }

    m_bIsEventViewerOpen = false;
}

void MainInformationWindow::updateMenus ( void )
{
    bool bHasMdiChild = (0 != activeMdiChild());
    m_CascadeAct->setEnabled(bHasMdiChild);
    m_TileAct->setEnabled(bHasMdiChild); 
    m_CloseAllAct->setEnabled(bHasMdiChild);
}

MdiChild *MainInformationWindow::activeMdiChild()
{
    if (QMdiSubWindow *activeSubWindow = m_MDIArea->activeSubWindow())
        return qobject_cast<MdiChild *>(activeSubWindow->widget());

    return 0;
}

void MainInformationWindow::createMdiChild ( const QString &sWhatWindow )
{
    if(!m_MDIChildList.isEmpty())
    {
        for(int i=0; i< m_MDIChildList.size(); i++)
        {
            if(m_bIsLogging)
                m_bIsLogging = false;

            /* window is already available */
            if( 0 == m_MDIChildList.at(i)->getName().compare(sWhatWindow)) 
            {
                /* close and delete it */
                QMdiSubWindow *activeSubWindow = NULL;
                
                /* make sure that the window to close is active */
                do 
                {
                    m_MDIArea->activateNextSubWindow();
                    activeSubWindow = m_MDIArea->activeSubWindow();

                } while ( 0 != activeSubWindow->windowTitle().compare(sWhatWindow) );

                if(0 == sWhatWindow.compare("Event Viewer"))
                {
                    /* make sure to stop logging before delete the object */
                    m_bIsLogging = false;
                    m_MDIChildList.at(i)->stopLogger();
                }

                m_MDIArea->removeSubWindow(activeSubWindow);    
                disconnect(m_MDIChildList.at(i), SIGNAL(destroyed(QObject*)), this, SLOT(onDestroyed(QObject*)));
                m_MDIChildList.removeAt(i);
                updateEventViewerPosition();
                m_bIsLogging = true;
                return ;
            }
        }    
    }

    m_MDIChildList.append(new MdiChild(sWhatWindow));
    
    if( 0 == sWhatWindow.compare("Logging") )
    {
        m_MDIArea->addSubWindow(m_MDIChildList.back())->setWindowIcon(QIcon(":/VimbaViewer/Images/logger.png"));
        m_MDIChildList.back()->setObjectName("Logging");
        m_MDIChildList.back()->showMaximized();
    }
    else if( 0 == sWhatWindow.compare("Event Viewer") )
    {
        m_MDIArea->addSubWindow(m_MDIChildList.back())->setWindowIcon(QIcon(":/VimbaViewer/Images/open_eventviewer.png"));
        m_MDIChildList.back()->setObjectName("Event Viewer");
        m_MDIChildList.back()->showMaximized();
    }

    updateEventViewerPosition();
    m_bIsLogging = true;

    connect(m_MDIChildList.back(), SIGNAL(destroyed(QObject*)), this, SLOT(onDestroyed(QObject*)));
    m_MDIChildList.back()->show();
}

void MainInformationWindow::setEventMessage ( const QStringList &sMessage )
{    
    if( (m_bIsEventViewerOpen) && (m_bIsLogging) )
    {
        if ( ! m_MDIChildList.at(m_nEventViewerPosition)->getPauseState() )
        {
            /* show the last 100 rows */
            if ( MAX_LOGGING == ++m_NumberOfLogging ) 
            {
                m_NumberOfLogging = 0;
                m_MDIChildList.at(m_nEventViewerPosition)->clearLogger();
            }

            m_MDIChildList.at(m_nEventViewerPosition)->setLogger(sMessage, VimbaViewerLogCategory_INFO);
        }
    }
}
/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        MainWindow.h

  Description: The main window framework. This contains of camera tree, a toolbar and logging
               

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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_MainWindow.h"
#include "ui_StartOption.h"
#include "ui_OpenByID.h"
#include <QSettings>

#include "UI/LoggerWindow.h"
#include "UI/CameraTreeWindow.h"
#include "ViewerWindow.h"
#include "ForceIP.h"
#include "ActionCommand.h"
#include <QtCore/QSharedPointer>
#include <QMutex>
#include <QMutexLocker>

#include <VimbaCPP/Include/VimbaSystem.h>

using AVT::VmbAPI::VimbaSystem;
using AVT::VmbAPI::CameraPtrVector;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public: 
                
    protected:
                      
    private:
                QMutex                          m_Lock;
                Ui::mainWindow                  m_MainWindow;
                LoggerWindow*                   m_Logger;
                CameraTreeWindow*               m_CameraTree;
                QVector <ViewerWindow*>         m_Viewer;
                QVector<QTreeWidgetItem*>       m_GigE;
                QVector<QTreeWidgetItem*>       m_1394;
                QVector<QTreeWidgetItem*>       m_USB;
                QVector<QTreeWidgetItem*>       m_CL;
                QVector<QTreeWidgetItem*>       m_CSI2;

                class CameraInfo
                {
                private:
                    CameraPtr       m_Cam;
                    QString         m_DisplayName;
                    bool            m_IsOpen;
                    QStringList     m_PermittedAccess;
                    QStringList     m_PermittedAccessState;
                public:
                    CameraInfo()
                        : m_IsOpen( false )
                    {
                    }
                    explicit CameraInfo(CameraPtr cam, const QString & name)
                        : m_Cam( cam )
                        , m_DisplayName( name )
                        , m_IsOpen( false )
                    {
                    }
                    const CameraPtr &   Cam()                                       const   { return m_Cam; }
                    CameraPtr &         Cam()                                               { return m_Cam; }
                    const QString&      DisplayName()                               const   { return m_DisplayName; }
                    bool                IsOpen()                                    const   { return m_IsOpen; }
                    void                SetOpen(bool s)                                     { m_IsOpen = s; }
                    const QStringList&  PermittedAccess()                           const   { return m_PermittedAccess; }
                    void                PermittedAccess( const QStringList &l)              { m_PermittedAccess = l; }
                    const QStringList&  PermittedAccessState()                      const   { return m_PermittedAccessState; }
                    void                PermittedAccessState(const QStringList &l)          { m_PermittedAccessState = l; }
                    void SetPermittedAccessState(unsigned int pos, QString status)
                    {
                        m_PermittedAccessState[pos] = status;
                    }
                    void ResetPermittedAccessState()
                    {
                        for( int pos = 0; pos < m_PermittedAccessState.size(); ++ pos )
                        {
                            m_PermittedAccessState[pos] = "false";
                        }
                    }


                    bool operator==( const CameraPtr &other)                        const   { return SP_ACCESS(other) == SP_ACCESS( m_Cam); }
                    bool operator<( const CameraPtr &other)                         const   { return SP_ACCESS(other) < SP_ACCESS( m_Cam); }
                    VmbInterfaceType InterfaceType() const
                    {
                        VmbInterfaceType interfaceType;
                        VmbErrorType result = SP_ACCESS( m_Cam )->GetInterfaceType( interfaceType );
                        if( result != VmbErrorSuccess )
                        {
                            throw std::runtime_error("could not read interface type from camera");
                        }
                        return interfaceType;
                    }
                    QString InterfaceTypeString() const
                    {
                        VmbInterfaceType interfaceType;
                        VmbErrorType result = SP_ACCESS( m_Cam )->GetInterfaceType( interfaceType );
                        if( result != VmbErrorSuccess )
                        {
                            throw std::runtime_error("could not read interface type from camera");
                        }
                        switch( interfaceType )
                        {
                            case VmbInterfaceEthernet:
                                return "GigE";
                            case VmbInterfaceFirewire:
                                return "1394";
                            case VmbInterfaceUsb:
                                return "USB";
                            case VmbInterfaceCL:
                                return "CL";
                            case VmbInterfaceCSI2:
                                return "CSI2";
                        }
                    }
                };
                class FindByName
                {
                    const QString m_Name;
                public:
                    FindByName( const QString &s)
                        : m_Name( s )
                    {}
                    bool operator()( const CameraInfo &i) const
                    {
                        return i.DisplayName() == m_Name;
                    }
                };
                class CameraPtrCompare
                {
                    const CameraPtr m_Camera;
                public:
                    CameraPtrCompare( const CameraPtr& cam)
                        : m_Camera( cam )
                    {}
                    bool operator() ( const CameraPtr& other) const
                    {
                        return m_Camera.get() == other.get();
                    }
                };
                typedef QVector<CameraInfo>     CameraInfoVector;
                
                CameraInfoVector                m_CameraInfos;

                QString                         m_sOpenAccessType;
                QList <QAction *>               m_RightMouseAction;

                VimbaSystem&                    m_VimbaSystem;
                QString                         m_sAPIVersion;
                bool                            m_bIsRightMouseClicked;
                
                QString                         m_SelectedCamera;
                bool                            m_bIsCurrentModelChecked;
                bool                            m_bIsOpenByRightMouseClick;
                bool                            m_bIsInitialized;

                /*Start Option (auto adjust packet size) */
                Ui::StartOptionsDialog          m_StartOption;
                QDialog*                        m_StartOptionDialog;
                bool                            m_bIsAutoAdjustPacketSize;

                /* Open camera by ID*/
                Ui::OpenByIDDialog              m_OpenByID;
                QDialog*                        m_OpenByIDDialog;

                ForceIPDialog*                  m_ForceIPDialog;

                ActionCommandDialog*            m_ActionCommandDialog;
                bool                            m_ActionCommandFlag;

                QMenu rightMouseMenu;

    public:
                 MainWindow ( QWidget *parent = 0, Qt::WindowFlags flags = 0 );
                ~MainWindow ( void );

    protected:
                                
    private:
                void searchCameras                  ( const CameraPtrVector &Cameras );
                void openViewer                     ( CameraInfo &info );
                void closeViewer                    ( CameraPtr cam );
                QString getBestAccess               ( const CameraInfo &info );
                unsigned int getAccessListPosition  ( const QString &sAccessName );
                VmbErrorType getCameraDisplayName   ( const CameraPtr &camera, std::string &sDisplayName );
                VmbErrorType getIPAddress           ( const AVT::VmbAPI::CameraPtr &camera, QString &sIPAdress);

                virtual void closeEvent             ( QCloseEvent *event );
                virtual void showEvent              ( QShowEvent *event );

    private slots:
                /* when you use this std convention, you don't need any "connect..." */
                void on_ActionDiscover_triggered    ( void ); 
                void on_ActionClear_triggered       ( void );
                void on_ActionOpenByID_triggered    ( void );
                void on_ActionForceIP_triggered     ( void );
                void on_ActionCommand_triggered     ( void );
                void on_ActionStartOptions_triggered( void );
                
                /* Custom */
                void onCameraClicked                ( const QString &sModel, const bool &bIsChecked);
                void onRightMouseClicked            ( const bool &bIsClicked );
                void onCloseFromViewer              ( CameraPtr cam );
                void onUpdateDeviceList             ( void );
                void about                          ( void );
                void rightMouseOpenCamera           ( bool bOpenAccesState );
                void onInitializeVimba();

};

#endif 

/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        ForceIP.h

  Description: Dialog for sending GigEVision FORCEIP_CMD


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

#ifndef FORCE_IP_H
#define FORCE_IP_H

#include "Helper.h"
#include "ui_ForceIP.h"
#include <QMessageBox>
#include <VimbaCPP/Include/VimbaSystem.h>

// representation of Force IP command
typedef struct ForceIPCommand
{
    VmbInt64_t  mMACAddress;
    VmbInt64_t  mIPAddress;
    VmbInt64_t  mSubnetMask;
    VmbInt64_t  mGateway;
    VmbInt64_t  mDeviceSelector;

} ForceIPCommand;

// struct collecting information about a device
typedef struct DeviceInfo
{
    QString                 mDeviceID;
    AVT::VmbAPI::CameraPtr  mDevicePtr;
    QString                 mModelName;
    QString                 mAccessStatus;
    VmbUint64_t             mMACAddress;
    VmbUint64_t             mIPAddress;
    VmbUint64_t             mSubnetMask;
    VmbUint64_t             mGateway;
    VmbInt64_t              mDeviceSelector;

} DeviceInfo;

// struct collecting information about an interface
typedef struct InterfaceInfo
{
    QString                     mInterfaceID;
    AVT::VmbAPI::InterfacePtr   mInterfacePtr;
    QList<DeviceInfo*>*         mDevices;

} InterfaceInfo;

class ForceIPDialog : public QDialog
{
    Q_OBJECT

    private:
        Ui::ForceIPDialog                   m_Ui;
        QDialog*                            m_Dialog;
        bool                                m_InitializedFlag;
        QMessageBox*                        m_MsgBox;
        AVT::VmbAPI::VimbaSystem&           m_Sys;
        QList<InterfaceInfo*>*              m_InterfaceList;
        int                                 m_SelectedInterface;
        int                                 m_SelectedDevice;

    // constructors and destructors
    public:
         ForceIPDialog( AVT::VmbAPI::VimbaSystem &system );
        ~ForceIPDialog();

    // public methods called by Viewer instance
    public:
        bool InitializeDialog();
        void ResetDialog();
        bool RunDialog();

    // Qt slots
    private slots:
        void OnComboBoxNetworkInterface( int index );
        void OnComboBoxDeviceSelector( int index );

        void OnCheckBoxNetworkInterface( void );

    // Interface and Device methods
    private:
        bool GetInterfaces();
        bool GetDevices();

        void InitializeInterfaceInfoStruct( InterfaceInfo** interfaceInfoStruct );
        void InitializeDeviceInfoStruct( DeviceInfo** deviceInfoStruct );

    private:
        bool SetFeatureValue( QMap<const char*,AVT::VmbAPI::FeaturePtr>* featureMap, const char* featureName, VmbInt64_t featureValue );

    // Methods for fetching user input
    private:
        bool GetUserMACAddress( VmbInt64_t* macAddress );
        bool GetUserIPAddress( VmbInt64_t* ipAddress );
        bool GetUserSubnetMask( VmbInt64_t* subnetMask );
        bool GetUserGateway( VmbInt64_t* gateway );

    // Methods for creating, validating and sending a ForceIP command
    private:
        bool CreateCommand( ForceIPCommand* command );
        bool SendCommand( ForceIPCommand* command );

    // Widget update methods
    private:
        void UpdateInterfaceComboBox();
        void UpdateDeviceComboBox();
        void UpdateDeviceInformation( DeviceInfo* deviceInfoStruct );

    // Methods for resetting Widgets with default values
    private:
        void ResetAllWidgets();
        void ResetNetworkInterfaceGroup( bool withCheckBox );
        void ResetDeviceSelectorGroup( bool withCheckBox );
        void ResetForceIPCommandGroup();
        void ResetDeviceInformationGroup();

    // Methods for enabling and disabling widgets
    private:
        void EnableAllWidgets( bool enable );
        void EnableNetworkInterfaceGroup( bool enable );
        void EnableDeviceSelectorGroup( bool enable );
        void EnableForceIPCommandGroup( bool enable );
        void EnableDeviceInformationGroup( bool enable );

    // Cleanup methods
    private:
        void ClearInterfaceList( QList<InterfaceInfo*>** interfaceList );
        void ClearDeviceList( QList<DeviceInfo*>** deviceList );

    // Messaging and Error handling methods
    private:
        void PrepareMsgBox( QMessageBox::Icon icon, QString text, VmbErrorType err );
        void ShowMsgBox();

    // getter & setter
    public:
        bool GetInitializedFlag();
        void SetInitializedFlag( bool flag );
};

#endif
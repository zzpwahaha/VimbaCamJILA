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

#include "ForceIP.h"
#include "qdebug.h"
#include "qendian.h"

// define feature names for Force IP API usage
const char* kFeatureSend            = "GevDeviceForceIP";
const char* kFeatureMacAddress      = "GevDeviceForceMACAddress";
const char* kFeatureIpAddress       = "GevDeviceForceIPAddress";
const char* kFeatureSubnetMask      = "GevDeviceForceSubnetMask";
const char* kFeatureGateway         = "GevDeviceForceGateway";
const char* kFeatureDeviceSelector  = "DeviceSelector";

ForceIPDialog::ForceIPDialog( AVT::VmbAPI::VimbaSystem &system )
    : m_Dialog( NULL )
    , m_InitializedFlag( false )
    , m_MsgBox( NULL )
    , m_Sys( system )
    , m_InterfaceList( NULL )
    , m_SelectedInterface( -1 )
    , m_SelectedDevice( -1 )
{
    // create and setup Qt dialog
    this->m_Dialog = new QDialog( this, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint );
    this->m_Ui.setupUi( m_Dialog );

    // setup message box
    this->m_MsgBox = new QMessageBox( this );

    // create new interface list
    this->m_InterfaceList = new QList<InterfaceInfo*>();
}

ForceIPDialog::~ForceIPDialog()
{
    // delete allocated pointer
    if( NULL != this->m_Dialog ) delete m_Dialog;
    if( NULL != this->m_MsgBox ) delete m_MsgBox;

    // clear and delete interface list
    if( NULL != this->m_InterfaceList )
    {
        this->ClearInterfaceList( &this->m_InterfaceList );
        delete this->m_InterfaceList;
        this->m_InterfaceList = NULL;
    }
}

bool ForceIPDialog::InitializeDialog()
{
    bool            check   = true;
    VmbErrorType    err     = VmbErrorSuccess;

    // connect slots
    check = connect( this->m_Ui.comboBox_networkInterface,  SIGNAL(currentIndexChanged(int)),   this, SLOT(OnComboBoxNetworkInterface(int)) );
    check = connect( this->m_Ui.checkBox_networkInterface,  SIGNAL(clicked(bool)),              this, SLOT(OnCheckBoxNetworkInterface(void)) );
    check = connect( this->m_Ui.comboBox_deviceSelector,    SIGNAL(currentIndexChanged(int)),   this, SLOT(OnComboBoxDeviceSelector(int)) );

    if( false == check )
    {
        this->PrepareMsgBox( QMessageBox::Critical, "Failed to initialize Force IP dialog", err );
    }
    else
    {
        // reset and disable all widgets
        this->ResetAllWidgets();
        this->EnableAllWidgets( false );

        // get available interfaces
        check = this->GetInterfaces();
        if( (true == check) && (false == this->m_InterfaceList->isEmpty()) )
        {
            // enable network interface group widgets
            // and update combobox with found interfaces
            this->EnableNetworkInterfaceGroup( true );
            this->UpdateInterfaceComboBox();

            // enable force ip command group
            this->EnableForceIPCommandGroup( true );

            bool enableMACInput = this->m_Ui.checkBox_networkInterface->isChecked();
            this->m_Ui.lineEdit_forceIPCommandMAC->setEnabled(enableMACInput);
            this->m_Ui.label_forceIPCommandMAC->setEnabled(enableMACInput);
        }
    }

    // in case of any failure, show message box
    if( false == check )
    {
        this->ShowMsgBox();
    }
    else
    {
        this->m_InitializedFlag = true;
    }

    return check;
}

void ForceIPDialog::ResetDialog()
{
    // reset all widgets
    this->ResetAllWidgets();

    // refill interface combobox
    if( (NULL != this->m_InterfaceList) && (false == this->m_InterfaceList->isEmpty()) )
    {
        this->EnableNetworkInterfaceGroup( true );
        this->UpdateInterfaceComboBox();

        bool enableMACInput = this->m_Ui.checkBox_networkInterface->isChecked();
        this->m_Ui.lineEdit_forceIPCommandMAC->setEnabled(enableMACInput);
        this->m_Ui.label_forceIPCommandMAC->setEnabled(enableMACInput);
    }

}

bool ForceIPDialog::RunDialog()
{
    bool            rval        = false;
    VmbErrorType    err         = VmbErrorSuccess;
    QString         msg         = "";

    if( NULL != this->m_Dialog )
    {
        // run and show Qt dialog
        int dialogResult = this->m_Dialog->exec();

        // check dialog result (if Send button was clicked or dialog canceled)
        if( QDialog::Accepted == dialogResult )
        {
            // create ForceIP command struct
            ForceIPCommand command;

            // fetch user input and fill command struct
            rval = this->CreateCommand( &command );
            if( true == rval )
            {
                // prepare local variables for further usage
                InterfaceInfo*                              interfaceInfoStruct;
                AVT::VmbAPI::FeaturePtr                     feature;
                AVT::VmbAPI::InterfacePtr                   interfacePtr;
                QMap<const char*, AVT::VmbAPI::FeaturePtr>  featureMapSystem;
                QMap<const char*, AVT::VmbAPI::FeaturePtr>  featureMapInterface;

                // transer fetched user information to Vimba API
                // depending which setting was chosen by user
                // and send command
                try
                {
                    // user System module and send command via all interfaces
                    // or user Interface module and send on chosen interface
                    if( -1 == this->m_SelectedInterface )
                    {
                        // get MAC address feature
                        err = m_Sys.GetFeatureByName( kFeatureMacAddress, feature );
                        if( VmbErrorSuccess != err )
                        {
                            msg = "Failed to get feature '" + QString( kFeatureMacAddress ) + "' from System module";
                            this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                            throw std::exception();
                        }

                        // add MAC address feature to system map
                        featureMapSystem.insert( kFeatureMacAddress, feature );

                        // get IP address feature
                        err = m_Sys.GetFeatureByName( kFeatureIpAddress, feature );
                        if( VmbErrorSuccess != err )
                        {
                            msg = "Failed to get feature '" + QString( kFeatureIpAddress ) + "' from System module";
                            this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                            throw std::exception();
                        }

                        // add IP address feature to system map
                        featureMapSystem.insert( kFeatureIpAddress, feature );

                        // get Subnet mask feature
                        err = m_Sys.GetFeatureByName( kFeatureSubnetMask, feature );
                        if( VmbErrorSuccess != err )
                        {
                            msg = "Failed to get feature '" + QString( kFeatureSubnetMask ) + "' from System module";
                            this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                            throw std::exception();
                        }

                        // add Subnet mask feature to system map
                        featureMapSystem.insert( kFeatureSubnetMask, feature );

                        // get Gateway feature
                        err = m_Sys.GetFeatureByName( kFeatureGateway, feature );
                        if( VmbErrorSuccess != err )
                        {
                            msg = "Failed to get feature '" + QString( kFeatureGateway ) + "' from System module";
                            this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                            throw std::exception();
                        }

                        // add MAC address feature to system map
                        featureMapSystem.insert( kFeatureGateway, feature );
                    }
                    else
                    {
                        // get interface info struct
                        interfaceInfoStruct = this->m_InterfaceList->at( this->m_SelectedInterface );
                        if( NULL != interfaceInfoStruct )
                        {
                            // get interface pointer and open interface
                            interfacePtr = interfaceInfoStruct->mInterfacePtr;
                            if( NULL != interfacePtr )
                            {
                                err = interfacePtr->Open();
                                if( VmbErrorSuccess != err )
                                {
                                    msg = "Failed to open interface '" + interfaceInfoStruct->mInterfaceID + "'";
                                    this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                                    throw std::exception();
                                }

                                // get device selector feature
                                err = interfacePtr->GetFeatureByName( kFeatureDeviceSelector, feature );
                                if( VmbErrorSuccess != err )
                                {
                                    msg = "Failed to get feature '" + QString( kFeatureDeviceSelector ) + "' from Interface module";
                                    this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                                    throw std::exception();
                                }

                                // add device selector feature to system map
                                featureMapInterface.insert( kFeatureDeviceSelector, feature );

                                // get IP address feature
                                err = interfacePtr->GetFeatureByName( kFeatureIpAddress, feature );
                                if( VmbErrorSuccess != err )
                                {
                                    msg = "Failed to get feature '" + QString( kFeatureIpAddress ) + "' from Interface module";
                                    this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                                    throw std::exception();
                                }

                                // add IP address feature to system map
                                featureMapInterface.insert( kFeatureIpAddress, feature );

                                // get Subnet mask feature
                                err = interfacePtr->GetFeatureByName( kFeatureSubnetMask, feature );
                                if( VmbErrorSuccess != err )
                                {
                                    msg = "Failed to get feature '" + QString( kFeatureSubnetMask ) + "' from Interface module";
                                    this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                                    throw std::exception();
                                }

                                // add Subnet mask feature to system map
                                featureMapInterface.insert( kFeatureSubnetMask, feature );

                                // get Gateway geature
                                err = interfacePtr->GetFeatureByName( kFeatureGateway, feature );
                                if( VmbErrorSuccess != err )
                                {
                                    msg = "Failed to get feature '" + QString( kFeatureGateway ) + "' from Interface module";
                                    this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                                    throw std::exception();
                                }

                                // add Gateway feature to system map
                                featureMapInterface.insert( kFeatureGateway, feature );
                            }
                        }
                    }

                    // set feature value to Vimba (with regard to System or Interface module choice)

                    // fetch MAC address feature
                    QMap<const char*, AVT::VmbAPI::FeaturePtr>* featureMapPtr = NULL;
                    if( -1 == this->m_SelectedInterface )
                    {
                        featureMapPtr = &featureMapSystem;

                        // set MAC address to Vimba
                        rval = this->SetFeatureValue( featureMapPtr, kFeatureMacAddress, command.mMACAddress );
                        if( false == rval )
                        {
                            msg = "Failed to set value '" + Helper::MACToString(command.mMACAddress) + "' for feature '" + QString( kFeatureMacAddress );
                            this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                            throw std::exception();
                        }
                    }
                    else
                    {
                        featureMapPtr = &featureMapInterface;

                        // set device selector to Vimba
                        rval = this->SetFeatureValue( featureMapPtr, kFeatureDeviceSelector, command.mDeviceSelector );
                        if( false == rval )
                        {
                            msg = "Failed to set value '" + QString::number(command.mDeviceSelector) + "' for feature '" + QString( kFeatureDeviceSelector );
                            this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                            throw std::exception();
                        }
                    }

                    // set IP address to Vimba
                    rval = this->SetFeatureValue( featureMapPtr, kFeatureIpAddress, command.mIPAddress );
                    if( false == rval )
                    {
                        msg = "Failed to set value '0x" + QString::number(command.mIPAddress, 16).toUpper() + "' for feature '" + QString( kFeatureIpAddress );
                        this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                        throw std::exception();
                    }

                    // set Subnet mask to Vimba
                    rval = this->SetFeatureValue( featureMapPtr, kFeatureSubnetMask, command.mSubnetMask );
                    if( false == rval )
                    {
                        msg = "Failed to set value '0x" + QString::number(command.mSubnetMask, 16).toUpper() + "' for feature '" + QString( kFeatureSubnetMask );
                        this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                        throw std::exception();
                    }

                    // set Gateway to Vimba
                    rval = this->SetFeatureValue( featureMapPtr, kFeatureGateway, command.mGateway );
                    if( false == rval )
                    {
                        msg = "Failed to set value '0x" + QString::number(command.mGateway, 16).toUpper() + "' for feature '" + QString( kFeatureGateway );
                        this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                        throw std::exception();
                    }

                    rval = this->SendCommand( &command );

                    // close interface, if necessary
                    if( (-1 != this->m_SelectedInterface) && (false == SP_ISNULL( interfacePtr )) )
                    {
                        err = interfacePtr->Close();
                        if( VmbErrorSuccess != err )
                        {
                            msg = "Failed to close interface '" + interfaceInfoStruct->mInterfaceID + "'";
                            this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                            throw std::exception();
                        }
                    }

                }
                catch( std::exception& e )
                {
                    this->ShowMsgBox();
                }
                catch(...)
                {
                    this->ShowMsgBox();
                }

            }

        }
    }

    return rval;
}

void ForceIPDialog::OnComboBoxNetworkInterface( int index )
{
    // update selected interface
    this->m_SelectedInterface = index;

    // get devices for selected interface and update combobox
    bool check = this->GetDevices();
    this->UpdateDeviceComboBox();

    // in case devices were found
    if( true == check )
    {
        if( (NULL != this->m_InterfaceList->at(index)->mDevices) && (0 != this->m_InterfaceList->at(index)->mDevices->count()))
        {
            this->EnableDeviceSelectorGroup( true );
        }
        else
        {
            this->ResetDeviceSelectorGroup( true );
            this->ResetDeviceInformationGroup();

            this->m_Ui.lineEdit_forceIPCommandMAC->setText("");

            this->EnableDeviceSelectorGroup( false );
            this->EnableDeviceInformationGroup( false );
        }
    }
    else
    {
        this->ResetDeviceSelectorGroup( true );
        this->ResetDeviceInformationGroup();

        this->EnableDeviceSelectorGroup( false );
        this->EnableDeviceInformationGroup( false );
    }

}

void ForceIPDialog::OnComboBoxDeviceSelector( int index )
{
    // update selected device
    this->m_SelectedDevice = index;

    if( -1 != this->m_SelectedInterface )
    {
        // get device info struct
        DeviceInfo* deviceInfoStruct = this->m_InterfaceList->at( this->m_SelectedInterface )->mDevices->at( index );
        if( NULL != deviceInfoStruct )
        {
            // enable update device information
            this->EnableDeviceInformationGroup( true );
            this->UpdateDeviceInformation( deviceInfoStruct );
        }
    }
}

void ForceIPDialog::OnCheckBoxNetworkInterface( void )
{
    // if checkbox is checked,
    // set internal variable accordingly
    if( true == this->m_Ui.checkBox_networkInterface->isChecked() )
    {
        // reset and disable network interface combobox
        this->ResetNetworkInterfaceGroup( false );
        this->m_Ui.comboBox_networkInterface->setEnabled( false );

        //Enable the MAC field
        this->m_Ui.label_forceIPCommandMAC->setEnabled(true);
        this->m_Ui.lineEdit_forceIPCommandMAC->setEnabled(true);

        // set selected interface index to -1 (no interface selected)
        this->m_SelectedInterface = -1;

        // reset and disable device- selector and information group
        this->ResetDeviceSelectorGroup( true );
        this->ResetDeviceInformationGroup();
        this->EnableDeviceSelectorGroup( false );
        this->EnableDeviceInformationGroup( false );
    }
    else
    {
        // enable network interface combobox and update it
        this->m_Ui.comboBox_networkInterface->setEnabled( true );

        //Enable the MAC field
        this->m_Ui.label_forceIPCommandMAC->setEnabled(false);
        this->m_Ui.lineEdit_forceIPCommandMAC->setEnabled(false);

        this->UpdateInterfaceComboBox();
    }
}

bool ForceIPDialog::GetInterfaces()
{
    VmbErrorType err = VmbErrorSuccess;

    // get available interfaces from Vimba
    // return and show message box, if no interface found
    AVT::VmbAPI::InterfacePtrVector interfacePtrList;
    err = m_Sys.GetInterfaces( interfacePtrList );
    if( VmbErrorSuccess != err )
    {
        this->PrepareMsgBox( QMessageBox::Critical, "Failed to get avaiable Interfaces from Vimba", err );
        return false;
    }
    else if( true == interfacePtrList.empty() )
    {
        this->PrepareMsgBox( QMessageBox::Critical, "No Interfaces found", err );
        return false;
    }

    // set selected interface to first in list
    this->m_SelectedInterface = 0;

    // prepare local variables for upcoming loop
    std::string                                 interfaceId = "";
    VmbInterfaceType                            interfaceType = VmbInterfaceUnknown;
    AVT::VmbAPI::InterfacePtr                   interfacePtr;
    AVT::VmbAPI::InterfacePtrVector::iterator   iter_interfaces;

    // iterate through interface list, determine GigE interfaces,
    // create interface info structs and add them to internal list
    for( iter_interfaces = interfacePtrList.begin(); iter_interfaces != interfacePtrList.end(); ++iter_interfaces )
    {
        // get current interface pointer
        interfacePtr = *iter_interfaces;
        if( false == SP_ISNULL(interfacePtr) )
        {
            // determine interface type (GigE only)
            err = interfacePtr->GetType( interfaceType );
            if( (VmbErrorSuccess != err) || (VmbInterfaceEthernet != interfaceType) )
            {
                continue;
            }

            // get interface ID
            err = interfacePtr->GetID( interfaceId );
            if( VmbErrorSuccess != err )
            {
                continue;
            }

            // create interface info struct and initialize it
            InterfaceInfo* interfaceInfoStruct = new InterfaceInfo;
            this->InitializeInterfaceInfoStruct( &interfaceInfoStruct );

            // add interface id and pointer to info struct
            interfaceInfoStruct->mInterfaceID       = QString( interfaceId.c_str() );
            interfaceInfoStruct->mInterfacePtr      = interfacePtr;
            interfaceInfoStruct->mDevices           = NULL;

            // add info struct to internal list
            this->m_InterfaceList->append( interfaceInfoStruct );
        }
    }

    // check if GigE interfaces were found
    if( true == this->m_InterfaceList->isEmpty() )
    {
        this->PrepareMsgBox( QMessageBox::Critical, "No GigE Interfaces found", err );
        return false;
    }

    return true;
}

bool ForceIPDialog::GetDevices()
{
    bool            rval    = true;
    VmbErrorType    err     = VmbErrorSuccess;

    // check if valid interface was selected
    if( -1 != this->m_SelectedInterface )
    {
        // get interface info struct for selected interface
        InterfaceInfo* interfaceInfoStruct = this->m_InterfaceList->at( this->m_SelectedInterface );
        if( NULL != interfaceInfoStruct )
        {
            // check if info struct has no device list allocated
            if( NULL == interfaceInfoStruct->mDevices )
            {
                // allocate new device info struct list
                interfaceInfoStruct->mDevices = new QList<DeviceInfo*>();
            }
            else
            {
                // clear already existing device info struct list
                this->ClearDeviceList( &interfaceInfoStruct->mDevices );
            }

            // get interface pointer
            AVT::VmbAPI::InterfacePtr interfacePtr = interfaceInfoStruct->mInterfacePtr;
            if( false == SP_ISNULL(interfacePtr) )
            {
                // open current interface
                err = interfacePtr->Open();
                if( VmbErrorSuccess == err )
                {
                    AVT::VmbAPI::FeaturePtr feature;
                    err = interfacePtr->GetFeatureByName( "DeviceUpdateList", feature );
                    if( VmbErrorSuccess == err )
                    {
                        err = feature->RunCommand();
                    }

                    // get device count
                    VmbInt64_t deviceCount = 0;
                    if( VmbErrorSuccess == err )
                    {
                        err = interfacePtr->GetFeatureByName( "DeviceCount", feature );
                        if( VmbErrorSuccess == err )
                        {
                            err = feature->GetValue( deviceCount );
                        }
                    }

                    // proceed if no error occured and devices were found on interface
                    if( (VmbErrorSuccess == err) && ( 0 < deviceCount ) )
                    {
                        // prepare local variables for loop
                        std::string temp = "";
                        VmbInt64_t temp2 = 0;
                        AVT::VmbAPI::CameraPtr devicePtr;
                        AVT::VmbAPI::FeaturePtr selectorFeature;

                        // get interface device selector
                        err = interfacePtr->GetFeatureByName( kFeatureDeviceSelector, selectorFeature );
                        if( VmbErrorSuccess == err )
                        {
                            // iterate through device list
                            for( int i=0; i<deviceCount; ++i )
                            {
                                // select current device
                                err = selectorFeature->SetValue(i);
                                if( VmbErrorSuccess == err )
                                {
                                    // create device info struct and initialize it
                                    DeviceInfo* deviceInfoStruct = new DeviceInfo;
                                    this->InitializeDeviceInfoStruct( &deviceInfoStruct );
                                    deviceInfoStruct->mDeviceSelector = (VmbInt64_t)i;

                                    // get device id
                                    err = interfacePtr->GetFeatureByName( "DeviceID", feature );
                                    if( VmbErrorSuccess == err )
                                    {
                                        err = feature->GetValue( temp );
                                        if( VmbErrorSuccess == err )
                                        {
                                            deviceInfoStruct->mDeviceID = QString( temp.c_str() );
                                        }

                                        // get device pointer
                                        err = m_Sys.GetCameraByID( temp.c_str(), devicePtr );
                                        if( VmbErrorSuccess == err )
                                        {
                                            deviceInfoStruct->mDevicePtr = devicePtr;
                                        }
                                    }

                                    // get device model name
                                    err = interfacePtr->GetFeatureByName( "DeviceModelName", feature );
                                    if( VmbErrorSuccess == err )
                                    {
                                        err = feature->GetValue( temp );
                                        if( VmbErrorSuccess == err )
                                        {
                                            deviceInfoStruct->mModelName = QString( temp.c_str() );
                                        }
                                    }

                                    // get device access status
                                    err = interfacePtr->GetFeatureByName( "DeviceAccessStatus", feature );
                                    if( VmbErrorSuccess == err )
                                    {
                                        err = feature->GetValue( temp );
                                        if( VmbErrorSuccess == err )
                                        {
                                            deviceInfoStruct->mAccessStatus = QString( temp.c_str() );
                                        }
                                    }

                                    // get device MAC address
                                    err = interfacePtr->GetFeatureByName( "GevDeviceMACAddress", feature );
                                    if( VmbErrorSuccess == err )
                                    {
                                        err = feature->GetValue( temp2 );
                                        if( VmbErrorSuccess == err )
                                        {
                                            deviceInfoStruct->mMACAddress = temp2;
                                        }
                                    }

                                    // get device IP address
                                    err = interfacePtr->GetFeatureByName( "GevDeviceIPAddress", feature );
                                    if( VmbErrorSuccess == err )
                                    {
                                        err = feature->GetValue( temp2 );
                                        if( VmbErrorSuccess == err )
                                        {
                                            deviceInfoStruct->mIPAddress = temp2;
                                        }
                                    }

                                    // get device subnet mask
                                    err = interfacePtr->GetFeatureByName( "GevDeviceSubnetMask", feature );
                                    if( VmbErrorSuccess == err )
                                    {
                                        err = feature->GetValue( temp2 );
                                        if( VmbErrorSuccess == err )
                                        {
                                            deviceInfoStruct->mSubnetMask = temp2;
                                        }
                                    }

                                    // in case of no error add device info struct to list
                                    if( VmbErrorSuccess == err )
                                    {
                                        interfaceInfoStruct->mDevices->append( deviceInfoStruct );
                                    }
                                }

                            } // end of for loop
                        }
                    }

                    // close interface
                    err = interfacePtr->Close();
                }

                // in case of any failure clear device list
                if( VmbErrorSuccess != err )
                {
                    this->ClearDeviceList( &interfaceInfoStruct->mDevices );
                    rval = false;
                }
           }
        }
    }
    return rval;
}

void ForceIPDialog::InitializeInterfaceInfoStruct( InterfaceInfo** interfaceInfoStruct )
{
    if( NULL != interfaceInfoStruct )
    {
        (*interfaceInfoStruct)->mInterfaceID    = "";
        (*interfaceInfoStruct)->mDevices        = NULL;

        SP_RESET( (*interfaceInfoStruct)->mInterfacePtr );
    }
}

void ForceIPDialog::InitializeDeviceInfoStruct( DeviceInfo** deviceInfoStruct )
{
    if( NULL != deviceInfoStruct )
    {
        (*deviceInfoStruct)->mDeviceID          = "";
        (*deviceInfoStruct)->mModelName         = "";
        (*deviceInfoStruct)->mAccessStatus      = "";
        (*deviceInfoStruct)->mMACAddress        = 0;
        (*deviceInfoStruct)->mIPAddress         = 0;
        (*deviceInfoStruct)->mSubnetMask        = 0;
        (*deviceInfoStruct)->mGateway           = 0;
        (*deviceInfoStruct)->mDeviceSelector    = 0;

        SP_RESET( (*deviceInfoStruct)->mDevicePtr );
    }
}

bool ForceIPDialog::SetFeatureValue( QMap<const char*,AVT::VmbAPI::FeaturePtr>* featureMap, const char* featureName, VmbInt64_t featureValue )
{
    bool            rval    = false;
    VmbErrorType    err     = VmbErrorSuccess;
    QString         msg     = "";

    // fetch feature from given map
    if( (NULL != featureMap) && (true == featureMap->contains(featureName)) )
    {
        AVT::VmbAPI::FeaturePtr feature = (*featureMap)[featureName];
        if( false == SP_ISNULL( feature ) )
        {
            // set value to Vimba
            err = feature->SetValue( featureValue );
            if( VmbErrorSuccess == err )
            {
                rval = true;
            }
        }
    }

    return rval;
}

bool ForceIPDialog::GetUserMACAddress( VmbInt64_t* macAddress )
{
    bool        rval    = false;
    QString     temp    = "";
    VmbInt64_t  temp2   = 0;

    // get user input
    temp = this->m_Ui.lineEdit_forceIPCommandMAC->text();
    if( false == temp.isEmpty() )
    {
        temp2 = Helper::StringToMAC( temp );
        if( -1 != temp2 )
        {
            rval = true;
            *macAddress = temp2;
        }
    }

    return rval;
}

bool ForceIPDialog::GetUserIPAddress( VmbInt64_t* ipAddress )
{
    bool        rval    = false;
    QString     temp    = "";
    VmbInt64_t  temp2   = 0;

    // get user input
    temp = this->m_Ui.lineEdit_forceIPCommandIP->text();
    if( false == temp.isEmpty() )
    {
        temp2 = Helper::StringToIPv4( temp, false );
        if( -1 != temp2 )
        {
            rval = true;
            *ipAddress = temp2;
        }
    }

    return rval;
}

bool ForceIPDialog::GetUserSubnetMask( VmbInt64_t* subnetMask )
{
    bool        rval    = false;
    QString     temp    = "";
    VmbInt64_t  temp2   = 0;

    // get user input
    temp = this->m_Ui.lineEdit_forceIPCommandSubnet->text();
    if( false == temp.isEmpty() )
    {
        temp2 = Helper::StringToIPv4( temp, false );
        if( -1 != temp2 )
        {
            rval = true;
            *subnetMask = temp2;
        }
    }

    return rval;
}

bool ForceIPDialog::GetUserGateway( VmbInt64_t* gateway )
{
    bool        rval    = false;
    QString     temp    = "";
    VmbInt64_t  temp2   = 0;

    // get user input
    temp = this->m_Ui.lineEdit_forceIPCommandGateway->text();
    if( false == temp.isEmpty() )
    {
        temp2 = Helper::StringToIPv4( temp, false );
        if( -1 != temp2 )
        {
            rval = true;
            *gateway = temp2;
        }
    }

    return rval;
}

bool ForceIPDialog::CreateCommand( ForceIPCommand* command )
{
    bool        rval = false;
    VmbInt64_t  temp = 0;

    if( NULL != command )
    {
        try
        {
            // get user MAC address
            rval = this->GetUserMACAddress( &temp );
            if( true == rval )
            {
                command->mMACAddress = temp;
            }
            else
            {
                this->PrepareMsgBox( QMessageBox::Critical, "Invalid MAC address given", VmbErrorBadParameter );
                throw std::exception();
            }

            // get user IP address
            rval = this->GetUserIPAddress( &temp );
            if( true == rval )
            {
                command->mIPAddress = temp;
            }
            else
            {
                this->PrepareMsgBox( QMessageBox::Critical, "Invalid IP address given", VmbErrorBadParameter );
                throw std::exception();
            }

            // get user subnet mask
            rval = this->GetUserSubnetMask( &temp );
            if( true == rval )
            {
                command->mSubnetMask = temp;
            }
            else
            {
                this->PrepareMsgBox( QMessageBox::Critical, "Invalid Subnet mask given", VmbErrorBadParameter );
                throw std::exception();
            }

            // get user gateway
            rval = this->GetUserGateway( &temp );
            if( true == rval )
            {
                command->mGateway = temp;
            }
            else
            {
                this->PrepareMsgBox( QMessageBox::Critical, "Invalid Gateway given", VmbErrorBadParameter );
                throw std::exception();
            }

            //get device selector
            if( -1 != m_SelectedDevice )
            {
                command->mDeviceSelector = m_Ui.comboBox_deviceSelector->currentIndex();
            }
        }
        catch( std::exception& e )
        {
            this->ShowMsgBox();
        }
        catch(...)
        {
            this->ShowMsgBox();
        }

    }

    return rval;
}

bool ForceIPDialog::SendCommand( ForceIPCommand* command )
{
    bool            rval    = false;
    VmbErrorType    err     = VmbErrorSuccess;
    QString         msg     = "";

    if( NULL != command )
    {
        AVT::VmbAPI::FeaturePtr feature;

        // determine if command shall be sent via all interfaces
        // or on specific interface
        if( -1 == this->m_SelectedInterface )
        {
            err = m_Sys.GetFeatureByName( kFeatureSend, feature );
        }
        else
        {
            // get interface info struct and poniter
            InterfaceInfo* interfaceInfoStruct = this->m_InterfaceList->at( this->m_SelectedInterface );
            if( NULL != interfaceInfoStruct )
            {
                AVT::VmbAPI::InterfacePtr interfacePtr = interfaceInfoStruct->mInterfacePtr;
                if( false == SP_ISNULL( interfacePtr ) )
                {
                    err = interfacePtr->GetFeatureByName( kFeatureSend, feature );
                }
            }
        }

        if( VmbErrorSuccess == err )
        {
            err = feature->RunCommand();
            if( VmbErrorSuccess != err )
            {
                msg = "Failed to send Force IP command { MAC: " + QString::number(command->mMACAddress) + ", IP: " + QString::number(command->mIPAddress) + ", Snet: " + QString::number(command->mSubnetMask) + ", Gw: " + QString::number(command->mGateway) + "}";
                this->PrepareMsgBox( QMessageBox::Critical, msg, err );
            }
            else
            {

                this->PrepareMsgBox( QMessageBox::Information, "Force IP command has been sent successfully", err );
                rval = true;
            }
        }
    }

    return rval;
}

void ForceIPDialog::UpdateInterfaceComboBox()
{
    // block signals for combobox
    this->m_Ui.comboBox_networkInterface->blockSignals( true );

    // if combo box is already filled, reset it
    if( 0 != this->m_Ui.comboBox_networkInterface->count() )
    {
        this->ResetNetworkInterfaceGroup( false );
    }

    // iterate through internal interface list
    QList<InterfaceInfo*>::Iterator iter;
    for( iter = this->m_InterfaceList->begin(); iter != this->m_InterfaceList->end(); ++iter )
    {
        // get current interface id
        // and add it to combobox
        QString interfaceId = (*iter)->mInterfaceID;
        if( false == interfaceId.isEmpty() )
        {
            this->m_Ui.comboBox_networkInterface->addItem( interfaceId );
        }
    }

    // set combobox index to 0
    if( 0 != this->m_Ui.comboBox_networkInterface->count() )
    {
        this->OnComboBoxNetworkInterface( 0 );
    }

    // unblock signals for combobox
    this->m_Ui.comboBox_networkInterface->blockSignals( false );
}

void ForceIPDialog::UpdateDeviceComboBox()
{
    // block signals for combobox
    this->m_Ui.comboBox_deviceSelector->blockSignals( true );

    // if combo box is already filled, reset it
    if( 0 != this->m_Ui.comboBox_deviceSelector->count() )
    {
        this->ResetDeviceSelectorGroup( false );
    }

    if( -1 != this->m_SelectedInterface )
    {
        // get pointer to interface info struct
        InterfaceInfo* interfaceInfoStruct = this->m_InterfaceList->at( this->m_SelectedInterface );
        if( NULL != interfaceInfoStruct )
        {
            // check if internal device list has been allocated
            if( NULL != interfaceInfoStruct->mDevices )
            {
                // iterate through list
                QList<DeviceInfo*>::Iterator iter;
                for( iter = interfaceInfoStruct->mDevices->begin(); iter != interfaceInfoStruct->mDevices->end(); ++iter )
                {
                    // get current device id and add it to combobox
                    QString deviceId = (*iter)->mDeviceID;
                    if( false == deviceId.isEmpty() )
                    {
                        this->m_Ui.comboBox_deviceSelector->addItem( deviceId );
                    }
                }

                // set combobox index to 0
                if( 0 != this->m_Ui.comboBox_deviceSelector->count() )
                {
                    this->OnComboBoxDeviceSelector( 0 );
                }
            }
        }
    }

    // unblock signals for combobox
    this->m_Ui.comboBox_deviceSelector->blockSignals( false );
}

void ForceIPDialog::UpdateDeviceInformation( DeviceInfo* deviceInfoStruct )
{
    if( NULL != deviceInfoStruct )
    {
        QString temp = "";

        this->m_Ui.lineEdit_deviceInformationID->setText( deviceInfoStruct->mDeviceID );
        this->m_Ui.lineEdit_deviceInformationModel->setText( deviceInfoStruct->mModelName );
        this->m_Ui.lineEdit_deviceInformationAccess->setText( deviceInfoStruct->mAccessStatus );

        // update device MAC address
        temp = Helper::MACToString( deviceInfoStruct->mMACAddress );
        temp = temp.toUpper();
        this->m_Ui.lineEdit_deviceInformationMAC->setText( temp );
        this->m_Ui.lineEdit_forceIPCommandMAC->setText( temp );

        // update device IP address
        temp = Helper::IPv4ToString( deviceInfoStruct->mIPAddress, false);
        this->m_Ui.lineEdit_deviceInformationIP->setText( temp );
        this->m_Ui.lineEdit_forceIPCommandIP->setText( temp );

        // update device subnet mask
        temp = Helper::IPv4ToString( deviceInfoStruct->mSubnetMask, false );
        this->m_Ui.lineEdit_deviceInformationSubnet->setText( temp );
        this->m_Ui.lineEdit_forceIPCommandSubnet->setText( temp );

        // update device gateway
        temp = Helper::IPv4ToString( deviceInfoStruct->mGateway, false );
        this->m_Ui.lineEdit_deviceInformationGateway->setText( temp );
        this->m_Ui.lineEdit_forceIPCommandGateway->setText( temp );
    }
}

void ForceIPDialog::ResetAllWidgets()
{
    this->ResetNetworkInterfaceGroup( true );
    this->ResetDeviceSelectorGroup( true );
    this->ResetForceIPCommandGroup();
    this->ResetDeviceInformationGroup();
}

void ForceIPDialog::ResetNetworkInterfaceGroup( bool withCheckBox )
{
    this->m_Ui.comboBox_networkInterface->blockSignals( true );
    this->m_Ui.checkBox_networkInterface->blockSignals( true );
    this->m_Ui.comboBox_networkInterface->clear();

    if( true == withCheckBox )
    {
        this->m_Ui.checkBox_networkInterface->setChecked( false );
    }

    this->m_Ui.comboBox_networkInterface->blockSignals( false );
    this->m_Ui.checkBox_networkInterface->blockSignals( false );
}

void ForceIPDialog::ResetDeviceSelectorGroup( bool withCheckBox )
{
    this->m_Ui.comboBox_deviceSelector->blockSignals( true );
    this->m_Ui.comboBox_deviceSelector->clear();

    this->m_Ui.comboBox_deviceSelector->blockSignals( false );
}

void ForceIPDialog::ResetForceIPCommandGroup()
{
    this->m_Ui.lineEdit_forceIPCommandMAC->setText( "" );
    this->m_Ui.lineEdit_forceIPCommandIP->setText( "" );
    this->m_Ui.lineEdit_forceIPCommandSubnet->setText( "" );
    this->m_Ui.lineEdit_forceIPCommandGateway->setText( "" );
}

void ForceIPDialog::ResetDeviceInformationGroup()
{
    this->m_Ui.lineEdit_deviceInformationID->setText( "" );
    this->m_Ui.lineEdit_deviceInformationModel->setText( "" );
    this->m_Ui.lineEdit_deviceInformationAccess->setText( "" );
    this->m_Ui.lineEdit_deviceInformationMAC->setText( "" );
    this->m_Ui.lineEdit_deviceInformationIP->setText( "" );
    this->m_Ui.lineEdit_deviceInformationSubnet->setText( "" );
    this->m_Ui.lineEdit_deviceInformationGateway->setText( "" );
}

void ForceIPDialog::EnableAllWidgets( bool enable )
{
    this->EnableNetworkInterfaceGroup( enable );
    this->EnableDeviceSelectorGroup( enable );
    this->EnableForceIPCommandGroup( enable );
    this->EnableDeviceInformationGroup( enable );
}

void ForceIPDialog::EnableNetworkInterfaceGroup( bool enable )
{
    this->m_Ui.comboBox_networkInterface->setEnabled( enable );
    this->m_Ui.checkBox_networkInterface->setEnabled( enable );
}

void ForceIPDialog::EnableDeviceSelectorGroup( bool enable )
{
    this->m_Ui.comboBox_deviceSelector->setEnabled( enable );
}

void ForceIPDialog::EnableForceIPCommandGroup( bool enable )
{
    this->m_Ui.lineEdit_forceIPCommandMAC->setEnabled( enable );
    this->m_Ui.lineEdit_forceIPCommandIP->setEnabled( enable );
    this->m_Ui.lineEdit_forceIPCommandSubnet->setEnabled( enable );
    this->m_Ui.lineEdit_forceIPCommandGateway->setEnabled( enable );
    this->m_Ui.pushButton_forceIPCommandSend->setEnabled( enable );
    this->m_Ui.pushButton_forceIPCommandCancel->setEnabled( enable );
}

void ForceIPDialog::EnableDeviceInformationGroup( bool enable )
{
    this->m_Ui.lineEdit_deviceInformationID->setEnabled( enable );
    this->m_Ui.lineEdit_deviceInformationModel->setEnabled( enable );
    this->m_Ui.lineEdit_deviceInformationAccess->setEnabled( enable );
    this->m_Ui.lineEdit_deviceInformationMAC->setEnabled( enable );
    this->m_Ui.lineEdit_deviceInformationIP->setEnabled( enable );
    this->m_Ui.lineEdit_deviceInformationSubnet->setEnabled( enable );
    this->m_Ui.lineEdit_deviceInformationGateway->setEnabled( enable );
}

void ForceIPDialog::ClearInterfaceList( QList<InterfaceInfo*>** interfaceList )
{
    if( (NULL != interfaceList) && (false == (*interfaceList)->isEmpty()) )
    {
        // iterate through given list
        QList<InterfaceInfo*>::Iterator iter;
        for( iter = (*interfaceList)->begin(); iter != (*interfaceList)->end(); ++iter )
        {
            // get current interface info struct
            InterfaceInfo* interfaceInfoStruct = *iter;
            if( NULL != interfaceInfoStruct )
            {
                // check for internal device list
                // and clear list
                if( NULL != interfaceInfoStruct->mDevices )
                {
                    this->ClearDeviceList( &interfaceInfoStruct->mDevices );
                }

                delete interfaceInfoStruct;
                interfaceInfoStruct = NULL;
            }

        }

        // clear list
        (*interfaceList)->clear();
    }
}

void ForceIPDialog::ClearDeviceList( QList<DeviceInfo*>** deviceList )
{
    if( (NULL != deviceList) && (false == (*deviceList)->isEmpty()) )
    {
        // iterate through given list
        QList<DeviceInfo*>::Iterator iter;
        for( iter = (*deviceList)->begin(); iter != (*deviceList)->end(); ++iter )
        {
            // get current device info struct
            DeviceInfo* deviceInfoStruct = *iter;
            if( NULL != deviceInfoStruct )
            {
                delete deviceInfoStruct;
                deviceInfoStruct = NULL;
            }
        }

        // clear list
        (*deviceList)->clear();
    }
}

void ForceIPDialog::PrepareMsgBox( QMessageBox::Icon icon, QString text, VmbErrorType err )
{
    if( NULL != this->m_MsgBox )
    {
        this->m_MsgBox->setIcon( icon );
        QString msg = text;
        if( 0 != err )
        {
            msg + ".\n[error code: " + QString::number(err) + "]";
        }
        this->m_MsgBox->setText( msg );
    }
}

void ForceIPDialog::ShowMsgBox()
{
    if( NULL != this->m_MsgBox )
    {
        this->m_MsgBox->exec();
    }
}

bool ForceIPDialog::GetInitializedFlag()
{
    return this->m_InitializedFlag;
}

void ForceIPDialog::SetInitializedFlag( bool flag )
{
    this->m_InitializedFlag = flag;
}

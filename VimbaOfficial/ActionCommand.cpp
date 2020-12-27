/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        ActionCommand.cpp

  Description: Dialog for creating and sending Action Commands
               

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

#include "ActionCommand.h"

typedef enum eInterfaceIndex
{
    eInterfaceIndexUnknown  = -2,
    eInterfaceIndexAll      = -1

} eInterfaceIndex;

ActionCommandDialog::ActionCommandDialog( AVT::VmbAPI::VimbaSystem &system )
    : m_Dialog( NULL )
    , m_Sys( system )
    , m_Interfaces( NULL )
    , m_SelectedInterface( eInterfaceIndexUnknown )
    , m_Unicast( false )
    , m_MsgBox( NULL )
    , m_FeatureList( NULL )
    , m_FeatureValues( NULL )
{
    bool check = false;

    // create and setup Qt dialog
    this->m_Dialog = new QDialog( this, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint );
    this->m_Ui.setupUi( m_Dialog );
    this->m_Dialog->setFixedSize( 790, 340 );

    // create interface vector
    this->m_Interfaces = new QVector<AVT::VmbAPI::InterfacePtr>();

    // connect slots
    check = connect( this->m_Ui.pushButton_Send,            SIGNAL(clicked(bool)),              this, SLOT(OnSendButtonActionCommand(void)) );
    check = connect( this->m_Ui.checkBox_singleDevice,      SIGNAL(clicked(bool)),              this, SLOT(OnCheckBoxSingleDevice(void)) );
    check = connect( this->m_Ui.checkBox_networkInterface,  SIGNAL(clicked(bool)),              this, SLOT(OnCheckBoxNetworkInterface(void)) );
    check = connect( this->m_Ui.comboBox_networkInterface,  SIGNAL(currentIndexChanged(int)),   this, SLOT(OnComboBoxNetworkInterface(int)) );

    // setup message box
    this->m_MsgBox = new QMessageBox( this );

    // setup feature list
    this->m_FeatureList = new QVector<QString>();
    this->m_FeatureList->append( "ActionDeviceKey" );
    this->m_FeatureList->append( "ActionGroupKey" );
    this->m_FeatureList->append( "ActionGroupMask" );
    this->m_FeatureList->append( "ActionCommand" );

    // setup feature value list
    this->m_FeatureValues = new QVector<VmbInt64_t>();

    // disable ip input for unicast
    // and set network selection as unchecked
    this->m_Ui.lineEdit_singleDevice->setEnabled( false );
    this->m_Ui.checkBox_networkInterface->setChecked( false );
}

ActionCommandDialog::~ActionCommandDialog()
{
    // delete Qt dialog
    if( NULL != m_Dialog )
    {
        delete m_Dialog;
    }

    // delete interface vector
    if( NULL != m_Interfaces )
    {
        delete m_Interfaces;
    }

    // delete message box
    if( NULL != m_MsgBox )
    {
        delete m_MsgBox;
    }

    // delete feature list
    if( NULL != m_FeatureList )
    {
        delete m_FeatureList;
    }

    // delete feature value list
    if( NULL != m_FeatureValues )
    {
        delete m_FeatureValues;
    }
}

void ActionCommandDialog::RunDialog()
{
    VmbErrorType    err         = VmbErrorSuccess;
    bool            check       = false;
    bool            failureFlag = true;
    int             rval        = 0;
    QString         msg         = "";

    if( NULL != this->m_Dialog )
    {
        // reset dialog
        ResetDialog();

        try
        {
            // get avaialable interfaces from Vimba
            // and fill combo box (GigE only)
            err = this->CreateInterfaceList();
            if( VmbErrorSuccess != err )
            {
                throw std::exception();
            }

            // run dialog
            this->m_Dialog->show();
        }
        catch( std::exception& /*e*/ )
        {
            this->ShowMsgBox();
        }
        catch(...)
        {
            this->ShowMsgBox();
        }

        if( (VmbErrorSuccess == err) && (QDialog::Accepted == rval) )
        {
            this->PrepareMsgBox( QMessageBox::Information, "Action Command has been sent successfully", VmbErrorSuccess );
            this->ShowMsgBox();
        }

    }
}

void ActionCommandDialog::OnSendButtonActionCommand()
{
    bool                        check       = false;
    VmbErrorType                err         = VmbErrorSuccess;
    VmbInt64_t                  ipAddress   = 0;
    QString                     msg         = "";
    AVT::VmbAPI::InterfacePtr   selectedInterface;

    try
    {
        // clear feature values
        this->m_FeatureValues->clear();

        // get deviceKey from user input
        check = this->GetDeviceKey();
        if( false == check )
        {
            this->PrepareMsgBox( QMessageBox::Critical, "Invalid Device Key entered", VmbErrorBadParameter );
            throw std::exception();
        }

        // get groupKey from user input
        check = this->GetGroupKey();
        if( false == check )
        {
            this->PrepareMsgBox( QMessageBox::Critical, "Invalid Group Key entered", VmbErrorBadParameter );
            throw std::exception();
        }

        // get groupMask from user input
        check = this->GetGroupMask();
        if( false == check )
        {
            this->PrepareMsgBox( QMessageBox::Critical, "Invalid Group Mask entered", VmbErrorBadParameter );
            throw std::exception();
        }

        // if unicast is enabled, get IP address
        if( true == this->m_Unicast )
        {
            ipAddress = this->GetIpAddress();
            if( -1 == ipAddress )
            {
                this->PrepareMsgBox( QMessageBox::Critical, "Invalid IP address entered", VmbErrorBadParameter );
                throw std::exception();
            }
        }

        // check if specific interface was selcted
        if( eInterfaceIndexAll != this->m_SelectedInterface )
        {
            // get interface pointer
            selectedInterface = this->m_Interfaces->at( this->m_SelectedInterface );
            if( false == SP_ISNULL(selectedInterface) )
            {
                // open interface
                err = selectedInterface->Open();
                if( VmbErrorSuccess != err )
                {
                    this->PrepareMsgBox( QMessageBox::Critical, "Failed to open interface", err );
                    throw std::exception();
                }
            }
        }

        // proceed if no error occured
        if( VmbErrorSuccess == err )
        {
            // iterate through feature names and values
            for( int i=0; i<3; ++i )
            {
                // get current feature name and corresponding value to set
                QString     featureName     = m_FeatureList->at(i);
                VmbInt64_t  featureValue    = m_FeatureValues->at(i);

                // set feature value
                err = this->SetFeatureValue( featureName, featureValue, this->m_SelectedInterface );
                if( VmbErrorSuccess != err )
                {
                    throw std::exception();
                }
            }
        }

        // if unicast is enabled, set corresponding ip feature
        if( true == this->m_Unicast )
        {
            // get feature pointer
            AVT::VmbAPI::FeaturePtr feature;
            const char *featureName = "GevActionDestinationIPAddress";
            if( eInterfaceIndexAll == this->m_SelectedInterface )
            {
                err = this->m_Sys.GetFeatureByName( featureName, feature );
            }
            else
            {
                err = selectedInterface->GetFeatureByName( featureName, feature );
            }

            if( VmbErrorSuccess == err )
            {
                // set feature value
                err = feature->SetValue( ipAddress );
                if( VmbErrorSuccess != err )
                {
                    msg = "Failed to set feature value '" + QString::number(ipAddress) + "' for '" + QString(featureName) + "'";
                    this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                    throw std::exception();
                }
            }
            else
            {
                msg = "Failed to retrieve feature pointer for '" + QString(featureName) + "'";
                this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                throw std::exception();
            }
        }

        // send Action Command
        err = this->SendCommand( this->m_SelectedInterface );
        if( VmbErrorSuccess != err )
        {
            throw std::exception();
        }

        this->LogActionCommand();

        // close interface, if one selected and opened
        if( eInterfaceIndexAll != this->m_SelectedInterface )
        {
            if( false == SP_ISNULL( selectedInterface ) )
            {
                err = selectedInterface->Close();
                if( VmbErrorSuccess != err )
                {
                    this->PrepareMsgBox( QMessageBox::Critical, "Failed to close interface", err );
                    throw std::exception();
                }
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

void ActionCommandDialog::OnComboBoxNetworkInterface( int index )
{
    // get selected interface index
    this->m_SelectedInterface = index;
}

void ActionCommandDialog::OnCheckBoxNetworkInterface()
{
    // if checkbox was checked,
    // set internal variable accordingly
    if( true == this->m_Ui.checkBox_networkInterface->isChecked() )
    {
        this->m_Ui.comboBox_networkInterface->setEnabled( false );
        this->m_SelectedInterface = eInterfaceIndexAll;
    }
    else
    {
        this->m_Ui.comboBox_networkInterface->setEnabled( true );
        this->m_SelectedInterface = this->m_Ui.comboBox_networkInterface->currentIndex();
    }
}

void ActionCommandDialog::OnCheckBoxSingleDevice()
{
    // if checkbox was checked,
    // set internal variable accordingly
    if( true == this->m_Ui.checkBox_singleDevice->isChecked() )
    {
        this->m_Ui.lineEdit_singleDevice->setEnabled( true );
        this->m_Unicast = true;
    }
    else
    {
        this->m_Ui.lineEdit_singleDevice->setEnabled( false );
        this->m_Unicast = false;
    }
}

VmbErrorType ActionCommandDialog::CreateInterfaceList()
{
    VmbErrorType    err = VmbErrorOther;
    QString         msg = "Failed to retrieve interfaces";

    // get interfaces from vimba
    AVT::VmbAPI::InterfacePtrVector interfaceList;
    err = m_Sys.GetInterfaces( interfaceList );
    if( VmbErrorSuccess == err )
    {
        // iterate through Vimba interface list
        // and retrieve IDs (ethernet only)
        AVT::VmbAPI::InterfacePtrVector::iterator iter;
        for( iter = interfaceList.begin();
                iter != interfaceList.end();
                ++iter )
        {
            // determine interface type
            VmbInterfaceType interfaceType;
            err = (*iter)->GetType( interfaceType );
            if( VmbErrorSuccess == err )
            {
                if( VmbInterfaceEthernet == interfaceType )
                {
                    // store current interface to internal vector
                    this->m_Interfaces->append( *iter );

                    // get interface ID
                    std::string interfaceId = "";
                    err = (*iter)->GetID( interfaceId );
                    if( VmbErrorSuccess == err )
                    {
                        this->m_Ui.comboBox_networkInterface->addItem( QString(interfaceId.c_str()) );
                    }
                    else
                    {
                        msg += QString( ". GetID() failed" );
                        this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                        break;
                    }
                }
            }
            else
            {
                msg += QString( ". Unable to determine interface type" );
                this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                break;
            }
        }
    }
    else
    {
        this->PrepareMsgBox( QMessageBox::Critical, msg, err );
    }

    // in case of any failure,
    // show message box
    if( VmbErrorSuccess == err )
    {
        this->m_Ui.comboBox_networkInterface->setCurrentIndex( 0 );
        this->m_SelectedInterface = 0;
    }

    return err;
}

VmbErrorType ActionCommandDialog::SetFeatureValue( QString featureName, VmbInt64_t featureValue, int selectedInterface )
{
    VmbErrorType                err = VmbErrorSuccess;
    QString                     msg = "";
    AVT::VmbAPI::InterfacePtr   interfacePtr;
    AVT::VmbAPI::FeaturePtr     feature;
    
    // check parameter
    if( false == featureName.isEmpty() )
    {
        // check if 'all interfaces' is selected
        if( eInterfaceIndexAll == selectedInterface )
        {
            // get feature pointer
            err = this->m_Sys.GetFeatureByName( featureName.toStdString().c_str(), feature );
            if( VmbErrorSuccess != err )
            {
                msg = "Failed to retrieve feature '" + featureName + "'";
                this->PrepareMsgBox( QMessageBox::Critical, msg, err );
            }
        }
        else
        {
            // get feature pointer
            interfacePtr = this->m_Interfaces->at( selectedInterface );
            if( false == SP_ISNULL( interfacePtr ) )
            {
                err = interfacePtr->GetFeatureByName( featureName.toStdString().c_str(), feature );
                if( VmbErrorSuccess != err )
                {
                    msg = "Failed to retrieve feature '" + featureName + "' from selected interface";
                    this->PrepareMsgBox( QMessageBox::Critical, msg, err );
                }
            }
        }

        if( VmbErrorSuccess == err )
        {
            // set given feature value
            err = feature->SetValue( featureValue );
            if( VmbErrorSuccess != err )
            {
                msg = "Failed to set feature value '" + QString::number(featureValue) + "' for '" + featureName + "'";
                this->PrepareMsgBox( QMessageBox::Critical, msg, err );
            }
        }
    }

    return err;
}

bool ActionCommandDialog::GetDeviceKey()
{
    bool rval = true;

    // get user input from device key field
    QString stringValue = this->m_Ui.lineEdit_DeviceKey->text();
    if( false == stringValue.isEmpty() )
    {
        // convert string value to integer
        VmbInt64_t intValue = stringValue.toUInt( &rval );
        if( true == rval )
        {
            this->m_FeatureValues->append( intValue );
        }
    }
    else
    {
        rval = false;
    }

    return rval;
}

bool ActionCommandDialog::GetGroupKey()
{
    bool rval = true;

    // get user input from group key field
    QString stringValue = this->m_Ui.lineEdit_GroupKey->text();
    if( false == stringValue.isEmpty() )
    {
        // convert string value to integer
        VmbInt64_t intValue = stringValue.toUInt( &rval );
        if( true == rval )
        {
            this->m_FeatureValues->append( intValue );
        }
    }
    else
    {
        rval = false;
    }

    return rval;
}

bool ActionCommandDialog::GetGroupMask()
{
    bool rval = true;

    // get user input from group mask field
    QString stringValue = this->m_Ui.lineEdit_GroupMask->text();
    if( false == stringValue.isEmpty() )
    {
        // convert string value to integer
        VmbInt64_t intValue = stringValue.toUInt( &rval );
        if( true == rval )
        {
            this->m_FeatureValues->append( intValue );
        }
    }
    else
    {
        rval = false;
    }

    return rval;
}

VmbInt64_t ActionCommandDialog::GetIpAddress()
{
    VmbInt64_t rval = 0;

    // get user input from ip address field
    QString stringValue = this->m_Ui.lineEdit_singleDevice->text();
    if( false == stringValue.isEmpty() )
    {
        // convert ip address string to integer value
        VmbInt64_t intValue = Helper::StringToIPv4( stringValue, true );
        if( -1 != intValue )
        {
            rval = intValue;
        }
    }
    else
    {
        rval = -1;
    }

    return rval;
}

VmbErrorType ActionCommandDialog::SendCommand( int selectedInterface )
{
    VmbErrorType            err = VmbErrorSuccess;
    QString                 msg = "";
    AVT::VmbAPI::FeaturePtr feature;

    // get feature name for sending Action Command
    QString featureName = this->m_FeatureList->at(3);

    // get feature pointer
    // on interface or system level (all interfaces)
    if( eInterfaceIndexAll == selectedInterface )
    {
        err = m_Sys.GetFeatureByName( featureName.toStdString().c_str(), feature );
    }
    else
    {
        AVT::VmbAPI::InterfacePtr interfacePtr = this->m_Interfaces->at( selectedInterface );
        if( false == SP_ISNULL(interfacePtr) )
        {
            err = interfacePtr->GetFeatureByName( featureName.toStdString().c_str(), feature );
        }
    }
    
    if( VmbErrorSuccess == err )
    {
        // send action command
        err = feature->RunCommand();
        if( VmbErrorSuccess != err )
        {
            this->PrepareMsgBox( QMessageBox::Critical, "Failed to send Action Command", err );
        }
    }
    else
    {
        msg = "Failed to retrieve feature '" + featureName + "'";
        this->PrepareMsgBox( QMessageBox::Critical, msg, err );
    }

    return err;
}

void ActionCommandDialog::PrepareMsgBox( QMessageBox::Icon icon, QString text, VmbErrorType err )
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

void ActionCommandDialog::ShowMsgBox()
{
    if( NULL != this->m_MsgBox )
    {
        this->m_MsgBox->exec();
    }
}

void ActionCommandDialog::ResetDialog()
{
    // network interface group
    this->m_Ui.comboBox_networkInterface->clear();
    this->m_Ui.comboBox_networkInterface->setEnabled( true );
    this->m_Ui.checkBox_networkInterface->setChecked( false );
    this->m_SelectedInterface = eInterfaceIndexUnknown;

    // action command group
    this->m_FeatureValues->clear();

    // single device group
    this->m_Ui.lineEdit_singleDevice->setEnabled( false );
    this->m_Ui.checkBox_singleDevice->setChecked( false );

    // command log
    this->m_Ui.listWidget_commandLog->clear();
}

void ActionCommandDialog::LogActionCommand()
{
    QTime timeStamp;
    QString temp;
    temp = timeStamp.currentTime().toString() + QString( "..." ) + QString( "Action Command has been sent" );
    this->m_Ui.listWidget_commandLog->insertItem( 0, temp );
}

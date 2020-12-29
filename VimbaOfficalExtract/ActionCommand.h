/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        ActionCommand.h

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

#ifndef ACTION_COMMAND_H
#define ACTION_COMMAND_H

#include "Helper.h"
#include "ui_ActionCommand.h"
#include <QMessageBox>
#include <QDateTime>
#include <VimbaCPP/Include/VimbaSystem.h>

class ActionCommandDialog : public QDialog
{
    Q_OBJECT

    private:
        Ui::ActionCommandDialog              m_Ui;
        QDialog                             *m_Dialog;
        AVT::VmbAPI::VimbaSystem            &m_Sys;
        QVector<AVT::VmbAPI::InterfacePtr>  *m_Interfaces;
        int                                  m_SelectedInterface;
        bool                                 m_Unicast;
        QMessageBox                         *m_MsgBox;
        QVector<QString>                    *m_FeatureList;
        QVector<VmbInt64_t>                *m_FeatureValues;

    public:
         ActionCommandDialog( AVT::VmbAPI::VimbaSystem &system );
        ~ActionCommandDialog();

        void RunDialog();

    private slots:
        void OnSendButtonActionCommand();
        void OnComboBoxNetworkInterface( int index );
        void OnCheckBoxNetworkInterface( void );
        void OnCheckBoxSingleDevice( void );

    private:
        VmbErrorType CreateInterfaceList();
        VmbErrorType SetFeatureValue( QString featureName, VmbInt64_t featureValue, int selectedInterface );

        bool    GetDeviceKey();
        bool    GetGroupKey();
        bool    GetGroupMask();
        VmbInt64_t  GetIpAddress();

        VmbErrorType SendCommand( int selectedInterface );

        void PrepareMsgBox( QMessageBox::Icon icon, QString text, VmbErrorType err );
        void ShowMsgBox();

        void ResetDialog();
        void LogActionCommand();


};

#endif
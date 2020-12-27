/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        HexMainWindow.h

  Description: a hex editor to show raw data

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

#ifndef HEXMAINWINDOW_H
#define HEXMAINWINDOW_H

#include "QHexEdit.h"
#include "HexOptionDialog.h"
#include <QMainWindow>



QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QUndoStack;
QT_END_NAMESPACE


#include <VimbaCPP/Include/VimbaSystem.h>

using AVT::VmbAPI::FeaturePtr;

class HexMainWindow : public QMainWindow
{
    Q_OBJECT
    
    protected:

    private:    
             FeaturePtr m_RawDataFeatPtr;

    public:
             HexMainWindow ( QWidget *parent = 0, Qt::WindowFlags flag = 0,  QString sID = " ", bool bIsReadOnly = false, FeaturePtr featPtr = FeaturePtr() );
            ~HexMainWindow ( void );

             void setData (const QByteArray &array);

    protected:
             void closeEvent(QCloseEvent *event);

    private:
             void init();
             void createActions();
             void createMenus();
             void createStatusBar();
             void createToolBars();
             void readSettings();
             void saveFile(void);
             void writeSettings();

             QMenu           *m_FileMenu;
             QMenu           *m_EditMenu;

             QToolBar        *m_FileToolBar;

             QAction         *m_SaveAct;
             QAction         *m_OptionsAct;

             QHexEdit        *m_HexEdit;
             HexOptionDialog *m_OptionsDialog;
             QLabel          *m_LbAddress, *m_LbAddressName;
             QLabel          *m_LbOverwriteMode, *m_LbOverwriteModeName;
             QLabel          *m_LbSize, *m_LbSizeName;


    private slots:
             void settingsAccepted();
             void save( void);
             void setAddress(int address);
             void setOverwriteMode(bool mode);
             void setSize(int size);
             void showOptionsDialog();
};

#endif
/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        HexMainWindow.cpp

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

#include "HexMainWindow.h"


HexMainWindow::HexMainWindow ( QWidget *parent, Qt::WindowFlags flag, QString sID, bool bIsReadOnly, FeaturePtr featPtr ) : QMainWindow( parent, flag )
{
    m_RawDataFeatPtr = featPtr;
    setWindowModality(Qt::WindowModal);
    
    std::string sFeatureDisplayName;
    m_RawDataFeatPtr->GetDisplayName(sFeatureDisplayName);
    setWindowTitle(sID.append(" <").append(QString::fromStdString(sFeatureDisplayName)).append(">"));
    init();
    QPoint p = QCursor::pos();
    readSettings();
    move(p.x()-100, p.y()-100);
    m_HexEdit->setReadOnly(bIsReadOnly);
    show();
}

HexMainWindow::~HexMainWindow ( void )
{
    
}

void HexMainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
}

void HexMainWindow::settingsAccepted()
{
    writeSettings();
    readSettings();
}

void HexMainWindow::save()
{
    saveFile();
}

void HexMainWindow::setAddress(int address)
{
    m_LbAddress->setText(QString("%1").arg(address, 1, 16));
}

void HexMainWindow::setOverwriteMode(bool mode)
{
    if (mode)
        m_LbOverwriteMode->setText(tr("Overwrite"));
    else
        m_LbOverwriteMode->setText(tr("Insert"));
}

void HexMainWindow::setSize(int size)
{
    m_LbSize->setText(QString("%1").arg(size));
}

void HexMainWindow::showOptionsDialog()
{
    m_OptionsDialog->show();
}

void HexMainWindow::init()
{
    setAttribute(Qt::WA_DeleteOnClose);
    m_OptionsDialog = new HexOptionDialog(this);
    m_OptionsDialog->setWindowFlags(m_OptionsDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    connect(m_OptionsDialog, SIGNAL(accepted()), this, SLOT(settingsAccepted()));

    m_HexEdit = new QHexEdit;
    setCentralWidget(m_HexEdit);
    connect(m_HexEdit, SIGNAL(overwriteModeChanged(bool)), this, SLOT(setOverwriteMode(bool)));
    
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    setUnifiedTitleAndToolBarOnMac(true);
}

void HexMainWindow::createActions()
{
    m_SaveAct = new QAction(QIcon(":/VimbaViewer/Images/save.png"), tr("&Save"), this);
    m_SaveAct->setShortcuts(QKeySequence::Save);
    m_SaveAct->setStatusTip(tr("Save data to buffer"));
    connect(m_SaveAct, SIGNAL(triggered()), this, SLOT(save()));

    m_OptionsAct = new QAction(QIcon(":/VimbaViewer/Images/option.png"), tr("Se&ttings"), this);
    m_OptionsAct->setStatusTip(tr("Show the Dialog to select editor options"));
    connect(m_OptionsAct, SIGNAL(triggered()), this, SLOT(showOptionsDialog()));
}

void HexMainWindow::createMenus()
{
    m_FileMenu = menuBar()->addMenu(tr("&File"));
    m_FileMenu->addAction(m_SaveAct);

    m_EditMenu = menuBar()->addMenu(tr("&Edit"));
    m_EditMenu->addAction(m_OptionsAct);
}

void HexMainWindow::createStatusBar()
{
    // Address Label
    m_LbAddressName = new QLabel();
    m_LbAddressName->setText(tr("Address:"));
    statusBar()->addPermanentWidget(m_LbAddressName);
    m_LbAddress = new QLabel();
    m_LbAddress->setFrameShape(QFrame::Panel);
    m_LbAddress->setFrameShadow(QFrame::Sunken);
    m_LbAddress->setMinimumWidth(70);
    statusBar()->addPermanentWidget(m_LbAddress);
    connect(m_HexEdit, SIGNAL(currentAddressChanged(int)), this, SLOT(setAddress(int)));

    // Size Label
    m_LbSizeName = new QLabel();
    m_LbSizeName->setText(tr("Size:"));
    statusBar()->addPermanentWidget(m_LbSizeName);
    m_LbSize = new QLabel();
    m_LbSize->setFrameShape(QFrame::Panel);
    m_LbSize->setFrameShadow(QFrame::Sunken);
    m_LbSize->setMinimumWidth(70);
    statusBar()->addPermanentWidget(m_LbSize);
    connect(m_HexEdit, SIGNAL(currentSizeChanged(int)), this, SLOT(setSize(int)));

    // Overwrite Mode Label
    m_LbOverwriteModeName = new QLabel();
    m_LbOverwriteModeName->setText(tr("Mode:"));
    statusBar()->addPermanentWidget(m_LbOverwriteModeName);
    m_LbOverwriteMode = new QLabel();
    m_LbOverwriteMode->setFrameShape(QFrame::Panel);
    m_LbOverwriteMode->setFrameShadow(QFrame::Sunken);
    m_LbOverwriteMode->setMinimumWidth(70);
    statusBar()->addPermanentWidget(m_LbOverwriteMode);
    setOverwriteMode(m_HexEdit->overwriteMode());

    statusBar()->showMessage(tr("Ready"));
}

void HexMainWindow::createToolBars()
{
    m_FileToolBar = addToolBar(tr("File"));
    m_FileToolBar->addAction(m_SaveAct);
}

void HexMainWindow::setData (const QByteArray &array)
{
    m_HexEdit->setData(array);
    QApplication::restoreOverrideCursor();
}

void HexMainWindow::readSettings()
{
    QSettings settings("Allied Vision", "Vimba Viewer");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(610, 460)).toSize();
    move(pos);
    resize(size);

    m_HexEdit->setAddressArea(settings.value("AddressArea").toBool());
    m_HexEdit->setAsciiArea(settings.value("AsciiArea").toBool());
    m_HexEdit->setHighlighting(settings.value("Highlighting").toBool());
    m_HexEdit->setOverwriteMode(settings.value("OverwriteMode").toBool());
    m_HexEdit->setReadOnly(settings.value("ReadOnly").toBool());

    m_HexEdit->setHighlightingColor(settings.value("HighlightingColor").value<QColor>());
    m_HexEdit->setAddressAreaColor(settings.value("AddressAreaColor").value<QColor>());
    m_HexEdit->setSelectionColor(settings.value("SelectionColor").value<QColor>());
    m_HexEdit->setFont(settings.value("WidgetFont").value<QFont>());

    m_HexEdit->setAddressWidth(settings.value("AddressAreaWidth").toInt());
}

void HexMainWindow::saveFile( void )
{    
    std::vector <VmbUchar_t> v;
    for(int i=0; i < m_HexEdit->data().size(); i++)
    {
        v.push_back(m_HexEdit->data().at(i));
    }
    
    VmbError_t error = m_RawDataFeatPtr->SetValue(v);
    if(VmbErrorSuccess != error)
    {
        QMessageBox::critical(this, tr("VimbaViewer"), tr("writing data to camera FAILED"));
    }
}

void HexMainWindow::writeSettings()
{
    QSettings settings("Allied Vision", "Vimba Viewer");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}


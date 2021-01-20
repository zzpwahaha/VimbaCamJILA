
//Description: The viewer window framework.
//             This contains of dock widgets like camera feature tree, a histogram, a toolbar and MDI window



#include "ViewerWindow.h"
#include <QTimer>
#include "UI/LineEditCompleter.h"
#include "UI/SortFilterProxyModel.h"
//#include "SplashScreen.h"
#include "UI/tabextensioninterface.h"
#include "UI/ControllerTreeWindow.h"
#include "UI/DockWidgetWindow.h"
#include "UI/Histogram/HistogramWindow.h"
#include "UI/MainInformationWindow.h"
#include "UI/Viewer.h"
#include "VmbImageTransformHelper.hpp"


using AVT::VmbAPI::Frame;
using AVT::VmbAPI::FramePtr;

#ifndef _WIN32
    #include <unistd.h>
#endif


ViewerWindow::ViewerWindow ( QWidget *parent, Qt::WindowFlags flag, QString sID, QString sAccess, bool bAutoAdjustPacketSize, CameraPtr pCam )
    : QMainWindow( NULL, flag )
    , m_DockController( NULL )
    , m_DockInformation( NULL )
    , m_Controller( NULL )
    , m_ScreenViewer( NULL )
    , m_InformationWindow( NULL )
    , m_bHasJustStarted ( false )
    , m_bIsFirstStart   ( true )
    , m_bIsCameraRunning ( false )
    , m_bIsCamOpen ( false )
    , m_bIsRedHighlighted ( false )
    , m_bIsViewerWindowClosing ( false )
    , m_bIsDisplayEveryFrame ( false )
    , m_ImageOptionDialog ( NULL )
    , m_saveFileDialog ( NULL )
    , m_getDirDialog ( NULL )
    , m_bIsTriggeredByMultiSaveBtn (false)
    , m_nNumberOfFramesToSave (0)
    , m_FrameBufferCount (BUFFER_COUNT)
    , m_pCam( pCam )
{
    

    VmbError_t errorType;
    QTime openTimer;
    openTimer.start();

    /* setup information window */
    m_InformationWindow = new MainInformationWindow(m_DockInformation, 0, m_pCam);
    m_InformationWindow->openLoggingWindow();

    if( 0 == sAccess.compare(tr("Open FULL ACCESS")))
    {
        errorType = m_pCam->Open(VmbAccessModeFull);
        m_sAccessMode = tr("(FULL ACCESS)");
    }
    else if( 0 == sAccess.compare(tr("Open READ ONLY")))
    {
        errorType = m_pCam->Open(VmbAccessModeRead);
        m_sAccessMode = tr("(READ ONLY)");
        bAutoAdjustPacketSize = false;
    }
    else if( 0 == sAccess.compare(tr("Open CONFIG")))
    {
        errorType = m_pCam->Open(VmbAccessModeConfig);
        m_sAccessMode = tr("(CONFIG MODE)");
        bAutoAdjustPacketSize = false;
    }
    else if( 0 == sAccess.compare(tr("Open LITE")))
    {
        errorType = m_pCam->Open(VmbAccessModeLite);
        m_sAccessMode = tr("(LITE)");
        bAutoAdjustPacketSize = false;
    }
    else
    {
        errorType = VmbErrorInvalidAccess;
        bAutoAdjustPacketSize = false;
    }

    m_OpenError = errorType;

    if( VmbErrorSuccess != errorType )
    {
        openTimer.elapsed();
        return;
    }

    m_sCameraID = sID;

    /* create ViewerWindow */
    setupUi(this);
    if(!m_sAccessMode.isEmpty())
    {
        sID.append(" ");
        sID.append(m_sAccessMode);
    }
    this->setWindowTitle(sID);

    ActionFreerun->setEnabled(isStreamingAvailable());

    /* add Viewer Widget to ViewerWindow*/
    m_pScene        = QSharedPointer<QGraphicsScene>(new QGraphicsScene());
    m_PixmapItem    = new QGraphicsPixmapItem();
    m_ScreenViewer  = new Viewer(Ui::ViewerWindow::centralWidget);
    m_ScreenViewer->setScene(m_pScene.data());
    m_pScene->addItem(m_PixmapItem);
    this->setCentralWidget( m_ScreenViewer );
    QPixmap image( ":/VimbaViewer/Images/stripes_256.png" );
    m_PixmapItem->setPixmap(image);
    m_ScreenViewer->show();
    m_bIsCamOpen = true;

    /* add DockWidgets: Controller and Information */
    m_DockController   = new DockWidgetWindow(tr("Controller for ")+sID, this);
    m_DockInformation  = new DockWidgetWindow(tr("Information for ")+sID, this);
    m_DockHistogram    = new DockWidgetWindow(tr("Histogram for ")+sID, this);
    m_DockController->setObjectName("Controller");
    m_DockInformation->setObjectName("Information");
    m_DockHistogram->setObjectName("Histogram");

    this->addDockWidget(static_cast<Qt::DockWidgetArea>(2), m_DockController);
    this->addDockWidget(Qt::BottomDockWidgetArea, m_DockInformation);
    this->addDockWidget(static_cast<Qt::DockWidgetArea>(1), m_DockHistogram);
    m_DockHistogram->hide();

    /* add Controller Tree */

    QWidget     *dockWidgetContents   = new QWidget();
    QVBoxLayout *verticalLayout2      = new QVBoxLayout(dockWidgetContents);
    QSplitter   *splitter             = new QSplitter(dockWidgetContents);
    QWidget     *verticalLayoutWidget = new QWidget(splitter);
    QVBoxLayout *verticalLayout       = new QVBoxLayout(verticalLayoutWidget);
    QTabWidget  *tabWidget            = new QTabWidget(verticalLayoutWidget);
    QWidget     *widgetTree           = new QWidget();
    QVBoxLayout *verticalLayout3      = new QVBoxLayout(widgetTree);

    m_Description = new QTextEdit();
    m_Controller = new ControllerTreeWindow(m_sCameraID, widgetTree, bAutoAdjustPacketSize, m_pCam);

    if (VmbErrorSuccess != m_Controller->getTreeStatus())
    {
        onFeedLogger("ERROR: ControllerTree returned: "+QString::number(m_Controller->getTreeStatus()) + " " +  Helper::mapReturnCodeToString(m_Controller->getTreeStatus()));
    }

    m_Description->setLineWrapMode(QTextEdit::NoWrap);
    m_Description->setReadOnly(true);
    m_Description->setStyleSheet("font: 12px;\n" "font-family: Verdana;\n");

    QList<int> listSize;
    listSize << 5000;
    splitter->setChildrenCollapsible(false);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(verticalLayoutWidget);
    splitter->addWidget(m_Description);
    splitter->setSizes(listSize);

    /* Filter Pattern */
    QHBoxLayout *pattern_HLayout = new QHBoxLayout();
    m_FilterPatternLineEdit = new LineEditCompleter(this);
    m_FilterPatternLineEdit->setText(tr("Example: Gain|Width"));
    m_FilterPatternLineEdit->setToolTip(tr("To filter multiple features, e.g: Width|Gain|xyz|etc"));
    m_FilterPatternLineEdit->setCompleter(m_Controller->getListCompleter());
    m_FilterPatternLineEdit->setMinimumWidth(200);
    QLabel *filterPatternLabel = new QLabel(tr("Filter pattern:"));
    filterPatternLabel->setStyleSheet("font-weight: bold;");
    QPushButton *patternButton = new QPushButton(tr("Search"));

    pattern_HLayout->addWidget(filterPatternLabel);
    pattern_HLayout->addWidget(m_FilterPatternLineEdit);
    pattern_HLayout->addWidget(patternButton);
    verticalLayout3->addLayout(pattern_HLayout);

    connect(m_FilterPatternLineEdit, SIGNAL(returnPressed()), this, SLOT(textFilterChanged()));
    connect(patternButton, SIGNAL(clicked(bool)), this, SLOT(textFilterChanged()));

    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->addWidget(tabWidget);
    verticalLayout2->addWidget(splitter);
    verticalLayout3->addWidget(m_Controller);


    tabWidget->setStyleSheet("color: rgb(0, 0, 0);");

    // closed source injected

    if (loadPlugin())
    {
        FeaturePtrVector featVec;
        pCam->GetFeatures(featVec);
        QVector<FeaturePtr> tmpFeatures = QVector<FeaturePtr>::fromStdVector(featVec);
        QSharedPointer<QVector<FeaturePtr> > qFeatVec = QFeatureVectorPtr( new QVector<FeaturePtr>(tmpFeatures));
        for (int i = 0; i < m_TabPluginCount; i++)
        {
            QWidget *plugtabWidget = new QWidget();
            TabExtensionResult result = m_tabExtensionInterface[i]->get(qFeatVec, *plugtabWidget);
            if ( result == TER_OK)
            {
                m_tabExtensionInterface[i]->connectToResetFps(this, SLOT(onResetFPS()));
                m_tabExtensionInterface[i]->connectFromAcquire(this, SIGNAL(acquisitionRunning(bool)));
                tabWidget->addTab(plugtabWidget, plugtabWidget->accessibleName());
            }
            else
            {
                delete plugtabWidget;
            }
        }
    }

    tabWidget->addTab(widgetTree, tr("All"));
    m_DockController->setWidget(dockWidgetContents);
    connect(tabWidget, SIGNAL(currentChanged(int)), m_Controller, SLOT(closeControls()));

    /* tooltip checkbox */
    m_ToolTipCheckBox     = new QCheckBox();
    m_ToolTipCheckBox->setText(tr("Tooltip ON"));
    m_ToolTipCheckBox->setChecked(true);
    verticalLayout3->addWidget(m_ToolTipCheckBox);
    connect( m_ToolTipCheckBox, SIGNAL(clicked(bool)), this, SLOT(onTooltipCheckBoxClick(bool)) );


    connect(m_DockController,  SIGNAL(topLevelChanged  (bool)), this, SLOT(onfloatingDockChanged(bool)));
    connect(m_DockInformation, SIGNAL(topLevelChanged  (bool)), this, SLOT(onfloatingDockChanged(bool)));
    connect(m_DockHistogram,   SIGNAL(topLevelChanged  (bool)), this, SLOT(onfloatingDockChanged(bool)));

    connect(m_DockController,  SIGNAL(visibilityChanged(bool)), this, SLOT(onVisibilityChanged(bool)));
    connect(m_DockInformation, SIGNAL(visibilityChanged(bool)), this, SLOT(onVisibilityChanged(bool)));
    connect(m_DockHistogram,   SIGNAL(visibilityChanged(bool)), this, SLOT(onVisibilityChanged(bool)));

    connect(m_Controller,      SIGNAL(setDescription(const QString &)),  this, SLOT(onSetDescription(const QString &)));
    connect(m_Controller,      SIGNAL(setEventMessage(const QStringList &)), this, SLOT(onSetEventMessage(const QStringList &)), Qt::QueuedConnection);
    connect(m_Controller,      SIGNAL(acquisitionStartStop(const QString &)), this, SLOT(onAcquisitionStartStop(const QString &))); // this eventually calls RegisterObserver in Frame.h which make sure  As new frames arrive, the observer's FrameReceived method will be called.  Only one observer can be registered.
    connect(m_Controller,      SIGNAL(updateBufferSize()), this, SLOT(onPrepareCapture()));
    connect(m_Controller,      SIGNAL(resetFPS()), this, SLOT(onResetFPS()));
    connect(m_Controller,      SIGNAL(logging(const QString &)), this, SLOT(onFeedLogger( const QString &)));
    connect(m_ScreenViewer,    SIGNAL(savingImage()), this, SLOT(on_ActionSaveAs_triggered()));
    connect(m_ScreenViewer,    SIGNAL(setColorInterpolationState(const bool &)), this, SLOT(onSetColorInterpolation(const bool &)));

    /* create FrameObserver to get frames from camera */
    SP_SET( m_pFrameObs, new FrameObserver(m_pCam) );

    connect(SP_ACCESS( m_pFrameObs ), SIGNAL(frameReadyFromObserver (QImage , const QString&, const QString&, const QString&)),
        this, SLOT(onimageReady(QImage , const QString&, const QString&, const QString&)));

    connect(SP_ACCESS( m_pFrameObs ), SIGNAL(frameReadyFromObserverFullBitDepth (tFrameInfo)),
        this, SLOT(onFullBitDepthImageReady(tFrameInfo)));

    connect(SP_ACCESS( m_pFrameObs ), SIGNAL(setCurrentFPS (const QString &)),
        this, SLOT(onSetCurrentFPS(const QString &)));

    connect(SP_ACCESS( m_pFrameObs ), SIGNAL(setFrameCounter (const unsigned int &)),
        this, SLOT(onSetFrameCounter(const unsigned int &)));

    /* HISTOGRAM: We need to register QVector<QVector <quint32> > because it is not known to Qt's meta-object system */
    qRegisterMetaType< QVector<QVector <quint32> > >("QVector<QVector <quint32> >");
    qRegisterMetaType< QVector <QStringList> >("QVector <QStringList>");

    connect(m_pFrameObs.get(), SIGNAL(histogramDataFromObserver ( const QVector<QVector <quint32> > &, const QString &, const double &, const double &, const QVector <QStringList>& )),
        this, SLOT(onSetHistogramData( const QVector<QVector <quint32> > &, const QString &, const double &, const double & , const QVector <QStringList>&)));

    m_DockInformation->setWidget(m_InformationWindow);
    int openElapsedTime = openTimer.elapsed();
    if( openElapsedTime <= 2500 )
    {
#ifdef WIN32
        Sleep(2500-openElapsedTime);
#else
        usleep( (2500-openElapsedTime) * 1000 );
#endif
    }

    /* use default setting position and geometry */
    QSettings settings("Allied Vision", "Vimba Viewer");
    this->restoreGeometry(settings.value("geometry").toByteArray());
    this->restoreState( settings.value("state").toByteArray(), 0 );
    this->show();

    (!m_DockController->isFloating() && !m_DockInformation->isFloating() && m_DockController->isVisible() && m_DockInformation->isVisible()) ? ActionResetPosition->setEnabled(false) : ActionResetPosition->setEnabled(true);

    if( VmbErrorSuccess != m_Controller->getTreeStatus())
    {
        onFeedLogger("ERROR: GetFeatures returned: "+QString::number(m_Controller->getTreeStatus()) + " " +  Helper::mapReturnCodeToString(m_Controller->getTreeStatus()));
    }

    /* Histogram */
    m_HistogramWindow = new HistogramWindow(this);
    m_DockHistogram->setWidget(m_HistogramWindow);
    m_HistogramWindow->createGraphWidgets();

    m_DockHistogram->isVisible() ? ActionHistogram->setChecked(true) : ActionHistogram->setChecked(false);

    /* Statusbar */
    m_OperatingStatusLabel = new QLabel(" Ready ");
    m_FormatLabel          = new QLabel;
    m_ImageSizeLabel       = new QLabel;
    m_FramesLabel          = new QLabel;
    m_FramerateLabel       = new QLabel;

    statusbar->addWidget(m_OperatingStatusLabel);
    statusbar->addWidget(m_ImageSizeLabel);
    statusbar->addWidget(m_FormatLabel);
    statusbar->addWidget(m_FramesLabel);
    statusbar->addWidget(m_FramerateLabel);

    m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,0, 0); color: rgb(255,255,255)");

    m_TextItem =  new QGraphicsTextItem;
    QFont serifFont("Arial", 12, QFont::Bold);
    m_TextItem->setFont(serifFont);
    m_TextItem->setDefaultTextColor(Qt::red);

    /*Save Images Option */
    m_ImageOptionDialog = new QDialog(this, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);
    m_SaveImageOption.setupUi(m_ImageOptionDialog);
    if(!settings.value(m_sCameraID).toString().isEmpty())
        m_SaveImageOption.ImageDestination_Edit->setText (settings.value(m_sCameraID).toString());
    else
    {
        m_SaveImageOption.ImageDestination_Edit->setText( QString(
#ifdef WIN32
            "C:\\"
#else
            "/home/"
#endif
            ));
    }


    connect(m_SaveImageOption.ImageDestinationButton, SIGNAL(clicked()),this, SLOT(getSaveDestinationPath()));
    connect(m_ImageOptionDialog, SIGNAL(accepted()),this, SLOT(acceptSaveImagesDlg()));
    connect(m_ImageOptionDialog, SIGNAL(rejected()),this, SLOT(rejectSaveImagesDlg()));

    if(!settings.value(m_sCameraID+"SaveImageName").toString().isEmpty())
        m_SaveImageOption.ImageName_Edit->setText (settings.value(m_sCameraID+"SaveImageName").toString());
    else
        m_SaveImageOption.ImageName_Edit->setText("VimbaImage");

    /* Direct Access */
    m_AccessDialog = new QDialog(this, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);
    m_DirectAccess.setupUi(m_AccessDialog);
    m_DirectAccess.RegAdd_Edit->setMaxLength(8);
    m_DirectAccess.RegData_Edit->setMaxLength(8);
    m_DirectAccess.RegAdd_Edit->setText("FFFFFFFC");
    m_DirectAccess.RegData_Edit->setText("00000000");
    m_DirectAccess.RegDataDec_Edit->setText("0");
    VmbInterfaceType InterfaceType = VmbInterfaceUnknown;
    m_DirectAccess.CheckBoxEndianess->setVisible( VmbErrorSuccess == m_pCam->GetInterfaceType(InterfaceType) );
    m_DirectAccess.CheckBoxEndianess->setChecked( VmbInterfaceUsb == InterfaceType );
    m_DirectAccess.CheckBoxEndianess->setToolTip( m_DirectAccess.CheckBoxEndianess->toolTip() + " (your device was detected to be " + (VmbInterfaceUsb == InterfaceType ? "little" : "big") + " endian)");

    m_AccessDialog->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint);

    QRegExp rxHex("[0-9A-Fa-f]{1,8}");
    m_DirectAccess.RegAdd_Edit->setValidator(new QRegExpValidator(rxHex, m_DirectAccess.RegAdd_Edit));
    m_DirectAccess.RegData_Edit->setValidator(new QRegExpValidator(rxHex, m_DirectAccess.RegData_Edit));

    QRegExp rxDec("[0-9]{1,10}");
    m_DirectAccess.RegDataDec_Edit->setValidator(new QRegExpValidator(rxDec, m_DirectAccess.RegDataDec_Edit));

    connect(m_DirectAccess.RegWrite_Button  , SIGNAL(clicked())                     , this, SLOT(writeRegisterData()));
    connect(m_DirectAccess.RegRead_Button   , SIGNAL(clicked())                     , this, SLOT(readRegisterData()));
    connect(m_DirectAccess.RegData_Edit     , SIGNAL(textChanged(const QString&))   , this, SLOT(directAccessHexTextChanged(const QString&)));
    connect(m_DirectAccess.RegDataDec_Edit  , SIGNAL(textChanged(const QString&))   , this, SLOT(directAccessDecTextChanged(const QString&)));
    connect(m_DirectAccess.CheckBoxEndianess, SIGNAL(stateChanged(int))             , this, SLOT(endianessChanged(int)));

    /*Viewer Option */
    m_ViewerOptionDialog = new QDialog(this, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);
    m_ViewerOption.setupUi(m_ViewerOptionDialog);
    m_ViewerOption.lineEdit_FramesCount->setText(QString::number( m_FrameBufferCount ));
    connect( m_ViewerOption.lineEdit_FramesCount, SIGNAL(textChanged(const QString&)),this, SLOT(optionsFrameCountChanged(const QString&)));
    connect( m_ViewerOption.DisplayInterval_CheckBox, SIGNAL(clicked(bool)), this, SLOT(displayEveryFrameClick(bool)));
    connect( m_ViewerOption.buttonBox, SIGNAL(accepted()),this, SLOT(optionsAccepted()));

    //connect(parent, SIGNAL(closeViewer()), this, SLOT(close()));


    if (ActionHistogram->isChecked())
    {
        m_HistogramWindow->initializeStatistic();
        m_pFrameObs->enableHistogram(true);
    }
    else
        m_pFrameObs->enableHistogram(false);

    m_Timer = new QTimer(this);
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(updateColorInterpolationState()));

    /* enable/disable menu */
    connect(m_Controller, SIGNAL(enableViewerMenu(bool)), this, SLOT(enableMenuAndToolbar(bool)));

    /* saving images */
    m_SaveImageThread = QSharedPointer<SaveImageThread>(new SaveImageThread());
    connect( m_SaveImageThread.data(), SIGNAL(setPosition(unsigned int)), this, SLOT(onSaving(unsigned int)));
    connect( m_SaveImageThread.data(), SIGNAL( LogMessage(const QString&)), this, SLOT(onFeedLogger(const QString&)) );

    if (m_TabPluginCount == 0) // All Tab is visible set focus to "search"
    {
        m_FilterPatternLineEdit->setFocus();
    }

    m_FilterPatternLineEdit->selectAll();

#ifdef WIN32
    m_SelectedExtension = ".bmp";
#else
    m_SelectedExtension = ".png";
#endif

    // test if the LibTiff library is available on this system
    m_LibTiffAvailable = m_TiffWriter.IsAvailable();
    ActionAllow16BitTiffSaving->setEnabled(m_LibTiffAvailable);
}

ViewerWindow::~ViewerWindow()
{
    /* save setting position and geometry from last session */
    QSettings settings("Allied Vision", "Vimba Viewer");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState(0));

    /* If cam is open */
    if(!m_sCameraID.isEmpty())
    {
        settings.setValue(m_sCameraID,  m_SaveImageOption.ImageDestination_Edit->text());
        if(!m_SaveName.isEmpty())
            settings.setValue(m_sCameraID +"SaveImageName", m_SaveName);

        if( NULL != m_saveFileDialog )
        {
            delete m_saveFileDialog;
            m_saveFileDialog = NULL;
        }

        releaseBuffer();

        m_pCam->Close();
    }
}

void ViewerWindow::textFilterChanged()
{
    
    QRegExp::PatternSyntax syntax = QRegExp::PatternSyntax(0);
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;

    QRegExp regExp(m_FilterPatternLineEdit->text(), caseSensitivity, syntax);
    m_Controller->m_ProxyModel->setFilterRegExp(regExp);
    m_Controller->expandAll();
    m_Controller->updateUnRegisterFeature();
    m_Controller->updateRegisterFeature();
    m_FilterPatternLineEdit->setFocus();
    m_FilterPatternLineEdit->selectAll();
}

void ViewerWindow::enableMenuAndToolbar ( bool bValue )
{
    menuBarMainWindow->setEnabled(bValue);
    toolBar->setEnabled(bValue);
}

void ViewerWindow::onTooltipCheckBoxClick ( bool bValue )
{
    m_Controller->showTooltip ( bValue );
}

void ViewerWindow::updateColorInterpolationState()
{
    /* update interpolation state after start */
    m_ScreenViewer->updateInterpolationState(m_pFrameObs->getColorInterpolation());
    m_Timer->stop();
}

void ViewerWindow::onSetColorInterpolation ( const bool &bState )
{
    m_pFrameObs->setColorInterpolation(bState);
}

bool ViewerWindow::getCamOpenStatus() const
{
    return m_bIsCamOpen;
}
CameraPtr ViewerWindow::getCameraPtr()
{
    return m_pCam;
}
bool ViewerWindow::isControlledCamera( const CameraPtr &cam) const
{
    return SP_ACCESS( cam) == SP_ACCESS( m_pCam );
}


bool  ViewerWindow::getAdjustPacketSizeMessage ( QString &sMessage )
{
    if(m_Controller->isGigE())
    {
        if(VmbErrorSuccess == m_Controller->getTreeStatus())
        {
            sMessage = "Packet Size Adjusted:\t";
        }
        else
        {
            sMessage = "Failed To Adjust Packet Size!";
            sMessage.append(" Reason: " + Helper::mapReturnCodeToString(m_Controller->getTreeStatus()));
        }

        return true;
    }

    return false;
}

VmbError_t ViewerWindow::getOpenError() const
{
    return m_OpenError;
}

QString ViewerWindow::getCameraID() const
{
    return m_sCameraID;
}

void ViewerWindow::closeEvent ( QCloseEvent *event )
{
    if (m_bIsCameraRunning)
    {
        m_bIsViewerWindowClosing = true;
        onAcquisitionStartStop("AcquisitionStopFreerun");
    }
    emit closeViewer( m_pCam );
}

void ViewerWindow::displayEveryFrameClick ( bool bValue )
{
    m_ViewerOption.Note_Label->setEnabled(bValue);
    m_bIsDisplayEveryFrame = bValue;
}

/* Viewer Option */
void ViewerWindow::on_ActionDisplayOptions_triggered()
{
    m_ViewerOption.lineEdit_FramesCount->setText(QString::number ( m_FrameBufferCount ));
    m_ViewerOptionDialog->exec();
}

void ViewerWindow::on_ActionResetPosition_triggered()
{
    m_DockController ->setFloating(false);
    m_DockInformation->setFloating(false);
    m_DockHistogram  ->setFloating(false);
    m_DockController ->show();
    m_DockInformation->show();
    m_DockHistogram  ->hide();
    ActionHistogram  ->setChecked(false);
    ActionResetPosition->setEnabled(false);
}

/* Direct Register Access */
void ViewerWindow::on_ActionRegister_triggered()
{
    m_DirectAccess.RegAccessError_Label->clear();
    m_DirectAccess.RegAccessError_Label->setStyleSheet("");
    m_DirectAccess.RegAdd_Edit->setFocus();
    m_DirectAccess.RegAdd_Edit->selectAll();
    m_AccessDialog->show();
}

void ViewerWindow::optionsFrameCountChanged ( const QString &sText )
{
    bool bOk;
    unsigned int frames = sText.toUInt(&bOk);

    if(!bOk)
    {
        m_ViewerOption.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }
    if(  (0 != QString::number(frames).compare(m_ViewerOption.lineEdit_FramesCount->text()))
       ||(frames < 2))
    {
        m_ViewerOption.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }
    m_ViewerOption.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void ViewerWindow::optionsAccepted()
{
    bool bOk;

    unsigned int frames = m_ViewerOption.lineEdit_FramesCount->text().toUInt(&bOk);

    if (!bOk)
    {
        return;
    }
    m_FrameBufferCount = frames;
}


void ViewerWindow::directAccessHexTextChanged ( const QString &sText )
{
    bool bOk;
    unsigned int sDec = sText.toUInt(&bOk, 16);
    endianessConvert( sDec );
    if(0 == QString::number(sDec).compare(m_DirectAccess.RegDataDec_Edit->text()))
        return;
    else
        m_DirectAccess.RegDataDec_Edit->setText(QString::number(sDec));
}

void ViewerWindow::directAccessDecTextChanged ( const QString &sText )
{
    unsigned int lDecValue = sText.toUInt();
    endianessConvert( lDecValue );
    QString sHex;
    sHex.setNum(lDecValue,16);

    if(0 == sHex.compare(m_DirectAccess.RegData_Edit->text()))
        return;
    else
        m_DirectAccess.RegData_Edit->setText(sHex);
}

void ViewerWindow::writeRegisterData()
{
    bool bOk;
    qlonglong lRegAddress = m_DirectAccess.RegAdd_Edit->text().toLongLong(&bOk, 16);
    qlonglong lRegData    = m_DirectAccess.RegData_Edit->text().toLongLong(&bOk, 16);
    m_DirectAccess.RegAccessError_Label->clear();
    m_DirectAccess.RegAccessError_Label->setStyleSheet("background-color: rgb(240,240,240); color: rgb(0,0,0)");

    m_DirectAccess.RegDataDec_Edit->setFocus();
    m_DirectAccess.RegDataDec_Edit->selectAll();

    std::vector<VmbUint64_t> address;
    address.push_back((VmbUint64_t)lRegAddress);
    std::vector<VmbUint64_t> data;
    data.push_back((VmbUint64_t)lRegData);
    VmbError_t errorType = m_pCam->WriteRegisters(address, data);
    if( VmbErrorSuccess != errorType )
    {
        m_DirectAccess.RegAccessError_Label->setStyleSheet("background-color: rgb(0,0,0); color: rgb(255,0,0)");
        m_DirectAccess.RegAccessError_Label->setText(" "+tr("Write Register Failed!")+" <"+tr("Error")+": " + QString::number(errorType) + ">");
    }
}

void ViewerWindow::readRegisterData()
{
    bool bOk;
    qlonglong lRegAddress = m_DirectAccess.RegAdd_Edit->text().toLongLong(&bOk, 16);

    m_DirectAccess.RegAccessError_Label->clear();
    m_DirectAccess.RegAccessError_Label->setStyleSheet("background-color: rgb(240,240,240); color: rgb(0,0,0)");

    std::vector<VmbUint64_t> address;
    address.push_back((VmbUint64_t)lRegAddress);

    std::vector<VmbUint64_t> data;
    data.resize(1);
    VmbError_t errorType = m_pCam->ReadRegisters(address, data);
    if( VmbErrorSuccess != errorType )
    {
        m_DirectAccess.RegAccessError_Label->setStyleSheet("background-color: rgb(0,0,0); color: rgb(255,0,0)");
        m_DirectAccess.RegAccessError_Label->setText(" "+tr("Read Register Failed!")+" <"+tr("Error:")+" " + QString::number(errorType) + ">");
        return;
    }
    QString sData = QString("%1").arg(data[0], 8, 16, QLatin1Char('0'));
    m_DirectAccess.RegData_Edit->setText(sData);
}

void ViewerWindow::endianessChanged(int index)
{
    emit directAccessDecTextChanged( m_DirectAccess.RegDataDec_Edit->text() );
}
template <typename T>
void ViewerWindow::endianessConvert(T &v)
{
    if( Qt::Checked == m_DirectAccess.CheckBoxEndianess->checkState() )
    {
        v = qFromBigEndian( v );
    }
}
/* Saving an image */
void ViewerWindow::on_ActionSaveAs_triggered()
{
    // make a copy of the images before save-as dialog appears (image can change during time dialog open)
    QImage      image = m_PixmapItem->pixmap().toImage();
    tFrameInfo  imageFullBitdepth = m_FullBitDepthImage;
    QString     fileExtension;
    bool        isImageAvailable = true;

    if ( image.isNull() )
    {
        isImageAvailable = false;
    }
    else
    {
        /* Get all inputformats */
        unsigned int nFilterSize = QImageReader::supportedImageFormats().count();
        for (int i = nFilterSize-1; i >= 0; i--)
        {
            fileExtension += "."; /* Insert wildcard */
            fileExtension += QString(QImageReader::supportedImageFormats().at(i)).toLower(); /* Insert the format */
            if(0 != i)
                fileExtension += ";;"; /* Insert a space */
        }

         if( NULL != m_saveFileDialog )
         {
             delete m_saveFileDialog;
             m_saveFileDialog = NULL;
         }

        m_saveFileDialog = new QFileDialog ( this, tr("Save Image"), m_SaveFileDir, fileExtension );
        m_saveFileDialog->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);
        m_saveFileDialog->selectNameFilter(m_SelectedExtension);
        m_saveFileDialog->setAcceptMode(QFileDialog::AcceptSave);

        if(m_saveFileDialog->exec())
        {   //OK
           m_SelectedExtension = m_saveFileDialog->selectedNameFilter();
           m_SaveFileDir = m_saveFileDialog->directory().absolutePath();
           QStringList files = m_saveFileDialog->selectedFiles();

           if(!files.isEmpty())
           {
                QString fileName = files.at(0);

                if(!fileName.endsWith(m_SelectedExtension))
                {
                    fileName.append(m_SelectedExtension);
                }

                bool saved = false;

                // save image using LibTiff library for 16 Bit
                if( m_LibTiffAvailable && ActionAllow16BitTiffSaving->isChecked() && m_SelectedExtension.contains(".tif") && isSupportedPixelFormat())
                {
                    saved = m_TiffWriter.WriteTiff(imageFullBitdepth, fileName.toUtf8());
                }
                // use default QImage save functionality
                else
                {
                    if( true == CanReduceBpp() )
                    {
                        saved = ReduceBpp( image ).save( fileName );
                    }
                    else
                    {
                        saved = image.save( fileName );
                    }
                }

                if ( true == saved )
                {
                    QMessageBox::information( this, tr( "Vimba Viewer" ), tr( "Image: " ) + fileName + tr( " saved successfully" ));
                }
                else
                {
                    QMessageBox::warning( this, tr( "Vimba Viewer" ), tr( "Error saving image" ));
                }

            }
        }
    }

    if ( !isImageAvailable )
    {
        QMessageBox::warning( this, tr( "Vimba Viewer" ), tr( "No image to save" ));
    }
}

/* Saving multiple images */
void ViewerWindow::on_ActionSaveOptions_triggered()
{
    QStringList sListFormat;

    for (int i = 0; i < QImageReader::supportedImageFormats().count(); i++)
    {
        QString sTemp;
        sTemp.append("."); /* Insert wildcard */
        sTemp.append(QString(QImageReader::supportedImageFormats().at(i)).toLower()); /* Insert the format */
        sListFormat << sTemp;
    }

    sListFormat << ".bin";

    m_SaveImageOption.ImageFormat_ComboBox->clear();
    m_SaveImageOption.ImageFormat_ComboBox->addItems(sListFormat);

    // restore previously selected format (if possible)
    if ( false == m_SaveFormat.isEmpty() )
    {
        int index = m_SaveImageOption.ImageFormat_ComboBox->findText(m_SaveFormat);
        if( -1 != index )
        {
            m_SaveImageOption.ImageFormat_ComboBox->setCurrentIndex(index);
        }
    }

    m_ImageOptionDialog->setModal(true);
    m_ImageOptionDialog->show();
}

void ViewerWindow::acceptSaveImagesDlg()
{
    if( !m_SaveImageOption.ImageDestination_Edit->text().isEmpty() &&
        !m_SaveImageOption.ImageName_Edit->text().isEmpty() )
    {
        m_ImagePath =  m_SaveImageOption.ImageDestination_Edit->text();
    }
    else
    {
        if(m_SaveImageOption.ImageDestination_Edit->text().isEmpty())
            QMessageBox::warning( this, tr("Vimba Viewer"), "<Save Image Options> "+tr("Please choose your destination path!") );

        if(m_SaveImageOption.ImageName_Edit->text().isEmpty())
            QMessageBox::warning( this, tr("Vimba Viewer"), "<Save Image Options> "+tr("Please give a name!") );

        m_ImageOptionDialog->setModal(true);
        m_ImageOptionDialog->show();
    }

    /* name check existing files */
    QDir destDir( m_SaveImageOption.ImageDestination_Edit->text());
    QStringList filter;

    for (int i = 0; i < QImageReader::supportedImageFormats().count(); i++)
    {
        QString sTemp;
        sTemp.append("*."); /* Insert wildcard */
        sTemp.append(QString(QImageReader::supportedImageFormats().at(i)).toLower()); /* Insert the format */
        filter << sTemp;
    }

    destDir.setNameFilters(filter);
    QStringList files = destDir.entryList();

    bool bRes = true;
    while(bRes)
    {
        bRes = checkUsedName(files);
    }

    if(0 < m_SaveImageOption.NumberOfFrames_SpinBox->value())
        ActionSaveImages->setEnabled(true);
    else
        ActionSaveImages->setEnabled(false);

    m_SaveFormat = m_SaveImageOption.ImageFormat_ComboBox->currentText();
}

void ViewerWindow::rejectSaveImagesDlg()
{
    QString sPathBefore = m_SaveImageOption.ImageDestination_Edit->text();
    m_SaveImageOption.ImageDestination_Edit->setText(sPathBefore);
}

bool ViewerWindow::checkUsedName ( const QStringList &files )
{
    for( int nPos = 0; nPos < files.size(); nPos++ )
    {
        if(0 == files.at(nPos).compare(m_SaveImageOption.ImageName_Edit->text()+"_1"+m_SaveImageOption.ImageFormat_ComboBox->currentText()))
        {
            m_SaveImageOption.ImageName_Edit->setText(m_SaveImageOption.ImageName_Edit->text()+"_New");
            return true;
        }
    }

    return false;
}

bool ViewerWindow::isDestPathWritable()
{
    /* check permission of destination
       QFileInfo can not be used, because a windows dir can be write protected, but files inside can be created
    */
    QString sTmpPath = m_SaveImageOption.ImageDestination_Edit->text().append("/").append("fileTmp.dat");
    QFile fileTmp(sTmpPath);

    if(!fileTmp.open( QIODevice::ReadWrite|QIODevice::Text))
    {
        QMessageBox::warning( this, tr("Vimba Viewer"), "<Save Image Options> "+tr("No permission to write to destination path. Please select another one! ") );
        return false;
    }

    fileTmp.close();
    QFile::remove(sTmpPath);
    return true;
}

/* confirm saving from toolbar */
void ViewerWindow::on_ActionSaveImages_triggered()
{

#ifdef _WIN32 // Do the check always on Windows
    if (!isDestPathWritable())
    {
        ActionSaveImages->setEnabled(false);
        return;
    }
#else
    #ifndef WIN32
        if (!isDestPathWritable())
        {
            ActionSaveImages->setEnabled(false);
            return;
        }
    #endif
#endif

    //overwrite existing name
    if(0 != m_SaveImageOption.ImageName_Edit->text().compare(m_SaveImageName))
    {
        m_SaveImageName = m_SaveImageOption.ImageName_Edit->text();
    }

    m_nImageCounter         = 0;
    m_nNumberOfFramesToSave = m_SaveImageOption.NumberOfFrames_SpinBox->value();
    m_SaveFormat            = m_SaveImageOption.ImageFormat_ComboBox->currentText();
    m_SaveName              = m_SaveImageOption.ImageName_Edit->text();

    m_SaveImageThread->SetNumberToSave  ( m_nNumberOfFramesToSave );
    m_SaveImageThread->SetSaveFormat    ( m_SaveFormat );
    m_SaveImageThread->SetPath          ( m_ImagePath );
    m_SaveImageThread->SetBaseName      ( m_SaveName );

    if( 0 < m_nNumberOfFramesToSave )
    {
        if( 0 == m_SaveImageOption.ImageFormat_ComboBox->currentText().compare(".bin"))
            m_pFrameObs->saveRawData(m_nNumberOfFramesToSave, m_ImagePath, m_SaveName);

        // save full bit depth series of images when requested and LibTif is available
        if( m_LibTiffAvailable && ActionAllow16BitTiffSaving->isChecked() && m_SaveFormat.contains(".tif") && isSupportedPixelFormat())
        {
            m_Allow16BitMultiSave = true;
            m_SaveImageThread->SetSave16Bit ( true );
            // start transferring full bit depth frames from frame observer
            m_pFrameObs->enableFullBitDepthTransfer(true);
        }
        else
        {
            m_Allow16BitMultiSave = false;
            m_SaveImageThread->SetSave16Bit ( false );
            m_bIsTriggeredByMultiSaveBtn = true;
            m_pFrameObs->enableFullBitDepthTransfer(false);
        }

        ActionFreerun->setChecked(isStreamingAvailable());
        on_ActionFreerun_triggered();
        ActionFreerun->setEnabled(false);
    }
}

void ViewerWindow::getSaveDestinationPath()
{
    if( NULL != m_getDirDialog )
    {
        delete m_getDirDialog;
        m_getDirDialog = NULL;
    }

    m_getDirDialog = new QFileDialog ( this, tr("Destination"), m_SaveImageOption.ImageDestination_Edit->text());
    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly |QFileDialog::DontUseNativeDialog;
    m_getDirDialog->setOptions(options);
    m_getDirDialog->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);
    m_getDirDialog->setLabelText(QFileDialog::LookIn, tr("Destination Path"));
    m_getDirDialog->setLabelText(QFileDialog::FileType, tr("Type"));
    m_getDirDialog->setFileMode( QFileDialog::Directory);

    QString sDir;
    if( m_getDirDialog->exec() )
    {   //OK
        sDir = m_getDirDialog->directory().absolutePath();
    }

    if(!sDir.isEmpty())
        m_SaveImageOption.ImageDestination_Edit->setText(sDir);
}

void ViewerWindow::on_ActionLeftRotation_triggered()
{
    m_ScreenViewer->rotate(-90);
}

void ViewerWindow::on_ActionRightRotation_triggered()
{
    m_ScreenViewer->rotate(90);
}

void ViewerWindow::on_ActionZoomOut_triggered()
{
    m_ScreenViewer->zoomOut();
}

void ViewerWindow::on_ActionZoomIn_triggered()
{
    m_ScreenViewer->zoomIn();
}

void ViewerWindow::on_ActionOriginalSize_triggered()
{
    m_ScreenViewer->setDefaultSize();
}

void ViewerWindow::on_ActionFitToWindow_triggered()
{
    m_ScreenViewer->setDefaultSize();

    if (ActionFitToWindow->isChecked())
    {
        m_ScreenViewer->setToolTip(tr(""));
        m_ScreenViewer->fitInView(m_pScene->itemsBoundingRect(), Qt::IgnoreAspectRatio);
        m_ScreenViewer->enableFitToWindow(true);
        ActionLeftRotation->setEnabled(false);
        ActionRightRotation->setEnabled(false);
        ActionOriginalSize->setEnabled(false);
        ActionZoomIn->setEnabled(false);
        ActionZoomOut->setEnabled(false);
    }
    else
    {
        m_ScreenViewer->enableFitToWindow(false);
        ActionLeftRotation->setEnabled(true);
        ActionRightRotation->setEnabled(true);
        ActionOriginalSize->setEnabled(true);
        ActionZoomIn->setEnabled(true);
        ActionZoomOut->setEnabled(true);
    }
}

void ViewerWindow::on_ActionHistogram_triggered()
{
    m_HistogramWindow->deinitializeStatistic();

    if (ActionHistogram->isChecked())
    {
        m_HistogramWindow->initializeStatistic();
        m_DockHistogram->show();
        m_pFrameObs->enableHistogram(true);
    }
    else
    {
        m_DockHistogram->hide();
        m_pFrameObs->enableHistogram(false);
    }

    if(!ActionHistogram->isChecked() && m_DockController->isVisible() && m_DockInformation->isVisible())
        ActionResetPosition->setEnabled(false);
}

void ViewerWindow::on_ActionLoadCameraSettings_triggered()
{
    bool proceedLoading = true;

//  create window title
    QString windowTitle = tr( "Load Camera Settings" );

//  setup message boxes
    QMessageBox msgbox;
    msgbox.setWindowTitle( windowTitle );
    QMessageBox msgbox2;
    msgbox2.setStandardButtons( QMessageBox::Yes );
    msgbox2.addButton( QMessageBox::No );
    msgbox2.setDefaultButton( QMessageBox::No );

//  check if camera was opened in 'full access' mode
    if( 0 != m_sAccessMode.compare( tr("(FULL ACCESS)") ) )
    {
        msgbox.setIcon( QMessageBox::Critical );
        msgbox.setText( tr("Camera must be opened in FULL ACCESS mode to use this feature") );
        msgbox.exec();
        return;
    }

//  check if any file dialog was created already
    if( NULL != m_saveFileDialog )
    {
        delete m_saveFileDialog;
        m_saveFileDialog = NULL;
     }

//  create file dialog
    m_saveFileDialog = new QFileDialog( this, windowTitle, QDir::home().absolutePath(), "*.xml" );
    m_saveFileDialog->setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint );
    m_saveFileDialog->selectNameFilter( "*.xml" );
    m_saveFileDialog->setAcceptMode( QFileDialog::AcceptOpen );
    m_saveFileDialog->setFileMode( QFileDialog::ExistingFile );

//  show dialog
    int rval = m_saveFileDialog->exec();
    if( 0 == rval )
    {
        return;
    }

//  get selected file
    m_SaveFileDir = m_saveFileDialog->directory().absolutePath();
    QStringList selectedFiles = m_saveFileDialog->selectedFiles();
    if( true == selectedFiles.isEmpty() )
    {
        msgbox.setIcon( QMessageBox::Critical );
        msgbox.setText( tr("No file selected") );
        msgbox.exec();
        return;
    }

//  delete file dialog
//  (to prevent OCT-1870 bug occured with Qt v4.7.1)
    delete m_saveFileDialog;
    m_saveFileDialog = NULL;

//  get selected file
    QString selectedFile = selectedFiles.at(0);

//  check if xml file is valid
    if( false == selectedFile.endsWith( ".xml" ) )
    {
        msgbox.setIcon( QMessageBox::Critical );
        msgbox.setText( tr("Invalid xml file selected.\nFile must be of type '*.xml'") );
        msgbox.exec();
        return;
    }

//  create and prepare xml parser
//  to check if model name differences between xxml file
//  and connected camera exist
    QXmlStreamReader xml;
    QFile xmlFile( selectedFile );
    QString deviceModel = QString( "" );

//  open xml file stream
    bool check = xmlFile.open( QIODevice::ReadOnly );
    if( false == check )
    {
        msgbox2.setIcon( QMessageBox::Warning );
        msgbox2.setText( tr("Could not validate camera model.\nDo you want to proceed loading settings to selected camera ?") );
        rval = msgbox2.exec();
        if( QMessageBox::No == rval )
        {
            proceedLoading = false;
        }
    }
    else
    {
    //  connect opened file with xml stream object
        xml.setDevice( &xmlFile );
    }

//  proceed loading camera settings only if flag still true
    if( true == proceedLoading )
    {
    //  read xml structure
        while( false == xml.atEnd() )
        {
        //  get current xml token
            xml.readNext();
            QString currentToken = xml.name().toString();

        //  check if token is named 'CameraSettings'
            if( 0 == currentToken.compare( "CameraSettings" ) )
            {
            //  get token attributes and iterate through them
                QXmlStreamAttributes attributes = xml.attributes();
                for( int i=0; i<attributes.count(); ++i )
                {
                //  get current attribute
                    QXmlStreamAttribute currentAttribute = attributes.at(i);

                //  check if current attribute is name 'CameraModel'
                    QString attributeName = currentAttribute.name().toString();
                    if( 0 == attributeName.compare( "CameraModel" ) )
                    {
                        deviceModel = currentAttribute.value().toString();
                        break;
                    }
                }
                break;
            }
        }

    //  close xml file stream
        xmlFile.close();

    //  check if deviceModel was retrieved from xml file
        if( true == deviceModel.isEmpty() )
        {
            msgbox2.setIcon( QMessageBox::Warning );
            msgbox2.setText( tr("Could not validate camera model.\nDo you want to proceed loading settings to selected camera ?") );
            rval = msgbox2.exec();
            if( QMessageBox::No == rval )
            {
                proceedLoading = false;
            }
        }
    }

//  proceed loading camera settings only if flag still true
    std::string modelName;
    if( true == proceedLoading )
    {
    //  get model name from connected camera
        const VmbErrorType err = m_pCam->GetModel( modelName );
        if( VmbErrorSuccess != err )
        {
            msgbox2.setIcon( QMessageBox::Warning );
            msgbox2.setText( tr("Could not validate camera model.\nDo you want to proceed loading settings to selected camera ?") );
            rval = msgbox2.exec();
            if( QMessageBox::No == rval )
            {
                proceedLoading = false;
            }
        }
    }

//  proceed loading camera settings only if flag still true
    if( true == proceedLoading )
    {
    //  compare mode names from xml file and from
    //  connected device with each other
        if( 0 != deviceModel.compare( QString(modelName.c_str()) ) )
        {
            QString msgtext = tr( "Selected camera model is different from xml file.\n");
            msgtext.append( tr("[camera: %1]\n").arg( modelName.c_str() ) );
            msgtext.append( tr("[xml: %1]\n\n").arg( deviceModel) );
            msgtext.append( tr("Do you want to proceed loading operation ?" ) );
            msgbox2.setIcon( QMessageBox::Warning );
            msgbox2.setText( msgtext );
            rval = msgbox2.exec();
            if( QMessageBox::No == rval )
            {
                proceedLoading = false;
            }
        }
    }

//  proceed loading camera settings only if flag still true
    if( true == proceedLoading )
    {
    //  setup behaviour for loading and saving camera features
        m_pCam->LoadSaveSettingsSetup( VmbFeaturePersistNoLUT, 5, 4 );

    //  call load method from VimbaCPP
        const VmbErrorType err = m_pCam->LoadCameraSettings( selectedFile.toStdString() );
        if( VmbErrorSuccess != err )
        {
            QString msgtext = tr( "There have been errors during loading of feature values.\n" );
            msgtext.append( tr("[Error code: %1]\n").arg( err ) );
            msgtext.append( tr("[file: %1]").arg( selectedFile) );
            onFeedLogger( "ERROR: LoadCameraSettings returned: " + QString::number(err) + ". For details activate VimbaC logging and check out VmbCameraSettingsLoad.log" );
            msgbox.setIcon( QMessageBox::Warning );
            msgbox.setText( msgtext );
            msgbox.exec();
            return;
        }
        else
        {
            msgbox.setIcon( QMessageBox::Information );
            QString msgtext = tr( "Successfully loaded device settings\nfrom '%1'" ).arg( selectedFile );
            msgbox.setText( msgtext );
            msgbox.exec();
        }
    }
}

void ViewerWindow::on_ActionSaveCameraSettings_triggered()
{
    VmbErrorType err = VmbErrorSuccess;

//  create window title
    QString windowTitle = tr( "Save Camera Settings" );

//  create message box
    QMessageBox msgbox;
    msgbox.setWindowTitle( windowTitle );

//  check if camera was opened in 'full access' mode
    if( 0 != m_sAccessMode.compare( tr("(FULL ACCESS)") ) )
    {
        msgbox.setIcon( QMessageBox::Critical );
        msgbox.setText( tr("Camera must be opened in FULL ACCESS mode to use this feature") );
        msgbox.exec();
        return;
    }

//  check if any file dialog was created already
    if( NULL != m_saveFileDialog )
    {
        delete m_saveFileDialog;
        m_saveFileDialog = NULL;
     }

//  setup file dialog
    m_saveFileDialog = new QFileDialog( this, windowTitle, QDir::home().absolutePath(), "*.xml" );
    m_saveFileDialog->setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint );
    m_saveFileDialog->selectNameFilter( "*.xml" );
    m_saveFileDialog->setAcceptMode( QFileDialog::AcceptSave );

//  show dialog
    int rval = m_saveFileDialog->exec();
    if( 0 == rval )
    {
        return;
    }

//  get selected file
    m_SaveFileDir = m_saveFileDialog->directory().absolutePath();
    QStringList selectedFiles = m_saveFileDialog->selectedFiles();
    if( true == selectedFiles.isEmpty() )
    {
        msgbox.setIcon( QMessageBox::Critical );
        msgbox.setText( tr("No file selected") );
        msgbox.exec();
        return;
    }

//  delete file dialog
//  (to prevent OCT-1870 bug occured with Qt v4.7.1)
    delete m_saveFileDialog;
    m_saveFileDialog = NULL;

//  get selected file
    QString selectedFile = selectedFiles.at(0);
    if( false == selectedFile.endsWith( ".xml" ) )
    {
        selectedFile.append( ".xml" );
    }

//  setup behaviour for loading and saving camera features
    m_pCam->LoadSaveSettingsSetup( VmbFeaturePersistNoLUT, 5, 4 );

//  call VimbaCPP save function
    QString msgtext;
    err = m_pCam->SaveCameraSettings( selectedFile.toStdString() );
    if( VmbErrorSuccess != err )
    {
        msgtext = tr( "There have been errors during saving feature values.\n" );
        msgtext.append( tr("[Error code: %1]\n").arg( err ) );
        msgtext.append( tr("[file: %1]").arg( selectedFile) );
        onFeedLogger( "ERROR: SaveCameraSettings returned: " + QString::number(err) + ". For details activate VimbaC logging and check out VmbCameraSettingsSave.log" );
        msgbox.setIcon( QMessageBox::Warning );
        msgbox.setText( msgtext );
        msgbox.exec();
        return;
    }
    else
    {
        msgtext = tr( "Successfully saved device settings to\n'" );
        msgtext.append( selectedFile );
        msgtext.append( "'" );
        msgbox.setIcon( QMessageBox::Information );
        msgbox.setText( msgtext );
        msgbox.exec();
    }
}

void ViewerWindow::on_ActionLoadCameraSettingsMenu_triggered()
{
    on_ActionLoadCameraSettings_triggered();
}

void ViewerWindow::on_ActionSaveCameraSettingsMenu_triggered()
{
    on_ActionSaveCameraSettings_triggered();
}

void ViewerWindow::on_ActionAllow16BitTiffSaving_triggered()
{
    if ( ActionAllow16BitTiffSaving->isChecked() && m_LibTiffAvailable && isSupportedPixelFormat())
    {
        m_pFrameObs->enableFullBitDepthTransfer(true);
    }
    else
    {
        m_pFrameObs->enableFullBitDepthTransfer(false);
    }
}


void ViewerWindow::onfloatingDockChanged ( bool bIsFloating )
{
    if( (m_DockController->isFloating()  || (false == m_DockController->isVisible())) ||
        (m_DockInformation->isFloating() || (false == m_DockInformation->isVisible())) )
        ActionResetPosition->setEnabled(true);
    else
        ActionResetPosition->setEnabled(false);

    if(m_DockHistogram->isVisible())
        ActionHistogram->setChecked(true);
}

void ViewerWindow::onVisibilityChanged ( bool bIsVisible )
{
    if( m_DockController->isVisible()  && !m_DockController->isFloating() &&
        m_DockInformation->isVisible() && !m_DockInformation->isFloating() )
    {
        ActionResetPosition->setEnabled(false);
    }
    else
    {
        ActionResetPosition->setEnabled(true);
    }

    if(!m_DockHistogram->isVisible())
    {
        if (ActionHistogram->isChecked() )
        {
            ActionHistogram->setChecked(false);
        }
    }
    else
    {
        ActionHistogram->setChecked(true);
    }
}

void ViewerWindow::onResetFPS()
{
    SP_ACCESS( m_pFrameObs )->resetFrameCounter(false);
}

void ViewerWindow::onSetCurrentFPS( const QString &sFPS )
{
    m_FramerateLabel->setText( QString::fromStdString(" Current FPS: ")+ sFPS + " " );
}

void ViewerWindow::onSetFrameCounter( const unsigned int &nNumberOfFrames )
{
    m_FramesLabel->setText( "Frames: " + QString::number(nNumberOfFrames) + " " );
}

void ViewerWindow::onSetEventMessage ( const QStringList &sMsg )
{
    m_InformationWindow->setEventMessage(sMsg);
}

void ViewerWindow::onSetDescription ( const QString &sDesc )
{
    m_Description->setText(sDesc);
}

void ViewerWindow::onSetHistogramData ( const QVector<QVector <quint32> > &histData, const QString &sHistogramTitle,
                                        const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis, const QVector <QStringList> &statistics )
{
    if( ActionHistogram->isChecked() )
    {
        QStringList ColorComponentList;
        QStringList MinimumValueList;
        QStringList MaximumValueList;
        QStringList AverageValueList;
        QString sFormat;

        if(sHistogramTitle.contains("Mono8"))
        {
            sFormat = "Mono8";
            ColorComponentList << statistics.at(0).at(0);
            MinimumValueList << statistics.at(0).at(1);
            MaximumValueList << statistics.at(0).at(2);
            AverageValueList << statistics.at(0).at(3);
            m_HistogramWindow->setStatistic(ColorComponentList, MinimumValueList, MaximumValueList, AverageValueList, sFormat);
        }

        if(sHistogramTitle.contains("RGB8") || sHistogramTitle.contains("BGR8") || sHistogramTitle.contains("YUV") || sHistogramTitle.contains("Bayer"))
        {
            if(sHistogramTitle.contains("RGB8") || sHistogramTitle.contains("BGR8"))
                sFormat = "RGB";
            if(sHistogramTitle.contains("Bayer"))
                sFormat = "Bayer";
            if(sHistogramTitle.contains("YUV"))
                sFormat = "YUV";

            ColorComponentList << statistics.at(0).at(0) << statistics.at(1).at(0) << statistics.at(2).at(0);
            MinimumValueList << statistics.at(0).at(1) << statistics.at(1).at(1) << statistics.at(2).at(1);
            MaximumValueList << statistics.at(0).at(2) << statistics.at(1).at(2) << statistics.at(2).at(2);
            AverageValueList << statistics.at(0).at(3) << statistics.at(1).at(3) << statistics.at(2).at(3);
            m_HistogramWindow->setStatistic(ColorComponentList, MinimumValueList, MaximumValueList, AverageValueList, sFormat);
        }

        m_HistogramWindow->updateHistogram(histData, sHistogramTitle, nMaxHeight_YAxis, nMaxWidth_XAxis);
    }
    else
         m_pFrameObs->enableHistogram(false);
}

/* display frames on viewer */
void ViewerWindow::onimageReady ( QImage image, const QString &sFormat, const QString &sHeight, const QString &sWidth )
{
    m_FormatLabel->setText("Pixel Format: " + sFormat + " ");
    m_ImageSizeLabel->setText("Size H: " + sHeight + " ,W: "+ sWidth + " ");

    if(m_bHasJustStarted)
    {
        foreach( QGraphicsItem *item, m_pScene->items() )
        {
            if( item->type() == QGraphicsTextItem::Type )
            {
                 m_pScene->removeItem(m_TextItem);
                 m_FormatLabel->setStyleSheet("background-color: rgb(195,195,195); color: rgb(0,0,0)");
                 m_bIsRedHighlighted = false;
                 continue;
            }

            m_pScene->removeItem(m_PixmapItem);
        }

        m_pScene->addItem(m_PixmapItem);
        m_bHasJustStarted = false;
    }

    if( (sFormat.contains("Convert Error")) )
    {
        if( false == m_bIsRedHighlighted )
        {
            m_bIsRedHighlighted = true;
            m_FormatLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");

            if (sFormat.contains("height") || sFormat.contains("width"))
            {
                m_TextItem->setPlainText("The Resolution you set is not supported by VimbaImageTransform.\n"
                                         "Please change height or width !");
            }
            else
            {
                m_TextItem->setPlainText("PixelFormat transformation not supported.");
            }

            m_TextItem->setPos(m_pScene->width()/6, m_pScene->height()/2);
            m_pScene->addItem(m_TextItem);
        }
    }
    else
    {
        if( m_bIsRedHighlighted )
        {
            m_FormatLabel->setStyleSheet("background-color: rgb(195,195,195); color: rgb(0,0,0)");
            m_bIsRedHighlighted = false;

            if (!m_pScene->items().isEmpty())
            {
                m_pScene->removeItem(m_TextItem);
            }
        }
    }

    /* display it at centre whenever changed */
    m_pScene->setSceneRect(0, 0, image.width(), image.height() );
    m_PixmapItem->setPixmap(QPixmap::fromImage(image));
    m_ScreenViewer->show();

    /* save series of images */
    if( (0 < m_nNumberOfFramesToSave) && m_bIsTriggeredByMultiSaveBtn )
    {
        ++m_nImageCounter;

        if(m_nImageCounter <= m_nNumberOfFramesToSave)
        {
            m_SaveImageThread->start();
            try
            {
                static bool bCanReduceBpp;
                // Test only once for every batch to save
                if ( 1 == m_nImageCounter )
                {
                    bCanReduceBpp = CanReduceBpp();
                }
                // Save with 8 bpp
                if ( true == bCanReduceBpp )
                {
                    m_SaveImageThread->Enqueue( ReduceBpp( image ), m_nImageCounter );
                }
                // Save as is
                else
                {
                    m_SaveImageThread->Enqueue( image, m_nImageCounter );
                }
                // Reset at end of batch
                if ( m_nImageCounter == m_nNumberOfFramesToSave )
                {
                    bCanReduceBpp = false;
                }
            }
            catch( const std::bad_alloc &/*bex*/)
            {
                m_bIsRedHighlighted = false;

                ActionFreerun->setChecked(false);
                on_ActionFreerun_triggered();
                ActionFreerun->setEnabled(isStreamingAvailable());
                m_SaveImagesDialog->hide();
                delete m_SaveImagesDialog;
                m_SaveImagesDialog = NULL;
                m_SaveImageThread->StopProcessing();
                m_SaveImageThread->wait();
                m_bIsTriggeredByMultiSaveBtn = false;
                m_Allow16BitMultiSave = false;
            }
        }
    }
}

void ViewerWindow::onFullBitDepthImageReady(tFrameInfo mFullImageInfo)
{
    // store a full bit depth image frame in case user wants to save to file
    m_FullBitDepthImage = mFullImageInfo;

    // save series of TIFF images using LibTif
    if( (0 < m_nNumberOfFramesToSave) && m_Allow16BitMultiSave )
    {
        m_SaveImageThread->start();
        ++m_nImageCounter;

        if(m_nImageCounter <= m_nNumberOfFramesToSave)
        {
            try
            {
                m_SaveImageThread->Enqueue( mFullImageInfo, m_nImageCounter );
            }
            catch( const std::bad_alloc &bex)
            {
                m_bIsRedHighlighted = false;

                ActionFreerun->setChecked(false);
                on_ActionFreerun_triggered();
                ActionFreerun->setEnabled(isStreamingAvailable());
                m_SaveImagesDialog->hide();
                delete m_SaveImagesDialog;
                m_SaveImagesDialog = NULL;
                m_SaveImageThread->StopProcessing();
                m_SaveImageThread->wait();
                m_bIsTriggeredByMultiSaveBtn = false;
                m_Allow16BitMultiSave = false;
            }
        }
    }
}

void ViewerWindow::onSaving ( unsigned int nPos )
{
    if( 1 == nPos )
    {
        //Start Progressbar
        m_SaveImagesDialog = new QDialog(0, Qt::CustomizeWindowHint|Qt::WindowTitleHint);
        m_SaveImagesUIDialog.setupUi(m_SaveImagesDialog);
        m_SaveImagesUIDialog.saveImagesProgress->setMaximum(m_nNumberOfFramesToSave);
        m_SaveImagesUIDialog.saveImagesProgress->setMinimum(1);
        m_SaveImagesDialog->show();
    }

    m_SaveImagesUIDialog.saveImagesProgress->setValue(nPos);

    if (m_nNumberOfFramesToSave == nPos)
    {
        m_FormatLabel->setStyleSheet("background-color: rgb(195,195,195); color: rgb(0,0,0)");
        m_bIsRedHighlighted = false;

        ActionFreerun->setChecked(false);
        on_ActionFreerun_triggered();
        ActionFreerun->setEnabled(isStreamingAvailable());

        foreach( QGraphicsItem *item, m_pScene->items() )
        {
            if( item->type() == QGraphicsTextItem::Type )
            {
                m_pScene->removeItem(m_TextItem);
                break;
            }
        }

        m_SaveImagesDialog->hide();
        delete m_SaveImagesDialog;
        m_SaveImagesDialog = NULL;
        m_SaveImageThread->StopProcessing();
        m_SaveImageThread->wait();
        m_nImageCounter = 0;
        m_bIsTriggeredByMultiSaveBtn = false;
        m_Allow16BitMultiSave = false;

        // restore state of full bit depth image transfer flag
        on_ActionAllow16BitTiffSaving_triggered();
    }
}

void ViewerWindow::onAcquisitionStartStop ( const QString &sThisFeature )
{
    QIcon icon;

    /* this is intended to stop and start the camera again since PixelFormat, Height and Width have been changed while camera running
    *  ignore this when the changing has been made while camera not running
    */
    if( ((0 == sThisFeature.compare("AcquisitionStart")) && (m_bIsCameraRunning)) )
    {
        ActionFreerun->setChecked(true);
        on_ActionFreerun_triggered();
    }
    else if( sThisFeature.contains("AcquisitionStartFreerun") )
    {
        SP_ACCESS( m_pFrameObs )->resetFrameCounter(true);
        if(!m_bIsCameraRunning)
        {
            icon.addFile(QString::fromUtf8(":/VimbaViewer/Images/stop.png"), QSize(), QIcon::Normal, QIcon::On);
            ActionFreerun->setIcon(icon);
            checkDisplayInterval();
            releaseBuffer();
            onPrepareCapture();

            ActionFreerun->setChecked(true);
            m_bIsCameraRunning = true;
            m_bHasJustStarted = true;
            emit acquisitionRunning(true);

            if(ActionDisplayOptions->isEnabled())
                ActionDisplayOptions->setEnabled(false);

            if(ActionSaveOptions->isEnabled())
                ActionSaveOptions->setEnabled(false);

            /* if save images settings set, and acquisition starts */
            if( (0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()) && ActionSaveImages->isEnabled() )
            {
                ActionSaveImages->setEnabled(false);
                m_nImageCounter = 0;
                m_nNumberOfFramesToSave = 0;
            }

            m_OperatingStatusLabel->setText( " Running... " );
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,128, 0); color: rgb(255,255,255)");
        }
    }
    else if( sThisFeature.contains("AcquisitionStopFreerun") )
    {
        if(m_bIsCameraRunning)
        {
            icon.addFile(QString::fromUtf8(":/VimbaViewer/Images/play.png"), QSize(), QIcon::Normal, QIcon::Off);
            ActionFreerun->setIcon(icon);
            releaseBuffer();
            ActionFreerun->setChecked(false);
            if (m_bIsViewerWindowClosing)
                on_ActionFreerun_triggered();

            m_bIsCameraRunning = false;
            emit acquisitionRunning(false);

            if(!ActionSaveOptions->isEnabled())
                ActionSaveOptions->setEnabled(true);

            if(!ActionFreerun->isEnabled())
                ActionFreerun->setEnabled(isStreamingAvailable());

            if(!ActionDisplayOptions->isEnabled())
                ActionDisplayOptions->setEnabled(true);

            /* if save images running, and acquisition stops */
            if( (0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()) && !ActionSaveImages->isEnabled() )
            {
                ActionSaveImages->setEnabled(true);
            }

            m_Controller->synchronizeEventFeatures();
        }

        m_OperatingStatusLabel->setText( " Ready " );
        m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,0, 0); color: rgb(255,255,255)");
    }
    else if( ((0 == sThisFeature.compare("AcquisitionStop")) && (m_bIsCameraRunning)) ||
              (sThisFeature.contains("AcquisitionStopWidthHeight")))
    {
        if(m_bIsCameraRunning)
        {
            ActionFreerun->setChecked(false);
            on_ActionFreerun_triggered();

            /* use this for GigE, so you can change the W/H "on the fly" */
            if(0 == sThisFeature.compare("AcquisitionStopWidthHeight"))
            {
                m_bIsCameraRunning = true;
                emit acquisitionRunning(true);
            }
        }

    // update state of full bit depth image transfer flag in case pixel format has changed
    on_ActionAllow16BitTiffSaving_triggered();
    }

    // update state of full bit depth image transfer flag in case pixel format has changed
    on_ActionAllow16BitTiffSaving_triggered();
}

void ViewerWindow::checkDisplayInterval()
{
    FeaturePtr pFeatMode;

    if(VmbErrorSuccess == m_pCam->GetFeatureByName( "AcquisitionMode", pFeatMode ))
    {
        std::string sValue("");
        if( VmbErrorSuccess == pFeatMode->GetValue(sValue) )
        {
            /* display all received frames for SingleFrame and MultiFrame mode or if the user wants to have it */
            if( 0 == sValue.compare("SingleFrame") || 0 == sValue.compare("MultiFrame") || m_bIsDisplayEveryFrame )
                SP_ACCESS( m_pFrameObs )->setDisplayInterval( 0 );
            /* display frame in a certain interval to save CPU consumption for continuous mode */
            else
                SP_ACCESS( m_pFrameObs )->setDisplayInterval( 1 );
        }
    }
}

void ViewerWindow::on_ActionFreerun_triggered()
{
    VmbError_t error;
    FeaturePtr pFeat;

    checkDisplayInterval();

     /* update interpolation state after start */
    if( !m_Timer->isActive())
    {
        m_Timer->start(200);
    }

    QIcon icon;
    /* ON */
    if( ActionFreerun->isChecked() )
    {
        icon.addFile(QString::fromUtf8(":/VimbaViewer/Images/stop.png"), QSize(), QIcon::Normal, QIcon::On);
        ActionFreerun->setIcon(icon);

        error = onPrepareCapture();
        if( VmbErrorSuccess != error )
        {
            m_bIsCameraRunning = false;
            emit acquisitionRunning(false);
            m_OperatingStatusLabel->setText( " Failed to start! Error: " + QString::number(error)+" "+Helper::mapReturnCodeToString(error) );
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");
            icon.addFile(QString::fromUtf8("D:/Chimera/Chimera-Cryo-Test/VimbaOfficial/Images/play.png"), QSize(), QIcon::Normal, QIcon::Off);
            ActionFreerun->setIcon(icon);
            ActionFreerun->setChecked(false);
            return;
        }

        error = m_pCam->GetFeatureByName( "AcquisitionStart", pFeat );
        int nResult = m_sAccessMode.compare(tr("(READ ONLY)")) ;
        if ( (VmbErrorSuccess == error) && ( 0 != nResult ) )
        {
            SP_ACCESS( m_pFrameObs )->resetFrameCounter(true);

            // Do some GUI-related preparations before really starting (to avoid timing problems)
            m_OperatingStatusLabel->setText( " Running... " );
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,128, 0); color: rgb(255,255,255)");

            if(ActionDisplayOptions->isEnabled())
                ActionDisplayOptions->setEnabled(false);

            if(ActionSaveOptions->isEnabled())
                ActionSaveOptions->setEnabled(false);

            if( ActionSaveImages->isEnabled() && (0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()) )
                ActionSaveImages->setEnabled(false);

            error = pFeat->RunCommand();

            if(VmbErrorSuccess == error)
            {
                if(m_bIsFirstStart)
                {
                    m_bIsFirstStart = false;
                }

                m_bHasJustStarted = true;
                m_bIsCameraRunning = true;
                emit acquisitionRunning(true);
            }
            else
            {
                m_bIsCameraRunning = false;
                emit acquisitionRunning(false);
                m_OperatingStatusLabel->setText( " Failed to execute AcquisitionStart! Error: " + QString::number(error)+" "+Helper::mapReturnCodeToString(error) );
                m_OperatingStatusLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");

                m_InformationWindow->feedLogger("Logging", QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" +
                                                " RunCommand [AcquisitionStart] Failed! Error: " + QString::number(error)+" "+
                                                Helper::mapReturnCodeToString(error)), VimbaViewerLogCategory_ERROR);

                if(ActionDisplayOptions->isEnabled())
                    ActionDisplayOptions->setEnabled(true);

                if(ActionSaveOptions->isEnabled())
                    ActionSaveOptions->setEnabled(true);

                if( ActionSaveImages->isEnabled() && (0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()) )
                    ActionSaveImages->setEnabled(true);

            }
        }
    }
    /* OFF */
    else
    {
        error = m_pCam->GetFeatureByName("AcquisitionStop", pFeat);
        if ( (VmbErrorSuccess == error) )
        {
            if(0 != m_sAccessMode.compare(tr("(READ ONLY)")))
                error = pFeat->RunCommand();

            icon.addFile(QString::fromUtf8(":/Images/play.png"), QSize(), QIcon::Normal, QIcon::Off);
            ActionFreerun->setIcon(icon);

            if(VmbErrorSuccess == error)
            {
                m_bIsCameraRunning = false;
                emit acquisitionRunning(false);
                m_OperatingStatusLabel->setText( " Ready " );
                m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,0, 0); color: rgb(255,255,255)");

                releaseBuffer();
            }
            else
            {
                m_InformationWindow->feedLogger("Logging", QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" +
                                                " RunCommand [AcquisitionStop] Failed! Error: " + QString::number(error) + " " +
                                                Helper::mapReturnCodeToString(error) ), VimbaViewerLogCategory_ERROR);
            }
        }


        if(!ActionDisplayOptions->isEnabled())
            ActionDisplayOptions->setEnabled(true);

        if(!ActionSaveOptions->isEnabled())
            ActionSaveOptions->setEnabled(true);

        if( !ActionSaveImages->isEnabled() && (0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()))
            ActionSaveImages->setEnabled(true);

        m_Controller->synchronizeEventFeatures();
    }
}

VmbError_t ViewerWindow::releaseBuffer()
{
    m_pFrameObs->Stopping();
    VmbError_t error = m_pCam->EndCapture();
    if( VmbErrorSuccess == error )
        error = m_pCam->FlushQueue();
    if( VmbErrorSuccess == error )
        error = m_pCam->RevokeAllFrames();

    return error;
}

VmbError_t ViewerWindow::onPrepareCapture()
{
    FeaturePtr pFeature;
    VmbInt64_t nPayload = 0;
    QVector <FramePtr> frames;
    VmbError_t error = m_pCam->GetFeatureByName("PayloadSize", pFeature);
    VmbUint32_t nCounter = 0;
    if( VmbErrorSuccess == error )
    {
        error = pFeature->GetValue(nPayload);
        if(VmbErrorSuccess == error)
        {
            frames.resize(m_FrameBufferCount);

            bool bIsStreamingAvailable = isStreamingAvailable();

            if (bIsStreamingAvailable)
            {
                for (int i=0; i<frames.size(); i++)
                {
                    try
                    {
                        frames[i] = FramePtr(new Frame(nPayload));
                        nCounter++;
                    }
                    catch(std::bad_alloc& )
                    {
                         frames.resize((VmbInt64_t) (nCounter * 0.7));
                         break;
                    }
                    m_pFrameObs->Starting();
                    error = frames[i]->RegisterObserver(m_pFrameObs);
                    if( VmbErrorSuccess != error )
                    {
                        m_InformationWindow->feedLogger("Logging",
                                                        QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" + " RegisterObserver frame["+ QString::number(i)+ "] Failed! Error: " + QString::number(error)+" "+ Helper::mapReturnCodeToString(error)),
                                                        VimbaViewerLogCategory_ERROR);
                        return error;
                    }
                }

                for (int i=0; i<frames.size(); i++)
                {
                    error = m_pCam->AnnounceFrame( frames[i] );
                    if( VmbErrorSuccess != error )
                    {
                        m_InformationWindow->feedLogger("Logging",
                                                        QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" + " AnnounceFrame ["+ QString::number(i)+ "] Failed! Error: " + QString::number(error)+" "+ Helper::mapReturnCodeToString(error)),
                                                        VimbaViewerLogCategory_ERROR);
                        return error;
                    }
                }
            }

            if(VmbErrorSuccess == error)
            {
                error = m_pCam->StartCapture();
                if( VmbErrorSuccess != error )
                {
                    QString sMessage = " StartCapture Failed! Error: ";

                    if(0 != m_sAccessMode.compare(tr("(READ ONLY)")))
                        m_InformationWindow->feedLogger("Logging",
                                                         QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" + sMessage + QString::number(error)+" "+ Helper::mapReturnCodeToString(error)),
                                                         VimbaViewerLogCategory_ERROR);
                    return error;
                }
            }

            if (bIsStreamingAvailable)
            {
                for (int i=0; i<frames.size(); i++)
                {
                    error = m_pCam->QueueFrame( frames[i] );
                    if( VmbErrorSuccess != error )
                    {
                        m_InformationWindow->feedLogger("Logging",
                                                        QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" + " QueueFrame ["+ QString::number(i)+ "] Failed! Error: " + QString::number(error)+" "+ Helper::mapReturnCodeToString(error)),
                                                        VimbaViewerLogCategory_ERROR);
                        return error;
                    }
                }
            }
        }
        else
        {
            m_InformationWindow->feedLogger("Logging",
                                            QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" + " GetValue [PayloadSize] Failed! Error: " + QString::number(error)+" "+ Helper::mapReturnCodeToString(error)),
                                            VimbaViewerLogCategory_ERROR);
            return error;
        }
    }
    else
    {
        m_InformationWindow->feedLogger("Logging",
                                        QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" + " GetFeatureByName [PayloadSize] Failed! Error: " + QString::number(error)+" "+ Helper::mapReturnCodeToString(error)),
                                        VimbaViewerLogCategory_ERROR);
        return error;
    }

    return error;
}

void ViewerWindow::onFeedLogger    ( const QString &sMessage )
{
    m_InformationWindow->feedLogger("Logging", QString(QTime::currentTime().toString("hh:mm:ss:zzz")+"\t" + sMessage), VimbaViewerLogCategory_ERROR);
}

void ViewerWindow::changeEvent ( QEvent * event )
{
    if( event->type() == QEvent::WindowStateChange )
    {
        if( isMinimized() )
            m_Controller->showControl(false);
        else if( isMaximized() )
            m_Controller->showControl(true);
    }
}

bool ViewerWindow::loadPlugin()
{
    const QDir pluginsDir(qApp->applicationDirPath() + "/plugins");

    m_TabPluginCount = 0;

    foreach ( const QString fileName, pluginsDir.entryList(QDir::Files))
    {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject* const plugin = pluginLoader.instance();
        if (plugin)
        {
            TabExtensionInterface* const tabInterface = qobject_cast<TabExtensionInterface *>(plugin);
            m_tabExtensionInterface.push_back(tabInterface);
            if (m_tabExtensionInterface[m_TabPluginCount])
            {
                m_TabPluginCount++;
            }
        }
    }

    return m_TabPluginCount > 0;
}

bool ViewerWindow::CanReduceBpp()
{
    bool res = false;

    FeaturePtr pFeature;
    // TODO: Persist pixel format in viewer. Pixel format might be altered in between capture and save. Then color would be transformed to mono.
    // Bug or feature?
    VmbError_t error = m_pCam->GetFeatureByName( "PixelFormat", pFeature );
    if( VmbErrorSuccess == error )
    {
        VmbInt64_t nFormat;
        error = pFeature->GetValue( nFormat );
        if( VmbErrorSuccess == error )
        {
            res = Helper::convertFormatToString(static_cast<VmbPixelFormatType>(nFormat)).contains("Mono");
        }
    }

    return res;
}

// Reduces bpp to 8
QImage ViewerWindow::ReduceBpp( QImage image )
{
    // Create 8 bit grey scale color table
    if ( 0 == m_ColorTable.size() )
    {
        for ( int i = 0; i < 256; ++i )
        {
            m_ColorTable.push_back( QColor( i, i, i ).rgb() );
        }
    }
    // Convert to 8 bit grey scale
    return image.convertToFormat( QImage::Format_Indexed8, m_ColorTable );
}

// Check if the TL supports streaming
bool ViewerWindow::isStreamingAvailable()
{
    AVT::VmbAPI::FeaturePtr pStreamIDFeature;
    m_pCam->GetFeatureByName("StreamID", pStreamIDFeature);
    return (NULL == pStreamIDFeature) ? false : true;
}


// test if the current pixel format is supported and should use LibTiff to save image
bool ViewerWindow::isSupportedPixelFormat()
{
    bool result = false;
    FeaturePtr pFeature;

    VmbError_t error = m_pCam->GetFeatureByName( "PixelFormat", pFeature );
    if( VmbErrorSuccess == error )
    {
        VmbInt64_t nFormat;

        error = pFeature->GetValue( nFormat );
        if( VmbErrorSuccess == error )
        {
            switch(nFormat)
            {
            // supported images formats:
            case VmbPixelFormatMono10:
            case VmbPixelFormatMono10p:
            case VmbPixelFormatMono12:
            case VmbPixelFormatMono12p:
            case VmbPixelFormatMono12Packed:
            case VmbPixelFormatMono14:
            case VmbPixelFormatMono16:
            case VmbPixelFormatBayerGR10:
            case VmbPixelFormatBayerRG10:
            case VmbPixelFormatBayerGB10:
            case VmbPixelFormatBayerBG10:
            case VmbPixelFormatBayerGR10p:
            case VmbPixelFormatBayerRG10p:
            case VmbPixelFormatBayerGB10p:
            case VmbPixelFormatBayerBG10p:
            case VmbPixelFormatBayerGR12:
            case VmbPixelFormatBayerRG12:
            case VmbPixelFormatBayerGB12:
            case VmbPixelFormatBayerBG12:
            case VmbPixelFormatBayerGR12Packed:
            case VmbPixelFormatBayerRG12Packed:
            case VmbPixelFormatBayerGB12Packed:
            case VmbPixelFormatBayerBG12Packed:
            case VmbPixelFormatBayerGR12p:
            case VmbPixelFormatBayerRG12p:
            case VmbPixelFormatBayerGB12p:
            case VmbPixelFormatBayerBG12p:
            case VmbPixelFormatBayerGR16:
            case VmbPixelFormatBayerRG16:
            case VmbPixelFormatBayerGB16:
            case VmbPixelFormatBayerBG16:
                result = true;
                break;
            default:
                result = false;
            }
        }
    }
    return result;
}

void SaveImageThread::run()
{
    m_Images.StartProcessing();
    m_16BitImages.StartProcessing();
    // save 16 bit depth images if requested
    if(m_bSave16BitImages)
    {
        ImageWriter tiffWriter;
        if( !tiffWriter.IsAvailable() )
        {
            return;
        }

        FramePair tmpInfo;
        while( m_16BitImages.WaitData(tmpInfo) )
        {
            QString sFullPath = m_sImagePath;
            sFullPath.append("//").append(m_sSaveName);
            sFullPath.append("_"+QString::number(tmpInfo.first)).append(m_sSaveFormat);

            // save image using LibTif

            if( ! tiffWriter.WriteTiff( tmpInfo.second, sFullPath.toUtf8(),this) )
            {
                emit LogMessage(QString("error could not write TIFF image ") +sFullPath );
            }
            emit setPosition ( tmpInfo.first );
            if( tmpInfo.first >= m_nTNumber2Save )
            {
                break;
            }
        }
    }
    // save standard QImage format
    else
    {
        ImagePair tmpImage;
        while( m_Images.WaitData( tmpImage ) )
        {
            QString sFullPath = m_sImagePath;
            sFullPath.append("//").append(m_sSaveName);
            sFullPath.append("_"+QString::number(tmpImage.first)).append(m_sSaveFormat);

            // save image
            if( ! tmpImage.second.save(sFullPath) )
            {
                emit LogMessage(QString("error could not write image ") +sFullPath );
            }
            emit setPosition ( tmpImage.first);
            if( tmpImage.first >= m_nTNumber2Save )
            {
                break;
            }
        }
    }
}

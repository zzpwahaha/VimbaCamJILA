#include "ViewerWidget.h"
#include <QTimer>

#include "VmbImageTransformHelper.hpp"
#include "ExternLib/qcustomplot/qcustomplot.h"

#include <QDebug>

using AVT::VmbAPI::Frame;
using AVT::VmbAPI::FramePtr;

ViewerWidget::ViewerWidget(QWidget* parent, Qt::WindowFlags flag,
    QString sID, 
    bool bAutoAdjustPacketSize, CameraPtr pCam)
    : QWidget(parent, flag)
    //, m_DockController(NULL)
    //, m_DockInformation(NULL)
    , m_Controller(NULL)
    , m_ScreenViewer(NULL)
    , m_InformationWindow(NULL)
    , m_bHasJustStarted(false)
    , m_bIsFirstStart(true)
    , m_bIsCameraRunning(false)
    , m_bIsCamOpen(false)
    , m_bIsRedHighlighted(false)
    , m_bIsViewerWindowClosing(false)
    , m_bIsDisplayEveryFrame(false)
    //, m_ImageOptionDialog(NULL)
    //, m_saveFileDialog(NULL)
    //, m_getDirDialog(NULL)
    //, m_bIsTriggeredByMultiSaveBtn(false)
    //, m_nNumberOfFramesToSave(0)
    , m_FrameBufferCount(BUFFER_COUNT)
    , m_pCam(pCam)
{
    VmbError_t errorType;
    QTime openTimer;
    openTimer.start();

    /* setup information window */
    m_DiagInfomation = new QDialog(this);
    m_DiagInfomation->setModal(false);
    m_DiagInfomation->setWindowTitle("Information/Logger for " + sID);
    m_DiagInfomation->setStyleSheet("background-color: rgb(255, 255, 255); font: 9pt;");//font: 10pt;
    m_InformationWindow = new MainInformationWindow(0, 0, m_pCam);
    m_InformationWindow->openLoggingWindow();
    QVBoxLayout* infoLayout = new QVBoxLayout(m_DiagInfomation);
    infoLayout->addWidget(m_InformationWindow);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    //m_DiagInfomation->show();


    /*connect to camera*/
    errorType = m_pCam->Open(VmbAccessModeFull);
    m_sAccessMode = tr("(FULL ACCESS)"); //it is always full access since we do not allow other connection type
    bAutoAdjustPacketSize = false;
    m_OpenError = errorType;
    
    if (VmbErrorSuccess != errorType)
    {
        openTimer.elapsed();
        return;
    }

    m_sCameraID = sID;
    if (!m_sAccessMode.isEmpty())
    {
        sID.append(" ");
        sID.append(m_sAccessMode);
    }
    this->setWindowTitle(sID);



    /*QCP viewer widget: colormap + side/bottom plot*/
    m_QCP = QSharedPointer<QCustomPlot>(new QCustomPlot());
    //m_QCP->xAxis->setTickLabels(false);
    //m_QCP->yAxis->setTickLabels(false);
    
    m_QCP->xAxis->grid()->setVisible(false);
    m_QCP->yAxis->grid()->setVisible(false);
    
    /*m_QCP->setNotAntialiasedElements(QCP::aeAll);*/
    m_QCP->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    m_QCP->axisRect()->setupFullAxesBox(true);
    m_colorMap = QSharedPointer<QCPColorMap>(new QCPColorMap(m_QCP->xAxis, m_QCP->yAxis));
    m_colorMap->setInterpolate(false);
    m_colorScale = new QCPColorScale(m_QCP.data());
    m_QCP->plotLayout()->addElement(0, 1, m_colorScale);
    m_colorScale->setType(QCPAxis::atRight);
    m_colorScale->setRangeZoom(false);
    m_colorScale->setRangeDrag(false);
    m_colorMap->setColorScale(m_colorScale);
    m_colorMap->setGradient(QCPColorGradient::gpGrayscale);
    
    QCPMarginGroup* marginGroup = new QCPMarginGroup(m_QCP.data());
    m_QCP->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    m_colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

    connect(m_QCP.data(), &QCustomPlot::mouseMove, this, &ViewerWidget::onSetMousePosInCMap);
    //another connect for image coming in update is made afater m_pImgCThread


    /* add Viewer Widget to ViewerWindow*/
    m_pScene = QSharedPointer<QGraphicsScene>(new QGraphicsScene());
    m_PixmapItem = new QGraphicsPixmapItem();
    m_ScreenViewer = new GraphViewer(this);
    m_ScreenViewer->setAlignment(Qt::AlignCenter);
    m_ScreenViewer->setScene(m_pScene.data());
    m_pScene->addItem(m_PixmapItem);
    m_ScreenViewer->show();
    //m_ScreenViewer->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");
    m_bIsCamOpen = true;

    /*set image layout*/
    QVBoxLayout* m_VertLayout = new QVBoxLayout(this);
    QLabel* namelabel = new QLabel(sID);
    namelabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_VertLayout->addWidget(namelabel, 0);
    //m_VertLayout->addWidget(m_ScreenViewer, 1);
    m_VertLayout->addWidget(m_QCP.data(), 1);
    //this->setStyleSheet("background-color: rgb(85, 100, 100)");
    //this->setWindowFlags(Qt::Widget);


    /* add DiagController Controller */
    m_DiagController = new QDialog(this);
    m_DiagController->setModal(false);
    m_DiagController->setWindowTitle("Controller for " + sID);

    /* add Controller Tree */
    QWidget* widgetTree = new QWidget();
    m_Description = new QTextEdit();
    m_Controller = new ControllerTreeWindow(m_sCameraID, widgetTree, bAutoAdjustPacketSize, m_pCam);
    if (VmbErrorSuccess != m_Controller->getTreeStatus())
    {
        onFeedLogger("ERROR: ControllerTree returned: " + QString::number(m_Controller->getTreeStatus()) + " " + Helper::mapReturnCodeToString(m_Controller->getTreeStatus()));
    }

    m_Description->setLineWrapMode(QTextEdit::NoWrap);
    m_Description->setReadOnly(true);
    m_Description->setStyleSheet("font: 12px;\n" "font-family: Verdana;\n");

    /* add Filter Pattern */
    QHBoxLayout* pattern_HLayout = new QHBoxLayout();
    m_FilterPatternLineEdit = new LineEditCompleter(this);
    m_FilterPatternLineEdit->setText(tr("Example: Gain|Width"));
    m_FilterPatternLineEdit->setToolTip(tr("To filter multiple features, e.g: Width|Gain|xyz|etc"));
    m_FilterPatternLineEdit->setCompleter(m_Controller->getListCompleter());
    m_FilterPatternLineEdit->setMinimumWidth(200);
    QLabel* filterPatternLabel = new QLabel(tr("Filter pattern:"));
    filterPatternLabel->setStyleSheet("font-weight: bold;");
    QPushButton* patternButton = new QPushButton(tr("Search"));

    /*put controller related into controller dialog*/
    pattern_HLayout->addWidget(filterPatternLabel);
    pattern_HLayout->addWidget(m_FilterPatternLineEdit);
    pattern_HLayout->addWidget(patternButton);

    QWidget* ctrlTreeVerticalLayoutWidget = new QWidget();
    QVBoxLayout* ctrlTreeVerticalLayout = new QVBoxLayout();
    ctrlTreeVerticalLayout->addLayout(pattern_HLayout);
    ctrlTreeVerticalLayout->addWidget(m_Controller);
    ctrlTreeVerticalLayout->setContentsMargins(0, 0, 0, 0);
    ctrlTreeVerticalLayoutWidget->setLayout(ctrlTreeVerticalLayout);

    QSplitter* splitter = new QSplitter(m_DiagController);
    QList<int> listSize;
    listSize << 5000;
    splitter->setChildrenCollapsible(false);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(ctrlTreeVerticalLayoutWidget);
    splitter->addWidget(m_Description);
    splitter->setSizes(listSize);

    QVBoxLayout* ctrlDiaVLayout = new QVBoxLayout();
    ctrlDiaVLayout->addWidget(splitter);
    m_DiagController->setLayout(ctrlDiaVLayout);
    
    connect(m_FilterPatternLineEdit, SIGNAL(returnPressed()), this, SLOT(textFilterChanged()));
    connect(patternButton, SIGNAL(clicked(bool)), this, SLOT(textFilterChanged()));
    
    connect(m_Controller, SIGNAL(setDescription(const QString&)), this, SLOT(onSetDescription(const QString&)));
    connect(m_Controller, SIGNAL(setEventMessage(const QStringList&)), this, SLOT(onSetEventMessage(const QStringList&)), Qt::QueuedConnection);
    connect(m_Controller, SIGNAL(acquisitionStartStop(const QString&)), this, SLOT(onAcquisitionStartStop(const QString&))); // this eventually calls RegisterObserver in Frame.h which make sure  As new frames arrive, the observer's FrameReceived method will be called.  Only one observer can be registered.
    connect(m_Controller, SIGNAL(updateBufferSize()), this, SLOT(onPrepareCapture()));
    connect(m_Controller, SIGNAL(resetFPS()), this, SLOT(onResetFPS()));
    connect(m_Controller, SIGNAL(logging(const QString&)), this, SLOT(onFeedLogger(const QString&)));


    /*create context menu*/
    m_ContextMenu = new QMenu;
    this->setContextMenuPolicy(Qt::CustomContextMenu);
        
    m_aStartStopCap = new QAction("&Streaming");
    m_ContextMenu->addAction(m_aStartStopCap);
    m_aStartStopCap->setCheckable(true);
    m_aStartStopCap->setEnabled(isStreamingAvailable());
    connect(m_aStartStopCap, &QAction::triggered, this, &ViewerWidget::on_ActionFreerun_triggered);
    
    m_aDiagCtrler = new QAction("Con&troller");
    m_ContextMenu->addAction(m_aDiagCtrler);
    connect(m_aDiagCtrler, &QAction::triggered, this, [&]() {m_DiagController->show(); });
    
    m_aDiagInfo = new QAction("Infor&mation");
    m_ContextMenu->addAction(m_aDiagInfo);
    connect(m_aDiagInfo, &QAction::triggered, this, [&]() {m_DiagInfomation->show(); });

    m_ContextMenu->addSeparator();

    m_aSetCurrScrROI = new QAction("SetCurrentRO&I");
    m_ContextMenu->addAction(m_aSetCurrScrROI);
    connect(m_aSetCurrScrROI, &QAction::triggered, this, &ViewerWidget::SetCurrentScreenROI);

    m_aResetFullROI = new QAction("ResetFullROI");
    m_ContextMenu->addAction(m_aResetFullROI);
    connect(m_aResetFullROI, &QAction::triggered, this, &ViewerWidget::ResetFullROI);

    m_ContextMenu->addSeparator();

    //connecting the following action to a slot happens at cameraMainWindow, be careful of the order
    m_aCamlist = new QAction("&Camera");
    m_ContextMenu->addAction(m_aCamlist);
    m_aDisconnect = new QAction("&Disconnect");
    m_ContextMenu->addAction(m_aDisconnect);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(OnShowContextMenu(const QPoint&)));
    
    /* create FrameObserver to get frames from camera, add for QCPColorMap */
    SP_SET(m_pFrameObs, new FrameObserver(m_pCam));
    connect(SP_ACCESS(m_pFrameObs), SIGNAL(frameReadyFromObserver(QImage, const QString&, const QString&, const QString&)),
        this, SLOT(onimageReady(QImage, const QString&, const QString&, const QString&)));
    /*connect(SP_ACCESS(m_pFrameObs), SIGNAL(frameReadyFromObserver(QVector<ushort>, const QString&, const QString&, const QString&)),
        this, SLOT(onimageReady(QVector<ushort>, const QString&, const QString&, const QString&)));*/
    connect(SP_ACCESS(m_pFrameObs), SIGNAL(frameReadyFromObserver(std::vector<ushort>, const QString&, const QString&, const QString&)),
        this, SLOT(onimageReady(std::vector<ushort>, const QString&, const QString&, const QString&)));
    
    connect(SP_ACCESS(m_pFrameObs), SIGNAL(frameReadyFromObserverFullBitDepth(tFrameInfo)),
        this, SLOT(onFullBitDepthImageReady(tFrameInfo)));
    connect(SP_ACCESS(m_pFrameObs), SIGNAL(setCurrentFPS(const QString&)),
        this, SLOT(onSetCurrentFPS(const QString&)));
    connect(SP_ACCESS(m_pFrameObs), SIGNAL(setFrameCounter(const unsigned int&)),
        this, SLOT(onSetFrameCounter(const unsigned int&)));

    connect(SP_ACCESS(m_pFrameObs)->ImageProcessThreadPtr().data(),
        &ImageProcessingThread::logging, this, &ViewerWidget::onFeedLogger);


    /*create image calculating thread*/
    m_pImgCThread = new ImageCalculatingThread(m_pFrameObs, m_pCam, m_QCP, m_colorMap);
    connect(m_pImgCThread, &ImageCalculatingThread::imageReadyForPlot,
        this, &ViewerWidget::onimageReadyFromCalc);
    connect(this, &ViewerWidget::acquisitionRunning, this, &ViewerWidget::onImageCalcStartStop);
    connect(m_QCP.data(), &QCustomPlot::mouseMove, m_pImgCThread, &ImageCalculatingThread::updateMousePos);
    connect(m_pImgCThread, &ImageCalculatingThread::logging, this, &ViewerWidget::onFeedLogger);


    m_Timer = new QTimer(this);

    //this->addDockWidget(static_cast<Qt::DockWidgetArea>(2), m_DockController);
    
    /* Statusbar */
    QStatusBar* statusbar = new QStatusBar;
    m_OperatingStatusLabel = new QLabel(" Ready ");
    m_FormatLabel = new QLabel;
    m_ImageSizeLabel = new QLabel;
    m_FramesLabel = new QLabel;
    m_FramerateLabel = new QLabel;
    m_CursorScenePosLabel = new QLabel;

    statusbar->addWidget(m_OperatingStatusLabel);
    statusbar->addWidget(m_ImageSizeLabel);
    statusbar->addWidget(m_FormatLabel);
    statusbar->addWidget(m_FramesLabel);
    statusbar->addWidget(m_FramerateLabel);
    statusbar->addWidget(m_CursorScenePosLabel);
    connect(m_ScreenViewer, &GraphViewer::mousePositionInScene, this, &ViewerWidget::onSetMousePosInScene);
    //statusbar->setContentsMargins(0, 0, 0, 0);
    m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,0, 0); color: rgb(255,255,255)");
    m_VertLayout->addWidget(statusbar, 0);

    m_TextItem = new QGraphicsTextItem;
    QFont serifFont("Arial", 12, QFont::Bold);
    m_TextItem->setFont(serifFont);
    m_TextItem->setDefaultTextColor(Qt::red);

    //setMaximumSize(600, 600);
}

void ViewerWidget::onSetMousePosInScene(const QPointF& pPoint)
{
    if (!m_PixmapItem->pixmap().isNull())
    {        
        int pixValue=m_PixmapItem->pixmap().toImage().pixel(
            static_cast<int>(pPoint.x()),
            static_cast<int>(pPoint.y()));
        if (m_FormatLabel->text().contains("Mono8"))
        {
            //pixValue = pixValue & 0xFF;
        }
        else if (m_FormatLabel->text().contains("Mono12"))
        {
            //pixValue = pixValue & 0x0FFF;
        }
        else
        {
            m_CursorScenePosLabel->setText("MONO");
            return;
        }
        m_CursorScenePosLabel->setText("(" + QString::number(static_cast<int>(pPoint.x())) + " , " +
            QString::number(static_cast<int>(pPoint.y())) + " , " +
            QString::number(pixValue,16) + ")");
    }
    else
    {
        m_CursorScenePosLabel->setText("N/A");
    }
}

void ViewerWidget::onSetMousePosInCMap(QMouseEvent* event)
{
    double x = m_QCP->xAxis->pixelToCoord(event->pos().x());
    double y = m_QCP->yAxis->pixelToCoord(event->pos().y());
    //double z = m_colorMap->data()->data(x, y);
    //qDebug() << x << "," << std::floor(x + 0.5) << "," << y << "," << std::floor(y + 0.5) << "," << z;
    m_CursorScenePosLabel->setText("(" + QString::number(std::floor(x + 0.5)) + " , " +
        QString::number(std::floor(y + 0.5)) + " , " +
        QString::number(m_colorMap->data()->data(x, y)) + ")");
}



void ViewerWidget::OnShowContextMenu(const QPoint& pos)
{
    m_ContextMenu->exec(QCursor::pos());
}

ViewerWidget::~ViewerWidget()
{
    /* save setting position and geometry from last session */
    //QSettings settings("Allied Vision", "Vimba Viewer");
    //settings.setValue("geometry", saveGeometry());
    //settings.setValue("state", saveState(0));

    /* If cam is open */
    if (!m_sCameraID.isEmpty())
    {
        //settings.setValue(m_sCameraID, m_SaveImageOption.ImageDestination_Edit->text());
        //if (!m_SaveName.isEmpty())
        //    settings.setValue(m_sCameraID + "SaveImageName", m_SaveName);

        //if (NULL != m_saveFileDialog)
        //{
        //    delete m_saveFileDialog;
        //    m_saveFileDialog = NULL;
        //}

        releaseBuffer();

        m_pCam->Close();
    }
}

QString ViewerWidget::getCameraID () const
{
    return m_sCameraID;
}

bool ViewerWidget::getCamOpenStatus() const
{
    return m_bIsCamOpen;
}

CameraPtr ViewerWidget::getCameraPtr()
{
    return m_pCam;
}

bool ViewerWidget::isControlledCamera(const CameraPtr& cam) const
{
    return SP_ACCESS(cam) == SP_ACCESS(m_pCam);
}


bool  ViewerWidget::getAdjustPacketSizeMessage(QString& sMessage)
{
    if (m_Controller->isGigE())
    {
        if (VmbErrorSuccess == m_Controller->getTreeStatus())
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

void ViewerWidget::onSetCurrentFPS(const QString& sFPS)
{
    m_FramerateLabel->setText(QString::fromStdString(" Current FPS: ") + sFPS + " ");
}

void ViewerWidget::onResetFPS()
{
    SP_ACCESS(m_pFrameObs)->resetFrameCounter(false);
}

void ViewerWidget::onSetFrameCounter(const unsigned int& nNumberOfFrames)
{
    m_FramesLabel->setText("Frames: " + QString::number(nNumberOfFrames) + " ");
}


void ViewerWidget::onSetEventMessage(const QStringList& sMsg)
{
    m_InformationWindow->setEventMessage(sMsg);
}

void ViewerWidget::onSetDescription(const QString& sDesc)
{
    m_Description->setText(sDesc);
}


/*controller filter action*/
void ViewerWidget::textFilterChanged()
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

void ViewerWidget::onImageCalcStartStop(bool start)
{
    start ? m_pImgCThread->StartProcessing() : m_pImgCThread->StopProcessing();
}

/* display frames on viewer, the ultimate signal comes from ImageProcessingThread::run() in FrameObserver.cpp */
void ViewerWidget::onimageReady(QImage image, const QString& sFormat, const QString& sHeight, const QString& sWidth)
{
    m_FormatLabel->setText("Pixel Format: " + sFormat + " ");
    m_ImageSizeLabel->setText("Size H: " + sHeight + " ,W: " + sWidth + " ");
    if (m_bHasJustStarted)
    {
        foreach(QGraphicsItem * item, m_pScene->items())
        {
            if (item->type() == QGraphicsTextItem::Type)
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

    if ((sFormat.contains("Convert Error")))
    {
        if (false == m_bIsRedHighlighted)
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

            m_TextItem->setPos(m_pScene->width() / 6, m_pScene->height() / 2);
            m_pScene->addItem(m_TextItem);
        }
    }
    else
    {
        if (m_bIsRedHighlighted)
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
    m_pScene->setSceneRect(0, 0, image.width(), image.height());
    m_PixmapItem->setPixmap(QPixmap::fromImage(image));
    m_ScreenViewer->show();
    /*update pixel value picked by mouse position*/
    onSetMousePosInScene(m_ScreenViewer->getMouseScenePos());
    //Sleep(200);
    /* save series of images */
    //if ((0 < m_nNumberOfFramesToSave) && m_bIsTriggeredByMultiSaveBtn)
    //{
    //    ++m_nImageCounter;

    //    if (m_nImageCounter <= m_nNumberOfFramesToSave)
    //    {
    //        m_SaveImageThread->start();
    //        try
    //        {
    //            static bool bCanReduceBpp;
    //            // Test only once for every batch to save
    //            if (1 == m_nImageCounter)
    //            {
    //                bCanReduceBpp = CanReduceBpp();
    //            }
    //            // Save with 8 bpp
    //            if (true == bCanReduceBpp)
    //            {
    //                m_SaveImageThread->Enqueue(ReduceBpp(image), m_nImageCounter);
    //            }
    //            // Save as is
    //            else
    //            {
    //                m_SaveImageThread->Enqueue(image, m_nImageCounter);
    //            }
    //            // Reset at end of batch
    //            if (m_nImageCounter == m_nNumberOfFramesToSave)
    //            {
    //                bCanReduceBpp = false;
    //            }
    //        }
    //        catch (const std::bad_alloc&/*bex*/)
    //        {
    //            m_bIsRedHighlighted = false;
    //            ActionFreerun->setChecked(false);
    //            on_ActionFreerun_triggered();
    //            ActionFreerun->setEnabled(isStreamingAvailable());
    //            m_SaveImagesDialog->hide();
    //            delete m_SaveImagesDialog;
    //            m_SaveImagesDialog = NULL;
    //            m_SaveImageThread->StopProcessing();
    //            m_SaveImageThread->wait();
    //            m_bIsTriggeredByMultiSaveBtn = false;
    //            m_Allow16BitMultiSave = false;
    //        }
    //    }
    //}
}

void ViewerWidget::onimageReady(QVector<ushort> vec1d, const QString& sFormat, const QString& sHeight, const QString& sWidth)
{
    m_FormatLabel->setText("Pixel Format: " + sFormat + " ");
    m_ImageSizeLabel->setText("Size H: " + sHeight + " ,W: " + sWidth + " ");
    if (m_bHasJustStarted)
    {
        foreach(QGraphicsItem * item, m_pScene->items())
        {
            if (item->type() == QGraphicsTextItem::Type)
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

    if ((sFormat.contains("Convert Error")))
    {
        if (false == m_bIsRedHighlighted)
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

            m_TextItem->setPos(m_pScene->width() / 6, m_pScene->height() / 2);
            m_pScene->addItem(m_TextItem);
        }
    }
    else
    {
        if (m_bIsRedHighlighted)
        {
            m_FormatLabel->setStyleSheet("background-color: rgb(195,195,195); color: rgb(0,0,0)");
            m_bIsRedHighlighted = false;

            if (!m_pScene->items().isEmpty())
            {
                m_pScene->removeItem(m_TextItem);
            }
        }
    }
    
    /* display it on QCPColorMap and replot it */
    m_colorMap->data()->setSize(sWidth.toInt(), sHeight.toInt());
    /*QVector<double> tmp(vec1d.begin(),vec1d.end());*/
    auto width = sWidth.toInt();
    auto height = sHeight.toInt();
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            //m_colorMap->setData()
            m_colorMap->data()->setCell(j, i, vec1d.at(i * width + j)); 
        }
    }


    m_colorMap->rescaleDataRange();
    m_QCP->rescaleAxes();
    m_QCP->replot();


    /*update pixel value picked by mouse position*/
    //onSetMousePosInScene(m_ScreenViewer->getMouseScenePos());
}

void ViewerWidget::onimageReady(std::vector<ushort> vec1d, const QString& sFormat, const QString& sHeight, const QString& sWidth)
{
    m_FormatLabel->setText("Pixel Format: " + sFormat + " ");
    m_ImageSizeLabel->setText("Size H: " + sHeight + " ,W: " + sWidth + " ");
    if (m_bHasJustStarted)
    {
        foreach(QGraphicsItem * item, m_pScene->items())
        {
            if (item->type() == QGraphicsTextItem::Type)
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

    if ((sFormat.contains("Convert Error")))
    {
        if (false == m_bIsRedHighlighted)
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

            m_TextItem->setPos(m_pScene->width() / 6, m_pScene->height() / 2);
            m_pScene->addItem(m_TextItem);
        }
    }
    else
    {
        if (m_bIsRedHighlighted)
        {
            m_FormatLabel->setStyleSheet("background-color: rgb(195,195,195); color: rgb(0,0,0)");
            m_bIsRedHighlighted = false;

            if (!m_pScene->items().isEmpty())
            {
                m_pScene->removeItem(m_TextItem);
            }
        }
    }

    /* display it on QCPColorMap and replot it */
    m_colorMap->data()->setSize(sWidth.toInt(), sHeight.toInt());
    //QVector<double> tmp(vec1d.begin(),vec1d.end());
    auto width = sWidth.toInt();
    auto height = sHeight.toInt();
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            //m_colorMap->setData()
            m_colorMap->data()->setCell(j, i, vec1d.at(i * width + j));
        }
    }


    m_colorMap->rescaleDataRange();
    m_QCP->rescaleAxes();
    m_QCP->replot();


    /*update pixel value picked by mouse position*/
    //onSetMousePosInScene(m_ScreenViewer->getMouseScenePos());
}

void ViewerWidget::onimageReadyFromCalc()
{
    m_colorMap->rescaleDataRange(true);
    m_QCP->yAxis->setScaleRatio(m_QCP->xAxis, 1.0);
    QMouseEvent event(QMouseEvent::None, m_pImgCThread->mousePos(), Qt::NoButton, 0, 0);

    m_pImgCThread->mutex().lock();

    m_FormatLabel->setText("Pixel Format: " + m_pImgCThread->format() + " ");
    auto [w, h] = m_pImgCThread->WidthHeight();
    m_ImageSizeLabel->setText("Size H: " + QString::number(h) + " ,W: " + QString::number(w) + " ");
    m_QCP->replot();
    onSetMousePosInCMap(&event);
    m_pImgCThread->mutex().unlock();
    
    
    qDebug() << m_QCP->xAxis->range() << "," << m_QCP->yAxis->range();
}

void ViewerWidget::SetCurrentScreenROI()
{
    auto [maxw, maxh] = m_pImgCThread->maxWidthHeight();
    QCPRange xr = m_QCP->xAxis->range();
    QCPRange yr = m_QCP->yAxis->range();
    if (xr.lower > 0 && xr.upper < maxw && yr.lower>0 && yr.upper < maxh)
    {
        int xlower = 2 * std::floor(xr.lower / 2);
        int xupper = 2 * std::ceil(xr.upper / 2);
        int ylower = 2 * std::floor(yr.lower / 2);
        int yupper = 2 * std::ceil(yr.upper / 2);
        xupper += (xupper - xlower) % 4 == 0 ? 0 : 2;
        yupper += (yupper - ylower) % 4 == 0 ? 0 : 2;
        int xw = (xupper - xlower) > 2 ? xupper - xlower : 4;
        int yw = (yupper - ylower) > 2 ? yupper - ylower : 4;
        emit m_Controller->acquisitionStartStop("AcquisitionStopWidthHeight");

        /*first reset the value to full*/
        ResetFullROI(true);
        FeaturePtr pFeat;
        if (VmbErrorSuccess == m_pCam->GetFeatureByName("Width", pFeat))
        {
            auto tmp = pFeat->SetValue(xw);
            if (VmbErrorSuccess != tmp)
            {
                m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to x width " + QString::number(xw) + ", " + QString::number(tmp), VimbaViewerLogCategory_ERROR);
                return;
            }
        }
        if (VmbErrorSuccess == m_pCam->GetFeatureByName("Height", pFeat))
        {
            if (VmbErrorSuccess != pFeat->SetValue(yw))
            {
                m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to y height " + QString::number(yw), VimbaViewerLogCategory_ERROR);
                return;
            }
        }
        if (VmbErrorSuccess == m_pCam->GetFeatureByName("OffsetX", pFeat))
        {
            if (VmbErrorSuccess != pFeat->SetValue(xlower))
            {
                m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to lower bound of x " + QString::number(xlower), VimbaViewerLogCategory_ERROR);
                return;
            }
        }
        if (VmbErrorSuccess == m_pCam->GetFeatureByName("OffsetY", pFeat))
        {
            if (VmbErrorSuccess != pFeat->SetValue(ylower))
            {
                m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to lower bound of y " + QString::number(ylower), VimbaViewerLogCategory_ERROR);
                return;
            }
        }
        emit m_Controller->acquisitionStartStop("AcquisitionStart");
        //m_QCP->rescaleAxes();
    }
    else
    {
        m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to invalid region", VimbaViewerLogCategory_ERROR);
    }
}

void ViewerWidget::ResetFullROI(bool notStartReStart)
{
    if (!notStartReStart)
    {
        emit m_Controller->acquisitionStartStop("AcquisitionStopWidthHeight");
    }
    FeaturePtr pFeat;
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("OffsetX", pFeat))
    {
        if (VmbErrorSuccess != pFeat->SetValue(0))
        {
            m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to lower bound of x", VimbaViewerLogCategory_ERROR);
            return;
        }
    }
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("OffsetY", pFeat))
    {
        if (VmbErrorSuccess != pFeat->SetValue(0))
        {
            m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to lower bound of y", VimbaViewerLogCategory_ERROR);
            return;
        }
    }
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("Width", pFeat))
    {
        auto tt = m_pImgCThread->maxWidthHeight();
        if (VmbErrorSuccess != pFeat->SetValue(m_pImgCThread->maxWidthHeight().first))
        {
            m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to x width", VimbaViewerLogCategory_ERROR);
            return;
        }
    }
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("Height", pFeat))
    {
        if (VmbErrorSuccess != pFeat->SetValue(m_pImgCThread->maxWidthHeight().second))
        {
            m_InformationWindow->feedLogger("Logging", "Failed to set ROI due to y height", VimbaViewerLogCategory_ERROR);
            return;
        }
    }
    if (!notStartReStart)
    {
        emit m_Controller->acquisitionStartStop("AcquisitionStart");
    }
}

void ViewerWidget::onFullBitDepthImageReady(tFrameInfo mFullImageInfo)
{
    //// store a full bit depth image frame in case user wants to save to file
    //m_FullBitDepthImage = mFullImageInfo;

    //// save series of TIFF images using LibTif
    //if ((0 < m_nNumberOfFramesToSave) && m_Allow16BitMultiSave)
    //{
    //    m_SaveImageThread->start();
    //    ++m_nImageCounter;

    //    if (m_nImageCounter <= m_nNumberOfFramesToSave)
    //    {
    //        try
    //        {
    //            m_SaveImageThread->Enqueue(mFullImageInfo, m_nImageCounter);
    //        }
    //        catch (const std::bad_alloc& bex)
    //        {
    //            m_bIsRedHighlighted = false;

    //            ActionFreerun->setChecked(false);
    //            on_ActionFreerun_triggered();
    //            ActionFreerun->setEnabled(isStreamingAvailable());
    //            m_SaveImagesDialog->hide();
    //            delete m_SaveImagesDialog;
    //            m_SaveImagesDialog = NULL;
    //            m_SaveImageThread->StopProcessing();
    //            m_SaveImageThread->wait();
    //            m_bIsTriggeredByMultiSaveBtn = false;
    //            m_Allow16BitMultiSave = false;
    //        }
    //    }
    //}
}

void ViewerWidget::onFeedLogger(const QString& sMessage)
{
    m_InformationWindow->feedLogger("Logging", QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" + sMessage), VimbaViewerLogCategory_ERROR);
}

void ViewerWidget::checkDisplayInterval()
{
    FeaturePtr pFeatMode;

    if (VmbErrorSuccess == m_pCam->GetFeatureByName("AcquisitionMode", pFeatMode))
    {
        std::string sValue("");
        if (VmbErrorSuccess == pFeatMode->GetValue(sValue))
        {
            /* display all received frames for SingleFrame and MultiFrame mode or if the user wants to have it */
            if (0 == sValue.compare("SingleFrame") || 0 == sValue.compare("MultiFrame") || m_bIsDisplayEveryFrame)
                SP_ACCESS(m_pFrameObs)->setDisplayInterval(0);
            /* display frame in a certain interval to save CPU consumption for continuous mode */
            else
                SP_ACCESS(m_pFrameObs)->setDisplayInterval(1);
        }
    }
}

bool ViewerWidget::isStreamingAvailable()
{
    AVT::VmbAPI::FeaturePtr pStreamIDFeature;
    m_pCam->GetFeatureByName("StreamID", pStreamIDFeature);
    return (NULL == pStreamIDFeature) ? false : true;
}

VmbError_t ViewerWidget::onPrepareCapture()
{
    FeaturePtr pFeature;
    VmbInt64_t nPayload = 0;
    QVector <FramePtr> frames;
    VmbError_t error = m_pCam->GetFeatureByName("PayloadSize", pFeature);
    VmbUint32_t nCounter = 0;
    if (VmbErrorSuccess == error)
    {
        error = pFeature->GetValue(nPayload);
        if (VmbErrorSuccess == error)
        {
            frames.resize(m_FrameBufferCount);

            bool bIsStreamingAvailable = isStreamingAvailable();

            if (bIsStreamingAvailable)
            {
                for (int i = 0; i < frames.size(); i++)
                {
                    try
                    {
                        frames[i] = FramePtr(new Frame(nPayload));
                        nCounter++;
                    }
                    catch (std::bad_alloc&)
                    {
                        frames.resize((VmbInt64_t)(nCounter * 0.7));
                        break;
                    }
                    /*this is the key part to set the frame thread start to receive signal*/
                    m_pFrameObs->Starting();
                    /*the start() will do a lot of overhead to create the thread and it eventually call run()*/

                    error = frames[i]->RegisterObserver(m_pFrameObs);
                    if (VmbErrorSuccess != error)
                    {
                        m_InformationWindow->feedLogger("Logging",
                            QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" + " RegisterObserver frame[" + QString::number(i) + "] Failed! Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error)),
                            VimbaViewerLogCategory_ERROR);
                        return error;
                    }
                }

                for (int i = 0; i < frames.size(); i++)
                {
                    error = m_pCam->AnnounceFrame(frames[i]);
                    if (VmbErrorSuccess != error)
                    {
                        m_InformationWindow->feedLogger("Logging",
                            QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" + " AnnounceFrame [" + QString::number(i) + "] Failed! Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error)),
                            VimbaViewerLogCategory_ERROR);
                        return error;
                    }
                }
            }

            if (VmbErrorSuccess == error)
            {
                error = m_pCam->StartCapture();
                if (VmbErrorSuccess != error)
                {
                    QString sMessage = " StartCapture Failed! Error: ";

                    if (0 != m_sAccessMode.compare(tr("(READ ONLY)")))
                        m_InformationWindow->feedLogger("Logging",
                            QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" + sMessage + QString::number(error) + " " + Helper::mapReturnCodeToString(error)),
                            VimbaViewerLogCategory_ERROR);
                    return error;
                }
            }

            if (bIsStreamingAvailable)
            {
                for (int i = 0; i < frames.size(); i++)
                {
                    error = m_pCam->QueueFrame(frames[i]);
                    if (VmbErrorSuccess != error)
                    {
                        m_InformationWindow->feedLogger("Logging",
                            QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" + " QueueFrame [" + QString::number(i) + "] Failed! Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error)),
                            VimbaViewerLogCategory_ERROR);
                        return error;
                    }
                }
            }
        }
        else
        {
            m_InformationWindow->feedLogger("Logging",
                QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" + " GetValue [PayloadSize] Failed! Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error)),
                VimbaViewerLogCategory_ERROR);
            return error;
        }
    }
    else
    {
        m_InformationWindow->feedLogger("Logging",
            QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" + " GetFeatureByName [PayloadSize] Failed! Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error)),
            VimbaViewerLogCategory_ERROR);
        return error;
    }

    return error;
}



void ViewerWidget::on_ActionFreerun_triggered()
{
    VmbError_t error;
    FeaturePtr pFeat;

    checkDisplayInterval();

    /* update interpolation state after start */
    if (!m_Timer->isActive())
    {
        m_Timer->start(200);
    }


    /* ON */
    if (m_aStartStopCap->isChecked())
    {
        

        error = onPrepareCapture();
        if (VmbErrorSuccess != error)
        {
            m_bIsCameraRunning = false;
            emit acquisitionRunning(false);
            m_OperatingStatusLabel->setText(" Failed to start! Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error));
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");
            m_aStartStopCap->setChecked(false);
            return;
        }

        error = m_pCam->GetFeatureByName("AcquisitionStart", pFeat);
        int nResult = m_sAccessMode.compare(tr("(READ ONLY)"));
        if ((VmbErrorSuccess == error) && (0 != nResult))
        {
            SP_ACCESS(m_pFrameObs)->resetFrameCounter(true);

            // Do some GUI-related preparations before really starting (to avoid timing problems)
            m_OperatingStatusLabel->setText(" Running... ");
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,128, 0); color: rgb(255,255,255)");

            //if (ActionDisplayOptions->isEnabled())
            //    ActionDisplayOptions->setEnabled(false);

            //if (ActionSaveOptions->isEnabled())
            //    ActionSaveOptions->setEnabled(false);

            //if (ActionSaveImages->isEnabled() && (0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()))
            //    ActionSaveImages->setEnabled(false);

            error = pFeat->RunCommand();

            if (VmbErrorSuccess == error)
            {
                if (m_bIsFirstStart)
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
                m_OperatingStatusLabel->setText(" Failed to execute AcquisitionStart! Error: " + QString::number(error) + " " + Helper::mapReturnCodeToString(error));
                m_OperatingStatusLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");

                m_InformationWindow->feedLogger("Logging", QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" +
                    " RunCommand [AcquisitionStart] Failed! Error: " + QString::number(error) + " " +
                    Helper::mapReturnCodeToString(error)), VimbaViewerLogCategory_ERROR);

                //if (ActionDisplayOptions->isEnabled())
                //    ActionDisplayOptions->setEnabled(true);

                //if (ActionSaveOptions->isEnabled())
                //    ActionSaveOptions->setEnabled(true);

                //if (ActionSaveImages->isEnabled() && (0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()))
                //    ActionSaveImages->setEnabled(true);

            }
        }
    }
    /* OFF */
    else
    {
        error = m_pCam->GetFeatureByName("AcquisitionStop", pFeat);
        if ((VmbErrorSuccess == error))
        {
            if (0 != m_sAccessMode.compare(tr("(READ ONLY)")))
                error = pFeat->RunCommand();

            

            if (VmbErrorSuccess == error)
            {
                m_bIsCameraRunning = false;
                emit acquisitionRunning(false);
                m_OperatingStatusLabel->setText(" Ready ");
                m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,0, 0); color: rgb(255,255,255)");

                releaseBuffer();
            }
            else
            {
                m_InformationWindow->feedLogger("Logging", QString(QTime::currentTime().toString("hh:mm:ss:zzz") + "\t" +
                    " RunCommand [AcquisitionStop] Failed! Error: " + QString::number(error) + " " +
                    Helper::mapReturnCodeToString(error)), VimbaViewerLogCategory_ERROR);
            }
        }


        //if (!ActionDisplayOptions->isEnabled())
        //    ActionDisplayOptions->setEnabled(true);

        //if (!ActionSaveOptions->isEnabled())
        //    ActionSaveOptions->setEnabled(true);

        //if (!ActionSaveImages->isEnabled() && (0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()))
        //    ActionSaveImages->setEnabled(true);

        m_Controller->synchronizeEventFeatures();
    }
}

VmbError_t ViewerWidget::releaseBuffer()
{
    m_pFrameObs->Stopping();
    VmbError_t error = m_pCam->EndCapture();
    if (VmbErrorSuccess == error)
        error = m_pCam->FlushQueue();
    if (VmbErrorSuccess == error)
        error = m_pCam->RevokeAllFrames();

    return error;
}




VmbError_t ViewerWidget::getOpenError() const
{
    return m_OpenError;
}


/*this is for the command from tree controller*/
void ViewerWidget::onAcquisitionStartStop(const QString& sThisFeature)
{


    /* this is intended to stop and start the camera again since PixelFormat, Height and Width have been changed while camera running
    *  ignore this when the changing has been made while camera not running
    */
    if (((0 == sThisFeature.compare("AcquisitionStart")) && (m_bIsCameraRunning)))
    {
        m_aStartStopCap->setChecked(true);
        on_ActionFreerun_triggered(); //mainly for the gui and logger stuff
    }
    else if (sThisFeature.contains("AcquisitionStartFreerun"))
    {
        SP_ACCESS(m_pFrameObs)->resetFrameCounter(true);
        if (!m_bIsCameraRunning)
        {

            checkDisplayInterval();
            releaseBuffer();
            onPrepareCapture();

            m_aStartStopCap->setChecked(true);
            m_bIsCameraRunning = true;
            m_bHasJustStarted = true;
            emit acquisitionRunning(true);

            //if (ActionDisplayOptions->isEnabled())
            //    ActionDisplayOptions->setEnabled(false);

            //if (ActionSaveOptions->isEnabled())
            //    ActionSaveOptions->setEnabled(false);

            /* if save images settings set, and acquisition starts */
            //if ((0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()) && ActionSaveImages->isEnabled())
            //{
            //    ActionSaveImages->setEnabled(false);
            //    m_nImageCounter = 0;
            //    m_nNumberOfFramesToSave = 0;
            //}

            m_OperatingStatusLabel->setText(" Running... ");
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,128, 0); color: rgb(255,255,255)");
        }
    }
    else if (sThisFeature.contains("AcquisitionStopFreerun"))
    {
        if (m_bIsCameraRunning)
        {
            
            releaseBuffer();
            m_aStartStopCap->setChecked(false);
            if (m_bIsViewerWindowClosing)
                on_ActionFreerun_triggered();

            m_bIsCameraRunning = false;
            emit acquisitionRunning(false);

            //if (!ActionSaveOptions->isEnabled())
            //    ActionSaveOptions->setEnabled(true);

            if (!m_aStartStopCap->isEnabled())
                m_aStartStopCap->setEnabled(isStreamingAvailable());

            //if (!ActionDisplayOptions->isEnabled())
            //    ActionDisplayOptions->setEnabled(true);

            /* if save images running, and acquisition stops */
            //if ((0 < m_SaveImageOption.NumberOfFrames_SpinBox->value()) && !ActionSaveImages->isEnabled())
            //{
            //    ActionSaveImages->setEnabled(true);
            //}

            m_Controller->synchronizeEventFeatures();
        }

        m_OperatingStatusLabel->setText(" Ready ");
        m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,0, 0); color: rgb(255,255,255)");
    }
    else if (((0 == sThisFeature.compare("AcquisitionStop")) && (m_bIsCameraRunning)) ||
        (sThisFeature.contains("AcquisitionStopWidthHeight")))
    {
        if (m_bIsCameraRunning)
        {
            m_aStartStopCap->setChecked(false);
            on_ActionFreerun_triggered();

            /* use this for GigE, so you can change the W/H "on the fly" */
            if (0 == sThisFeature.compare("AcquisitionStopWidthHeight"))
            {
                m_bIsCameraRunning = true;
                emit acquisitionRunning(true);
            }
        }

        // update state of full bit depth image transfer flag in case pixel format has changed
        //on_ActionAllow16BitTiffSaving_triggered();
    }

    // update state of full bit depth image transfer flag in case pixel format has changed
    //on_ActionAllow16BitTiffSaving_triggered();
}

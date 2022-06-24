#include "stdafx.h"
#include <cameraMainWindow.h>
#include "Accessory/my_str.h"
#include "MakoCamera.h"
#include <qstatusbar.h>
#include <qapplication.h>
#include <qdialog.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qaction.h>

MakoCamera::MakoCamera(CameraInfo camInfo, cameraMainWindow* parent)
    : IChimeraSystem(parent)
    , core(camInfo)
    , viewer(core.CameraName(), this)
    , imgCThread(SP_DECL(FrameObserver)(core.getFrameObs()), core, false,
        viewer.plot(), viewer.cmap(), viewer.bottomPlot(), viewer.leftPlot())
    , camInfo(camInfo)
    , saveFileDialog(nullptr)
{
    
	//connect()

	// context menu of viewer

}

MakoCamera::~MakoCamera()
{
    //releaseBuffer();
}

void MakoCamera::initialize()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    /*image title and expActive checkbox*/
    QHBoxLayout* nameLayout = new QHBoxLayout(this);
    nameLayout->setContentsMargins(0, 0, 0, 0);
    QLabel* namelabel = new QLabel(qstr(core.CameraName()));
    namelabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QLabel* repLabel = new QLabel("PicsPerRep: ");
    m_picsPerRep = new QSpinBox(this);
    QLabel* activeLabel = new QLabel("Exp. Active?", this);
    m_expActive = new QCheckBox(this);
    nameLayout->addWidget(namelabel, 1);
    nameLayout->addWidget(repLabel, 0);
    nameLayout->addWidget(m_picsPerRep, 0);
    nameLayout->addWidget(activeLabel, 0);
    nameLayout->addWidget(m_expActive, 0);
    layout->addLayout(nameLayout, 0);
    connect(m_picsPerRep, qOverload<int>(&QSpinBox::valueChanged), [this](int pics) {
        core.setPicsPerRep(pics); });
    connect(m_expActive, &QCheckBox::clicked, [this](bool checked) {
        core.setExpActive(checked);
        imgCThread.setExpActive(checked); });

    /*status bar*/
    QStatusBar* statusbar1 = new QStatusBar(this);
    QStatusBar* statusbar2 = new QStatusBar(this);
    m_OperatingStatusLabel = new QLabel(" Ready ", this);
    m_TrigOnOffButton = new QPushButton("Trig:On/Off", this);
    m_TrigSourceButton = new QPushButton("TrigSrc", this);
    m_ImageSizeButtonX = new QPushButton("X", this);
    m_ImageSizeButtonY = new QPushButton("Y", this);
    m_ImageSizeButtonH = new QPushButton("sizeH", this);
    m_ImageSizeButtonW = new QPushButton("sizeW", this);
    QWidget* imageSizeBtn = new QWidget();
    QHBoxLayout* imageSizeBtnLayout = new QHBoxLayout(imageSizeBtn);
    imageSizeBtnLayout->setMargin(0);
    imageSizeBtnLayout->addWidget(m_ImageSizeButtonX);
    imageSizeBtnLayout->addWidget(m_ImageSizeButtonY);
    imageSizeBtnLayout->addWidget(m_ImageSizeButtonH);
    imageSizeBtnLayout->addWidget(m_ImageSizeButtonW);
    QWidget* imageBinBtn = new QWidget();
    QHBoxLayout* imageBinBtnLayout = new QHBoxLayout(imageBinBtn);
    imageBinBtnLayout->setMargin(0);
    m_ImageBinButtonH = new QPushButton("BinH");
    m_ImageBinButtonW = new QPushButton("BinW");
    imageBinBtnLayout->addWidget(m_ImageBinButtonH);
    imageBinBtnLayout->addWidget(m_ImageBinButtonW);
    QLabel* framesLabel = new QLabel("frame#", this);
    QPushButton* framerateButton = new QPushButton("FPS", this);
    m_avgSetButton = new QPushButton("Avg", this);
    auto bkgsubLabel = new QLabel("Bkg. Sub.: Off", this);
    m_CursorScenePosLabel = new QLabel("pos", this);
    m_DataMaxMinLabel = new QLabel("Max/Min", this);
    m_ExposureTimeButton = new QPushButton("exposure", this);
    m_CameraGainButton = new QPushButton("gain", this);
    statusbar1->addWidget(m_OperatingStatusLabel);
    statusbar1->addWidget(imageSizeBtn);
    statusbar1->addWidget(imageBinBtn);
    statusbar1->addWidget(m_TrigOnOffButton);
    statusbar1->addWidget(m_TrigSourceButton);
    statusbar1->addWidget(m_CursorScenePosLabel);
    statusbar1->addWidget(m_DataMaxMinLabel);
    statusbar2->addWidget(m_ExposureTimeButton);
    statusbar2->addWidget(m_CameraGainButton);
    statusbar2->addWidget(framesLabel);
    statusbar2->addWidget(framerateButton);
    statusbar2->addWidget(m_avgSetButton);
    statusbar2->addWidget(bkgsubLabel);

    m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,0, 0); color: rgb(255,255,255)");
    for (auto& btn : { m_ImageSizeButtonX,m_ImageSizeButtonY,m_ImageSizeButtonH ,m_ImageSizeButtonW,
        m_ImageBinButtonH,m_ImageBinButtonW,
        m_CameraGainButton,m_ExposureTimeButton,
        framerateButton,m_TrigOnOffButton,m_TrigSourceButton,m_avgSetButton })
        btn->setStyleSheet("border: none; color: rgb(128, 89, 255); font: 11pt");
    for (auto& btn : { m_CursorScenePosLabel, m_DataMaxMinLabel, m_OperatingStatusLabel,framesLabel, bkgsubLabel }) {
        btn->setStyleSheet("font: 11pt");
    }

    layout->addWidget(statusbar1, 0);
    layout->addWidget(statusbar2, 0);
    viewer.setMinimumSize(800, 800);
    layout->addWidget(&viewer, 1);
    //layout->addStretch(1);

    QRect rec = QApplication::desktop()->screenGeometry();
    this->setMaximumSize(rec.width() / 2, rec.height());

    //if (camInfo.safemode) {
    //    return; // do not do the follow handling in safemode
    //}
    connect(m_TrigOnOffButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("TriggerMode"); });
    connect(m_TrigSourceButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("TriggerSource"); });
    connect(m_ImageSizeButtonX, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("OffsetX"); });
    connect(m_ImageSizeButtonY, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("OffsetY"); });
    connect(m_ImageSizeButtonH, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("Height"); });
    connect(m_ImageSizeButtonW, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("Width"); });
    connect(m_ImageBinButtonH, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("BinningVertical"); });
    connect(m_ImageBinButtonW, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("BinningHorizontal"); });
    connect(m_CameraGainButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("Gain"); });
    connect(m_ExposureTimeButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("ExposureTimeAbs"); });
    connect(framerateButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("AcquisitionFrameRateAbs"); });
    connect(&imgCThread.averager(), &FrameAverager::avgNumberUpdate, this, [this](QString str) {
        m_avgSetButton->setText("Avg: " + str); });
    connect(m_avgSetButton, &QPushButton::clicked, this, [this]() {
        createAvgControlWidget(); });
    connect(&imgCThread, &ImageCalculatingThread::backgroundStatus, this, [this, bkgsubLabel](bool valid) {
        bool dobkg = imgCThread.doBkgSubtraction();
        if (!dobkg) {
            bkgsubLabel->setText("Bkg. Sub.: Off");
        }
        else {
            bkgsubLabel->setText(valid ? "Bkg. Sub.: On" : "Bkg.Sub. : Invalid Bkg");
        }
        });

    connect(core.getFrameObs(), &FrameObserver::setCurrentFPS, this,
        [this, framerateButton](const QString& sFPS) {
            framerateButton->setText(" FPS: " + sFPS + " "); });
    connect(core.getFrameObs(), &FrameObserver::setFrameCounter, this,
        [this, framesLabel](unsigned int nNumberOfFrames) {
            framesLabel->setText("Frames: " + qstr(nNumberOfFrames) + " "); });
    connect(&core.getMakoCtrl(), &MakoSettingControl::acquisitionStartStop, this,
        [this](QString sThisFeature) {
            acquisitionStartStopFromCtrler(sThisFeature); });
    connect(&core.getMakoCtrl(), &MakoSettingControl::updateStatusBar, this,
        [this]() {
            updateStatusBar(); });

    /*image calc thread*/
    connect(&imgCThread, &ImageCalculatingThread::imageReadyForPlot, this, [this]() {
        viewer.renderImgFromCalcThread(m_aManualCscale->isChecked());
        auto MaxMin = imgCThread.dataMaxMin();
        m_DataMaxMinLabel->setText("Max/Min: (" + qstr(MaxMin.first) + "/" + qstr(MaxMin.second) + ")");
        imgCThread.mutex().lock();
        QMouseEvent event(QMouseEvent::None, imgCThread.mousePos(), Qt::NoButton, 0, 0);
        viewer.onSetMousePosInCMap(&event, m_CursorScenePosLabel);
        imgCThread.mutex().unlock(); });

    connect(viewer.plot(), &QCustomPlot::mouseMove, this, [this](QMouseEvent* mouseEvn) {
        imgCThread.updateMousePos(mouseEvn); });
    connect(viewer.bottomAxes()->axis(QCPAxis::atTop), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
        this, [this](QCPRange range) {
            auto [ox, oy] = imgCThread.offsetXY();
            viewer.bottomAxes()->axis(QCPAxis::atBottom)->setRange(range - ox); });
    connect(viewer.leftAxes()->axis(QCPAxis::atRight), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
        this, [this](QCPRange range) {
            auto [ox, oy] = imgCThread.offsetXY();
            viewer.leftAxes()->axis(QCPAxis::atLeft)->setRange(range - oy); });

    connect(&imgCThread, &ImageCalculatingThread::imageReadyForExp, this, &MakoCamera::handleExpImage);
    connect(&core, &MakoCameraCore::makoFinished, this, &MakoCamera::finishExp);
    connect(&core, &MakoCameraCore::makoStarted, this, &MakoCamera::startExp);

    //connect(&imgCThread, &ImageCalculatingThread::currentFormat, this, [this](QString format) {
    //    if (0 == currentFormat.compare(format)) return;
    //    if (0 == format.compare("Mono12")) {
    //        viewer.rangeSlider()->setRange(0, 4095);
    //        viewer.rangeSlider()->upperSpinBox()->setRange(0, 4095);
    //        viewer.rangeSlider()->lowerSpinBox()->setRange(0, 4095);
    //        currentFormat = format;
    //    }
    //    else if (0 == format.compare("Mono8")) {
    //        viewer.rangeSlider()->setRange(0, 255);
    //        viewer.rangeSlider()->upperSpinBox()->setRange(0, 255);
    //        viewer.rangeSlider()->lowerSpinBox()->setRange(0, 255);
    //        currentFormat = format;
    //    }
    //    else {
    //        emit error("I am curious how on earth do you get format other than Mono8/12");
    //    } });


    /*set up mako controller gui*/
    //core.getMakoCtrl().setParent(this);
    makoCtrlDialog = new QDialog(this);
    makoCtrlDialog->setModal(false);
    makoCtrlDialog->setWindowTitle("Controller for " + qstr(core.CameraName()));
    QWidget* widgetTree = new QWidget(makoCtrlDialog);
    core.getMakoCtrl().initializeWidget(widgetTree);
    QVBoxLayout* diagLayout = new QVBoxLayout(makoCtrlDialog);
    makoCtrlDialog->setLayout(diagLayout);
    diagLayout->addWidget(widgetTree);

    //imgCThread.setParent(this);

    // basic viewer function, mouse position and double click
    connect(viewer.plot(), &QCustomPlot::mouseMove, [this](QMouseEvent* event) {
        viewer.onSetMousePosInCMap(event, m_CursorScenePosLabel);
        if (!isCamRunning) { viewer.plot()->replot(); } });

    connect(viewer.plot(), &QCustomPlot::mouseDoubleClick, this, [this]() {
        try {
            updateStatusBar();
            imgCThread.setDefaultView();
            viewer.plot()->replot();
        }
        catch (ChimeraError& e) {
            emit error(qstr("Error in handling Mako double click") + qstr(e.trace()));
        }});

    initPlotContextMenu();
}

void MakoCamera::handleStatusButtonClicked(QString featName)
{
    try {
        try {
            core.getMakoCtrl().updateRegisterFeature();
            updateStatusBar();
        }
        catch (...) {}
        QList<QStandardItem*> tmp = core.getMakoCtrl().controllerModel()->findItems(featName, Qt::MatchRecursive | Qt::MatchWrap);
        if (!tmp.isEmpty()) {
            core.getMakoCtrl().onClicked(tmp.at(0)->index().siblingAtColumn(1));
        }
    }
    catch (ChimeraError& e) {
        emit error(qstr("Error in handle the MakoStatusButton") + qstr(e.trace()));
    }
    
}

void MakoCamera::createAvgControlWidget()
{
    QDialog* wid = new  QDialog(this);
    wid->setModal(false);
    auto& avg = imgCThread.averager();
    QVBoxLayout* layout = new QVBoxLayout(wid);
    QHBoxLayout* layout1 = new QHBoxLayout(wid);
    QHBoxLayout* layout2 = new QHBoxLayout(wid);
    QLabel* avgt = new QLabel("Avg. Type: ");
    QComboBox* avgtComb = new QComboBox(wid);
    for (auto t : avg.allType) {
        avgtComb->addItem(qstr(avg.avgTypeName(t)));
    }
    avgtComb->setCurrentIndex(avg.currentAvgType());
    connect(avgtComb, qOverload<int>(&QComboBox::currentIndexChanged), [this](int idx) {
        imgCThread.averager().setAvgType(idx); });
    layout1->addWidget(avgt, 0);
    layout1->addWidget(avgtComb, 1);
    layout->addLayout(layout1);

    QLabel* avgn = new QLabel("Avg. Number: ");
    QLineEdit* avgnLE = new QLineEdit(qstr(avg.avgNumber()));
    connect(avgnLE, &QLineEdit::returnPressed, [this, avgnLE]() {
        QString s = avgnLE->text();
        int val;
        try {
            val = s.toInt();
        }
        catch (ChimeraError&) {
            errBox("The average number is not valid, please input a positive integer");
        }
        if (val > 0) {
            imgCThread.averager().setAvgNum(val);
        }
        else {
            errBox("The average number is not valid, please input a positive integer");
        } });
    layout2->addWidget(avgn, 0);
    layout2->addWidget(avgnLE, 1);
    layout->addLayout(layout2);

    wid->show();
}


void MakoCamera::startExp()
{
    m_expActive->setEnabled(false);
    m_picsPerRep->setEnabled(false);
    imgCThread.experimentStarted();
    isExpRunning = true;
}

void MakoCamera::finishExp()
{
    m_expActive->setEnabled(true);
    m_picsPerRep->setEnabled(true);
    imgCThread.experimentFinished();
    isExpRunning = false;
    MOTCalcActive = false;
}



void MakoCamera::initPlotContextMenu()
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    m_aStartStopCap = new QAction("&Streaming");
    viewer.contextMenu()->addAction(m_aStartStopCap);
    m_aStartStopCap->setCheckable(true);
    m_aStartStopCap->setEnabled(core.isStreamingAvailable());
    connect(m_aStartStopCap, &QAction::triggered, [this]() {acquisitionStartStopFromAction(); });

    QAction* aDiagCtrler = new QAction("Con&troller");
    viewer.contextMenu()->addAction(aDiagCtrler);
    connect(aDiagCtrler, &QAction::triggered, this, [this]() {makoCtrlDialog->show(); });

    viewer.contextMenu()->addSeparator();

    QAction* aSetCurrScrROI = new QAction("SetCurrentRO&I");
    viewer.contextMenu()->addAction(aSetCurrScrROI);
    connect(aSetCurrScrROI, &QAction::triggered, [this]() {setCurrentScreenROI(); });

    QAction* aResetFullROI = new QAction("ResetFullROI");
    viewer.contextMenu()->addAction(aResetFullROI);
    connect(aResetFullROI, &QAction::triggered, [this]() {resetFullROI(); });


    QAction* aPlotTracer = new QAction("Tracer");
    aPlotTracer->setCheckable(true);
    aPlotTracer->setChecked(true);
    viewer.contextMenu()->addAction(aPlotTracer);
    connect(aPlotTracer, &QAction::triggered, this, [this, aPlotTracer]() {
        for (auto& tra : { viewer.bottomTracer(),viewer.leftTracer() })
        {
            aPlotTracer->isChecked() ? tra->setVisible(true) : tra->setVisible(false);
        }
        for (auto& tra : { viewer.bottomTracerText(),viewer.leftTracerText() })
        {
            aPlotTracer->isChecked() ? tra->setVisible(true) : tra->setVisible(false);
        }
        viewer.plot()->replot(); });

    QMenu* mBkgSub = viewer.contextMenu()->addMenu("Bkg. subtraction");
    QAction* aBkgSubOnOff = mBkgSub->addAction("Turn on");
    QAction* aBkgSubSetBkg = mBkgSub->addAction("Set Bkg.");
    connect(aBkgSubOnOff, &QAction::triggered, [this, aBkgSubOnOff]() {
        if (aBkgSubOnOff->text() == "Turn on") {
            aBkgSubOnOff->setText("Turn off");
            imgCThread.toggleBkgSubtraction(true);
        }
        else if (aBkgSubOnOff->text() == "Turn off") {
            aBkgSubOnOff->setText("Turn on");
            imgCThread.toggleBkgSubtraction(false);
        }
        else errBox("Low-level bug in toggle the background subtraction"); });
    connect(aBkgSubSetBkg, &QAction::triggered, [this]() {
        try {
            imgCThread.setBackground();
        }
        catch (ChimeraError& e) {
            parentWin->reportErr("Capture image for background subtraction failed \n" + e.qtrace());
        }});


    QAction* aPlotAvg = new QAction("Averaging");
    aPlotAvg->setCheckable(true);
    aPlotAvg->setChecked(false);
    viewer.contextMenu()->addAction(aPlotAvg);
    connect(aPlotAvg, &QAction::triggered, this, [this, aPlotAvg]() {
        imgCThread.averager().toggleDoAveraging(aPlotAvg->isChecked());
        if (!aPlotAvg->isChecked()) m_avgSetButton->setText("Avg: OFF"); });

    QAction* aPlotFitter = new QAction("Fitting");
    aPlotFitter->setCheckable(true);
    aPlotFitter->setChecked(true);
    viewer.contextMenu()->addAction(aPlotFitter);
    connect(aPlotFitter, &QAction::triggered, this, [this, aPlotFitter]() {
        for (auto& fitgraph : { viewer.plot()->graph(2),viewer.plot()->graph(3) })
        {
            aPlotFitter->isChecked() ? fitgraph->setVisible(true) : fitgraph->setVisible(false);
        }
        for (auto& ax : { viewer.leftAxes()->axis(QCPAxis::atBottom),viewer.bottomAxes()->axis(QCPAxis::atLeft) })
        {
            aPlotFitter->isChecked() ? 0 : ax->setLabel(" ");
        }
        aPlotFitter->isChecked() ? imgCThread.toggleDoFitting(true) : imgCThread.toggleDoFitting(false);
        viewer.plot()->replot(); });

    QAction* aPlotFitter2D = new QAction("Fitting2D");
    aPlotFitter2D->setCheckable(true);
    aPlotFitter2D->setChecked(false);
    viewer.contextMenu()->addAction(aPlotFitter2D);
    connect(aPlotFitter2D, &QAction::triggered, this, [this, aPlotFitter2D]() {
        auto hairCurve = reinterpret_cast<QCPCurve*>(viewer.plot()->axisRect(1)->plottables().at(1));
        auto parametric = reinterpret_cast<QCPCurve*>(viewer.plot()->axisRect(1)->plottables().at(2));
        for (auto& fitgraph : { hairCurve,parametric })
        {
            aPlotFitter2D->isChecked() ? fitgraph->setVisible(true) : fitgraph->setVisible(false);
        }
        for (auto& ax : { viewer.centerAxes()->axis(QCPAxis::atTop) })
        {
            aPlotFitter2D->isChecked() ? 0 : ax->setLabel(" ");
        }
        aPlotFitter2D->isChecked() ? imgCThread.toggleDoFitting2D(true) : imgCThread.toggleDoFitting2D(false);
        viewer.plot()->replot(); });


    QAction* aCscale = new QAction("Color Scale");
    viewer.contextMenu()->addAction(aCscale);
    connect(aCscale, &QAction::triggered, this, [this]() {
        viewer.manualColorScaleDlg()->move(QCursor::pos());
        viewer.manualColorScaleDlg()->show(); });


    m_aManualCscale = new QAction("Manual Color Scale");
    m_aManualCscale->setCheckable(true);
    m_aManualCscale->setChecked(false);
    viewer.initManualColorRangeAction(m_aManualCscale);

    viewer.contextMenu()->addSeparator();
    QAction* aSaveImg = new QAction("Save Image");
    viewer.contextMenu()->addAction(aSaveImg);
    connect(aSaveImg, &QAction::triggered, [this]() {
        try {
            manualSaveImage();
        }
        catch (ChimeraError& e) {
            parentWin->reportErr("Capture image for saving failed \n" + e.qtrace());
        } });
}

void MakoCamera::releaseBuffer()
{
    core.releaseBuffer();
    imgCThread.StopProcessing();
}

void MakoCamera::acquisitionStartStopFromCtrler(const QString& sThisFeature)
{
    /* this is intended to stop and start the camera again since PixelFormat, Height and Width have been changed while camera running
     * ignore this when the changing has been made while camera not running */
    if ((0 == sThisFeature.compare("AcquisitionStart")) && !isCamRunning)
    {
        m_aStartStopCap->setChecked(true);
        acquisitionStartStopFromAction(); // only start if originally is not running, ignore if already running
    }
    else if (sThisFeature.contains("AcquisitionStop") && isCamRunning)
    {
        m_aStartStopCap->setChecked(false);
        acquisitionStartStopFromAction(); // only stop if originally is not running, ignore if already running
        if (!m_aStartStopCap->isEnabled())
            m_aStartStopCap->setEnabled(core.isStreamingAvailable());
    }
}

void MakoCamera::acquisitionStartStopFromAction()
{
    try {
        core.checkDisplayInterval();
        updateStatusBar();
    }
    catch (ChimeraError& e) {
        emit error(qstr("Error in trying to start Acquisition") + qstr(e.trace()));
    }
    /* ON */
    if (m_aStartStopCap->isChecked())
    {
        try {
            core.prepareCapture();
            core.startCapture();
        }
        catch (ChimeraError& e) {
            isCamRunning = false;
            imgCThread.StopProcessing();
            m_OperatingStatusLabel->setText("Error");
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");
            m_aStartStopCap->setChecked(false);
            emit IChimeraSystem::error(qstr("Failed to start! \n") + qstr(e.trace()));
            return;
        }
        // Do some GUI-related preparations before really starting (to avoid timing problems)
        m_OperatingStatusLabel->setText(" Running... ");
        m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,128, 0); color: rgb(255,255,255)");

        isCamRunning = true;
        imgCThread.StartProcessing();
    }
    /* OFF */
    else
    {
        try {
            core.stopCapture();
        }
        catch (ChimeraError& e) {
            m_OperatingStatusLabel->setText("Error");
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");
            emit IChimeraSystem::error(qstr("Failed to stop! \n") + qstr(e.trace()));
            return;
        }
        m_OperatingStatusLabel->setText(" Ready ");
        m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0, 0, 0); color: rgb(255,255,255)");
        
        isCamRunning = false;
        releaseBuffer();
    }
}


void MakoCamera::setCurrentScreenROI()
{
    auto [maxh, maxw] = core.getMakoCtrl().getMaxImageSize();
    QCPRange xr = viewer.centerAxes()->axis(QCPAxis::atBottom)->range();
    QCPRange yr = viewer.centerAxes()->axis(QCPAxis::atLeft)->range();
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
        
        resetFullROI(true);
        try {
            acquisitionStartStopFromCtrler("AcquisitionStop");
            Sleep(5);//give some time for it to shut down
            /*first reset the value to full*/
            core.setROI(xw, yw, xlower, ylower);
            acquisitionStartStopFromCtrler("AcquisitionStart");
        }
        catch (ChimeraError& e) {
            emit IChimeraSystem::error("Error in setting CMOS ROI \n" + qstr(e.trace()));
        }
        
        Sleep(50);
    }
    else {
        emit IChimeraSystem::warning("Warning in setting the ROI in CMOS camera, the values are out of bound");
    }

}

void MakoCamera::resetFullROI(bool notStartReStart)
{
    if (!notStartReStart)
    {
        acquisitionStartStopFromCtrler("AcquisitionStop");
    }
    core.resetFullROI();
    if (!notStartReStart)
    {
        acquisitionStartStopFromCtrler("AcquisitionStart");
    }
}

void MakoCamera::manualSaveImage()
{
    // make a copy of the images before save-as dialog appears (image can change during time dialog open)
    QVector<double> imgSave;
    imgSave = std::move(imgCThread.rawImageDefinite());
    //imgSave = QVector<double>(m_pImgCThread->rawImage().begin(), m_pImgCThread->rawImage().end());
    auto [imgWidth, imgHeight] = imgCThread.WidthHeight();


    QString     fileExtension;
    bool        isImageAvailable = true;

    if (imgSave.isEmpty())
    {
        isImageAvailable = false;
    }
    else
    {
        if (nullptr != saveFileDialog)
        {
            delete saveFileDialog;
            saveFileDialog = nullptr;
        }

        fileExtension = "*.pdf ;; *.csv";
        saveFileDialog = new QFileDialog(this, "Save Image", qstr(""/*DATA_SAVE_LOCATION*/), fileExtension);
        saveFileDialog->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);
        saveFileDialog->selectNameFilter("*.csv");
        saveFileDialog->setViewMode(QFileDialog::Detail);
        saveFileDialog->setAcceptMode(QFileDialog::AcceptSave);

        if (saveFileDialog->exec())
        {   //OK
            QString selectedExtension = saveFileDialog->selectedNameFilter();
            QString saveFileDir = saveFileDialog->directory().absolutePath();
            QStringList files = saveFileDialog->selectedFiles();

            if (!files.isEmpty())
            {
                QString fileName = files.at(0);

                bool saved = false;

                if (!fileName.endsWith(selectedExtension.section('*', -1)))
                {
                    fileName.append(selectedExtension);
                }
                if (0 == selectedExtension.compare("*.pdf"))
                {
                    saved = viewer.plot()->savePdf(fileName, 0, 0, QCP::epNoCosmetic);
                }
                else if (0 == selectedExtension.compare("*.csv"))
                {
                    QFile file(fileName);
                    if (file.open(QIODevice::ReadWrite))
                    {
                        QTextStream stream(&file);
                        for (size_t i = 0; i < imgHeight; i++)
                        {
                            for (size_t j = 0; j < imgWidth; j++)
                            {
                                stream << imgSave[i * imgWidth + j];
                                j == imgWidth - 1 ? stream << endl : stream << ",";
                            }
                        }
                    }
                    saved = true;
                }

                if (true == saved)
                {
                    QMessageBox::information(this, tr("Vimba Viewer"), tr("Image: ") + fileName + tr(" saved successfully"));
                }
                else
                {
                    QMessageBox::warning(this, tr("Vimba Viewer"), tr("Error saving image"));
                }

            }
        }
    }

    if (!isImageAvailable)
    {
        QMessageBox::warning(this, tr("Vimba Viewer"), tr("No image to save"));
    }
}

void MakoCamera::updateStatusBar()
{
    //if (camInfo.safemode) {
    //    return;
    //}
    core.updateCurrentSettings();
    MakoSettings ms = core.getRunningSettings();
    m_expActive->setChecked(ms.expActive);
    m_picsPerRep->setValue(ms.picsPerRep);
    core.setExpActive(ms.expActive);
    core.setPicsPerRep(ms.picsPerRep);
    imgCThread.setExpActive(ms.expActive);

    //m_FormatButton->setText("Pixel Format: " + qstr(imgCThread.format()) + " ");
    //auto [w, h] = imgCThread.WidthHeight();
    //QMouseEvent event(QMouseEvent::None, imgCThread.mousePos(), Qt::NoButton, 0, 0);
    //viewer.onSetMousePosInCMap(&event, m_CursorScenePosLabel);
    //imgCThread.updateExposureTime();
    //imgCThread.updateCameraGain();
    m_ImageSizeButtonX->setText("X: " + qstr(ms.dims.left));
    m_ImageSizeButtonY->setText("Y: " + qstr(ms.dims.bottom));
    m_ImageSizeButtonH->setText("Size H: " + qstr(ms.dims.height()));
    m_ImageSizeButtonW->setText(",W: " + qstr(ms.dims.width()) + " ");
    m_ImageBinButtonH->setText("Bin H: " + qstr(ms.dims.verticalBinning));
    m_ImageBinButtonW->setText(",W: " + qstr(ms.dims.horizontalBinning) + " ");
    m_ExposureTimeButton->setText("Exposure time (ms): " + qstr(ms.exposureTime / 1.0e3, 3));
    m_CameraGainButton->setText("Gain (dB): " + qstr(ms.rawGain, 0));
    m_TrigOnOffButton->setText(ms.trigOn ? "Trig: On" : "Trig: Off");
    m_TrigSourceButton->setText("TrigSource: " + qstr(MakoTrigger::toStr(ms.triggerMode)));

}

//void MakoCamera::prepareForExperiment()
//{
//    currentRepNumber = 0;
//    isExpRunning = true;
//    if (core.getRunningSettings().picsPerRep <= 0) {
//        thrower("The CMOS camera " + CameraInfo::toStr(camInfo.camName) + " is set to be Experiment Active but the Picture-Per-Repetion is " + str(core.getRunningSettings().picsPerRep)
//            + " which is not greater than or equal to 1");
//    }
//    acquisitionStartStopFromCtrler("AcquisitionStart");
//}

void MakoCamera::setMOTCalcActive(bool active)
{
    MOTCalcActive = active;
}

void MakoCamera::handleExpImage(QVector<double> img, int width, int height)
{
    //if (!isExpRunning) {
    //    return;
    //}
    //try {
    //    auto* andorWin = parentWin->andorWin;
    //    currentRepNumber++;
    //    m_OperatingStatusLabel->setText("Exp Running" + qstr(currentRepNumber));
    //    if (core.getRunningSettings().triggerMode == CMOSTrigger::mode::ContinuousSoftware) {
    //        // don't write data if continuous, that's a recipe for disaster.
    //        emit error("Mako camera mode is continuous, such high data rate is hard to write to "
    //            "disk continously. Double check if this is what you need.\n");
    //        return;
    //    }
    //    if (MOTCalcActive) {
    //        emit imgReadyForAnalysis(img, width, height, currentRepNumber - 1);
    //    }
    //    andorWin->getLogger().writeMakoPic(img.toStdVector(), width, height, camInfo.camName);

    //    if (currentRepNumber == core.getRunningSettings().totalPictures()) {
    //        // handle mako finish
    //        isExpRunning = false;
    //        imgCThread.experimentFinished();
    //        m_OperatingStatusLabel->setText("Exp Finished");
    //        // tell the andor window that the basler camera finished so that the data file can be handled appropriately.
    //        //mainWin->getComm ()->sendBaslerFin ();
    //        parentWin->makoWin->CMOSChkFinished();
    //    }
    //}
    //catch (ChimeraError& err) {
    //    emit error(err.qtrace());
    //    m_OperatingStatusLabel->setText("Exp ERROR!");
    //}
}



#include "stdafx.h"
#include "PictureViewer.h"
#include <tuple>
#include <filesystem>
#include <utility>
#include <csvReader.h>


PictureViewer::PictureViewer(std::string plotname, QWidget* parent)
    : QWidget(parent)
{
    /*QCP viewer widget: colormap + side/bottom plot*/
    m_QCP = QSharedPointer<QCustomPlot>(new QCustomPlot());
    m_QCP->plotLayout()->clear();
    m_QCPcenterAxisRect = new QCPAxisRect(m_QCP.data());
    m_QCPbottomAxisRect = new QCPAxisRect(m_QCP.data());
    m_QCPleftAxisRect = new QCPAxisRect(m_QCP.data());
    m_QCP->plotLayout()->addElement(0, 1, m_QCPcenterAxisRect);
    m_QCP->plotLayout()->addElement(0, 0, m_QCPleftAxisRect);
    m_QCP->plotLayout()->addElement(1, 1, m_QCPbottomAxisRect);
    /*note the index of the axrect is labeled with the position, i.e. left=0,cter=1,bottom=2, ignore cbar*/



    m_QCPcenterAxisRect->setupFullAxesBox(true);
    for (auto& plt : { m_QCPleftAxisRect, m_QCPbottomAxisRect })
    {
        plt->setupFullAxesBox(false);
    }
    m_QCPleftAxisRect->axis(QCPAxis::atRight)->setTickLabels(true);
    m_QCPbottomAxisRect->axis(QCPAxis::atTop)->setTickLabels(true);

    /*colorbar*/
    m_colorScale = new QCPColorScale(m_QCP.data());
    m_colorScale->setType(QCPAxis::atRight);
    m_colorScale->setRangeZoom(false);
    m_colorScale->setRangeDrag(false);
    m_QCP->plotLayout()->addElement(0, 2, m_colorScale);

    /*set alignment of three plots + 1 colorbar*/
    QCPMarginGroup* marginGroup = new QCPMarginGroup(m_QCP.data());
    m_QCPbottomAxisRect->setMarginGroup(QCP::msLeft | QCP::msRight, marginGroup);
    m_QCPleftAxisRect->setMarginGroup(QCP::msTop | QCP::msBottom, marginGroup);
    m_QCPcenterAxisRect->setMarginGroup(QCP::msAll, marginGroup);
    m_colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

    m_QCPbottomAxisRect->setMinimumSize(600, 120);
    m_QCPbottomAxisRect->setMaximumSize(600, 120);
    m_QCPleftAxisRect->setMinimumSize(120, 600);
    m_QCPleftAxisRect->setMaximumSize(120, 600);
    //QRect rec = QApplication::desktop()->screenGeometry();
    m_QCP->setMaximumSize(960, 900);

    // move newly created axes on "axes" layer and grids on "grid" layer:
    foreach(QCPAxisRect * rect, m_QCP->axisRects())
    {
        foreach(QCPAxis * axis, rect->axes())
        {
            axis->setLayer("axes");
            axis->grid()->setLayer("grid");
        }
    }

    /***********************************************************************/
    /*color map and cmap gradient*/
    m_colorMap = QSharedPointer<QCPColorMap>(new QCPColorMap(
        m_QCPcenterAxisRect->axis(QCPAxis::atBottom),
        m_QCPcenterAxisRect->axis(QCPAxis::atLeft)));
    m_colorMap->setInterpolate(false);
    m_colorMap->setColorScale(m_colorScale);
    //m_colorMap->setGradient(QCPColorGradient::gpGrayscale);
    //m_colorMap->valueAxis()->setTickLabelPadding(0);
    m_dCScale = new QDialog(this);
    m_cmapCombo = new QComboBox(m_dCScale);
    {
        m_dCScale->setWindowFlags(m_dCScale->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        m_dCScale->setWindowTitle("Change color map");
        m_dCScale->setMinimumWidth(300);
        auto layout = new QVBoxLayout(m_dCScale);
        layout->addWidget(m_cmapCombo, 1);
        auto layout1 = new QHBoxLayout();
        layout1->setContentsMargins(0, 0, 0, 0);
        layout1->addStretch(1);
        auto reloadB = new QPushButton("Reload Manual CMap");
        layout1->addWidget(reloadB);
        layout->addLayout(layout1, 0);
        connect(reloadB, &QPushButton::clicked, this, [this]() {
            loadColorCsv();
            std::for_each(m_cmapMap.keyValueBegin(), m_cmapMap.keyValueEnd(), [this](auto tmp) {
                if (m_cmapCombo->findText(tmp.second) == -1)
                {
                    m_cmapCombo->addItem(tmp.second);
                } });
            });
    }

    loadColorCsv();
    std::for_each(m_cmapMap.keyValueBegin(), m_cmapMap.keyValueEnd(), [this](auto tmp)
        {m_cmapCombo->addItem(tmp.second); });

    if (m_cmapMap.values().contains("inferno"))
    {
        auto tt = m_cmapMap.key("inferno");
        m_colorScale->setGradient(m_colorgradient.at(tt));
        m_cmapCombo->setCurrentIndex(tt);
    }
    else
    {
        m_colorScale->setGradient(QCPColorGradient::gpGrayscale);
        m_cmapCombo->setCurrentIndex(0);
    }

    connect(m_cmapCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int i) {
        m_colorScale->setGradient(m_colorgradient.at(i)); });

    /****************************************************************************/
    /*bottom/left graph and center parametric curve*/
    m_bottomGraph = QSharedPointer<QCPGraph>(
        new QCPGraph(m_QCPbottomAxisRect->axis(QCPAxis::atTop),
            m_QCPbottomAxisRect->axis(QCPAxis::atLeft)));
    m_leftGraph = QSharedPointer<QCPGraph>(
        new QCPGraph(m_QCPleftAxisRect->axis(QCPAxis::atRight),
            m_QCPleftAxisRect->axis(QCPAxis::atBottom)));
    m_leftGraph->valueAxis()->setTickLabelRotation(90);
    /*add axis for fitted curve: index=2 is for bot, index=3 is for left*/
    m_QCP->addGraph(m_bottomGraph->keyAxis(), m_bottomGraph->valueAxis());
    m_QCP->addGraph(m_leftGraph->keyAxis(), m_leftGraph->valueAxis());
    QCPCurve* hairCurve = new QCPCurve(m_colorMap->keyAxis(), m_colorMap->valueAxis());/*for two cross hairs, plottable(1)*/
    QCPCurve* parametricCurve = new QCPCurve(m_colorMap->keyAxis(), m_colorMap->valueAxis()); /*for parametric ellipse, plottable(2)*/

    qDebug() << m_QCP->axisRect(1)->plottables().at(2) << parametricCurve;

    /*set pen for all graph*/
    {
        QPen pen;
        pen.setStyle(Qt::DotLine);
        pen.setWidth(3);
        pen.setColor(QColor(230, 0, 0));
        m_QCP->graph(2)->setPen(pen);
        m_QCP->graph(3)->setPen(pen);
        pen.setWidth(4);
        pen.setColor(QColor(26, 255, 26));
        parametricCurve->setPen(pen);
        hairCurve->setPen(pen);
    }
    {
        QPen pen;
        pen.setStyle(Qt::SolidLine);
        pen.setWidth(2);
        pen.setColor(QColor(97, 53, 242));
        m_QCP->graph(0)->setPen(pen);
        m_QCP->graph(1)->setPen(pen);
    }

    m_QCP->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    m_QCPbottomAxisRect->setRangeDragAxes(m_bottomGraph->keyAxis(), m_bottomGraph->valueAxis());
    m_QCPbottomAxisRect->setRangeZoomAxes(m_bottomGraph->keyAxis(), m_bottomGraph->valueAxis());
    m_QCPleftAxisRect->setRangeDragAxes(m_leftGraph->valueAxis(), m_leftGraph->keyAxis());
    m_QCPleftAxisRect->setRangeZoomAxes(m_leftGraph->valueAxis(), m_leftGraph->keyAxis());
    for (auto& p : { m_QCPbottomAxisRect ,m_QCPleftAxisRect ,m_QCPcenterAxisRect })
    {
        p->setRangeZoomFactor(0.95);
    }

    // setup a ticker for colormap that only gives integer ticks:
    QSharedPointer<QCPAxisTickerFixed> intTicker(new QCPAxisTickerFixed);
    intTicker->setTickStep(1.0);
    intTicker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
    m_colorMap->keyAxis()->setTicker(intTicker);
    m_colorMap->valueAxis()->setTicker(intTicker);
    m_leftGraph->keyAxis()->setTicker(intTicker);
    m_bottomGraph->keyAxis()->setTicker(intTicker);


    /***********************************************************************/
    /*tracer for the bottom and left plot*/
    m_QCPtracerbottom = new QCPItemTracer(m_QCP.data());
    m_QCPtracerbottom->setClipAxisRect(m_bottomGraph->keyAxis()->axisRect());
    m_QCPtracerbottom->setGraph(m_bottomGraph.data());
    m_QCPtracerleft = new QCPItemTracer(m_QCP.data());
    m_QCPtracerleft->setClipAxisRect(m_leftGraph->keyAxis()->axisRect());
    m_QCPtracerleft->setGraph(m_leftGraph.data());
    for (auto& tracer : { m_QCPtracerbottom ,m_QCPtracerleft })
    {
        tracer->setInterpolating(false);
        tracer->setStyle(QCPItemTracer::tsCircle);
        tracer->setPen(QPen(Qt::red));
        tracer->setBrush(Qt::red);
        tracer->setSize(8);
    }

    m_QCPtraceTextbottom = new QCPItemText(m_QCP.data());
    m_QCPtraceTextbottom->position->setParentAnchor(m_QCPtracerbottom->position);
    m_QCPtraceTextbottom->setClipAxisRect(m_QCPbottomAxisRect);
    m_QCPtraceTextbottom->position->setCoords(0, 12);
    m_QCPtraceTextleft = new QCPItemText(m_QCP.data());
    m_QCPtraceTextleft->position->setParentAnchor(m_QCPtracerleft->position);
    m_QCPtraceTextleft->setClipAxisRect(m_QCPleftAxisRect);
    m_QCPtraceTextleft->setRotation(90);
    m_QCPtraceTextleft->position->setCoords(-12, 0);
    
    //set font size
    QFont vfont = QFont("Times", 12);
    QFont vfonts = QFont("Times", 10);
    m_QCPbottomAxisRect->axis(QCPAxis::atBottom)->setLabelFont(vfont);
    m_QCPleftAxisRect->axis(QCPAxis::atLeft)->setLabelFont(vfont);
    m_QCPcenterAxisRect->axis(QCPAxis::atTop)->setLabelFont(vfont);
    m_QCPtraceTextbottom->setFont(vfonts);
    m_QCPtraceTextleft->setFont(vfonts);
    m_colorScale->axis()->setTickLabelFont(vfont);
    for (auto* p : m_QCP->axisRects()) {
        for (auto* pp : p->axes()) {
            pp->setTickLabelFont(vfont);
        }
    }


    //connect(m_QCP.data(), &QCustomPlot::mouseMove, this, &PictureViewer::onSetMousePosInCMap);
    //connect(m_QCP.data(), &QCustomPlot::mouseDoubleClick, this, [this]() {
    //    m_pImgCThread->setDefaultView();
    //    m_QCP->replot(); });


    /***********************************************************************/
    /*manual range silder*/
    //handleManualColorRange();
    /***********************************************************************/
    /*set image layout*/
    QVBoxLayout* m_VertLayout = new QVBoxLayout(this);
    m_VertLayout->setContentsMargins(0, 0, 0, 0);
    //QLabel* namelabel = new QLabel(qstr(plotname));
    //namelabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    //m_VertLayout->addWidget(namelabel, 0);
    m_VertLayout->addWidget(m_QCP.data(), 1);
    //this->setStyleSheet("background-color: rgb(85, 100, 100)");
    //this->setWindowFlags(Qt::Widget);

    /***********************************************************************/
    // m_controller
    /***********************************************************************/
    /*create context menu*/
    m_QCP->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ContextMenu = new QMenu(this);
    connect(m_QCP.data(), &QCustomPlot::customContextMenuRequested, this, [this](QPoint) {
      m_ContextMenu->exec(QCursor::pos()); });
    /***********************************************************************/
    /* create FrameObserver to get frames from camera, add for QCPColorMap */
    /***********************************************************************/
    /*create image calculating thread*/
    /***********************************************************************/
    /* Statusbar */
    /***********************************************************************/

    //setMaximumSize(960, 600);

}

PictureViewer::~PictureViewer()
{
    m_QCP->~QCustomPlot();
}

// m_QCP replot should be taken care of manually depending on whether camera is running
// the info is set to the argument 'label'
void PictureViewer::onSetMousePosInCMap(QMouseEvent* event, QLabel* label)
{
    //or also use m_colorMap->keyAxis(), they are the same 
    double x = m_QCPcenterAxisRect->axis(QCPAxis::atBottom)->pixelToCoord(event->pos().x());
    double y = m_QCPcenterAxisRect->axis(QCPAxis::atLeft)->pixelToCoord(event->pos().y());
    //double z = m_colorMap->data()->data(x, y);
    //qDebug() << x << "," << std::floor(x + 0.5) << "," << y << "," << std::floor(y + 0.5) << "," << z;
    label->setText("(" + QString::number(std::floor(x + 0.5)) + " , " +
        QString::number(std::floor(y + 0.5)) + " , " +
        QString::number(m_colorMap->data()->data(x, y)) + ")");

    /*tracer part*/
    {
        double bottomKey = m_bottomGraph->keyAxis()->pixelToCoord(event->pos().x());
        m_QCPtracerbottom->setGraphKey(bottomKey);
        bottomKey = m_bottomGraph->keyAxis()->pixelToCoord(m_QCPtracerbottom->position->pixelPosition().x());
        /*the y value is more self-contained than obtaining it from imgCThread, which depends on the availability of m_Crx*/
        double bottomVal = m_bottomGraph->valueAxis()->pixelToCoord(m_QCPtracerbottom->position->pixelPosition().y());

        m_QCPtraceTextbottom->setText(QString::number(bottomKey) +
            "(" + QString::number(bottomKey - m_bottomGraph->dataMainKey(0)) + ")" + "," +
            QString::number(bottomVal, 'e', 3));
    }
    {
        double leftKey = m_leftGraph->keyAxis()->pixelToCoord(event->pos().y());
        m_QCPtracerleft->setGraphKey(leftKey);
        leftKey = m_leftGraph->keyAxis()->pixelToCoord(m_QCPtracerleft->position->pixelPosition().y());
        /*the y value is more self-contained than obtaining it from imgCThread, which depends on the availability of m_Crx*/
        double leftVal = m_leftGraph->valueAxis()->pixelToCoord(m_QCPtracerleft->position->pixelPosition().x());

        m_QCPtraceTextleft->setText(QString::number(leftKey) +
            "(" + QString::number(leftKey - m_leftGraph->dataMainKey(0)) + ")" + "," +
            QString::number(leftVal, 'e', 3));
    }
}

void PictureViewer::initManualColorRangeAction(QAction* mcbarRange)
{
    m_DiagRSlider = new QDialog(this);
    m_DiagRSlider->setWindowTitle("Color Scale Slider");
    m_DiagRSlider->setWindowFlags(m_DiagRSlider->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_DiagRSlider->setMaximumSize(300, 700);

    QVBoxLayout* layout = new QVBoxLayout(m_DiagRSlider);
    m_DiagRSlider->setLayout(layout);
    intgSlider = new RangeSliderIntg(Qt::Vertical, RangeSlider::Option::DoubleHandles);
    layout->addWidget(intgSlider);
    intgSlider->setRange(0, 4095);
    connect(intgSlider, &RangeSliderIntg::smlValueChanged, this, [this](int val) {
        m_colorScale->setDataRange(QCPRange(val, m_colorScale->dataRange().upper));
        /*m_QCP->replot();*/ });
    connect(intgSlider, &RangeSliderIntg::lrgValueChanged, this, [this](int val) {
        m_colorScale->setDataRange(QCPRange(m_colorScale->dataRange().lower, val));
        /*m_QCP->replot();*/ });


    connect(m_QCP.data(), &QCustomPlot::axisDoubleClick, this, 
        [this, mcbarRange](QCPAxis* axis, QCPAxis::SelectablePart part) {
        if (axis == m_colorScale->axis() && mcbarRange->isChecked())
        {
            m_DiagRSlider->move(QCursor::pos());
            m_DiagRSlider->show();
        } });
    m_ContextMenu->addAction(mcbarRange);
}

void PictureViewer::handleContextMenu()
{

}



void PictureViewer::loadColorCsv()
{
    size_t counter = m_cmapMap.size();
    if (counter == 0)
    {
        m_cmapMap.insert(counter, "grayScale");
        m_colorgradient.insert(counter, std::move(QCPColorGradient(QCPColorGradient::gpGrayscale)));
        counter++;
    }
    for (auto& p : std::filesystem::directory_iterator("./"))
    {
        if (p.path().extension() == ".csv")
        {
            QString filename = QString::fromStdWString(p.path().filename().c_str()).section('-', 0, 0);
            csvReader tmpCSV(p.path().c_str());

            if (!tmpCSV.isSuccess())
            {
                thrower("Failed loading colorscale from " + str(p.path()) + "failed");
                continue;
            }
            else if (m_cmapMap.values().contains(filename)) { continue; }
            auto tmpData = tmpCSV.getDataDouble();
            auto tmpCG = QCPColorGradient();
            tmpCG.setColorInterpolation(QCPColorGradient::ciRGB);
            for (size_t i = 0; i < tmpData[0].size(); i++)
            {
                tmpCG.setColorStopAt(tmpData[0][i], QColor(tmpData[1][i], tmpData[2][i], tmpData[3][i]));
            }
            m_cmapMap.insert(counter, filename);
            m_colorgradient.insert(counter, std::move(tmpCG));
            counter++;
        }
    }

}

void PictureViewer::renderImgFromCalcThread(bool manualColorScale)
{
    if (!manualColorScale)
    {
        m_colorMap->rescaleDataRange(true);
        intgSlider->upperSpinBox()->setValue(m_colorScale->dataRange().upper);
        intgSlider->lowerSpinBox()->setValue(m_colorScale->dataRange().lower);
    }
    m_QCPcenterAxisRect->axis(QCPAxis::atLeft)->setScaleRatio(m_QCPcenterAxisRect->axis(QCPAxis::atBottom), 1.0);
    m_bottomGraph->rescaleValueAxis(true, true); //only enlarge y and scale corresponde to visible x
    m_bottomGraph->keyAxis()->setRange(m_colorMap->keyAxis()->range());
    m_leftGraph->rescaleValueAxis(true, true);
    m_leftGraph->keyAxis()->setRange(m_colorMap->valueAxis()->range());

    /*set the secondary relative axis, now replaced with connect rangechanged*/
    //m_QCPleftAxisRect->axis(QCPAxis::atLeft)->setRange(m_leftGraph->keyAxis()->range() - m_leftGraph->data()->at(0)->key);
    //m_QCPbottomAxisRect->axis(QCPAxis::atBottom)->setRange(m_bottomGraph->keyAxis()->range() - m_bottomGraph->data()->at(0)->key);
    m_QCP->replot();


}




#include <QLayout>
#include "Histogram.h"

#include "./ExternLib/qwt/qwt_scale_engine.h"
#include "./ExternLib/qwt/qwt_point_data.h"
#include "./ExternLib/qwt/qwt_plot_canvas.h"



class HistogramSource : public QwtSyntheticPointData
{
public:
    HistogramSource(const QVector<quint32>& h, int maxValue = 0, int offset = 0)
        : QwtSyntheticPointData(h.size(), QwtInterval(0, h.size()-1)),
        mMaxValue(maxValue), mOffset(offset)
    {
        mData = h;
    }
    
    virtual QPointF sample( size_t index ) const
    {
        return QPointF((qreal)x(index), (qreal)y(x(index)));
    }

    virtual double y(double x) const
    {
        return mData[(int)x];
    }

    virtual double x( uint index ) const 
    { 
        return (index >= 0 && index < size() ? index : 0); 
    }

protected:
    QVector<quint32> mData;
    double mMaxValue;      //if zero do not normalize Y to scale of mMaxValue
    double mOffset;
};


Histogram::Histogram(const char* name, int maxValueX, QWidget *parent): QwtPlot(parent), 
                                                                        m_Mono(NULL),
                                                                        m_Red(NULL),
                                                                        m_Green(NULL),
                                                                        m_Blue(NULL),
                                                                        m_MonoCurve(NULL), 
                                                                        m_RedCurve(NULL), 
                                                                        m_GreenCurve(NULL), 
                                                                        m_BlueCurve(NULL),
                                                                        m_YCurve(NULL),
                                                                        m_UCurve(NULL),
                                                                        m_VCurve(NULL),
                                                                        m_Y(NULL),
                                                                        m_U(NULL),
                                                                        m_V(NULL),
                                                                        m_MaxValueX(maxValueX),
                                                                        m_MaxValueY(0),
                                                                        m_dYAxisMaximum(0)
{
    setAutoReplot(false);

    // panning with the left mouse button
    (void) new QwtPlotPanner( canvas() );

    // zoom in/out with the wheel
    (void) new QwtPlotMagnifier( canvas() );

    setAutoFillBackground( true );

    setTitle(name);
    insertLegend(new QwtLegend(), QwtPlot::RightLegend);

    // grid 
    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMajorPen(QPen(Qt::black, 0, Qt::DotLine));
    grid->setMinorPen(QPen(Qt::gray, 0 , Qt::DotLine));
    grid->attach(this);

    // axes
    //setAxisScale(xBottom, 0.0, mMaxValueX);
    //setAxisScale(yLeft, 0.0, 1400000.0);
    setAxisAutoScale(xBottom, true);
    setAxisAutoScale(yLeft, true);
    
    //// canvas
    QwtPlotCanvas *plotCanvas = new QwtPlotCanvas();
    plotCanvas->setBorderRadius( 5 );
    plotCanvas->setLineWidth(1); 
    plotCanvas->setFrameStyle(QFrame::Box | QFrame::Plain );
    setCanvas(plotCanvas); 

    QPalette canvasPalette( Qt::white );
    canvasPalette.setColor( QPalette::Foreground, QColor( 133, 190, 232 ) );
    canvas()->setPalette( canvasPalette );

    // Insert new curves
    QPen penMono(Qt::black);
    penMono.setWidth(2);
    m_MonoCurve = new QwtPlotCurve(tr("mono"));
    m_MonoCurve->setStyle(QwtPlotCurve::Lines);
    //m_MonoCurve->setBrush(Qt::black);
    m_MonoCurve->setPen(penMono);
    m_MonoCurve->setYAxis(QwtPlot::yLeft);
    m_MonoCurve->attach(this);

    QPen penRed(Qt::red);
    penRed.setWidth(2);
    m_RedCurve = new QwtPlotCurve(tr("red"));
    //m_RedCurve->setBrush(Qt::red);
    m_RedCurve->setStyle(QwtPlotCurve::Lines);
    m_RedCurve->setPen(penRed);
    m_RedCurve->setYAxis(QwtPlot::yLeft);
    m_RedCurve->attach(this);

    QPen penGreen(Qt::green);
    penGreen.setWidth(2);
    m_GreenCurve = new QwtPlotCurve(tr("green"));
    //m_GreenCurve->setBrush(Qt::green);
    m_GreenCurve->setStyle(QwtPlotCurve::Lines);
    m_GreenCurve->setPen(penGreen);
    m_GreenCurve->setYAxis(QwtPlot::yLeft);
    m_GreenCurve->attach(this);

    QPen penBlue(Qt::blue);
    penBlue.setWidth(2);
    m_BlueCurve = new QwtPlotCurve(tr("blue"));
    //m_BlueCurve->setBrush(Qt::blue);
    m_BlueCurve->setStyle(QwtPlotCurve::Lines);    
    m_BlueCurve->setPen(penBlue);
    m_BlueCurve->setYAxis(QwtPlot::yLeft);
    m_BlueCurve->attach(this);

    QPen penYellow(Qt::darkYellow);
    penYellow.setWidth(2);
    m_YCurve = new QwtPlotCurve(tr("Y"));
    //m_YCurve->setBrush(Qt::darkYellow);
    m_YCurve->setStyle(QwtPlotCurve::Lines);
    m_YCurve->setPen(penYellow);
    m_YCurve->setYAxis(QwtPlot::yLeft);
    m_YCurve->attach(this);

    QPen penCyan(Qt::darkCyan);
    penCyan.setWidth(2);
    m_UCurve = new QwtPlotCurve(tr("U"));
    //m_UCurve->setBrush(Qt::darkCyan);
    m_UCurve->setStyle(QwtPlotCurve::Lines);
    m_UCurve->setPen(penCyan);
    m_UCurve->setYAxis(QwtPlot::yLeft);
    m_UCurve->attach(this);

    QPen penMagenta(Qt::darkMagenta);
    penMagenta.setWidth(2);
    m_VCurve = new QwtPlotCurve(tr("V"));
    //m_VCurve->setBrush(Qt::darkMagenta);
    m_VCurve->setStyle(QwtPlotCurve::Lines);
    m_VCurve->setPen(penMagenta);
    m_VCurve->setYAxis(QwtPlot::yLeft);
    m_VCurve->attach(this);

    // picker 
    m_PlotPicker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft, QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn, canvas());
    m_PlotPicker->setStateMachine(new QwtPickerDragPointMachine());
    m_PlotPicker->setRubberBandPen(QColor( 128, 0, 0 ));
    m_PlotPicker->setRubberBand(QwtPicker::CrossRubberBand);
    m_PlotPicker->setTrackerPen(QColor( 128, 0, 0 ));
    
    setAutoReplot(true);
}

Histogram::~Histogram()
{
}

void Histogram::resizeEvent(QResizeEvent *event)
{
    QwtPlot::resizeEvent( event );
}

void Histogram::populate( const QVector<quint32> &histogramData, const QString &sColorComponent )
{
    const bool doReplot = autoReplot();
    setAutoReplot(false);

    if( 0 == sColorComponent.compare("M") )
    {
        m_Mono = new HistogramSource(histogramData, m_dYAxisMaximum, 0);
        m_MonoCurve->setData(m_Mono);
    }

    if( 0 == sColorComponent.compare("R") )
    {
        m_Red = new HistogramSource(histogramData, m_dYAxisMaximum, 0); 
        m_RedCurve->setData(m_Red);
    }

    if( 0 == sColorComponent.compare("G") )
    {
        m_Green = new HistogramSource(histogramData, m_dYAxisMaximum, 0); 
        m_GreenCurve->setData(m_Green);
    }

    if( 0 == sColorComponent.compare("B") )
    {
        m_Blue = new HistogramSource(histogramData, m_dYAxisMaximum, 0); 
        m_BlueCurve->setData(m_Blue);
    }

    if( 0 == sColorComponent.compare("Y") )
    {
        m_Y = new HistogramSource(histogramData, m_dYAxisMaximum, 0); 
        m_YCurve->setData(m_Y);
    }

    if( 0 == sColorComponent.compare("U") )
    {
        m_U = new HistogramSource(histogramData, m_dYAxisMaximum, 0); 
        m_UCurve->setData(m_U);
    }
        
    if( 0 == sColorComponent.compare("V") )
    {
        m_V = new HistogramSource(histogramData, m_dYAxisMaximum, 0); 
        m_VCurve->setData(m_V);
    }

    setAutoReplot(doReplot);
    replot();    
}

void Histogram::setHistogramTitle ( const QString &sTitle )
{
    setTitle (sTitle);
}

void Histogram::setLeftAxisY ( const double &dMaximum )
{
    //reset pen
    QPen penBlack(Qt::black);
    penBlack.setWidth(1);
    m_MonoCurve->setPen(penBlack);
    m_RedCurve->setPen(penBlack);
    m_GreenCurve->setPen(penBlack);
    m_BlueCurve->setPen(penBlack);
    setAxisScale(yLeft, 0.0, dMaximum ); 
    m_dYAxisMaximum = dMaximum;
}

void Histogram::setBottomAxisX ( const double &dMaximum )
{
    //set pen to default
    QPen penBlack(Qt::black);
    penBlack.setWidth(2);
    m_MonoCurve->setPen(penBlack);
    
    QPen penRed(Qt::red);
    penRed.setWidth(2);
    m_RedCurve->setPen(penRed);

    QPen penGreen(Qt::green);
    penGreen.setWidth(2);
    m_GreenCurve->setPen(penGreen);

    QPen penBlue(Qt::blue);
    penBlue.setWidth(2);
    m_BlueCurve->setPen(penBlue);

    setAxisScale(xBottom, 0.0, dMaximum ); 
}



#include "stdafx.h"
#include "TickSlider.h"
#include <QPainter>
#include <QStyleOptionSlider>
#include <QStyle>
#include <QStylePainter>

TickSlider::TickSlider(QWidget* parent)
    : QSlider(parent)
{
    
}

TickSlider::TickSlider(Qt::Orientation ori, QWidget* parent)
    : QSlider(ori, parent)
{
    
}

TickSlider::~TickSlider()
{

}

void TickSlider::setOffset(int x, int y)
{
    offsetx = x;
    offsety = y;
}

void TickSlider::paintEvent(QPaintEvent* e)
{
    if (tickPosition() == NoTicks)
    {
        QSlider::paintEvent(e);
        return;
    }

    QStylePainter p1(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    if (orientation() == Qt::Vertical)
    {
        opt.rect.adjust(15, 0, 0, 0);
        if (m_firstWidth == 0)
            m_firstWidth = width();
        setMinimumWidth(m_firstWidth * 2);
    }

    // draw the slider (this is basically copy/pasted from QSlider::paintEvent)
    opt.subControls = QStyle::SC_SliderTickmarks;
    p1.drawComplexControl(QStyle::CC_Slider, opt);


    opt.subControls = QStyle::SC_SliderGroove;
    p1.drawComplexControl(QStyle::CC_Slider, opt);

    // draw the slider handle
    opt.subControls = QStyle::SC_SliderHandle;
    p1.drawComplexControl(QStyle::CC_Slider, opt);

    int rightLast = 0;
    QRect rr;
    p1.drawText(rect(), Qt::TextDontPrint, QString::number(9999), &rr);

    for (int v = this->minimum(); v <= this->maximum(); v += tickInterval()) {

        QStyle* st = style();
        QStyleOptionSlider slider;
        slider.init(this);
        slider.orientation = orientation();
        int available = st->pixelMetric(QStyle::PM_SliderSpaceAvailable, &slider, this);

        int len = st->pixelMetric(QStyle::PM_SliderLength, &slider, this);
        int left = QStyle::sliderPositionFromValue(minimum(), maximum(), v, available) + len;
        QString vs = QString("%L1").arg(v);
        if (qAbs(maximum() - v) < tickInterval())
            left -= 2 * len;

        QPoint pos(left, rect().bottom());
        if (orientation() == Qt::Vertical)
        {
            pos = QPoint(rect().left(), left + len);
            vs = QString("%L1").arg(this->minimum() - v + this->maximum());
        }
        QRect boundingRect;
        if (left < available && rightLast <= left - len)
        {
            rr.setX(pos.x());
            rr.setY(pos.y());
            p1.drawText(rr, Qt::TextDontPrint, vs, &boundingRect);
            pos.rx() -= offsetx;
            pos.ry() += offsety;
            p1.drawText(pos, vs);
        }
        rightLast = left + boundingRect.width();

    }
}


DoubleTickSlider::DoubleTickSlider(QWidget* parent) 
    : QSlider(parent) 
{
    connect(this, &QSlider::valueChanged, this, &DoubleTickSlider::notifyValueChanged);
}

DoubleTickSlider::DoubleTickSlider(Qt::Orientation ori, QWidget* parent)
    : QSlider(ori,parent)
{
    connect(this, &QSlider::valueChanged, this, &DoubleTickSlider::notifyValueChanged);
}

DoubleTickSlider::~DoubleTickSlider()
{

}

void DoubleTickSlider::setDoubleScale(double min_, double max_)
{
    max = max_;
    min = min_;
    slope = ((double)steps)/ (max - min);
    setRange(0, steps);
}

void DoubleTickSlider::setDoubleTickInterval(double interval)
{
    setTickInterval(int(interval * slope));
}

void DoubleTickSlider::setDoubleSingleStep(double singleStep)
{
    setSingleStep(int(singleStep * slope));
}

void DoubleTickSlider::setDoubleValue(double value)
{
    setValue(int((value - min) * slope));
}

double DoubleTickSlider::doubleValue()
{
    return double(value()) / slope + min;
}

void DoubleTickSlider::setTotalSteps(unsigned int steps_)
{
    steps = steps_;
    slope = ((double)steps) / (max - min);
    setRange(0, steps);
}

void DoubleTickSlider::notifyValueChanged(int value)
{
    double doubleValue = double(value) / slope + min;
    emit doubleValueChanged(doubleValue);
}

void DoubleTickSlider::paintEvent(QPaintEvent* e)
{
    if (tickPosition() == NoTicks)
    {
        QSlider::paintEvent(e);
        return;
    }

    QStylePainter p1(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    if (orientation() == Qt::Vertical)
    {
        opt.rect.adjust(15, 0, 0, 0);
        if (m_firstWidth == 0)
            m_firstWidth = width();
        setMinimumWidth(m_firstWidth * 2);
    }

    // draw the slider (this is basically copy/pasted from QSlider::paintEvent)
    opt.subControls = QStyle::SC_SliderTickmarks;
    p1.drawComplexControl(QStyle::CC_Slider, opt);


    opt.subControls = QStyle::SC_SliderGroove;
    p1.drawComplexControl(QStyle::CC_Slider, opt);

    // draw the slider handle
    opt.subControls = QStyle::SC_SliderHandle;
    p1.drawComplexControl(QStyle::CC_Slider, opt);

    int rightLast = 0;
    QRect rr;
    p1.drawText(rect(), Qt::TextDontPrint, QString::number(9999), &rr);

    for (int v = this->minimum(); v <= this->maximum(); v += tickInterval()) {

        QStyle* st = style();
        QStyleOptionSlider slider;
        slider.init(this);
        slider.orientation = orientation();
        int available = st->pixelMetric(QStyle::PM_SliderSpaceAvailable, &slider, this);

        int len = st->pixelMetric(QStyle::PM_SliderLength, &slider, this);
        int left = QStyle::sliderPositionFromValue(minimum(), maximum(), v, available) + len;
        QString vs = QString("%L1").arg(int(double(v) / slope + min));
        if (qAbs(maximum() - v) < tickInterval())
            left -= 2 * len;

        QPoint pos(left, rect().bottom());
        if (orientation() == Qt::Vertical)
        {
            pos = QPoint(rect().left(), left + len);
            int tmp = this->minimum() - v + this->maximum();
            vs = QString("%L1").arg(int(double(tmp) / slope + min));
        }
        QRect boundingRect;
        if (left < available && rightLast <= left - len)
        {
            rr.setX(pos.x());
            rr.setY(pos.y());
            p1.drawText(rr, Qt::TextDontPrint, vs, &boundingRect);
            pos.rx() -= offsetx;
            pos.ry() += offsety;
            p1.drawText(pos, vs);
        }
        rightLast = left + boundingRect.width();

    }
}
#include <stdafx.h>
#include "RangeSlider.h"

RangeSlider::RangeSlider(QWidget* aParent)
    : QWidget(aParent),
    mMinimum(0),
    mMaximum(100),
    mLowerValue(0),
    mUpperValue(100),
    mFirstHandlePressed(false),
    mSecondHandlePressed(false),
    mInterval(mMaximum - mMinimum),
    mBackgroudColorEnabled(QColor(0x1E, 0x90, 0xFF)),
    mBackgroudColorDisabled(Qt::darkGray),
    mBackgroudColor(mBackgroudColorEnabled),
    orientation(Qt::Horizontal)
{
    setMouseTracking(true);
    //installEventFilter(this); //not use eventfilter but resizeevent
}

RangeSlider::RangeSlider(Qt::Orientation ori, Options t, QWidget* aParent)
    : QWidget(aParent),
    mMinimum(0),
    mMaximum(100),
    mLowerValue(0),
    mUpperValue(100),
    mFirstHandlePressed(false),
    mSecondHandlePressed(false),
    mInterval(mMaximum - mMinimum),
    mBackgroudColorEnabled(QColor(0x1E, 0x90, 0xFF)),
    mBackgroudColorDisabled(Qt::darkGray),
    mBackgroudColor(mBackgroudColorEnabled),
    orientation(ori),
    type(t)
{
    setMouseTracking(true);
    //installEventFilter(this); //not use eventfilter but resizeevent
}

bool RangeSlider::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == QEvent::Resize) {
        QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
        emit sizeChanged(validLength()); //resizeEvent->size() is the size of widget, i.e. the same as width() and height()
    }
    return false;
}

void RangeSlider::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    emit sizeChanged(validLength());
}

void RangeSlider::paintEvent(QPaintEvent* aEvent)
{
    Q_UNUSED(aEvent);
    QPainter painter(this);

    // Background
    QRectF backgroundRect;
    if (orientation == Qt::Horizontal)
        backgroundRect = QRectF(scLeftRightMargin, (height() - scSliderBarHeight) / 2, width() - scLeftRightMargin * 2, scSliderBarHeight);
    else
        backgroundRect = QRectF((width() - scSliderBarHeight) / 2, scLeftRightMargin, scSliderBarHeight, height() - scLeftRightMargin * 2);

    QPen pen(Qt::gray, 0.8);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Qt4CompatiblePainting);
    QBrush backgroundBrush(QColor(0xD0, 0xD0, 0xD0));
    painter.setBrush(backgroundBrush);
    painter.drawRoundedRect(backgroundRect, 1, 1);

    // First value handle rect
    pen.setColor(Qt::darkGray);
    pen.setWidth(0.5);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Antialiasing);
    QBrush handleBrush(QColor(0xFA, 0xFA, 0xFA));
    painter.setBrush(handleBrush);
    QRectF leftHandleRect = firstHandleRect();
    if (type.testFlag(LeftHandle))
        painter.drawRoundedRect(leftHandleRect, 2, 2);

    // Second value handle rect
    QRectF rightHandleRect = secondHandleRect();
    if (type.testFlag(RightHandle))
        painter.drawRoundedRect(rightHandleRect, 2, 2);

    // Handles
    painter.setRenderHint(QPainter::Antialiasing, false);
    QRectF selectedRect(backgroundRect);
    if (orientation == Qt::Horizontal) {
        selectedRect.setLeft((type.testFlag(LeftHandle) ? leftHandleRect.right() : leftHandleRect.left()) + 0.5);
        selectedRect.setRight((type.testFlag(RightHandle) ? rightHandleRect.left() : rightHandleRect.right()) - 0.5);
    }
    else {
        selectedRect.setTop((type.testFlag(LeftHandle) ? leftHandleRect.bottom() : leftHandleRect.top()) + 0.5);
        selectedRect.setBottom((type.testFlag(RightHandle) ? rightHandleRect.top() : rightHandleRect.bottom()) - 0.5);
    }
    QBrush selectedBrush(mBackgroudColor);
    painter.setBrush(selectedBrush);
    painter.drawRect(selectedRect);
}

QRectF RangeSlider::firstHandleRect() const
{
    float percentage = (mLowerValue - mMinimum) * 1.0 / mInterval;
    return handleRect(percentage * validLength() + scLeftRightMargin);
}

QRectF RangeSlider::secondHandleRect() const
{
    float percentage = (mUpperValue - mMinimum) * 1.0 / mInterval;
    return handleRect(percentage * validLength() + scLeftRightMargin + (type.testFlag(LeftHandle) ? scHandleSideLength : 0));
}

QRectF RangeSlider::handleRect(int aValue) const
{
    if (orientation == Qt::Horizontal)
        return QRect(aValue, (height() - scHandleSideLength) / 2, scHandleSideLength, scHandleSideLength);
    else
        return QRect((width() - scHandleSideLength) / 2, aValue, scHandleSideLength, scHandleSideLength);
}

void RangeSlider::mousePressEvent(QMouseEvent* aEvent)
{
    if (aEvent->buttons() & Qt::LeftButton)
    {
        int posCheck, posMax, posValue, firstHandleRectPosValue, secondHandleRectPosValue;
        posCheck = (orientation == Qt::Horizontal) ? aEvent->pos().y() : aEvent->pos().x();
        posMax = (orientation == Qt::Horizontal) ? height() : width();
        posValue = (orientation == Qt::Horizontal) ? aEvent->pos().x() : aEvent->pos().y();
        firstHandleRectPosValue = (orientation == Qt::Horizontal) ? firstHandleRect().x() : firstHandleRect().y();
        secondHandleRectPosValue = (orientation == Qt::Horizontal) ? secondHandleRect().x() : secondHandleRect().y();

        mSecondHandlePressed = secondHandleRect().contains(aEvent->pos());
        mFirstHandlePressed = !mSecondHandlePressed && firstHandleRect().contains(aEvent->pos());
        if (mFirstHandlePressed)
        {
            mDelta = posValue - (firstHandleRectPosValue + scHandleSideLength / 2);
        }
        else if (mSecondHandlePressed)
        {
            mDelta = posValue - (secondHandleRectPosValue + scHandleSideLength / 2);
        }

        if (posCheck >= 2
            && posCheck <= posMax - 2)
        {
            int step = mInterval / 10 < 1 ? 1 : mInterval / 10;
            if (posValue < firstHandleRectPosValue)
                setLowerValue(mLowerValue - step);
            else if (((posValue > firstHandleRectPosValue + scHandleSideLength) || !type.testFlag(LeftHandle))
                && ((posValue < secondHandleRectPosValue) || !type.testFlag(RightHandle)))
            {
                if (type.testFlag(DoubleHandles))
                    if (posValue - (firstHandleRectPosValue + scHandleSideLength) <
                        (secondHandleRectPosValue - (firstHandleRectPosValue + scHandleSideLength)) / 2)
                        setLowerValue((mLowerValue + step < mUpperValue) ? mLowerValue + step : mUpperValue);
                    else
                        setUpperValue((mUpperValue - step > mLowerValue) ? mUpperValue - step : mLowerValue);
                else if (type.testFlag(LeftHandle))
                    setLowerValue((mLowerValue + step < mUpperValue) ? mLowerValue + step : mUpperValue);
                else if (type.testFlag(RightHandle))
                    setUpperValue((mUpperValue - step > mLowerValue) ? mUpperValue - step : mLowerValue);
            }
            else if (posValue > secondHandleRectPosValue + scHandleSideLength)
                setUpperValue(mUpperValue + step);
        }
    }
}

void RangeSlider::mouseMoveEvent(QMouseEvent* aEvent)
{
    if (aEvent->buttons() & Qt::LeftButton)
    {
        int posValue, firstHandleRectPosValue, secondHandleRectPosValue;
        posValue = (orientation == Qt::Horizontal) ? aEvent->pos().x() : aEvent->pos().y();
        firstHandleRectPosValue = (orientation == Qt::Horizontal) ? firstHandleRect().x() : firstHandleRect().y();
        secondHandleRectPosValue = (orientation == Qt::Horizontal) ? secondHandleRect().x() : secondHandleRect().y();

        if (mFirstHandlePressed && type.testFlag(LeftHandle))
        {
            if (posValue - mDelta + scHandleSideLength / 2 <= secondHandleRectPosValue)
            {
                setLowerValue((posValue - mDelta - scLeftRightMargin - scHandleSideLength / 2) * 1.0 / validLength() * mInterval + mMinimum);
            }
            else
            {
                setLowerValue(mUpperValue);
            }
        }
        else if (mSecondHandlePressed && type.testFlag(RightHandle))
        {
            if (firstHandleRectPosValue + scHandleSideLength * (type.testFlag(DoubleHandles) ? 1.5 : 0.5) <= posValue - mDelta)
            {
                setUpperValue((posValue - mDelta - scLeftRightMargin - scHandleSideLength / 2 - (type.testFlag(DoubleHandles) ? scHandleSideLength : 0)) * 1.0 / validLength() * mInterval + mMinimum);
            }
            else
            {
                setUpperValue(mLowerValue);
            }
        }
    }
}

void RangeSlider::mouseReleaseEvent(QMouseEvent* aEvent)
{
    Q_UNUSED(aEvent);

    mFirstHandlePressed = false;
    mSecondHandlePressed = false;
}

void RangeSlider::changeEvent(QEvent* aEvent)
{
    if (aEvent->type() == QEvent::EnabledChange)
    {
        if (isEnabled())
        {
            mBackgroudColor = mBackgroudColorEnabled;
        }
        else
        {
            mBackgroudColor = mBackgroudColorDisabled;
        }
        update();
    }
}

QSize RangeSlider::minimumSizeHint() const
{
    return QSize(scHandleSideLength * 2 + scLeftRightMargin * 2, scHandleSideLength);
}

int RangeSlider::GetMinimun() const
{
    return mMinimum;
}

void RangeSlider::SetMinimum(int aMinimum)
{
    setMinimum(aMinimum);
}

int RangeSlider::GetMaximun() const
{
    return mMaximum;
}

void RangeSlider::SetMaximum(int aMaximum)
{
    setMaximum(aMaximum);
}

int RangeSlider::GetLowerValue() const
{
    return mLowerValue;
}

void RangeSlider::SetLowerValue(int aLowerValue)
{
    setLowerValue(aLowerValue);
}

int RangeSlider::GetUpperValue() const
{
    return mUpperValue;
}

void RangeSlider::SetUpperValue(int aUpperValue)
{
    setUpperValue(aUpperValue);
}

void RangeSlider::setLowerValue(int aLowerValue)
{
    if (aLowerValue > mMaximum)
    {
        aLowerValue = mMaximum;
    }

    if (aLowerValue < mMinimum)
    {
        aLowerValue = mMinimum;
    }

    mLowerValue = aLowerValue;
    emit lowerValueChanged(mLowerValue);

    update();
}

void RangeSlider::setUpperValue(int aUpperValue)
{
    if (aUpperValue > mMaximum)
    {
        aUpperValue = mMaximum;
    }

    if (aUpperValue < mMinimum)
    {
        aUpperValue = mMinimum;
    }

    mUpperValue = aUpperValue;
    emit upperValueChanged(mUpperValue);

    update();
}

void RangeSlider::setMinimum(int aMinimum)
{
    if (aMinimum <= mMaximum)
    {
        mMinimum = aMinimum;
    }
    else
    {
        int oldMax = mMaximum;
        mMinimum = oldMax;
        mMaximum = aMinimum;
    }
    mInterval = mMaximum - mMinimum;
    update();

    setLowerValue(mMinimum);
    setUpperValue(mMaximum);

    emit rangeChanged(mMinimum, mMaximum);
}

void RangeSlider::setMaximum(int aMaximum)
{
    if (aMaximum >= mMinimum)
    {
        mMaximum = aMaximum;
    }
    else
    {
        int oldMin = mMinimum;
        mMaximum = oldMin;
        mMinimum = aMaximum;
    }
    mInterval = mMaximum - mMinimum;
    update();

    setLowerValue(mMinimum);
    setUpperValue(mMaximum);

    emit rangeChanged(mMinimum, mMaximum);
}

int RangeSlider::validLength() const
{
    int len = (orientation == Qt::Horizontal) ? width() : height();
    return len - scLeftRightMargin * 2 - scHandleSideLength * (type.testFlag(DoubleHandles) ? 2 : 1);
}

void RangeSlider::SetRange(int aMinimum, int mMaximum)
{
    setMinimum(aMinimum);
    setMaximum(mMaximum);
}


/*******************************************************************************************/

// A big lesson learned here 01/28/2022: You should set the parent of contents of the widget to be the widget itself,
// not the big window that own this widget!!!! This can cause confusion of qt layout when you want to add the widget, which contains
// no contents and its contents is in the window widget and you can not remember to put those in place.
RangeSliderIntg::RangeSliderIntg(Qt::Orientation ori, RangeSlider::Option handles)
{
    auto aparent = this;

    orientation = ori;

    rslider = new RangeSlider(orientation, handles, aparent);
    upperSB = new QSpinBox(aparent);
    lowerSB = new QSpinBox(aparent);
    axslid = new QCustomPlot(aparent);


    QGridLayout* vRSlayout = new QGridLayout(this);
    QHBoxLayout* vSB1 = new QHBoxLayout();
    QHBoxLayout* vSB2 = new QHBoxLayout();
    vSB1->setContentsMargins(0, 0, 0, 0);
    vSB2->setContentsMargins(0, 0, 0, 0);
    vRSlayout->setContentsMargins(0, 0, 0, 0);

    vSB1->addWidget(new QLabel("Upper: "), 0);
    vSB1->addWidget(upperSB, 1);
    vSB2->addWidget(new QLabel("Lower: "), 0);
    vSB2->addWidget(lowerSB, 1);

    if (orientation == Qt::Vertical) {
        QVBoxLayout* layout = new QVBoxLayout(aparent);
        layout->setContentsMargins(0, 0, 0, 0);
        vRSlayout->addLayout(vSB1, 0, 0, 1, 2);
        vRSlayout->addLayout(vSB2, 1, 0, 1, 2);
        layout->addLayout(vRSlayout,1);
        //layout->addStretch();
    }
    else {
        QHBoxLayout* layout = new QHBoxLayout(aparent);
        layout->setContentsMargins(0, 0, 0, 0);
        vRSlayout->addLayout(vSB1, 0, 0, 1, 2);
        vRSlayout->addLayout(vSB2, 1, 0, 1, 2);
        layout->addLayout(vRSlayout,1);
        //layout->addStretch();
    }


    std::array<QCPAxis*,3> axisToHide;
    if (orientation == Qt::Vertical) {
        axisToHide = { axslid->xAxis, axslid->xAxis2, axslid->yAxis2 };
    }
    else {
        axisToHide = { axslid->yAxis, axslid->xAxis2, axslid->yAxis2 };
    }
    for (auto& ax : axisToHide)
    {
        ax->setVisible(false);
        ax->setPadding(0);
        ax->setLabelPadding(0);
        ax->setTickLabelPadding(0);
        ax->setTickLabels(false);
    }
    axslid->replot();

    if (orientation == Qt::Vertical) {
        vRSlayout->addWidget(axslid, 2, 0);
        vRSlayout->addWidget(rslider, 2, 1);
        axslid->setMaximumHeight(400);
    }
    else {
        vRSlayout->addWidget(axslid, 0, 2);
        vRSlayout->addWidget(rslider, 1, 2);
        axslid->setMaximumWidth(400);
    }
    connect(rslider, &RangeSlider::sizeChanged, axslid, [this](int height) {
        if (orientation == Qt::Vertical) {
            axslid->axisRect()->setMaximumSize(50, height + 16);
            axslid->axisRect()->setMinimumSize(10, height + 16);
            axslid->replot();
        }
        else {
            axslid->axisRect()->setMaximumSize(height + 16, 50);
            axslid->axisRect()->setMinimumSize(height + 16, 10);
            axslid->replot();
        } });

    connect(rslider, &RangeSlider::lowerValueChanged, this, [this](int val) {
        if (orientation == Qt::Vertical) { emit lrgValueChanged(abs(val)); }
        else { emit smlValueChanged(val); } });
    connect(rslider, &RangeSlider::upperValueChanged, this, [this](int val) {
        if (orientation == Qt::Vertical) { emit smlValueChanged(abs(val)); }
        else { emit lrgValueChanged(val); } });

    /*mechanism that dictates the relation between lower and upper bound*/
    if (orientation == Qt::Vertical) {
        /*for vertical, it is in this funny way because I want the upper one to be the larger one, so I put it to be negative in slider and take the abs afterwards*/
        connect(rslider, &RangeSlider::lowerValueChanged, upperSB, [this](int l) {
            if (abs(l) != upperSB->value() && abs(l) > lowerSB->value())
                upperSB->setValue(abs(l)); });
        connect(rslider, &RangeSlider::upperValueChanged, lowerSB, [this](int u) {
            if (abs(u) != lowerSB->value() && abs(u) < upperSB->value())
                lowerSB->setValue(abs(u)); });
    }
    else {
        /*for horizontal one, it is the common way: left <-> smaller, right <-> larger*/
        connect(rslider, &RangeSlider::lowerValueChanged, lowerSB, [this](int l) {
            if (abs(l) != lowerSB->value() && abs(l) < upperSB->value())
                lowerSB->setValue(abs(l)); });
        connect(rslider, &RangeSlider::upperValueChanged, upperSB, [this](int u) {
            if (abs(u) != upperSB->value() && abs(u) > lowerSB->value())
                upperSB->setValue(abs(u)); });
    }
    connect(lowerSB, qOverload<int>(&QSpinBox::valueChanged), rslider, [this](int val) {
        if (val != getSilderSmlVal() && val < getSilderLrgVal())
            setSilderSmlVal(val);
        });
    connect(upperSB, qOverload<int>(&QSpinBox::valueChanged), rslider, [this](int val) {
        if (val != getSilderLrgVal() && val > getSilderSmlVal())
            setSilderLrgVal(val);
        });

}

void RangeSliderIntg::setRange(int low, int upr)
{
    if (orientation == Qt::Vertical) {
        rslider->SetRange(-upr, -low);
        axslid->yAxis->setRange(low, upr);
    }
    else {
        rslider->SetRange(low, upr);
        axslid->xAxis->setRange(low, upr);
    }

    axslid->replot();
    upperSB->setRange(low, upr);
    lowerSB->setRange(low, upr);
    
}

void RangeSliderIntg::setMaxLength(int length)
{
    if (orientation == Qt::Vertical) {
        axslid->setMaximumHeight(length);
    }
    else {
        axslid->setMaximumWidth(length);
    }
}

/*for vertical, it is in this funny way because I want the upper one to be the larger one,
* so I put it to be negative in slider and take the abs afterwards.
* for horizontal one, it is the common way: left <-> smaller, right <-> larger
* now only support positive value: the rsilder can not cross zer*/
int RangeSliderIntg::getSilderSmlVal()
{
    return orientation == Qt::Vertical ? abs(rslider->GetUpperValue()) : rslider->GetLowerValue();
}

int RangeSliderIntg::getSilderLrgVal()
{
    return orientation == Qt::Vertical ? abs(rslider->GetLowerValue()) : rslider->GetUpperValue();
}

void RangeSliderIntg::setSilderSmlVal(int val)
{
    rslider->SetUpperValue(orientation == Qt::Vertical ? -val : val);
}

void RangeSliderIntg::setSilderLrgVal(int val)
{
    rslider->SetLowerValue(orientation == Qt::Vertical ? -val : val);
}

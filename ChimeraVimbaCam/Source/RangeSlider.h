#pragma once
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <../ExternLib/qcustomplot/qcustomplot.h>
#include <qspinbox.h>
enum {
    scHandleSideLength = 11,
    scSliderBarHeight = 5,
    scLeftRightMargin = 1
};

class RangeSlider :
    public QWidget
{
    Q_OBJECT
        Q_ENUMS(RangeSliderTypes)

public:
    enum Option {
        NoHandle = 0x0,
        LeftHandle = 0x1,
        RightHandle = 0x2,
        DoubleHandles = LeftHandle | RightHandle
    };
    Q_DECLARE_FLAGS(Options, Option)

    RangeSlider(QWidget* aParent = Q_NULLPTR);
    RangeSlider(Qt::Orientation ori, Options t = DoubleHandles, QWidget* aParent = Q_NULLPTR);

    QSize minimumSizeHint() const override;

    int GetMinimun() const;
    void SetMinimum(int aMinimum);

    int GetMaximun() const;
    void SetMaximum(int aMaximum);

    int GetLowerValue() const;
    void SetLowerValue(int aLowerValue);

    int GetUpperValue() const;
    void SetUpperValue(int aUpperValue);

    void SetRange(int aMinimum, int aMaximum);

protected:
    void paintEvent(QPaintEvent* aEvent) override;
    void mousePressEvent(QMouseEvent* aEvent) override;
    void mouseMoveEvent(QMouseEvent* aEvent) override;
    void mouseReleaseEvent(QMouseEvent* aEvent) override;
    void changeEvent(QEvent* aEvent) override;
    bool eventFilter(QObject* object, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    QRectF firstHandleRect() const;
    QRectF secondHandleRect() const;
    QRectF handleRect(int aValue) const;

signals:
    void lowerValueChanged(int aLowerValue);
    void upperValueChanged(int aUpperValue);
    void rangeChanged(int aMin, int aMax);
    void sizeChanged(int height);

public slots:
    void setLowerValue(int aLowerValue);
    void setUpperValue(int aUpperValue);
    void setMinimum(int aMinimum);
    void setMaximum(int aMaximum);

private:
    Q_DISABLE_COPY(RangeSlider)
    float currentPercentage();
    int validLength() const;

    int mMinimum;
    int mMaximum;
    int mLowerValue;
    int mUpperValue;
    bool mFirstHandlePressed;
    bool mSecondHandlePressed;
    int mInterval;
    int mDelta;
    QColor mBackgroudColorEnabled;
    QColor mBackgroudColorDisabled;
    QColor mBackgroudColor;
    Qt::Orientation orientation;
    Options type;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RangeSlider::Options)



/* integrated Rangeslider with spin box as well as a vertical indicator from QCP */
class RangeSliderIntg : public QWidget
{
    Q_OBJECT
public:
    RangeSliderIntg(Qt::Orientation ori, RangeSlider::Option handles);
    ~RangeSliderIntg() {};

    void setRange(int low, int upr);
    // for changing the value of slider and spinbox: eg upperSpinBox()->setValue(max);
    QSpinBox* upperSpinBox() { return upperSB; }
    QSpinBox* lowerSpinBox() { return lowerSB; }
    //set the maximum size of the slider bar, due to the bug in slider and spinbox, the bar will keep increasing. So no auto rescaling in Qt is propriate
    void setMaxLength(int length);
    int getSilderSmlVal();
    int getSilderLrgVal();
private:
    void setSilderSmlVal(int val);
    void setSilderLrgVal(int val);
    
signals:
    void smlValueChanged(int);
    void lrgValueChanged(int);

private:
    RangeSlider* rslider;
    QSpinBox* upperSB;
    QSpinBox* lowerSB;
    QCustomPlot* axslid;
    Qt::Orientation orientation;
};


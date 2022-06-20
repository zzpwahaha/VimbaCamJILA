#pragma once
#include <qslider.h>
class TickSlider : public QSlider
{
    Q_OBJECT

public:
    TickSlider(QWidget* parent = nullptr);
    TickSlider(Qt::Orientation ori, QWidget* parent = nullptr);
    ~TickSlider();
    void setOffset(int x, int y);

private:
    void paintEvent(QPaintEvent* e);

private:
    int m_firstWidth = 0;
    int offsetx = -7;
    int offsety = 0;
};

class DoubleTickSlider : public QSlider 
{
    Q_OBJECT

public:
    DoubleTickSlider(QWidget* parent = nullptr);
    DoubleTickSlider(Qt::Orientation ori, QWidget* parent = nullptr);
    ~DoubleTickSlider();

    void setDoubleScale(double min, double max);
    void setDoubleTickInterval(double interval);
    void setDoubleSingleStep(double singleStep);
    void setDoubleValue(double value);
    double doubleValue();
    void setTotalSteps(unsigned int steps);

signals:
    void doubleValueChanged(double value);

public slots:
    void notifyValueChanged(int value);

private:
    void paintEvent(QPaintEvent* e);

private:
    double min = 0.0;
    double max = 100.0;
    int steps = 10000; // int 0->10000 => double 0->100
    // int (0,steps) = (double(min, max)  -  min)* slope  
    double slope = 100.0; // = steps/(max-min)

    int m_firstWidth = 0;
    int offsetx = 0;
    int offsety = 0;
};




#ifndef FLOATSPINBOX_H
#define FLOATSPINBOX_H

#include <QSpinBox>

class FloatSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
    public: 

    protected:            

    private:

    public:
            FloatSpinBox ( QWidget *parent );
           ~FloatSpinBox ( void );

protected:

private:
    virtual void stepBy ( int steps );
};

#endif
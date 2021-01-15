


#include "FloatSpinBox.h"


FloatSpinBox::FloatSpinBox ( QWidget *parent ): QDoubleSpinBox ( parent )
{

}

FloatSpinBox::~FloatSpinBox ( void )
{

}

void FloatSpinBox::stepBy( int steps )
{
    double dInterval =  singleStep();
    double dValue    = value();

    if( (minimum() > value()) || (maximum() < value()) )
        return;

    if(0 < steps ) //stepUp
    {        
        setValue( dValue+dInterval);
    }
    else   //stepDown
    {
        setValue( dValue-dInterval);
    }

    editingFinished();
}

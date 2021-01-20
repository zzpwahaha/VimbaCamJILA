
#ifndef HISTOGRAM_H
#define HISTOGRAM_H


#include "./ExternLib/qwt/qwt_plot.h"
#include "./ExternLib/qwt/qwt_plot_curve.h"
#include "./ExternLib/qwt/qwt_plot_marker.h"
#include "./ExternLib/qwt/qwt_plot_picker.h"
#include "./ExternLib/qwt/qwt_plot_renderer.h"
#include "./ExternLib/qwt/qwt_plot_grid.h"
#include "./ExternLib/qwt/qwt_plot_picker.h"
#include "./ExternLib/qwt/qwt_legend.h"
#include "./ExternLib/qwt/qwt_plot_panner.h"
#include "./ExternLib/qwt/qwt_plot_magnifier.h"
#include "./ExternLib/qwt/qwt_picker_machine.h"


#include "./ExternLib/qwt/qwt_slider.h"

class HistogramSource;

class Histogram : public QwtPlot
{
    public:
            explicit Histogram(const char* name, int maxValue, QWidget *parent = 0);
            ~Histogram();

            void populate            ( const QVector<quint32> &histogramData, const QString &sColorComponent );
            void setHistogramTitle    ( const QString &sTitle );
            void setLeftAxisY        ( const double &dMaximum );
            void setBottomAxisX        ( const double &dMaximum );

    protected:
            virtual void resizeEvent( QResizeEvent * );

    private:
            HistogramSource    *m_Mono;
            HistogramSource    *m_Red;
            HistogramSource    *m_Green;
            HistogramSource    *m_Blue;
            HistogramSource    *m_Y;
            HistogramSource    *m_U;
            HistogramSource    *m_V;

            QwtPlotCurve       *m_MonoCurve;
            QwtPlotCurve       *m_RedCurve;
            QwtPlotCurve       *m_GreenCurve;
            QwtPlotCurve       *m_BlueCurve;

            QwtPlotCurve       *m_YCurve;
            QwtPlotCurve       *m_UCurve;
            QwtPlotCurve       *m_VCurve;

            QwtPlotPicker      *m_PlotPicker;

            quint32             m_MaxValueX;
            quint32             m_MaxValueY;
            double              m_dYAxisMaximum;

            signals:

            public slots:
};

#endif // HISTOGRAM_H

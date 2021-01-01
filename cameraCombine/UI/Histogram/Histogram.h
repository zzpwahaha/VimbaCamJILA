/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        Histogram.cpp

  Description: populate histogram 
               

-------------------------------------------------------------------------------

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
  NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


 [program/widget] is based in part on the work of
 the Qwt project (http://qwt.sf.net).
=============================================================================*/
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

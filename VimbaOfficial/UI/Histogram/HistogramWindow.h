/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        HistogramWindow.h

  Description: Histogram Window

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


#ifndef HISTOGRAMWINDOW_H
#define HISTOGRAMWINDOW_H

#include <QWidget>
#include <QGridLayout>
#include <QToolButton>
#include <QImageWriter>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QEvent>
#include <QCheckBox>

class Histogram;

class HistogramWindow : public QWidget
{
    Q_OBJECT
    public:
            explicit HistogramWindow(QWidget *parent = 0);
            ~HistogramWindow();

            void createGraphWidgets  ( void );
            void initializeStatistic ( void );
            void deinitializeStatistic ( void );
            void setStatistic ( const QStringList component, const QStringList minimum, const QStringList maximum, const QStringList average, const QString sFormat );

    private:
            
            QToolButton        *m_PrintButton;
            QToolButton        *m_ExportButton;
            QGridLayout        *m_DataLayout;
            Histogram          *m_Histogram;
            QString             m_sCurrentHistogramTitle;
            QTableWidget       *m_StatisticsTable;
            QCheckBox          *m_AutoScaleYAxis;
            unsigned int        m_nXAxisMax;
            bool   eventFilter              (QObject *object, QEvent *event);
    signals:

    private slots:
            void onPrint ( void );
            void onExport ( void );

    public slots:
            void updateHistogram( const QVector<QVector<quint32> > &histogramData, const QString &sNewHistogramTitle, 
            const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis );
            void onAutoScaleYAxisClick ( bool bValue );

};

#endif // HISTOGRAMWINDOW_H

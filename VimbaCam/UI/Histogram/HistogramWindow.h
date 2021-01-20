

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

/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        HistogramWindow.cpp

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


#include "HistogramWindow.h"
#include "Histogram.h"

HistogramWindow::HistogramWindow(QWidget *parent) :
QWidget(parent), m_Histogram ( NULL ), m_StatisticsTable ( NULL ), m_nXAxisMax ( 0 )
{
    m_DataLayout = new QGridLayout();
    QHBoxLayout *layoutHorizontal = new QHBoxLayout();
    m_DataLayout->addLayout(layoutHorizontal,0,0);
    setLayout(m_DataLayout);

    QIcon printIcon;
    printIcon.addFile( QString::fromUtf8(":/VimbaViewer/Images/print.png"), QSize(), QIcon::Normal, QIcon::Off);
    m_PrintButton = new QToolButton( this );
    m_PrintButton->setIcon(printIcon);
    m_PrintButton->setIconSize(QSize(32,32));
    m_PrintButton->setToolTip(tr("Print the histogram"));
    m_PrintButton->setCursor(Qt::PointingHandCursor);
	m_PrintButton->setStyleSheet(QString::fromUtf8("QToolButton{background-color: rgb(128, 0, 0);} QToolTip {}"));
    layoutHorizontal->addWidget(m_PrintButton);
    
    QIcon pdfIcon;
    pdfIcon.addFile( QString::fromUtf8(":/VimbaViewer/Images/pdf.png"), QSize(), QIcon::Normal, QIcon::Off);
    m_ExportButton = new QToolButton( this );
    m_ExportButton->setIcon(pdfIcon);
    m_ExportButton->setIconSize(QSize(32,32));
    m_ExportButton->setToolTip(tr("Export the histogram to PDF"));
    m_ExportButton->setCursor(Qt::PointingHandCursor);
    m_ExportButton->setStyleSheet(QString::fromUtf8("QToolButton{background-color: rgb(128, 0, 0);} QToolTip {}"));
    layoutHorizontal->addWidget(m_ExportButton);

    QObject::connect( m_PrintButton, SIGNAL(clicked()), this, SLOT(onPrint()));
    QObject::connect( m_ExportButton, SIGNAL(clicked()), this, SLOT(onExport()));
}

HistogramWindow::~HistogramWindow( )
{
    if( NULL != m_Histogram)
    {
        delete m_Histogram;
        m_Histogram = NULL;
    }
}

void HistogramWindow::initializeStatistic ( void )
{
    m_StatisticsTable = new QTableWidget();
    m_DataLayout->addWidget(m_StatisticsTable);
    m_StatisticsTable->setColumnCount(4);
    QStringList sHeader;
    sHeader << "" << tr("Minimum") << tr("Maximum") << tr("Mean");
    m_StatisticsTable->setHorizontalHeaderLabels(sHeader);
    m_StatisticsTable->setFixedHeight(135);
    m_StatisticsTable->verticalHeader()->setVisible(false);
}

void HistogramWindow::deinitializeStatistic ( void )
{
    if(0 != m_StatisticsTable)
    {
        delete m_StatisticsTable;
        m_StatisticsTable = NULL;
    }
}

void HistogramWindow::setStatistic ( const QStringList component, 
                                     const QStringList minimum, 
                                     const QStringList maximum, 
                                     const QStringList average,
                                     const QString sFormat )
{
    m_StatisticsTable->clearContents();
    m_StatisticsTable->setRowCount(component.size());
    for(int i=0; i < component.size(); i++) /* component */
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(component.at(i));
        
        /* Mono 8 */
        if(1 == component.size())
        {
            item->setBackgroundColor(Qt::black);
            item->setForeground(QBrush(QColor(255,255,255)));
        }

        /* RGB, YUV */
        if(3 == component.size())
        {
            if (0 == sFormat.compare("RGB")|| sFormat.contains("Bayer"))
            {
                switch(i)
                {
                case 0:
                    item->setBackgroundColor(Qt::red);
                    break;
                case 1:
                    item->setBackgroundColor(Qt::green);
                    break;
                case 2:
                    item->setForeground(QBrush(QColor(255,255,255)));
                    item->setBackgroundColor(Qt::blue);
                    break;
                default:
                    break;
                }
            }

            if(0 == sFormat.compare("YUV"))
            {
                switch(i)
                {
                case 0:
                    item->setBackgroundColor(Qt::darkYellow);
                    break;
                case 1:
                    item->setBackgroundColor(Qt::darkCyan);
                    break;
                case 2:
                    //item->setForeground(QBrush(QColor(255,255,255)));
                    item->setBackgroundColor(Qt::darkMagenta);
                    break;
                default:
                    break;
                }
            }
            
        }
        m_StatisticsTable->setItem(i, 0, item);
    }

    for(int j=0; j < minimum.size(); j++) /* minimum */
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(minimum.at(j));
        m_StatisticsTable->setItem(j, 1, item);
    }

    for(int k=0; k < maximum.size(); k++) /* maximum */
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(maximum.at(k));
        m_StatisticsTable->setItem(k, 2, item);
    }

    for(int l=0; l < average.size(); l++) /* average */
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(average.at(l));
        m_StatisticsTable->setItem(l, 3, item);
    }
}

void HistogramWindow::createGraphWidgets ( void )
{
    m_Histogram = new Histogram(" ", 255, this); //255: Default
    m_DataLayout->addWidget(m_Histogram, 2, 0);
    m_AutoScaleYAxis = new QCheckBox();
    m_AutoScaleYAxis->setText(tr("Auto scale Y-AXIS ON"));
    m_AutoScaleYAxis->setChecked(true);
    m_DataLayout->addWidget(m_AutoScaleYAxis);
    connect( m_AutoScaleYAxis, SIGNAL(clicked(bool)), this, SLOT(onAutoScaleYAxisClick(bool)) );
    m_Histogram->installEventFilter(this);
}

void HistogramWindow::onAutoScaleYAxisClick ( bool bValue )
{
    if(bValue)
    {
        m_Histogram->setAxisAutoScale(0, true);
        m_Histogram->setAxisAutoScale(2, true);
        m_Histogram->setAxisScale(2, 0.0, m_nXAxisMax ); 
    }
    else
    {
        m_Histogram->setAxisAutoScale(0, false);
        m_Histogram->setAxisAutoScale(2, false);
    }
}

void HistogramWindow::updateHistogram( const QVector<QVector<quint32> > &histogramData, const QString &sNewHistogramTitle, 
                                       const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis )
{
    if( 0 != m_sCurrentHistogramTitle.compare(sNewHistogramTitle) )
    {
        m_Histogram->setHistogramTitle( sNewHistogramTitle );
        //m_Histogram->setLeftAxisY(nMaxHeight_YAxis);
        
        if(m_sCurrentHistogramTitle.contains("Mono"))
        {
            QVector <quint32> resetValues(255, 0);
            m_Histogram->populate(resetValues, "M");
        }

        if(m_sCurrentHistogramTitle.contains("8"))
        {
            QVector <quint32> resetValues(255, 0);
            m_Histogram->populate(resetValues, "R"); // Red
            m_Histogram->populate(resetValues, "G"); // Green
            m_Histogram->populate(resetValues, "B"); // Blue
        }

        if( m_sCurrentHistogramTitle.contains("BayerRG12"))
        {
            QVector <quint32> resetValues(4095, 0);
            m_Histogram->populate(resetValues, "R"); // Red
            m_Histogram->populate(resetValues, "G"); // Green
            m_Histogram->populate(resetValues, "B"); // Blue
        }

        if(m_sCurrentHistogramTitle.contains("YUV"))
        {
            QVector <quint32> resetValues(255, 0);
            m_Histogram->populate(resetValues, "Y"); // Y
            m_Histogram->populate(resetValues, "U"); // U
            m_Histogram->populate(resetValues, "V"); // V
        }

        m_Histogram->setBottomAxisX(nMaxWidth_XAxis);
        m_nXAxisMax = nMaxWidth_XAxis;
        m_sCurrentHistogramTitle = sNewHistogramTitle;
    }

    if( 1 == histogramData.size() && (m_sCurrentHistogramTitle.contains("Mono"))) // 1 color component -> Mono
            m_Histogram->populate(histogramData.at(0), "M");
    
    if( 3 == histogramData.size() &&  m_sCurrentHistogramTitle.contains("RGB8") ||  
        3 == histogramData.size() &&  m_sCurrentHistogramTitle.contains("BGR8") ||  
        m_sCurrentHistogramTitle.contains("Bayer") ) // 3 color components -> R,G,B
    {
        m_Histogram->populate(histogramData.at(0), "R"); // Red
        m_Histogram->populate(histogramData.at(1), "G"); // Green
        m_Histogram->populate(histogramData.at(2), "B"); // Blue
    }

    if( 3 == histogramData.size() && m_sCurrentHistogramTitle.contains("YUV") ) // 3 color components -> Y,U,V
    {
        m_Histogram->populate(histogramData.at(0), "Y"); // Y
        m_Histogram->populate(histogramData.at(1), "U"); // U
        m_Histogram->populate(histogramData.at(2), "V"); // V
    }

    /*if( m_sCurrentHistogramTitle.contains("Not Supported Yet") )
        m_Histogram->populate(histogramData.at(0), "M");*/
}

void HistogramWindow::onPrint ( void )
{
#ifndef QT_NO_PRINTER

    QPrinter printer( QPrinter::HighResolution );

    QString docName = m_Histogram->title().text();
    if ( !docName.isEmpty() )
    {
        docName.replace ( QRegExp ( QString::fromLatin1 ( "\n" ) ), tr ( " -- " ) );
        printer.setDocName ( docName );
    }

    printer.setCreator( "Histogram" );
    printer.setOrientation( QPrinter::Landscape );

    QPrintDialog dialog( &printer );
    if ( dialog.exec() )
    {
        QwtPlotRenderer renderer;

        if ( printer.colorMode() == QPrinter::GrayScale )
        {
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasBackground );
            renderer.setLayoutFlag( QwtPlotRenderer::FrameWithScales );
        }

        renderer.renderTo( m_Histogram, printer );
    }

#endif
}

void HistogramWindow::onExport ( void )
{
#ifndef QT_NO_PRINTER
    QString fileName = "Histogram.pdf";
#else
    QString fileName = "Histogram.png";
#endif

#ifndef QT_NO_FILEDIALOG
    const QList<QByteArray> imageFormats = QImageWriter::supportedImageFormats();

    QStringList filter;
    filter += "PDF Documents (*.pdf)";

#ifndef QWT_NO_SVG
    filter += "SVG Documents (*.svg)";
#endif
    filter += "Postscript Documents (*.ps)";

    if ( imageFormats.size() > 0 )
    {
        QString imageFilter( "Images (" );
        
        for ( int i = 0; i < imageFormats.size(); i++ )
        {
            if ( i > 0 )
                imageFilter += " ";

            imageFilter += "*.";
            imageFilter += imageFormats[i];
        }
        imageFilter += ")";

        filter += imageFilter;
    }

    fileName = QFileDialog::getSaveFileName( this, "Export File Name", fileName,
                                             filter.join( ";;" ), NULL, QFileDialog::DontConfirmOverwrite );
#endif

    if ( !fileName.isEmpty() )
    {
        QwtPlotRenderer renderer;
        renderer.setDiscardFlag( QwtPlotRenderer::DiscardBackground, false );
        renderer.renderDocument( m_Histogram, fileName, QSizeF( 300, 200 ), 85 );
    }
}

bool HistogramWindow::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Wheel)
    {
        m_AutoScaleYAxis->setChecked(false);
    }

    return false;
}
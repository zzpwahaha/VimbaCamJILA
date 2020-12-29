/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        HistogramThread.cpp

  Description: a worker thread to  process histogram data

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

=============================================================================*/


#include "stdint.h"
#include <algorithm>
#include <limits>
#include <exception>
#include "HistogramThread.h"
#include <QStringList>
#include "Helper.h"
#ifdef max
#undef max
#endif

#define MIN( a, b ) (((a) < (b)) ? (a) : (b))
#define MAX( a, b ) (((a) > (b)) ? (a) : (b))

HistogramThread::HistogramThread ( )
    : m_Stopping ( false )
    , m_FrameInfo(1)
{

}

HistogramThread::~HistogramThread ( )
{

    stopProcessing();
    wait();
}
void HistogramThread::stopProcessing()
{
    QMutexLocker local_lock( &m_Lock    );
    m_Stopping = true;
    m_FrameInfo.StopProcessing();
}

void HistogramThread::setThreadFrame ( const tFrameInfo &info )
{
    QMutexLocker local_lock( &m_Lock    );
    if( m_Stopping)
    {
        return;
    }
    m_FrameInfo.Enqueue( info );
}
enum
{
    display_interval = 40,
};
void HistogramThread::run()
{
    QTime       throttle_timer;
    int         last_time = 0;
    
    tFrameInfo  localFrameInfo;
    throttle_timer.start();
    while( m_FrameInfo.WaitData( localFrameInfo )  )
    {
        if( m_Stopping)
        {
            return;
        };
        const int time_elapsed = throttle_timer.elapsed();
        const int delta_time = time_elapsed -last_time;
        if( delta_time >= display_interval)
        {
            last_time = time_elapsed -(delta_time - display_interval);
            switch(localFrameInfo.PixelFormat() )
            {
                /* Monochrome, 8 bits */
            case VmbPixelFormatMono8: 
                histogramMono8( localFrameInfo );
                break;

                // Monochrome, 10 bits in 16 bits
            case VmbPixelFormatMono10:
                histogramNotSupportedYet( localFrameInfo );
                break;

                // Monochrome, 12 bits in 16 bits
            case VmbPixelFormatMono12:
                histogramNotSupportedYet( localFrameInfo );
                break;

                // Monochrome, 2x12 bits in 24 bits
            case VmbPixelFormatMono12Packed:
                histogramNotSupportedYet( localFrameInfo );
                break;

                // Monochrome, 14 bits in 16 bits
            case VmbPixelFormatMono14:
                histogramNotSupportedYet( localFrameInfo );
                break;

                // Monochrome, 16 bits
            case VmbPixelFormatMono16:
                histogramNotSupportedYet( localFrameInfo );
                break;

                // Bayer-color, 8 bits
            case VmbPixelFormatBayerGR8:
                histogramNotSupportedYet( localFrameInfo );
                break;

                // Bayer-color, 8 bits
            case VmbPixelFormatBayerRG8:
                histogramBayerRG8(  localFrameInfo );
                break;

                // Bayer-color, 8 bits
            case VmbPixelFormatBayerGB8:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 8 bits
            case VmbPixelFormatBayerBG8:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 10 bits in 16 bits
            case VmbPixelFormatBayerGR10:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 10 bits in 16 bits
            case VmbPixelFormatBayerRG10:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 10 bits in 16 bits
            case VmbPixelFormatBayerGB10:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 10 bits in 16 bits
            case VmbPixelFormatBayerBG10:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 12 bits in 16 bits
            case VmbPixelFormatBayerGR12:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 12 bits in 16 bits
            case VmbPixelFormatBayerRG12:
                histogramBayerRG12( localFrameInfo);
                break;

                // Bayer-color, 12 bits in 16 bits
            case VmbPixelFormatBayerGB12:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 12 bits in 16 bits
            case VmbPixelFormatBayerBG12:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // Bayer-color, 12 bits in 16 bits
            case VmbPixelFormatBayerRG12Packed:
                histogramBayerRG12Packed( localFrameInfo);
                break;

            case VmbPixelFormatBayerGR12Packed:
                histogramNotSupportedYet( localFrameInfo);
                break;

            case VmbPixelFormatBayerGB12Packed:
                histogramNotSupportedYet( localFrameInfo);
                break;

            case VmbPixelFormatBayerBG12Packed:
                histogramNotSupportedYet( localFrameInfo);
                break;
        
            case VmbPixelFormatBayerGR16:
                histogramNotSupportedYet( localFrameInfo);
                break;

            case VmbPixelFormatBayerRG16:
                histogramNotSupportedYet( localFrameInfo);
                break;

            case VmbPixelFormatBayerGB16:
                histogramNotSupportedYet( localFrameInfo);
                break;

            case VmbPixelFormatBayerBG16:
                histogramNotSupportedYet( localFrameInfo);
                break;

                /* RGB, 8 bits x 3 */
            case VmbPixelFormatRgb8:
                histogramRGB8( localFrameInfo);
                break;

                // BGR, 8 bits x 3
            case VmbPixelFormatBgr8:
                histogramBGR8( localFrameInfo);
                break;

                // ARGB, 8 bits x 4
            case VmbPixelFormatArgb8:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // RGBA, 8 bits x 4, legacy name
                /*
            case VmbPixelFormatRgba8:
                break;
                */
                // BGRA, 8 bits x 4
            case VmbPixelFormatBgra8:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // RGB, 16 bits x 3
            case VmbPixelFormatRgb16:
                histogramNotSupportedYet( localFrameInfo);
                break;

                // YUV 411 with 8 bits
            case VmbPixelFormatYuv411:
                histogramYUV411( localFrameInfo);
                break;

                // YUV  422
            case VmbPixelFormatYuv422:
                histogramYUV422( localFrameInfo);
                break;

                // YUV 444
            case VmbPixelFormatYuv444:
                histogramYUV444( localFrameInfo);
                break;

            default:
                histogramNotSupportedYet( localFrameInfo);
                break;
            }
        }
    }
}

void HistogramThread::histogramMono8 (  const tFrameInfo &info )
{
    QVector <QStringList> vStatistics;
    QStringList sStatistics;
    
    QVector<QVector<quint32> >  monoValues( 1, QVector<quint32>( 256,0 ) );
    QVector<quint32> &          histo = *monoValues.begin();
    
    const unsigned char *lSrc = info.Data();
    const unsigned char *lDest = info.Data() + ( info.Width() * info.Height() );

    while ( lSrc < lDest )
    {
        const VmbUint8_t tmp = *lSrc;
        ++histo[ *lSrc ];
        ++lSrc;
    }


    statistics<VmbUint8_t> stat = calcWeightedMean<VmbUint8_t>( histo );

    sStatistics << "Y" <<  QString::number(stat.Min) << QString::number(stat.Max) << QString::number(stat.Mean); 
    vStatistics.push_back(sStatistics);
    if( !m_Stopping )
    {
        emit histogramDataFromThread ( monoValues, 
                                        QString( "Mono8 (" + QString::number( info.Width() ) + 
                                        " x " + QString::number( info.Height() ) + ")"  ),
                                        (double) info.Size(),
                                        double(255),  vStatistics );
    }
}

void HistogramThread::histogramRGB8 ( const tFrameInfo &info )
{
    QVector <QStringList> vStatistics;
    QStringList redStatistics;
    QStringList blueStatistics;
    QStringList greenStatistics;

    QVector<QVector<quint32> > RGBValues;
    QVector<quint32> redValues;
    QVector<quint32> greenValues;
    QVector<quint32> blueValues;

    redValues.resize(256);
    greenValues.resize(256);
    blueValues.resize(256);

    const unsigned char *lSrc = info.Data();
    const unsigned char *lDest = info.Data() + ( info.Size());

    while ( lSrc < lDest )
    {
        // R
        ++redValues[ *lSrc ];
        // G
        ++lSrc;
        ++greenValues[ *lSrc ];
        // B
        ++lSrc;
        ++blueValues[ *lSrc ];
        // Move to Red
        ++lSrc;
    }

    RGBValues.push_back(redValues);
    RGBValues.push_back(greenValues);
    RGBValues.push_back(blueValues);

    statistics<VmbUint8_t> RedStat      = calcWeightedMean<VmbUint8_t>( redValues );
    statistics<VmbUint8_t> GreenStat    = calcWeightedMean<VmbUint8_t>( greenValues);
    statistics<VmbUint8_t> BlueStat     = calcWeightedMean<VmbUint8_t>( blueValues );

    redStatistics   << "Red"   <<  QString::number(RedStat.Min)   << QString::number(RedStat.Max)   << QString::number(RedStat.Mean); 
    greenStatistics << "Green" <<  QString::number(GreenStat.Min) << QString::number(GreenStat.Max) << QString::number(GreenStat.Mean);
    blueStatistics  << "Blue"  <<  QString::number(BlueStat.Min)  << QString::number(BlueStat.Max)  << QString::number(BlueStat.Mean);

    vStatistics.push_back(redStatistics);
    vStatistics.push_back(greenStatistics);
    vStatistics.push_back(blueStatistics);

    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread ( RGBValues, 
                                        QString("RGB8 (" + QString::number( info.Width()) + 
                                        " x " + QString::number( info.Height()) + ")"  ),
                                        (double) ( info.Size()/3),
                                        double(255), vStatistics );
    }
}

void HistogramThread::histogramBGR8 ( const tFrameInfo &info )
{
    QVector <QStringList> vStatistics;
    QStringList redStatistics;
    QStringList blueStatistics;
    QStringList greenStatistics;


    QVector<QVector<quint32> > RGBValues;
    QVector<quint32> redValues;
    QVector<quint32> greenValues;
    QVector<quint32> blueValues;

    redValues.resize(256);
    greenValues.resize(256);
    blueValues.resize(256);

    const unsigned char *lSrc   = info.Data();
    const unsigned char *lDest  = info.Data() + ( info.Size() );

    while ( lSrc < lDest )
    {
        // B
        ++blueValues[ *lSrc ];

        // G
        ++lSrc;
        ++greenValues[ *lSrc ];

        // R
        ++lSrc;
        ++redValues[ *lSrc ];

        // Move to Blue
        ++lSrc;
    }

    RGBValues.push_back(redValues);
    RGBValues.push_back(greenValues);
    RGBValues.push_back(blueValues);

    statistics<VmbUint8_t> RedStat      = calcWeightedMean<VmbUint8_t>( redValues );
    statistics<VmbUint8_t> GreenStat    = calcWeightedMean<VmbUint8_t>( greenValues);
    statistics<VmbUint8_t> BlueStat     = calcWeightedMean<VmbUint8_t>( blueValues );

    redStatistics   << "Red"   <<  QString::number(RedStat.Min)   << QString::number(RedStat.Max)   << QString::number(RedStat.Mean); 
    greenStatistics << "Green" <<  QString::number(GreenStat.Min) << QString::number(GreenStat.Max) << QString::number(GreenStat.Mean);
    blueStatistics  << "Blue"  <<  QString::number(BlueStat.Min)  << QString::number(BlueStat.Max)  << QString::number(BlueStat.Mean);

    vStatistics.push_back(redStatistics);
    vStatistics.push_back(greenStatistics);
    vStatistics.push_back(blueStatistics);

    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread (  RGBValues, 
                                        QString("BGR8 (" + QString::number( info.Width() ) + 
                                        " x " + QString::number( info.Height() ) + ")"  ),
                                        (double) ( info.Size()/3),
                                        double(255), vStatistics );
    }
}

void HistogramThread::histogramBayerRG8( const tFrameInfo &info )
{
    QVector <QStringList> vStatistics;
    QStringList redStatistics;
    QStringList blueStatistics;
    QStringList greenStatistics;


    QVector<QVector<quint32> > RGBValues;
    QVector<quint32> redValues;
    QVector<quint32> greenValues;
    QVector<quint32> blueValues;

    redValues.resize(256);
    greenValues.resize(256);
    blueValues.resize(256);

    const unsigned char *lSrc = info.Data();

    for ( VmbUint32_t y= info.Height()/2; y>0; y-- )// loop counter also works for odd height
    {
        // RG Row
        for ( VmbUint32_t x= info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Red
            ++redValues[ *lSrc ];

            // Green 1
            ++lSrc;
            ++greenValues [*lSrc ];

            // Move to Red
            ++lSrc;
        }
        if (1 == (info.Width() & 1) ) // handling for odd width
        {
            // Red
            ++redValues[ *lSrc ];

            // Move to next line
            ++lSrc;
        }

        // GB Row
        for ( VmbUint32_t x= info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Green 2
            ++greenValues[ *lSrc ];

            // Blue
            ++lSrc;
            ++blueValues[ *lSrc ];

            // Move to Green
            ++lSrc;
        }
        if (1 == (info.Width() &1)) // handling for odd width
        {
            // Green 2
            ++greenValues[ *lSrc ];

            // Move to next row
            ++lSrc;
        }
    }
    if (1 == (info.Height() & 1) ) // handling for last row if height is odd
    {
        // RG Row
        for ( VmbUint32_t x = info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Red
            ++redValues[ *lSrc ];

            // Green 1
            ++lSrc;
            ++greenValues [*lSrc ];

            // Move to Red
            ++lSrc;
        }
        if (1 == (info.Width() & 1 ) ) // handling for odd width
        {
            // Red
            ++redValues[ *lSrc ];

            // Move to next line
            ++lSrc;
        }
    }


    RGBValues.push_back(redValues);
    RGBValues.push_back(greenValues);
    RGBValues.push_back(blueValues);

    statistics<VmbUint8_t> RedStat      = calcWeightedMean<VmbUint8_t>( redValues );
    statistics<VmbUint8_t> GreenStat    = calcWeightedMean<VmbUint8_t>( greenValues);
    statistics<VmbUint8_t> BlueStat     = calcWeightedMean<VmbUint8_t>( blueValues );

    redStatistics   << "Red"   <<  QString::number(RedStat.Min)   << QString::number(RedStat.Max)   << QString::number(RedStat.Mean); 
    greenStatistics << "Green" <<  QString::number(GreenStat.Min) << QString::number(GreenStat.Max) << QString::number(GreenStat.Mean);
    blueStatistics  << "Blue"  <<  QString::number(BlueStat.Min)  << QString::number(BlueStat.Max)  << QString::number(BlueStat.Mean);

    vStatistics.push_back(redStatistics);
    vStatistics.push_back(greenStatistics);
    vStatistics.push_back(blueStatistics);

    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread (  RGBValues, 
                                        QString("BayerRG8 (" + QString::number( info.Width() ) + 
                                        " x " + QString::number( info.Height() ) + ")"  ),
                                        (double) ( info.Size()/3),
                                        double(255), vStatistics );
    }
}

void HistogramThread::histogramBayerRG12( const tFrameInfo &info )
{
    QVector <QStringList> vStatistics;
    QStringList redStatistics;
    QStringList blueStatistics;
    QStringList greenStatistics;

    QVector<QVector<quint32> > RGBValues;
    QVector<quint32> redValues;
    QVector<quint32> greenValues;
    QVector<quint32> blueValues;

    redValues.resize(4096);
    greenValues.resize(4096);
    blueValues.resize(4096);

    /*
        12 bit packed in 16 bit. Maximum: 0F FF (For safety we only use the low nibble of the MSB)
        Little Endian Layout for 16 bit Buffer RGGB macro pixel:    R_LSB | R_MSB | G_LSB | G_MSB
                                                                    G_LSB | G_MSB | B_LSB | B_MSB

        We work with 8 bit uchar pointers on 16 bit data. For reconstructing 16 bit shorts we 
        rather use bit wise operations than (short*)((void*)pCharBuffer).
    */

    uint16_t shortValue;
    const unsigned char *lSrc = info.Data();

    for ( VmbUint32_t y = info.Height()/2; y>0; y-- ) // loop counter also works for odd height
    {
        // RG Row
        for ( VmbUint32_t x = info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Red
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++redValues[ shortValue ];

            // Green 1
            lSrc+=2;
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++greenValues[ shortValue ];

            // Move to Red
            lSrc+=2;
        }
        if (1 == ( info.Width() & 1 )) // handling for odd width
        {
            // Red
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++redValues[ shortValue ];

            // Move to next line
            lSrc+=2;
        }

        // GB Row
        for ( VmbUint32_t x = info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Green 2
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++greenValues[ shortValue ];

            // Blue
            lSrc+=2;
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++blueValues[ shortValue ];

            // Move to Green
            lSrc+=2;
        }
        if (1 == (info.Width() & 1) ) // handling for odd width
        {
            // Green 2
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++greenValues[ shortValue ];

            // Move to next line
            lSrc+=2;
        }
    }
    if (1 == ( info.Height() & 1) )  // handling for last row if height is odd
    {
        // RG Row
        for ( VmbUint32_t x= info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Red
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++redValues[ shortValue ];

            // Green 1
            lSrc+=2;
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++greenValues[ shortValue ];

            // Move to Red
            lSrc+=2;
        }
        if (1 == ( info.Width() & 1) ) // handling for odd width
        {
            // Red
            shortValue = ((*(lSrc+1) & 0x0F) << 8) + *lSrc;
            ++redValues[ shortValue ];
            lSrc+=2;
        }
    }

    RGBValues.push_back(redValues);
    RGBValues.push_back(greenValues);
    RGBValues.push_back(blueValues);

    statistics<VmbUint16_t> RedStat     = calcWeightedMean<VmbUint16_t>( redValues );
    statistics<VmbUint16_t> GreenStat   = calcWeightedMean<VmbUint16_t>( greenValues);
    statistics<VmbUint16_t> BlueStat    = calcWeightedMean<VmbUint16_t>( blueValues );

    redStatistics   << "Red"   <<  QString::number(RedStat.Min)   << QString::number(RedStat.Max)   << QString::number(RedStat.Mean); 
    greenStatistics << "Green" <<  QString::number(GreenStat.Min) << QString::number(GreenStat.Max) << QString::number(GreenStat.Mean);
    blueStatistics  << "Blue"  <<  QString::number(BlueStat.Min)  << QString::number(BlueStat.Max)  << QString::number(BlueStat.Mean);

    vStatistics.push_back(redStatistics);
    vStatistics.push_back(greenStatistics);
    vStatistics.push_back(blueStatistics);

    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread (  RGBValues, 
                                        QString("BayerRG12 (" + QString::number( info.Width() ) + 
                                        " x " + QString::number( info.Height() ) + ")"  ),
                                        (double) ( info.Size() /4.5),
                                        double(4095), vStatistics );
    }
}

void HistogramThread::histogramBayerRG12Packed( const tFrameInfo &info )
{
    QVector <QStringList> vStatistics;
    QStringList redStatistics;
    QStringList blueStatistics;
    QStringList greenStatistics;


    QVector<QVector<quint32> > RGBValues;
    QVector<quint32> redValues;
    QVector<quint32> greenValues;
    QVector<quint32> blueValues;

    redValues.resize(4096);
    greenValues.resize(4096);
    blueValues.resize(4096);

    /*
        Buffer Layout in nibbles for RGGB macro pixel:      R_Hi R_Hi | G_Lo R_Lo | G_Hi G_Hi
                                                            G_Hi G_Hi | B_Lo G_Lo | B_Hi B_Hi

        Bit mask to get first nibble of 'mixed byte':       F0
        Bit mask to get second nibble of 'mixed byte':      0F

        First mask (if needed) then shift low nibbles to lsb, high nibbles to msb.
    */

    uint16_t unpackedValue;
    const unsigned char *lSrc = info.Data();

    for ( VmbUint32_t y = info.Height()/2; y>0; y-- ) // loop counter also works for odd height
    {
        // RG Row
        for ( VmbUint32_t x = info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Red
            unpackedValue = (*lSrc << 4) + (*(lSrc+1) & 0x0F);
            ++redValues[ unpackedValue ];

            // Green 1
            lSrc += 2;
            unpackedValue = (*lSrc << 4) + ((*(lSrc-1) & 0xF0) >> 4);
            ++greenValues[ unpackedValue ];
            // Move to Red
            ++lSrc;
        }
        if (1 == (info.Height() & 1) ) // handling for odd width
        {
            // Red
            unpackedValue = (*lSrc << 4) + (*(lSrc+1) & 0x0F);
            ++redValues[ unpackedValue ];
            ++lSrc;
        }
        
        // GB Row
        for ( VmbUint32_t x = info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Green 2
            unpackedValue = (*lSrc << 4) + (*(lSrc+1) & 0x0F);
            ++greenValues[ unpackedValue ];

            // Blue
            lSrc += 2;
            unpackedValue = (*lSrc << 4) + ((*(lSrc-1) & 0xF0) >> 4);
            ++blueValues[ unpackedValue ];

            // Move to Green
            ++lSrc;
        }
        if (1 == ( info.Width() & 1 ) ) // handling for odd width
        {
            // Green 2
            unpackedValue = (*lSrc << 4) + (*(lSrc+1) & 0x0F);
            ++greenValues[ unpackedValue ];
            ++lSrc;
        }
    }
    if (1 == (info.Height() & 1) ) // handling for odd height
    {
        // RG Row
        for ( VmbUint32_t x = info.Width()/2; x>0; x-- ) // loop counter also works for odd width
        {
            // Red
            unpackedValue = (*lSrc << 4) + (*(lSrc+1) & 0x0F);
            ++redValues[ unpackedValue ];

            // Green 1
            lSrc += 2;
            unpackedValue = (*lSrc << 4) + ((*(lSrc-1) & 0xF0) >> 4);
            ++greenValues[ unpackedValue ];

            // Move to Red
            ++lSrc;
        }
        if (1 == ( info.Height() & 1 ) ) // handling for odd width
        {
            // Red
            unpackedValue = (*lSrc << 4) + (*(lSrc+1) & 0x0F);
            ++redValues[ unpackedValue ];
            ++lSrc;
        }
    }

    RGBValues.push_back(redValues);
    RGBValues.push_back(greenValues);
    RGBValues.push_back(blueValues);

    statistics<VmbUint16_t> RedStat     = calcWeightedMean<VmbUint16_t>( redValues );
    statistics<VmbUint16_t> GreenStat   = calcWeightedMean<VmbUint16_t>( greenValues);
    statistics<VmbUint16_t> BlueStat    = calcWeightedMean<VmbUint16_t>( blueValues );

    redStatistics   << "Red"   <<  QString::number(RedStat.Min)   << QString::number(RedStat.Max)   << QString::number(RedStat.Mean); 
    greenStatistics << "Green" <<  QString::number(GreenStat.Min) << QString::number(GreenStat.Max) << QString::number(GreenStat.Mean);
    blueStatistics  << "Blue"  <<  QString::number(BlueStat.Min)  << QString::number(BlueStat.Max)  << QString::number(BlueStat.Mean);

    vStatistics.push_back(redStatistics);
    vStatistics.push_back(greenStatistics);
    vStatistics.push_back(blueStatistics);

    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread (  RGBValues, 
                                        QString("BayerRG12Packed (" + QString::number( info.Width() ) + 
                                        " x " + QString::number( info.Height() ) + ")"  ),
                                        (double) ( info.Size() / 4.5),
                                        double(4095), vStatistics );
    }
}

void HistogramThread::histogramYUV411 ( const tFrameInfo &info )
{
    QVector <QStringList> Statistics;
    QStringList yStatistics;
    QStringList uStatistics;
    QStringList vStatistics;

    QVector<QVector<quint32> > YUVValues;
    QVector<quint32> YValues;
    QVector<quint32> UValues;
    QVector<quint32> VValues;

    YValues.resize(256);
    UValues.resize(256);
    VValues.resize(256);

    /*
        6 Bytes per 4 pixel, buffer layout YUV Y Y Y macro pixel:  U | Y | Y | V | Y | Y
        12 bpp
    */

    const unsigned char *lSrc   = info.Data();
    const unsigned char *lDest  = info.Data() + ( info.Size() );

    while (lSrc < lDest)
    {
        // U
        ++UValues[ *lSrc ];

        // Y
        ++lSrc;
        ++YValues[ *lSrc ];

        // Y
        ++lSrc;
        ++YValues[ *lSrc ];

        // V
        ++lSrc;
        ++VValues[ *lSrc ];

        // Y
        ++lSrc;
        ++YValues[ *lSrc ];

        // Y
        ++lSrc;
        ++YValues[ *lSrc ];

        // Move to U
        ++lSrc;
    }

    YUVValues.push_back(YValues);
    YUVValues.push_back(UValues);
    YUVValues.push_back(VValues);

    statistics<VmbUint8_t> YStat = calcWeightedMean<VmbUint8_t>( YValues );
    statistics<VmbUint8_t> UStat = calcWeightedMean<VmbUint8_t>( UValues );
    statistics<VmbUint8_t> VStat = calcWeightedMean<VmbUint8_t>( VValues );

    yStatistics   << "Y" <<  QString::number(YStat.Min) << QString::number(YStat.Max) << QString::number(YStat.Mean); 
    uStatistics   << "U" <<  QString::number(UStat.Min) << QString::number(UStat.Max) << QString::number(UStat.Mean);
    vStatistics   << "V" <<  QString::number(VStat.Min) << QString::number(VStat.Max) << QString::number(VStat.Mean);

    Statistics.push_back(yStatistics);
    Statistics.push_back(uStatistics);
    Statistics.push_back(vStatistics);

    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread (  YUVValues, 
                                        QString("YUV411 (" + QString::number( info.Width() ) + 
                                        " x " + QString::number( info.Height() ) + ")"  ),
                                        (double) ( info.Size()/1.5),
                                        double(255), Statistics);
    }
}

void HistogramThread::histogramYUV422 ( const tFrameInfo &info )
{
    QVector <QStringList> Statistics;
    QStringList yStatistics;
    QStringList uStatistics;
    QStringList vStatistics;

    QVector<QVector<quint32> > YUVValues;
    QVector<quint32> YValues;
    QVector<quint32> UValues;
    QVector<quint32> VValues;

    YValues.resize(256);
    UValues.resize(256);
    VValues.resize(256);

    /*
        8 Bytes per 4 pixel, buffer layout YUV Y YUV Y macro pixel:  U | Y | V | Y | U | Y | V | Y
        16 bpp
    */

    const unsigned char *lSrc   = info.Data();
    const unsigned char *lDest  = info.Data() + ( info.Size() );

    while ( lSrc < lDest )
    {
        // U
        ++UValues[ *lSrc ];
        // Y
        ++lSrc;
        ++YValues[*lSrc];

        // V
        ++lSrc;
        ++VValues[*lSrc];

        // Y
        ++lSrc;
        ++YValues[*lSrc];

        // U
        ++lSrc;
        ++UValues[*lSrc];

        // Y
        ++lSrc;
        ++YValues[*lSrc];

        // V
        ++lSrc;
        ++VValues[*lSrc];

        // Y
        ++lSrc;
        ++YValues[*lSrc];
        // Move to U
        ++lSrc;
    }

    YUVValues.push_back(YValues);
    YUVValues.push_back(UValues);
    YUVValues.push_back(VValues);

    statistics<VmbUint8_t> YStat = calcWeightedMean<VmbUint8_t>( YValues );
    statistics<VmbUint8_t> UStat = calcWeightedMean<VmbUint8_t>( UValues );
    statistics<VmbUint8_t> VStat = calcWeightedMean<VmbUint8_t>( VValues );

    yStatistics   << "Y" <<  QString::number(YStat.Min) << QString::number(YStat.Max) << QString::number(YStat.Mean); 
    uStatistics   << "U" <<  QString::number(UStat.Min) << QString::number(UStat.Max) << QString::number(UStat.Mean);
    vStatistics   << "V" <<  QString::number(VStat.Min) << QString::number(VStat.Max) << QString::number(VStat.Mean);

    Statistics.push_back(yStatistics);
    Statistics.push_back(uStatistics);
    Statistics.push_back(vStatistics);

    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread (  YUVValues, 
                                        QString("YUV422 (" + QString::number( info.Width() ) + 
                                        " x " + QString::number( info.Height() ) + ")"  ),
                                        (double) ( info.Size() /2),
                                        double(255), Statistics);
    }
}

void HistogramThread::histogramYUV444 ( const tFrameInfo &info )
{
    QVector <QStringList> Statistics;
    QStringList yStatistics;
    QStringList uStatistics;
    QStringList vStatistics;

    uint        nYMin  = 255;
    uint        nYMax  = 0;
    VmbUint16_t nYMean = 0;

    uint        nUMin  = 255;
    uint        nUMax  = 0;
    VmbUint16_t nUMean = 0;

    uint        nVMin  = 255;
    uint        nVMax  = 0;
    VmbUint16_t nVMean = 255;


    QVector<QVector<quint32> > YUVValues;
    QVector<quint32> YValues;
    QVector<quint32> UValues;
    QVector<quint32> VValues;

    YValues.resize(256);
    UValues.resize(256);
    VValues.resize(256);

    /*
        3 Bytes per pixel, buffer layout YUV macro pixel:  U | Y | V
        24 bpp
    */

    const unsigned char *lSrc   = info.Data();
    const unsigned char *lDest  = info.Data() + ( info.Size() );

    while ( lSrc < lDest )
    {
        // U
        ++UValues[ *lSrc ];

        // Y
        ++lSrc;
        ++YValues[ *lSrc ];

        // V
        ++lSrc;
        ++VValues[ *lSrc ];

        // Move to U
        ++lSrc;
    }

    YUVValues.push_back(YValues);
    YUVValues.push_back(UValues);
    YUVValues.push_back(VValues);

    statistics<VmbUint8_t> YStat    = calcWeightedMean<VmbUint8_t>( YValues );
    statistics<VmbUint8_t> UStat    = calcWeightedMean<VmbUint8_t>( UValues );
    statistics<VmbUint8_t> VStat    = calcWeightedMean<VmbUint8_t>( VValues );

    yStatistics   << "Y" <<  QString::number(YStat.Min) << QString::number(YStat.Max) << QString::number(YStat.Mean); 
    uStatistics   << "U" <<  QString::number(UStat.Min) << QString::number(UStat.Max) << QString::number(UStat.Mean);
    vStatistics   << "V" <<  QString::number(VStat.Min) << QString::number(VStat.Max) << QString::number(VStat.Mean);

    Statistics.push_back(yStatistics);
    Statistics.push_back(uStatistics);
    Statistics.push_back(vStatistics);

    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread (  YUVValues, 
                                        QString("YUV444 (" + QString::number( info.Width() ) + 
                                        " x " + QString::number( info.Height() ) + ")"  ),
                                        (double) ( info.Size()/3),
                                        double(255), Statistics);
    }
}

void HistogramThread::histogramNotSupportedYet ( const tFrameInfo &info )
{
    QVector <QStringList> vStatistics;
    QVector<QVector<quint32> > noValues;
    QVector<quint32> values(256, 0);
    
    noValues.push_back(values);
    QMutexLocker local_lock( &m_Lock    );
    if( !m_Stopping )
    {
        emit histogramDataFromThread (    noValues, 
                                        QString("No Histogram Support For This Format Yet"),
                                        (double) info.Size(),
                                        double(255), vStatistics );
    }
}

template <typename T>
HistogramThread::statistics<T> HistogramThread::calcWeightedMean( const QVector<VmbUint32_t>& rValues )
{
    if( rValues.empty() )
    {
        throw std::exception();
    }
    VmbUint64_t     res = 0;
    VmbUint32_t     nNumPixels = 0;
    statistics<T>   stat;
    stat.Max = 0;
    stat.Min= std::numeric_limits<T>::max();
    for ( int i=0; i<rValues.size(); ++i )
    {
        const VmbUint64_t tmp = rValues[ i ];
        if( tmp != 0 )
        {
            if( i < stat.Min)
            {
                stat.Min = static_cast<T>(i);
            }
            if( i > stat.Max)
            {
                stat.Max= static_cast<T>(i);
            }
            res += i * tmp;
            nNumPixels += tmp;
        }
    }
    if( nNumPixels > 0 )
    {
         stat.Mean = (VmbUint16_t)(res / nNumPixels);
    }
    else
    {
        stat.Mean =  0;
    }
    return stat;
}

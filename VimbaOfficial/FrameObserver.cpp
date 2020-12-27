/*=============================================================================
  Copyright (C) 2012 - 2016 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        FrameObserver.cpp

  Description: Frame callback.
               

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


#include "FrameObserver.h"
#include "VmbImageTransformHelper.hpp"
#include <QFile>
#include <QMetaType>
#include <QTextStream>
#include <math.h>

FrameObserver::FrameObserver ( CameraPtr pCam )
    : IFrameObserver                ( pCam )
    , m_nFramesCounter              ( 0 )
    , m_nRawImagesToSave            ( 0 )
    , m_nRawImagesCounter           ( 0 )
    , m_bIsReset                    ( false )
    , m_EmitFrame                   ( true )
    , m_nFrames                     ( MAX_FRAMES_TO_COUNT )
    , m_bIsHistogramEnabled         ( false )
    , m_bColorInterpolation         ( true )
    , m_IsStopping                  ( false )
    , m_bTransferFullBitDepthImage  ( false )
    , m_pCam                        ( pCam )
{ 
    m_pImageProcessingThread    = QSharedPointer<ImageProcessingThread>(new ImageProcessingThread());
    m_pHistogramThread          = QSharedPointer<HistogramThread>(new HistogramThread());

    connect ( m_pImageProcessingThread.data(), SIGNAL ( frameReadyFromThread (QImage, const QString &, const QString &, const QString &) ), 
              this, SLOT ( getFrameFromThread (QImage, const QString &, const QString &, const QString &) ) );

    connect ( m_pImageProcessingThread.data(), SIGNAL ( frameReadyFromThreadFullBitDepth ( tFrameInfo) ), 
              this, SLOT ( getFullBitDepthFrameFromThread ( tFrameInfo) ) );

    // We need to register QVector <quint32> because it is not known to Qt's meta-object system
    qRegisterMetaType< QVector<QVector <quint32> > >("QVector<QVector <quint32> >");
    qRegisterMetaType< QVector <QStringList> >("QVector <QStringList>");

    // register tFrameInfo type to use with frameReadyFromObserver signal
    qRegisterMetaType< tFrameInfo >("tFrameInfo");

    connect ( m_pHistogramThread.data(), SIGNAL ( histogramDataFromThread ( const QVector<QVector <quint32> > &, const QString &, const double &, const double &, const QVector<QStringList> & )), 
              this, SLOT ( getHistogramDataFromThread ( const QVector<QVector <quint32> > &, const QString &, const double &, const double &, const QVector<QStringList>& )) );
}

FrameObserver::~FrameObserver ( void )
{
    if( NULL != m_pImageProcessingThread || m_pImageProcessingThread->isRunning() )
    {
        m_pImageProcessingThread->quit();
    }
}

void FrameObserver::FrameReceived ( const AVT::VmbAPI::FramePtr frame  )
{    
    QMutexLocker guard ( &m_StoppingLock );
    if( m_IsStopping )
    {
        return;
    }
    VmbFrameStatusType statusType = VmbFrameStatusInvalid;
    if( VmbErrorSuccess == frame->GetReceiveStatus(statusType) )
    {
        /* ignore any incompletely frame */
        if( VmbFrameStatusComplete != statusType )
        {
            m_pCam->QueueFrame(frame);
            return; 
        }

        m_nFramesCounter++;
        emit setFrameCounter ( m_nFramesCounter );
        
        m_FPSReceived.count( m_nFramesCounter );
        VmbUint64_t camera_frame_id;
        frame->GetFrameID( camera_frame_id );
        m_FPSCamera.count( camera_frame_id ); 
        static const char token[] = {'-','\\','|','/'};
        QString fps(token[ m_nFramesCounter%4] );
        if( m_FPSReceived.isValid() )
        {
            
            fps = QString("Rcv:") + QString::number( static_cast<size_t>(m_FPSReceived.CurrentFPS()*100)/100.0 );
            if( m_FPSCamera.isValid() )
            {
                fps += " Cam:" + QString::number(static_cast<size_t>(m_FPSCamera.CurrentFPS()*100)/100.0 );
            }
            if( m_pImageProcessingThread->getFPSCounter().isValid() )
            {
                fps += " Dis:" + QString::number(static_cast<size_t>(m_pImageProcessingThread->getFPSCounter().CurrentFPS()*100)/100.0 );
            }
        }
        emit setCurrentFPS( fps );
        if( m_EmitFrame )
        {
            setFrame( frame );
        }
    }
    m_pCam->QueueFrame(frame);
}

void FrameObserver::enableHistogram ( bool bIsHistogramEnabled )
{
    m_bIsHistogramEnabled = bIsHistogramEnabled;
}

void FrameObserver::setColorInterpolation (bool bState )
{
    m_bColorInterpolation = bState;
}

bool FrameObserver::getColorInterpolation ( void )
{
    return m_bColorInterpolation;
}

void FrameObserver::enableFullBitDepthTransfer(bool bIsFullBitDepthEnabled)
{
    m_bTransferFullBitDepthImage = bIsFullBitDepthEnabled;
}

void FrameObserver::setEmitFrame( bool bEmitFrame )
{
    m_EmitFrame = bEmitFrame;
}
void FrameObserver::setDisplayInterval ( unsigned int nInterval )
{
    m_pImageProcessingThread->LimitFrameRate( nInterval != 0  );
}

void FrameObserver::resetFrameCounter ( bool bIsRestart )
{
    if( bIsRestart )
    {
        m_nFramesCounter        = 0;
    }
    
    m_nFrames                   = MAX_FRAMES_TO_COUNT;
    m_bIsReset = true;
    emit setCurrentFPS ( "-" );
}

bool FrameObserver::setFrame (const AVT::VmbAPI::FramePtr &frame )
{
    try
    {
        tFrameInfo tmpInfo( frame, m_bColorInterpolation );
        m_pImageProcessingThread->setThreadFrame( tmpInfo, m_bTransferFullBitDepthImage);
        if(m_bIsHistogramEnabled)
        {
            m_pHistogramThread->setThreadFrame ( tmpInfo );
            m_pHistogramThread->start();
        }

        /* saving Raw Data */
        if( m_nRawImagesToSave > 0)
        {
            m_nRawImagesCounter++;
            QString s = m_sPathDestination;
            s.append("\\");
            s.append(m_sRawImagesName).append(QString::number(m_nRawImagesCounter)).append(".bin");
            m_nRawImagesToSave--;

            QFile rawFile(s);
            rawFile.open(QIODevice::WriteOnly);
            QDataStream out(&rawFile);
            out.writeRawData((const char*)tmpInfo.Data(), tmpInfo.Size() );
            rawFile.close();
        }
    }
    catch (...)
    {
        return false;
    }
        
    return true;
}

void FrameObserver::saveRawData ( unsigned int nNumberOfRawImagesToSave, const QString& sPath, const QString &sFileName )
{
    m_nRawImagesCounter = 0;
    m_nRawImagesToSave = nNumberOfRawImagesToSave;
    m_sPathDestination = sPath;
    m_sRawImagesName   = sFileName;
}


void FrameObserver::getFrameFromThread ( QImage image, const QString &sFormat, const QString &sHeight, const QString &sWidth )
{    
    emit frameReadyFromObserver (image, sFormat, sHeight, sWidth);    
}


void FrameObserver::getFullBitDepthFrameFromThread ( tFrameInfo mFullImageInfo )
{
    if ( true == m_bTransferFullBitDepthImage )
    {
        emit frameReadyFromObserverFullBitDepth ( mFullImageInfo );
    }
}


void FrameObserver::getHistogramDataFromThread ( const QVector<QVector <quint32> > &histData, const QString &sHistogramTitle, 
                                                 const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis ,const QVector <QStringList> &statistics)
{
    emit histogramDataFromObserver ( histData, sHistogramTitle, nMaxHeight_YAxis, nMaxWidth_XAxis, statistics );
}

void ImageProcessingThread::run()
{
    FrameData tmpFrameData;
    while(      !m_Stopping
            &&  m_FrameQueue.WaitData( tmpFrameData) 
            )
    {
        if( m_LimitFrameRate )
        {
            double currentTime = m_Timer.elapsed();
            if ( m_LastTime.isValid() )
            {
               const double frameLimit = 33.0; // limit about time for 33 fps
               const double deltaTime = currentTime - m_LastTime();
               if( frameLimit > deltaTime)
               {
                   continue;
               }
               const double newTime =currentTime + (frameLimit - deltaTime);
               m_LastTime(newTime);
            }
            else
            {
                m_LastTime( currentTime);
            }
        }
        m_FrameCount++;
        m_FPSCounter.count( m_FrameCount );
        QImage convertedImage( tmpFrameData.Width(), tmpFrameData.Height(), QImage::Format_RGB32 ); 
   
        if(! convertedImage.isNull())
        {

            if (NULL != tmpFrameData.GetFrameData()) // unlikely
            {
                VmbError_t error;

                VmbPixelFormatType outputPixelFormat;
                if(tmpFrameData.ColorInterpolation())
                {
                    outputPixelFormat = tmpFrameData.PixelFormat();
                }
                else
                {
                    try
                    {
                        outputPixelFormat = AVT::GetCompatibleMonoPixelFormatForRaw(tmpFrameData.PixelFormat() ) ;

                    }
                    catch(...)// lands here if GetCompatibleMonoPixelFromatForRaw throws
                    {
                        outputPixelFormat =tmpFrameData.PixelFormat() ;
                    }
                }
                error = AVT::VmbImageTransform( convertedImage, tmpFrameData.GetFrameData().data(), tmpFrameData.Width(),  tmpFrameData.Height(), outputPixelFormat );
        
                QString sFormat = Helper::convertFormatToString( outputPixelFormat );

                if( VmbErrorSuccess != error )
                {
                    sFormat = "Convert Error: " + QString::number(error,16);
                    if( VmbErrorBadParameter == error )
                        if (((tmpFrameData.Width()%4!=0) || (tmpFrameData.Height()%2!=0)))
                        {
                            sFormat.append(" (height or width not supported!)");
                        }
                        else
                        {
                            sFormat.append(" (PixelFormat not supported!)");
                        }
                    convertedImage = QImage();
                }

                {
                    if( !m_Stopping )
                    {
                        emit frameReadyFromThread (convertedImage, sFormat, QString::number (tmpFrameData.Height() ),
                                           QString::number (tmpFrameData.Width() ));
                    }
                }
            }
        }
        // send the full bit depth image if requested
        if ( true == tmpFrameData.TransformFullBitDepth() )
        {
            emit frameReadyFromThreadFullBitDepth ( tmpFrameData.FrameInfo() );
        }  
    }
}


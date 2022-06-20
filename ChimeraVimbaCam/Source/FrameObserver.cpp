//Description: Frame callback.
#include <stdafx.h>
#include "FrameObserver.h"
#include <QFile>
#include <QMetaType>
#include <QTextStream>
#include <math.h>
#include <windows.h>

void FrameObserver::Stopping()
{
    m_imgPThread.StopProcessing();
    m_FPSCamera.stop();
    m_FPSReceived.stop();
    QMutexLocker guard(&m_StoppingLock);
    m_IsStopping = true;
}

void FrameObserver::Starting()
{
    QMutexLocker guard(&m_StoppingLock);
    m_IsStopping = false;
    m_imgPThread.StartProcessing();
}

FrameObserver::FrameObserver ( CameraPtr pCam )
    : IFrameObserver                ( pCam )
    , m_nFramesCounter              ( 0 )
    , m_nFrames                     ( MAX_FRAMES_TO_COUNT )
    , m_IsStopping                  ( false )
    , m_pCam                        ( pCam )
    , m_imgPThread                  (   )
{ 
    //m_imgPThread = ImageProcessingThread();
    //m_pImageProcessingThread    = QSharedPointer<ImageProcessingThread>(new ImageProcessingThread());

    //also this for QCustomPlot
    qRegisterMetaType< QVector<ushort> >("QVector<ushort>");
    qRegisterMetaType< std::vector<ushort> >("std::vector<ushort>");

    // register tFrameInfo type to use with frameReadyFromObserver signal
    qRegisterMetaType< tFrameInfo >("tFrameInfo");
}

FrameObserver::~FrameObserver ( void )
{
    if(m_imgPThread.isRunning() )
    {
        m_imgPThread.quit();
    }
}

// this is a overriden virtual method in IFrameObserver class, overwirte it to handle the received frame
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
        static const char token[] = {'-','\\','|','/'}; //quite interesting :) -zzp
        QString fps(token[m_nFramesCounter % 4]);
        if( m_FPSReceived.isValid() )
        {
            fps = QString("Rcv:") + qstr(m_FPSReceived.CurrentFPS(), 2);
            if( m_FPSCamera.isValid() )
            {
                fps += " Cam:" + qstr(m_FPSCamera.CurrentFPS(), 2);
            }
            if (m_imgPThread.getFPSCounter().isValid())
            {
                fps += " Dis:" + qstr(m_imgPThread.getFPSCounter().CurrentFPS(), 2);
            }
        }
        emit setCurrentFPS( fps );
        setFrame( frame );
    }
    m_pCam->QueueFrame(frame);
}

void FrameObserver::resetFrameCounter ( bool bIsRestart )
{
    if( bIsRestart )
    {
        m_nFramesCounter        = 0;
    }
    
    m_nFrames                   = MAX_FRAMES_TO_COUNT;
    emit setCurrentFPS ( "-" );
}

bool FrameObserver::setFrame (const AVT::VmbAPI::FramePtr &frame )
{
    try
    {
        tFrameInfo tmpInfo( frame );
        // the following funciton will call enque function in the processthread, which will wake the lock in the run function
        m_imgPThread.setThreadFrame( tmpInfo );
    }
    catch (...)
    {
        return false;
    }
    return true;
}






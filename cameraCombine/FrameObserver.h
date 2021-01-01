/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        FrameObserver.h

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


#ifndef FRAMEOBSERVER_H
#define FRAMEOBSERVER_H

#include <QObject>
#include <QImage>
#include <QVector>
#include <QTime>
#include <QThread>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

#include "Helper.h"
#include "Histogram/HistogramThread.h"
#include <VimbaCPP/Include/IFrameObserver.h>
#include <VimbaCPP/Include/Frame.h>
#include <VimbaCPP/Include/Camera.h>
#include "VmbImageTransformHelper.hpp"

/* Number of frames in use to count the first FPS since start*/
const unsigned int MAX_FRAMES_TO_COUNT = 50;

using AVT::VmbAPI::CameraPtr;
using AVT::VmbAPI::FramePtr;

#ifdef _WIN32
/*high resolution timer for windows*/
class PrecisionTimer
{
    const LARGE_INTEGER     m_Frequency;
    LARGE_INTEGER           m_Time;
    static LARGE_INTEGER getFrq()
    {
        LARGE_INTEGER tmp;
        QueryPerformanceFrequency( & tmp );
        return tmp;
    }
public:
    PrecisionTimer()
        : m_Frequency( getFrq() )
    {
    }
    LARGE_INTEGER Frequency() const { return m_Frequency; }
    LARGE_INTEGER Ticks() const
    {
        LARGE_INTEGER tmp;
        QueryPerformanceCounter( &tmp );
        return tmp;
    }
    void start()
    {
        QueryPerformanceCounter( &m_Time );
    }
    double elapsed() const
    {
        LARGE_INTEGER tmpTime;
        QueryPerformanceCounter( &tmpTime );
        tmpTime.QuadPart -= m_Time.QuadPart;
        return (tmpTime.QuadPart*1000.0)/ m_Frequency.QuadPart;
    }
};

typedef PrecisionTimer timer_type;
#else
typedef QTime timer_type;
#endif

/*class to calculate frames per second*/
class FPSCounter
{
    double          m_CurrentFPS;
    timer_type      m_Timer;
    bool            m_IsRunning;
    VmbInt64_t      m_LastFrameID;
    bool            m_Valid;
public:
    double  CurrentFPS()    const { return m_CurrentFPS; }

    FPSCounter( )
        : m_IsRunning( false )
        , m_LastFrameID( 0 )
        , m_Valid( false )
    {

    }
    void count( VmbInt64_t id )
    {
        if( ! m_IsRunning )
        {
            m_LastFrameID   = id;
            m_IsRunning     = true;
            m_Valid         = false;
            m_Timer.start();
            return;
        }
        double      time_ms     = m_Timer.elapsed();
        VmbInt64_t  delta_id = id - m_LastFrameID;
        if( (time_ms > 1000  && delta_id != 0) )
        {
            m_CurrentFPS    = (delta_id*1000) / time_ms;
            m_LastFrameID   = id;
            m_Valid         = true;
            m_Timer.start();
        }
    }
    bool isValid() const {return m_Valid;}
    void stop()
    {
        m_CurrentFPS    = 0.0;
        m_LastFrameID   = 0;
        m_Valid         = false;
        m_IsRunning     = false;
    }
    bool isRunning() const { return m_IsRunning; }


};

template <typename T>
class ValueWithState
{
    T       m_Value;
    bool    m_isValid;
public:
    ValueWithState()
        : m_Value( T())
        , m_isValid( false )
    {
    }
    ValueWithState( const ValueWithState<T> & o)
        : m_Value( o() )
        , m_isValid( o.isValid() )
    {}
    void        Invalidate()
    {
        m_isValid = false;
        m_Value = T();
    }
    bool        isValid() const { return m_isValid; }
    const T&    operator()() const { return m_Value; }
    T&          operator()() { return m_Value; }
    void        operator()( const T&o)
    {
        m_Value = o;
        m_isValid = true;
    }

};
class ImageProcessingThread : public QThread
{
    Q_OBJECT
    
    public:

    protected:
        struct FrameData
        {
        private:
            bool                            m_TransferFullBitDepthImage;
            tFrameInfo                      m_FrameInfo;
        public:
            FrameData()
            {}
            FrameData( const tFrameInfo &info, bool FullBitDepthImage)
                : m_TransferFullBitDepthImage( FullBitDepthImage )
                , m_FrameInfo( info )
            {}
            tFrameInfo                      FrameInfo()                                         const   { return m_FrameInfo; }
            bool                            TransformFullBitDepth()                             const   { return m_TransferFullBitDepthImage; }
            bool                            ColorInterpolation()                                const   { return m_FrameInfo.UseColorInterpolation(); }
            QSharedPointer<unsigned char>   GetFrameData()                                      const   { return m_FrameInfo.DataPtr(); }
            VmbPixelFormatType              PixelFormat()                                       const   { return m_FrameInfo.PixelFormat(); }
            VmbUint32_t                     Width()                                             const   { return m_FrameInfo.Width();}
            VmbUint32_t                     Height()                                            const   { return m_FrameInfo.Height(); }
            VmbUint32_t                     Size()                                              const   { return m_FrameInfo.Size(); }
        };
    private:
        ConsumerQueue<FrameData>    m_FrameQueue;
        bool                        m_Stopping;
        size_t                      m_DroppedFrames;

        VmbUint64_t                 m_FrameCount;
        FPSCounter                  m_FPSCounter;
        timer_type                  m_Timer;
        bool                        m_LimitFrameRate;
        ValueWithState<double>      m_LastTime;
    public:
        ImageProcessingThread( size_t MaxFrames = 3)
            : m_FrameQueue( MaxFrames )
            , m_Stopping( false )
            , m_DroppedFrames( 0 )
            , m_FrameCount ( 0 )
            , m_LimitFrameRate( false )
        {
            m_Timer.start();
        }
        const FPSCounter & getFPSCounter() const { return m_FPSCounter;}
        ~ImageProcessingThread()
        {
            StopProcessing();
        }
        void StopProcessing()
        {
            if( ! m_Stopping)
            {
                m_LastTime.Invalidate();
                m_Stopping = true;
                m_FrameQueue.StopProcessing();
                wait();
            }
        }
        void StartProcessing()
        {
            m_Stopping = false;
            m_FrameQueue.StartProcessing();
            start();
        }
        size_t DroppedFrames() const { return m_DroppedFrames; }

        void setThreadFrame ( const tFrameInfo &FrameInfo, bool FullBitDepthImage  )
        {
            FrameData tmpFrameData( FrameInfo,FullBitDepthImage);
            m_FrameQueue.Enqueue( tmpFrameData );
        }
        void LimitFrameRate( bool v ) { m_LimitFrameRate= v; }
private:

    protected:
            virtual void run();

    private:

    signals:
            void frameReadyFromThread             ( QImage image, const QString &sFormat, const QString &sHeight, const QString &sWidth );
            void frameReadyFromThreadFullBitDepth ( tFrameInfo mFullImageInfo );
};


class FrameObserver : public QObject, public AVT::VmbAPI::IFrameObserver
{
    Q_OBJECT

    public:

    protected:

    private:
        typedef QSharedPointer<unsigned char>   FrameDataPtr;

        FPSCounter                              m_FPSCamera;
        FPSCounter                              m_FPSReceived;
        CameraPtr                               m_pCam;                     // camera to wait for frames on

        QSharedPointer<HistogramThread>         m_pHistogramThread;         // histogram calculation thread

        unsigned int                            m_nFrames;
        unsigned int                            m_nFramesCounter;       // nr frames received

        QSharedPointer<ImageProcessingThread>   m_pImageProcessingThread;   // Image processing thread
        bool                                    m_bIsReset;
        bool                                    m_EmitFrame;
        /* Saving Raw Data */
        unsigned int                        m_nRawImagesToSave;
        unsigned int                        m_nRawImagesCounter;
        QString                             m_sPathDestination;
        QString                             m_sRawImagesName;

        /* Histogram */
        bool                                m_bIsHistogramEnabled;
        bool                                m_bColorInterpolation;

        bool                                m_IsStopping;
        QMutex                              m_StoppingLock;

        bool                                m_bTransferFullBitDepthImage;
    public:
            void Stopping()
            {
                m_pImageProcessingThread->StopProcessing();
                m_FPSCamera.stop();
                m_FPSReceived.stop();
                QMutexLocker guard( &m_StoppingLock );
                m_IsStopping = true;
            }
            void Starting()
            {
                QMutexLocker guard( &m_StoppingLock );
                m_IsStopping = false;
                m_pImageProcessingThread->StartProcessing();
            }
            FrameObserver ( CameraPtr pCam );
           ~FrameObserver ();

            virtual void FrameReceived          ( const FramePtr frame );
            void resetFrameCounter              ( bool bIsRestart );
            void setDisplayInterval             ( unsigned int nInterval );
            void saveRawData                    ( unsigned int nNumberOfRawImagesToSave, const QString &sPath, const QString &sFileName);
            void enableHistogram                ( bool bIsHistogramEnabled );
            void setColorInterpolation          ( bool bState);
            bool getColorInterpolation          ( void );
            void enableFullBitDepthTransfer     ( bool bIsFullBitDepthEnabled );
            void setEmitFrame                   ( bool bEmitFrame );
            
    protected:
           
    private:
            bool setFrame                       ( const FramePtr &frame );
            
    private slots:
            void getFrameFromThread             ( QImage image, const QString &sFormat, const QString &sHeight, const QString &sWidth );
            void getFullBitDepthFrameFromThread ( tFrameInfo mFullImageInfo );
            void getHistogramDataFromThread     ( const QVector<QVector <quint32> > &histData, const QString &sHistogramTitle, 
                                                  const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis, const QVector <QStringList> &statistics );
    signals:
            void frameReadyFromObserver              ( QImage image, const QString &sFormat, const QString &sHeight, const QString &sWidth );
            void frameReadyFromObserverFullBitDepth  ( tFrameInfo mFullImageInfo );
            void setCurrentFPS                       ( const QString &sFPS );
            void setFrameCounter                     ( const unsigned int &nFrame );
            void histogramDataFromObserver           ( const QVector<QVector <quint32> > &histData, const QString &sHistogramTitle, 
                                                       const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis, const QVector <QStringList> &statistics );
};


#endif
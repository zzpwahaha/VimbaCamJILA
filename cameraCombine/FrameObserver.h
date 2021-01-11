

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
#include "ImageProcessingThread.h"
#include <VimbaCPP/Include/IFrameObserver.h>
#include <VimbaCPP/Include/Frame.h>
#include <VimbaCPP/Include/Camera.h>
#include "VmbImageTransformHelper.hpp"

/* Number of frames in use to count the first FPS since start*/
const unsigned int MAX_FRAMES_TO_COUNT = 50;

using AVT::VmbAPI::CameraPtr;
using AVT::VmbAPI::FramePtr;


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
            
            const QSharedPointer<ImageProcessingThread>& ImageProcessThreadPtr() const { return m_pImageProcessingThread; }
            
protected:
           
    private:
            bool setFrame                       ( const FramePtr &frame );
            
    private slots:
            void getFrameFromThread             ( QImage image, const QString &sFormat, const QString &sHeight, const QString &sWidth );
            void getFrameFromThread             ( QVector<ushort> vec1d, const QString& sFormat, const QString& sHeight, const QString& sWidth);
            void getFrameFromThread             ( std::vector<ushort> vec1d, const QString& sFormat, const QString& sHeight, const QString& sWidth);

            void getFullBitDepthFrameFromThread ( tFrameInfo mFullImageInfo );
            void getHistogramDataFromThread     ( const QVector<QVector <quint32> > &histData, const QString &sHistogramTitle, 
                                                  const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis, const QVector <QStringList> &statistics );
    signals:
            void frameReadyFromObserver              ( QImage image, const QString &sFormat, const QString &sHeight, const QString &sWidth );
            void frameReadyFromObserver              ( QVector<ushort> vec1d, const QString& sFormat, const QString& sHeight, const QString& sWidth);
            void frameReadyFromObserver              ( std::vector<ushort> vec1d, const QString& sFormat, const QString& sHeight, const QString& sWidth);

            void frameReadyFromObserverFullBitDepth  ( tFrameInfo mFullImageInfo );
            void setCurrentFPS                       ( const QString &sFPS );
            void setFrameCounter                     ( const unsigned int &nFrame );
            void histogramDataFromObserver           ( const QVector<QVector <quint32> > &histData, const QString &sHistogramTitle, 
                                                       const double &nMaxHeight_YAxis, const double &nMaxWidth_XAxis, const QVector <QStringList> &statistics );

            
};


#endif
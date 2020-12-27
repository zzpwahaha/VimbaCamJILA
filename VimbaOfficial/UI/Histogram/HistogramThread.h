/*=============================================================================
  Copyright (C) 2012 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

  File:        HistogramThread.h

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


#ifndef HISTOGRAMTHREAD_H
#define HISTOGRAMTHREAD_H

#include <QTime>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <qwaitcondition.h>
#include "Helper.h"

#include <VimbaCPP/Include/IFrameObserver.h>
#include <VimbaCPP/Include/Frame.h>
#include <VimbaCPP/Include/Camera.h>


class HistogramThread : public QThread
{
    Q_OBJECT

        typedef QSharedPointer<unsigned char> data_storage;
    public:

    private:
        template<typename T>    
        struct statistics
        {
            typedef T value_type;
            T Min;
            T Max;
            T Mean;
        };
        ConsumerQueue<tFrameInfo>     m_FrameInfo;
        QMutex                        m_Lock;
        bool                          m_Stopping;
    public:
        HistogramThread ( );
        ~HistogramThread ( void );

        void stopProcessing();
        void setThreadFrame ( const tFrameInfo &info );
    

    protected:
        virtual void run();
        void histogramMono8           ( const tFrameInfo &info );
        void histogramRGB8            ( const tFrameInfo &info );
        void histogramBGR8            ( const tFrameInfo &info );
        void histogramYUV411          ( const tFrameInfo &info );
        void histogramYUV422          ( const tFrameInfo &info );
        void histogramYUV444          ( const tFrameInfo &info );
        void histogramNotSupportedYet ( const tFrameInfo &info );
        void histogramBayerRG12Packed ( const tFrameInfo &info );
        void histogramBayerRG8        ( const tFrameInfo &info );
        void histogramBayerRG12       ( const tFrameInfo &info );

    private:
        template<typename T>
        statistics<T> calcWeightedMean( const QVector<VmbUint32_t>& rValues );
          
    signals:
          void histogramDataFromThread ( const QVector<QVector <quint32> > &histData, 
                                         const QString &sHistogramTitle, 
                                         const double &nMaxHeight_YAxis, 
                                         const double &nMaxWidth_XAxis,
                                         const QVector <QStringList> &statistics);
};

#endif
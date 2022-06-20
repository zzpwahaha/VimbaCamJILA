#pragma once
#include <QString>

#include "VimbaCPP/Include/VimbaCPPCommon.h"
#include "VimbaCPP/Include/Frame.h"
#include "VimbaCPP/Include/Camera.h"

using AVT::VmbAPI::CameraPtr;
using AVT::VmbAPI::FramePtr;

#include <QSharedPointer>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QVector>
#include <QList>
#include <exception>
#include <stdexcept>

#include <cstring>
#include <QFuture>
#include <QFutureSynchronizer>
#include <QtConcurrentRun>


/** runable for memcopy/memmove
*/
struct copy_runable
{
    void* m_Src;      // source data pointer
    void* m_Dst;      // destination data pointer
    size_t      m_Size;     // size to copy
    /** run will execute the memcopy.
    */
    void run() const
    {
        std::memmove(m_Dst, m_Src, m_Size);
    }
};
/** thread function for a copy
*/
inline void copy_fun(copy_runable* prm)
{
    prm->run();
}
/** multi threaded mem copy
*/
template <size_t THREAD_COUNT>
inline void memcpy_threaded(void* dst, void* src, size_t len)
{
    if (len <= 0x200000) // if within L3 cache size use memcopy
    {
        std::memmove(dst, src, len);
        return;
    }
    size_t                      copy_len = len / THREAD_COUNT;
    copy_runable                prm[THREAD_COUNT];
    QFutureSynchronizer<void>   RoadBlock;
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        prm[i].m_Src = src;
        prm[i].m_Dst = dst;
        prm[i].m_Size = copy_len;

        RoadBlock.addFuture(QtConcurrent::run(copy_fun, &prm[i]));

        src = (char*)src + copy_len;
        dst = (char*)dst + copy_len;
        len -= copy_len;
        if (len < copy_len)
        {
            copy_len = len;
        }
    }
    RoadBlock.waitForFinished();
}



// QSharedPoinnter custom deleter
template <class T>
void DeleteArray(T *pArray)
{
    delete [] pArray;
}


/** consumer frame queue with max size limit.
*/
template <typename DATA_TYPE>
class ConsumerQueue
{
    QList<DATA_TYPE>    m_Data;             // frame storage
    mutable QMutex      m_Lock;             // queue synch lock
    QWaitCondition      m_DataAvailable;    // data available condition
    bool                m_Stopping;         // state of the queue
    const size_t        m_MaxSize;          // max number of frames before the oldest will get dropped
public:
    /**constructor with max queue size*/
    explicit ConsumerQueue( size_t maxSize = 0)
        : m_MaxSize( maxSize )
        , m_Stopping( false )
    {}
    /**start queue processing*/
    void StartProcessing()
    {
        QMutexLocker local_lock( &m_Lock );
        m_Stopping = false;
    }
    /**stop queue processing, signaling Halt to WaitData*/
    void StopProcessing()
    {
        {
            QMutexLocker local_lock( &m_Lock );
            m_Data.clear();
            m_Stopping = true;
        }
        m_DataAvailable.wakeOne();
    }
    /**test if queue is empty*/
    bool IsEmpty() const
    {
        QMutexLocker local_lock( & m_Lock );
        return m_Data.empty();
    }
    /**enqueue item in queue
    * might throw bad_alloc
    */
    void Enqueue( const DATA_TYPE &v)
    {
        {
            QMutexLocker local_lock( &m_Lock );
            if( m_Stopping)
            {
                m_DataAvailable.wakeOne();
                return;
            }
            if( m_MaxSize != 0 && m_Data.size() == m_MaxSize )
            {
                m_Data.pop_front();
            }
            m_Data.push_back( v );
        }
        m_DataAvailable.wakeOne();
    }
    /**wait for data item from queue
    * if queue is not empty, an item is returned, else function waits for either a data item to be available,
    * or StopProccessing signal
    * might return false if queue item canot be copied to v (bad alloc)
    * returns false if StopProcessing is called
    */
    bool WaitData( DATA_TYPE &v)
    {
        QMutexLocker local_lock( &m_Lock );
        while( !m_Stopping && m_Data.empty() )
        {
            m_DataAvailable.wait( local_lock.mutex() );
        }
        if( m_Stopping)
        {
            return false;
        }
        try
        {
            v = m_Data.front();
            m_Data.pop_front();
            return true;
        }
        catch(...)
        {
            return false;
        }
    }
};
/**base class for frame data infos*/
class BaseFrame
{
    VmbPixelFormatType      m_PixelFormat;              // Vimba frame pixel format
    VmbUint32_t             m_Width;                    // frame width
    VmbUint32_t             m_Height;                   // frame height
    VmbUint32_t             m_Size;                     // frame size in bytes
    bool                    m_ColorInterpolation;       // requirement if raw data shall be interpolated or displayed as mono

public:
    /**get frame pixel format.*/
    VmbPixelFormatType      PixelFormat()                           const   { return m_PixelFormat; }
    /**set frame pixel format.*/
    void                    PixelFormat(VmbPixelFormatType format)          { m_PixelFormat = format; }
    /**get frame widht.*/
    VmbUint32_t             Width()                                 const   { return m_Width; }
    /**set frame width.*/
    void                    Width(VmbUint32_t w)                            {m_Width = w; }
    /**get frame height.*/
    VmbUint32_t             Height()                                const   { return m_Height; }
    /**set frame height.*/
    void                    Height(VmbUint32_t h)                           { m_Height = h; }
    /**get frame size in bytes.*/
    VmbUint32_t             Size()                                  const   { return m_Size; }
    /**set frame size in bytes.*/
    void                    Size(VmbUint32_t s)                             { m_Size = s; }
    /**get state of color interpolation.*/
    bool                    UseColorInterpolation()                 const   { return m_ColorInterpolation; }
    /**set state of color interpolation.*/
    void                    UseColorInterpolation(bool v = false)                   { m_ColorInterpolation = v; }
    /**reset frame information from Vimba frame.*/
    void Set( const FramePtr &frame, bool color_interpolation = false)
    {
        if( frame== NULL )
        {
            throw std::invalid_argument("null pointer received");
        }
        UseColorInterpolation( color_interpolation);
        VmbFrameStatusType statusType = VmbFrameStatusInvalid;
        if( VmbErrorSuccess != frame->GetReceiveStatus( statusType ) )
        {
            throw std::runtime_error( "frame status not readable");
        }
        if( statusType != VmbFrameStatusComplete)
        {
            throw std::runtime_error("frame status not complete");
        }
        if( VmbErrorSuccess != frame->GetPixelFormat( m_PixelFormat ) )
        {
            throw std::runtime_error("could not get pixel format from frame");
        }
        if( VmbErrorSuccess != frame->GetWidth( m_Width ) )
        {
            throw std::runtime_error("could not get width from frame");
        }
        if( VmbErrorSuccess != frame->GetHeight( m_Height ) )
        {
            throw std::runtime_error("could not get height from frame");
        }
        if( VmbErrorSuccess != frame->GetImageSize( m_Size ) )
        {
            throw std::runtime_error("could not get image size from frame");
        }
    }
    /**default constructor.*/
    BaseFrame( )
    {}
    /**construct from Vimba frame informations.*/
    BaseFrame( const FramePtr &frame, bool color_interpolation = false)
    {
        Set( frame, color_interpolation);
    }
    /**construct from information values.*/
    BaseFrame   (   VmbPixelFormatType      pixelFormat,
                    VmbUint32_t             width,
                    VmbUint32_t             height,
                    VmbUint32_t             size,
                    bool                    colorInterpolation = false
                )
        : m_PixelFormat( pixelFormat )
        , m_Width( width )
        , m_Height( height )
        , m_Size( size )
        , m_ColorInterpolation( colorInterpolation )
    {
    }
};

typedef QSharedPointer<VmbUchar_t> FrameDataPtr;
/**class to hold info and a copy of frame data*/
class tFrameInfo: public BaseFrame
{
    FrameDataPtr    m_pFrameData;
public:
    /**access to shared data pointer*/
    FrameDataPtr        DataPtr()   const   { return m_pFrameData; }
    /**access to shared data pointer*/
    FrameDataPtr        DataPtr()           { return m_pFrameData; }

    /**access to raw data pointer*/
    const VmbUchar_t*   Data()      const   { return m_pFrameData.data(); }
    /**access to raw data pointer*/
    VmbUchar_t*         Data()              { return m_pFrameData.data(); }

    /**default constructor*/
    tFrameInfo()
    {}
    /**copy construct from Vimba frame*/
    tFrameInfo( const FramePtr& frame, bool color_interpolation = false );
};
typedef QSharedPointer<tFrameInfo> FrameInfoPtr;    // shared pointer for frame infos

enum VimbaViewerLogCategory
{
    VimbaViewerLogCategory_OK      = 0,
    VimbaViewerLogCategory_WARNING = 1,
    VimbaViewerLogCategory_ERROR   = 2,
    VimbaViewerLogCategory_INFO    = 3,
};

namespace Helper
{
        /**format ip4 to string.*/
        QString         IPv4ToString                ( const VmbInt64_t nIPAddress, const bool swap );
        /**format MAC address to string.*/
        QString         MACToString                 ( const VmbInt64_t nNetworkOrderMACAddress );
        /**format ip4 string to integer string.*/
        QString         displayValueToIPv4          ( const QString& sDecValue );
        /**format MAC address string to integer string.*/
        QString         displayValueToMAC           ( const QString& sMacAddress );

        /**format return code to string.*/
        QString         mapReturnCodeToString       ( const VmbError_t nReturnCode);

        /**format pixel format to string*/
        QString         convertFormatToString       ( const VmbPixelFormatType format);

        /**lookup if feature needs IP4 format.*/
        bool            needsIPv4Format             ( const QString &sFeatureName );
        /**lookup if feature needs IP4 format.*/
        bool            needsIPv4Format             ( const QString &sFeatureName, const std::string &sRepresentation);

        /**lookup if feature has auto mode.*/
        bool            isAutoFeature               ( const QString &sFeatureName);

        /**convert IP4 string to integer*/
        VmbInt64_t      StringToIPv4                ( const QString &sIPString, const bool swap );

        VmbInt64_t      StringToMAC                 (const QString& sMACString);

        /* constants */
        extern const QString         m_GIGE_STAT_FRAME_DELIVERED;
        extern const QString         m_GIGE_STAT_FRAME_DROPPED;
        extern const QString         m_GIGE_STAT_FRAME_RATE;
        extern const QString         m_GIGE_STAT_FRAME_RESCUED;
        extern const QString         m_GIGE_STAT_FRAME_SHOVED;
        extern const QString         m_GIGE_STAT_FRAME_UNDERRUN;
        extern const QString         m_GIGE_STAT_LOCAL_RATE;
        extern const QString         m_GIGE_STAT_PACKET_ERRORS;
        extern const QString         m_GIGE_STAT_PACKET_MISSED;
        extern const QString         m_GIGE_STAT_PACKET_RECEIVED;
        extern const QString         m_GIGE_STAT_PACKET_REQUESTED;
        extern const QString         m_GIGE_STAT_PACKET_RESENT;
        extern const QString         m_GIGE_STAT_TIME_ELAPSED;
        extern const QString         m_EXPOSURE_AUTO;
        extern const QString         m_GAIN_AUTO;
        extern const QString         m_BALANCE_WHITE_AUTO;

};


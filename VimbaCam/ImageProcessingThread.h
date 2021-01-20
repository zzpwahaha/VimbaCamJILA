#pragma once
#include <QThread>

#include <QImage>
#include "Helper.h"
#include <QVector>
#include <VimbaCPP/Include/Frame.h>

#ifdef _WIN32
/*high resolution timer for windows*/
class PrecisionTimer
{
    const LARGE_INTEGER     m_Frequency;
    LARGE_INTEGER           m_Time;
    static LARGE_INTEGER getFrq()
    {
        LARGE_INTEGER tmp;
        QueryPerformanceFrequency(&tmp);
        return tmp;
    }
public:
    PrecisionTimer()
        : m_Frequency(getFrq())
    {
    }
    LARGE_INTEGER Frequency() const { return m_Frequency; }
    LARGE_INTEGER Ticks() const
    {
        LARGE_INTEGER tmp;
        QueryPerformanceCounter(&tmp);
        return tmp;
    }
    void start()
    {
        QueryPerformanceCounter(&m_Time);
    }
    double elapsed() const
    {
        LARGE_INTEGER tmpTime;
        QueryPerformanceCounter(&tmpTime);
        tmpTime.QuadPart -= m_Time.QuadPart;
        return (tmpTime.QuadPart * 1000.0) / m_Frequency.QuadPart;
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

    FPSCounter()
        : m_IsRunning(false)
        , m_LastFrameID(0)
        , m_Valid(false)
    {

    }
    void count(VmbInt64_t id)
    {
        if (!m_IsRunning)
        {
            m_LastFrameID = id;
            m_IsRunning = true;
            m_Valid = false;
            m_Timer.start();
            return;
        }
        double      time_ms = m_Timer.elapsed();
        VmbInt64_t  delta_id = id - m_LastFrameID;
        if ((time_ms > 1000 && delta_id != 0))
        {
            m_CurrentFPS = (delta_id * 1000) / time_ms;
            m_LastFrameID = id;
            m_Valid = true;
            m_Timer.start();
        }
    }
    bool isValid() const { return m_Valid; }
    void stop()
    {
        m_CurrentFPS = 0.0;
        m_LastFrameID = 0;
        m_Valid = false;
        m_IsRunning = false;
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
        : m_Value(T())
        , m_isValid(false)
    {
    }
    ValueWithState(const ValueWithState<T>& o)
        : m_Value(o())
        , m_isValid(o.isValid())
    {}
    void        Invalidate()
    {
        m_isValid = false;
        m_Value = T();
    }
    bool        isValid() const { return m_isValid; }
    const T& operator()() const { return m_Value; }
    T& operator()() { return m_Value; }
    void        operator()(const T& o)
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
        FrameData(const tFrameInfo& info, bool FullBitDepthImage)
            : m_TransferFullBitDepthImage(FullBitDepthImage)
            , m_FrameInfo(info)
        {}
        tFrameInfo                      FrameInfo()                                         const { return m_FrameInfo; }
        bool                            TransformFullBitDepth()                             const { return m_TransferFullBitDepthImage; }
        bool                            ColorInterpolation()                                const { return m_FrameInfo.UseColorInterpolation(); }
        QSharedPointer<unsigned char>   GetFrameData()                                      const { return m_FrameInfo.DataPtr(); }
        VmbPixelFormatType              PixelFormat()                                       const { return m_FrameInfo.PixelFormat(); }
        VmbUint32_t                     Width()                                             const { return m_FrameInfo.Width(); }
        VmbUint32_t                     Height()                                            const { return m_FrameInfo.Height(); }
        VmbUint32_t                     Size()                                              const { return m_FrameInfo.Size(); }
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

    QVector<ushort>             m_uint16QVector;
    QVector<double>             m_doubleQVector;
    int                         m_width; //this width and height are used in data transfer to imgCThread
    int                         m_height;
    QString                     m_format;
    bool                        m_imageDataReady;

    QMutex                      m_imageLock;
    QWaitCondition              m_imageCalcWait;
    QWaitCondition              m_imageProcWait;

public:
    QMutex& mutex() { return m_imageLock; }
    QWaitCondition& calcWait() { return m_imageCalcWait; }
    QWaitCondition& procWait() { return m_imageProcWait; }
    QVector<ushort>& uint16Vec() { return m_uint16QVector; }
    QVector<double>& doubleVec() { return m_doubleQVector; }
    const int& width() const { return m_width; }
    const int& height() const { return m_height; }
    const QString& format() const { return m_format; }
    const int& frameCount() const { return m_FrameCount; }
    const bool& dataReady() const { return m_imageDataReady; }

    ImageProcessingThread(size_t MaxFrames = 3)
        : m_FrameQueue(MaxFrames)
        , m_Stopping(false)
        , m_DroppedFrames(0)
        , m_FrameCount(0)
        , m_LimitFrameRate(false)
        , m_width(0)
        , m_height(0)
        , m_format("")
        , m_imageDataReady(false)
    {
        m_Timer.start();
    }
    const FPSCounter& getFPSCounter() const { return m_FPSCounter; }
    ~ImageProcessingThread()
    {
        StopProcessing();
    }
    void StopProcessing()
    {
        if (!m_Stopping)
        {
            m_LastTime.Invalidate();
            m_Stopping = true;
            m_FrameCount = 0;
            m_FrameQueue.StopProcessing();
            wait();
        }
    }
    void StartProcessing()
    {
        m_Stopping = false;
        m_FrameCount = 0;
        m_FrameQueue.StartProcessing();
        start();
    }
    size_t DroppedFrames() const { return m_DroppedFrames; }

    void setThreadFrame(const tFrameInfo& FrameInfo, bool FullBitDepthImage)
    {
        FrameData tmpFrameData(FrameInfo, FullBitDepthImage);
        m_FrameQueue.Enqueue(tmpFrameData);
    }
    void LimitFrameRate(bool v) { m_LimitFrameRate = v; }
private:

protected:
    virtual void run();

private:

signals:
    void frameReadyFromThread(std::vector<ushort> vec1d, const QString& sFormat, const QString& sHeight, const QString& sWidth);
    void frameReadyFromThread(QImage image, const QString& sFormat, const QString& sHeight, const QString& sWidth);
    void frameReadyFromThread(QVector<ushort> vec1d, const QString& sFormat, const QString& sHeight, const QString& sWidth);
    void frameReadyFromThreadFullBitDepth(tFrameInfo mFullImageInfo);
    void logging(const QString& sMessage);
};


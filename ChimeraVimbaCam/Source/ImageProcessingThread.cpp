#include <stdafx.h>
#include "ImageProcessingThread.h"

//#include "VmbImageTransformHelper.hpp"

void ImageProcessingThread::run()
{
    FrameData tmpFrameData;
    while (!m_Stopping && m_FrameQueue.WaitData(tmpFrameData))
    {
        m_imageDataReady = false;
        if (m_LimitFrameRate)
        {
            double currentTime = m_Timer.elapsed();
            if (m_LastTime.isValid())
            {
                const double frameLimit = FRAMELIMIT; // limit about time for 33 fps
                const double deltaTime = currentTime - m_LastTime();
                if (frameLimit > deltaTime)
                {
                    continue;
                }
                const double newTime = currentTime + (frameLimit - deltaTime);
                m_LastTime(newTime);
            }
            else
            {
                m_LastTime(currentTime);
            }
        }

        if (NULL != tmpFrameData.GetFrameData()) 
        {
            VmbPixelFormatType outputPixelFormat = tmpFrameData.PixelFormat();
            /*my custom readout for mono8 and mono12*/
            QString sFormat = Helper::convertFormatToString(outputPixelFormat);
            //if (((tmpFrameData.Width() % 4 != 0) || (tmpFrameData.Height() % 2 != 0)))
            //{
            //    sFormat.append(" (height" + QString::number(tmpFrameData.Width()) + " or width" + QString::number(tmpFrameData.Height()) + " not supported!)");
            //    thrower(str("From FrameObserver: " + sFormat + "width is module zero for 4, height is module zero for 2"));
            //    continue;
            //}
            if (((tmpFrameData.Width() % 2 != 0) || (tmpFrameData.Height() % 2 != 0)))
            {
                sFormat.append(" (height" + QString::number(tmpFrameData.Width()) + " or width" + QString::number(tmpFrameData.Height()) + " not supported!)");
                thrower(str("From FrameObserver: " + sFormat + "width is module zero for 2, height is module zero for 2"));
                continue;
            }


            QVector<double> doubleQVector;
            if (0 == sFormat.compare("Mono12")){
                ushort* dstDataPtr = reinterpret_cast<ushort*> (tmpFrameData.GetFrameData().data());
                doubleQVector = QVector<double>(dstDataPtr, dstDataPtr + tmpFrameData.Height() * tmpFrameData.Width());
            }
            else if (0 == sFormat.compare("Mono8")){
                uint8_t* dstDataPtr = tmpFrameData.GetFrameData().data();
                doubleQVector = QVector<double>(dstDataPtr, dstDataPtr + tmpFrameData.Height() * tmpFrameData.Width());
            }
            else{
                thrower(str("From FrameObserver: " + sFormat + "is neither Mono8 nor Mono12. Only these two are supported now."));
                continue;
            }

            if (!m_Stopping)
            {
                m_imageLock.lock();
                m_FrameCount++;
                m_FPSCounter.count(m_FrameCount);

                //m_uint16QVector.swap(uint16QVector); //this does not involve any copy constructor, just switching the pointer hence super fast. After this, the uintQVector is junk and wait for destruction at the end of the loop
                m_doubleQVector.swap(doubleQVector);
                m_format = str(sFormat);
                m_height = tmpFrameData.Height();
                m_width = tmpFrameData.Width();
                m_imageDataReady = true;
                m_imageCalcWait.wakeOne();
                m_imageProcWait.wait(&m_imageLock, 2000);
                m_doubleQVector.clear();
                m_imageLock.unlock();
            }
        }
    }
}

ImageProcessingThread::ImageProcessingThread(size_t MaxFrames)
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

ImageProcessingThread::~ImageProcessingThread()
{
    StopProcessing();   
}

void ImageProcessingThread::StopProcessing()
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

void ImageProcessingThread::StartProcessing()
{
    m_Stopping = false;
    m_FrameCount = 0;
    m_FrameQueue.StartProcessing();
    start();
}

// This function is called from FrameObserver.setFrame, the arg FrameInfo contains the received frame info
void ImageProcessingThread::setThreadFrame(const tFrameInfo& FrameInfo)
{
    FrameData tmpFrameData(FrameInfo);
    // the following function put the tmpFrameData into queue which will be read in the run function
    m_FrameQueue.Enqueue(tmpFrameData);
}

void ImageProcessingThread::LimitFrameRate(bool v)
{
    m_LimitFrameRate = v;
}

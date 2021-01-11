#include "ImageProcessingThread.h"

#include "VmbImageTransformHelper.hpp"

void ImageProcessingThread::run()
{
    FrameData tmpFrameData;
    while (!m_Stopping
        && m_FrameQueue.WaitData(tmpFrameData)
        )
    {
        m_imageDataReady = false;
        if (m_LimitFrameRate)
        {
            double currentTime = m_Timer.elapsed();
            if (m_LastTime.isValid())
            {
                const double frameLimit = 33.0; // limit about time for 33 fps
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

        //QImage convertedImage( tmpFrameData.Width(), tmpFrameData.Height(), QImage::Format_RGB32); //QImage::Format_RGB32

        //if(! convertedImage.isNull())
        //{

        if (NULL != tmpFrameData.GetFrameData()) // unlikely
        {
            VmbError_t error;

            VmbPixelFormatType outputPixelFormat;
            if (tmpFrameData.ColorInterpolation())
            {
                outputPixelFormat = tmpFrameData.PixelFormat();
            }
            else
            {
                try
                {
                    outputPixelFormat = AVT::GetCompatibleMonoPixelFormatForRaw(tmpFrameData.PixelFormat());

                }
                catch (...)// lands here if GetCompatibleMonoPixelFromatForRaw throws
                {
                    outputPixelFormat = tmpFrameData.PixelFormat();
                }
            }
            //error = AVT::VmbImageTransform( convertedImage, tmpFrameData.GetFrameData().data(), tmpFrameData.Width(),  tmpFrameData.Height(), outputPixelFormat );

            /*my custom readout for mono8 and mono12*/
            QString sFormat = Helper::convertFormatToString(outputPixelFormat);

            if (((tmpFrameData.Width() % 4 != 0) || (tmpFrameData.Height() % 2 != 0)))
            {
                sFormat.append(" (height" + QString::number(tmpFrameData.Width()) + " or width" + QString::number(tmpFrameData.Height()) + " not supported!)");
                emit logging("From FrameObserver: " + sFormat + "width is module zero for 4, height is module zero for 2");
                continue;
            }


            QVector<ushort> uint16QVector;
            if (0 == sFormat.compare("Mono12"))
            {
                ushort* dstDataPtr = reinterpret_cast<ushort*> (tmpFrameData.GetFrameData().data());
                uint16QVector = QVector<ushort>(dstDataPtr, dstDataPtr + tmpFrameData.Height() * tmpFrameData.Width());
            }
            else if (0 == sFormat.compare("Mono8"))
            {
                uint8_t* dstDataPtr = tmpFrameData.GetFrameData().data();
                uint16QVector = QVector<ushort>(dstDataPtr, dstDataPtr + tmpFrameData.Height() * tmpFrameData.Width());
            }
            else
            {
                emit logging("From FrameObserver: " + sFormat + "is neither Mono8 nor Mono12. Only these two are supported now.");
                continue;
            }
            //QVector<double> double64QVector(dstDataPtr, dstDataPtr + tmpFrameData.Height() * tmpFrameData.Width());
            //std::vector<ushort> uint16Vector(dstDataPtr, dstDataPtr + tmpFrameData.Height() * tmpFrameData.Width());
            //to initialize a vector with different type, can directly use above two commented

            {
                if (!m_Stopping)
                {
                    m_imageLock.lock();
                    m_FrameCount++;
                    m_FPSCounter.count(m_FrameCount);

                    m_uint16QVector.swap(uint16QVector); //this does not involve any copy constructor, just switching the pointer hence super fast. After this, the uintQVector is junk and wait for destruction at the end of the loop
                    m_format = sFormat;
                    m_height = tmpFrameData.Height();
                    m_width = tmpFrameData.Width();
                    m_imageDataReady = true;
                    m_imageCalcWait.wakeOne();
                    m_imageProcWait.wait(&m_imageLock);

                    m_imageLock.unlock();


                    /*emit frameReadyFromThread(uint16Vector, sFormat, QString::number(tmpFrameData.Height()),
                        QString::number(tmpFrameData.Width()));*/

                    /*emit frameReadyFromThread(uint16QVector, sFormat, QString::number(tmpFrameData.Height()),
                            QString::number(tmpFrameData.Width()));*/

                    /*emit frameReadyFromThread (convertedImage, sFormat, QString::number (tmpFrameData.Height() ),
                                                QString::number (tmpFrameData.Width() ));*/

                }

            }
        }
        //}

    // send the full bit depth image if requested
        if (true == tmpFrameData.TransformFullBitDepth())
        {
            emit frameReadyFromThreadFullBitDepth(tmpFrameData.FrameInfo());
        }
    }
}
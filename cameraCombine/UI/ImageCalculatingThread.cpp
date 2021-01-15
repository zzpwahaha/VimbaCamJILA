#include "ImageCalculatingThread.h"

#include <numeric>

using AVT::VmbAPI::Frame;
using AVT::VmbAPI::FramePtr;
using AVT::VmbAPI::FeaturePtr;

ImageCalculatingThread::ImageCalculatingThread(
    const SP_DECL(FrameObserver) & pFrameObs,
    const CameraPtr& pCam,
    const QSharedPointer<QCustomPlot>& pQCP,
    const QSharedPointer<QCPColorMap>& pcmap,
    const QSharedPointer<QCPGraph>& pbot,
    const QSharedPointer<QCPGraph>& pleft)
	: QThread()
	, m_pFrameObs(pFrameObs)
    , m_pCam(pCam)
    , m_pQCP(pQCP)
	, m_pQCPColormap(pcmap)
    , m_pQCPbottomGraph(pbot)
    , m_pQCPleftGraph(pleft)
    , m_width(0)
    , m_height(0)
    , m_format("")
    , m_exposureTime(0.0)
    , m_firstStart(true)
    , m_Stopping(false)
    , m_mousePos(0,0)
{
    m_pProcessingThread = QSharedPointer<ImageProcessingThread>(SP_ACCESS(m_pFrameObs)->ImageProcessThreadPtr());
    /*get the max width and height*/
    FeaturePtr pFeat;
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("HeightMax", pFeat))
    {
        VmbInt64_t  nValue64 = 0;
        if (VmbErrorSuccess == pFeat->GetValue(nValue64))
        {
            m_heightMax = nValue64;
        }
    }
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("WidthMax", pFeat))
    {
        VmbInt64_t  nValue64 = 0;
        if (VmbErrorSuccess == pFeat->GetValue(nValue64))
        {
            m_widthMax = nValue64;
        }
    }


    updateExposureTime();
    updateXYOffset();

    m_doubleCrxX.reserve(m_widthMax);
    m_doubleCrxY.reserve(m_heightMax);

    

}

ImageCalculatingThread::~ImageCalculatingThread()
{
    //m_pQCP.clear(); do not need this, once the class object is destroyed, the shared pointer will automatically reduce one count, maybe it is special to QSharedPointer?
}

void ImageCalculatingThread::updateMousePos(QMouseEvent* event)
{
    m_mousePos = event->pos();
}

void ImageCalculatingThread::updateExposureTime()
{
    FeaturePtr pFeat;
    double  dValue = 0;
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("ExposureTimeAbs", pFeat))
    {
        auto tmp = pFeat->GetValue(dValue);
        if (VmbErrorSuccess != tmp)
        {
            emit logging("Failed to get Exposure time, error code: " + QString::number(tmp));
            return;
        }
        m_exposureTime = dValue;
    }
}

void ImageCalculatingThread::updateCameraGain()
{
    FeaturePtr pFeat;
    double  dValue = 0;
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("Gain", pFeat))
    {
        auto tmp = pFeat->GetValue(dValue);
        if (VmbErrorSuccess != tmp)
        {
            emit logging("Failed to get Gain, error code: " + QString::number(tmp));
            return;
        }
        m_cameraGain = dValue;
    }
}

void ImageCalculatingThread::updateXYOffset()
{
    FeaturePtr pFeat;
    VmbInt64_t xlower = 0;
    VmbInt64_t ylower = 0;
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("OffsetX", pFeat))
    {
        auto tmp = pFeat->GetValue(xlower);
        if (VmbErrorSuccess != tmp)
        {
            emit logging("image calculating thread get XY offset failed for X, error code: " + QString::number(tmp));
            return;
        }
    }
    if (VmbErrorSuccess == m_pCam->GetFeatureByName("OffsetY", pFeat))
    {
        auto tmp = pFeat->GetValue(ylower);
        if (VmbErrorSuccess != tmp)
        {
            emit logging("image calculating thread get XY offset failed for Y, error code: " + QString::number(tmp));
            return;
        }
    }

    m_offsetX = xlower;
    m_offsetY = ylower;
}

template <class T>
void ImageCalculatingThread::assignValue(const QVector<T>& vec1d, 
    const QString& sFormat, const int& Height, const int& Width,
    const int& offsetX, const int& offsetY)
{
    m_pQCPColormap->data()->setSize(Width, Height);
    m_pQCPColormap->data()->setRange(QCPRange(offsetX, Width + offsetX - 1), 
        QCPRange(offsetY, Height + offsetY - 1));
    /*QVector<double> tmp(vec1d.begin(),vec1d.end());*/
    for (size_t i = 0; i < Height; i++)
    {
        for (size_t j = 0; j < Width; j++)
        {
            m_pQCPColormap->data()->setCell(j, i, vec1d.at(i * Width + j));
        }
    }
    
}

void ImageCalculatingThread::StartProcessing()
{
    m_Stopping = false;
    m_firstStart = true;
    updateXYOffset();
    updateExposureTime();
    start();
}

void ImageCalculatingThread::StopProcessing()
{
    m_Stopping = true;
    m_firstStart = true;
    updateXYOffset();
    updateExposureTime();
    wait();
}

/*should only be called in the run, with the guard of mutex and everything properly initialized*/
void ImageCalculatingThread::calcCrossSectionXY()
{
    m_doubleCrxX.clear();
    m_doubleCrxY.clear();
    for (size_t i = 0; i < m_height; i++)
    {
        m_doubleCrxY.insert(i, std::accumulate(m_doubleQVector.begin() + i * m_width,
            m_doubleQVector.begin() + (i + 1) * m_width, 0));
    } 
    double i = 0.0;
    QVector<double> leftKey(m_height, m_offsetY);
    std::for_each(leftKey.begin(), leftKey.end(), [i](auto& key) mutable {key = key + i; i++; });
    m_pQCPleftGraph->setData(leftKey, m_doubleCrxY, true);

    for (size_t i = 0; i < m_width; i++)
    {
        double tmp = 0.0;
        //std::for_each_n(m_doubleQVector.begin(), m_height, [](auto& n) {tmp += n; });
        for (size_t j = 0; j < m_height; j++)
        {
            tmp += m_doubleQVector[i + j * m_width];
        }
        m_doubleCrxX.insert(i, tmp);
    }
    double ii = 0.0;
    QVector<double> bottomKey(m_width, m_offsetX);
    std::for_each(bottomKey.begin(), bottomKey.end(), [ii](auto& key) mutable {key = key + ii; ii++; });
    m_pQCPbottomGraph->setData(bottomKey, m_doubleCrxX, true);
    

}

void ImageCalculatingThread::setDefaultView()
{
    m_pQCP->axisRect(0)->axis(QCPAxis::atLeft)->setScaleRatio(
        m_pQCP->axisRect(0)->axis(QCPAxis::atBottom), 1.0);
    m_pQCP->rescaleAxes();
    for (auto& graph : { m_pQCPbottomGraph ,m_pQCPleftGraph })
    {
        graph->valueAxis()->setRangeLower(graph->valueAxis()->range().lower - graph->valueAxis()->range().size() * 0.1);
        graph->valueAxis()->setRangeUpper(graph->valueAxis()->range().upper + graph->valueAxis()->range().size() * 0.1);
    }
    m_pQCP->axisRect(2)->axis(QCPAxis::atBottom)->setRange(m_pQCPbottomGraph->keyAxis()->range() - m_pQCPbottomGraph->data()->at(0)->key);
    m_pQCP->axisRect(0)->axis(QCPAxis::atLeft)->setRange(m_pQCPleftGraph->keyAxis()->range() - m_pQCPleftGraph->data()->at(0)->key);
}

void ImageCalculatingThread::run()
{
    while (!m_Stopping)
    {
        
        m_pProcessingThread->mutex().lock();
        if (!m_pProcessingThread->dataReady())
        {
            m_pProcessingThread->calcWait().wait(&m_pProcessingThread->mutex());
        }
        else
        {
            //m_uint16QVector.swap(m_pProcessingThread->uint16Vec());
            m_doubleQVector.swap(m_pProcessingThread->doubleVec());
            m_width = m_pProcessingThread->width();
            m_height = m_pProcessingThread->height();
            m_format = m_pProcessingThread->format();
        }
        m_pProcessingThread->mutex().unlock();
        

        if (!m_doubleQVector.isEmpty())
        {
            //m_doubleQVector = QVector<double>(m_uint16QVector.begin(), m_uint16QVector.end());
            updateXYOffset();
            calcCrossSectionXY();
            assignValue(m_doubleQVector, m_format, m_height, m_width, m_offsetX, m_offsetY);
            
            if (m_firstStart)
            {
                setDefaultView();
                m_firstStart = false;
            }

            emit imageReadyForPlot();
            m_doubleQVector.clear();
        }
        
        m_pProcessingThread->procWait().wakeOne();

    }
}

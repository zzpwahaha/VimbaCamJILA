#include "ImageCalculatingThread.h"

using AVT::VmbAPI::Frame;
using AVT::VmbAPI::FramePtr;
using AVT::VmbAPI::FeaturePtr;

ImageCalculatingThread::ImageCalculatingThread(
    const SP_DECL(FrameObserver) & pFrameObs,
    const CameraPtr& pCam,
    const QSharedPointer<QCustomPlot>& pQCP,
    const QSharedPointer<QCPColorMap>& pcmap)
	: QThread()
	, m_pFrameObs(pFrameObs)
    , m_pCam(pCam)
    , m_pQCP(pQCP)
	, m_pQCPColormap(pcmap)
    , m_width(0)
    , m_height(0)
    , m_format("")
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

    updateXYOffset();

}


void ImageCalculatingThread::updateMousePos(QMouseEvent* event)
{
    m_mousePos = event->pos();
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

void ImageCalculatingThread::assignValue(const QVector<ushort>& vec1d, 
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
    start();
}

void ImageCalculatingThread::StopProcessing()
{
    m_Stopping = true;
    m_firstStart = true;
    updateXYOffset();
    wait();
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
            m_uint16QVector.swap(m_pProcessingThread->uint16Vec());
            m_width = m_pProcessingThread->width();
            m_height = m_pProcessingThread->height();
            m_format = m_pProcessingThread->format();
        }
        m_pProcessingThread->mutex().unlock();


        if (!m_uint16QVector.isEmpty())
        {
            updateXYOffset();
            assignValue(m_uint16QVector, m_format, m_height, m_width, m_offsetX, m_offsetY);
            if (m_firstStart)
            {
                m_pQCP->yAxis->setScaleRatio(m_pQCP->xAxis, 1.0);
                m_pQCP->rescaleAxes();
                m_firstStart = false;
            }

            emit imageReadyForPlot();
            m_uint16QVector.clear();
        }
        
        m_pProcessingThread->procWait().wakeOne();

    }
}

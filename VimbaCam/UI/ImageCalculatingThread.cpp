#include "ImageCalculatingThread.h"
#include <utility>
#include <numeric>
#include <array>

using AVT::VmbAPI::Frame;
using AVT::VmbAPI::FramePtr;
using AVT::VmbAPI::FeaturePtr;

ImageCalculatingThread::ImageCalculatingThread(
    const SP_DECL(FrameObserver)& pFrameObs,
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
    , m_offsetX(0)
    , m_offsetY(0)
    , m_format("")
    , m_exposureTime(0.0)
    , m_firstStart(true)
    , m_dataValid(false)
    , m_Stopping(false)
    , m_mousePos(0, 0)
    , m_doFitting(true)
    , m_doFitting2D(false)
    , m_gfitBottom(5, NULL, NULL, 0, 0, 0, 0) /*5 is greater than fit param 4, otherwise will break*/
    , m_gfitLeft(5, NULL, NULL, 0, 0, 0, 0)
    , m_gfit2D(16, 4, 4, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 100) /* emulating 4*4 matrix */
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
    m_dataValid = false;
    updateXYOffset();
    updateExposureTime();
    start();
}

void ImageCalculatingThread::StopProcessing()
{

    m_pProcessingThread->mutex().lock();
    m_Stopping = true;
    m_pProcessingThread->mutex().unlock();

    m_pProcessingThread->calcWait().wakeOne();
    m_pProcessingThread->calcWait().wakeOne();
    
    
    m_firstStart = true;
    m_dataValid = false;
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
    m_leftKey = QVector<double>(m_height, m_offsetY);
    std::for_each(m_leftKey.begin(), m_leftKey.end(), [i](auto& key) mutable {key = key + i; i++; });
    m_pQCPleftGraph->setData(m_leftKey, m_doubleCrxY, true);

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
    m_bottomKey = QVector<double>(m_width, m_offsetX);
    std::for_each(m_bottomKey.begin(), m_bottomKey.end(), [ii](auto& key) mutable {key = key + ii; ii++; });
    m_pQCPbottomGraph->setData(m_bottomKey, m_doubleCrxX, true);
    

}

void ImageCalculatingThread::setDefaultView()
{
    if (m_Stopping && !m_doFitting2D)
    {
        reinterpret_cast<QCPCurve*>(m_pQCP->axisRect(1)->plottables().at(2))->data()->clear();
        reinterpret_cast<QCPCurve*>(m_pQCP->axisRect(1)->plottables().at(1))->data()->clear();
    }

    m_pQCP->axisRect(1)->axis(QCPAxis::atLeft)->setScaleRatio(
        m_pQCP->axisRect(1)->axis(QCPAxis::atBottom), double(m_height)/ double(m_width));
    m_pQCPColormap->rescaleAxes();
    m_pQCPColormap->colorScale()->rescaleDataRange(false);
    
    m_width > m_height ? m_pQCPColormap->valueAxis()->scaleRange(double(m_width) / double(m_height)) :
        m_pQCPColormap->keyAxis()->scaleRange(double(m_height) / double(m_width));



    for (auto& graph : { m_pQCPbottomGraph ,m_pQCPleftGraph })
    {
        graph->rescaleAxes();
        graph->valueAxis()->setRangeLower(graph->valueAxis()->range().lower - graph->valueAxis()->range().size() * 0.1);
        graph->valueAxis()->setRangeUpper(graph->valueAxis()->range().upper + graph->valueAxis()->range().size() * 0.1);
    }
    //m_pQCP->axisRect(2)->axis(QCPAxis::atBottom)->setRange(m_pQCPbottomGraph->keyAxis()->range() - m_pQCPbottomGraph->data()->at(0)->key);
    //m_pQCP->axisRect(0)->axis(QCPAxis::atLeft)->setRange(m_pQCPleftGraph->keyAxis()->range() - m_pQCPleftGraph->data()->at(0)->key);
}

QVector<double> ImageCalculatingThread::rawImageDefinite()
{
    QTime time = QTime::currentTime();
    time.start();
    QVector<double> tmp;
    while (time.elapsed() < 300)
    {
        if (!m_doubleQVector.isEmpty())
        {
            m_pProcessingThread->mutex().lock();
            tmp = QVector(m_doubleQVector);
            m_pProcessingThread->mutex().unlock();
            return tmp;
        }
    }
    return tmp;
    
}

void ImageCalculatingThread::toggleDoFitting(bool dofit)
{
    m_doFitting = dofit;
    if (m_Stopping) { fit1dGaussian(); }
}

void ImageCalculatingThread::toggleDoFitting2D(bool dofit)
{
    m_pProcessingThread->mutex().lock();
    m_doFitting2D = dofit;
    if (m_Stopping) { fit2dGaussian(); }
    m_pProcessingThread->mutex().unlock();
}

void ImageCalculatingThread::fit1dGaussian()
{
    if (m_doFitting)
    {
        m_gfitBottom.set_data(m_width, m_bottomKey.begin(), m_doubleCrxX.begin());
        m_gfitLeft.set_data(m_height, m_leftKey.begin(), m_doubleCrxY.begin());

        auto [xmin_it, xmax_it] = std::minmax_element(m_doubleCrxX.begin(), m_doubleCrxX.end());
        double a0x = *xmax_it - *xmin_it;
        double b0x = m_bottomKey.at(xmax_it - m_doubleCrxX.begin());/*although this return a const reference, can nontheless force a copy ctor to get a copy of the returned value*/
        double c0x = 0.5 * m_width;
        double d0x = *xmin_it;

        auto [ymin_it, ymax_it] = std::minmax_element(m_doubleCrxY.begin(), m_doubleCrxY.end());
        double a0y = *ymax_it - *ymin_it;
        double b0y = m_leftKey.at(ymax_it - m_doubleCrxY.begin());/*although this return a const reference, can nontheless force a copy ctor to get a copy of the returned value*/
        double c0y = 0.5 * m_height;
        double d0y = *ymin_it;
        m_gfitBottom.set_initialP(a0x, b0x, c0x, d0x);
        m_gfitLeft.set_initialP(a0y, b0y, c0y, d0y);

        QVector<double> fittedx, fittedy;
        QFuture<void> resultx = QtConcurrent::run([this, &fittedx]() {
            m_gfitBottom.solve_system();
            fittedx = std::move(m_gfitBottom.calcFittedGaussian());
            m_pQCP->graph(2)->setData(m_bottomKey, fittedx, true);
            QVector<double> fitParax = m_gfitBottom.fittedPara();
            QVector<double> confi95x = m_gfitBottom.confidence95Interval();
            m_pQCP->axisRect(2)->axis(QCPAxis::atBottom)->setLabel(
                QString::fromWCharArray(L"\u03bc") + QString(": %1 +/- %2, ").
                arg(fitParax.at(1) - m_offsetX, 0, 'f', 2).arg(confi95x.at(1), 0, 'f', 2) +
                QString::fromWCharArray(L"\u03c3") + QString(": %3 +/- %4").
                arg(fitParax.at(2), 0, 'f', 2).arg(confi95x.at(2), 0, 'f', 2));
            });
        QFuture<void> resulty = QtConcurrent::run([this, &fittedy]() {
            m_gfitLeft.solve_system();
            fittedy = std::move(m_gfitLeft.calcFittedGaussian());
            m_pQCP->graph(3)->setData(m_leftKey, fittedy, true);
            QVector<double> fitParay = m_gfitLeft.fittedPara();
            QVector<double> confi95y = m_gfitLeft.confidence95Interval();
            m_pQCP->axisRect(0)->axis(QCPAxis::atLeft)->setLabel(
                QString::fromWCharArray(L"\u03bc") + QString(": %1 +/- %2, ").
                arg(fitParay.at(1) - m_offsetY, 0, 'f', 2).arg(confi95y.at(1), 0, 'f', 2) +
                QString::fromWCharArray(L"\u03c3") + QString(": %3 +/- %4").
                arg(fitParay.at(2), 0, 'f', 2).arg(confi95y.at(2), 0, 'f', 2));
            });

        resultx.waitForFinished();
        resulty.waitForFinished();
    }
    

}


void ImageCalculatingThread::fit2dGaussian()
{
    if (m_doFitting2D)
    {
        m_gfit2D.set_data(m_width * m_height, m_width, m_height,
            m_bottomKey.begin(), m_leftKey.begin(), m_doubleQVector.begin());

        auto [zmin_it, zmax_it] = std::minmax_element(m_doubleQVector.begin(), m_doubleQVector.end());
        double A0 = *zmax_it - *zmin_it;
        double x00 = m_bottomKey.at((zmax_it - m_doubleQVector.begin()) % m_width);/*although this return a const reference, can nontheless force a copy ctor to get a copy of the returned value*/
        double y00 = m_leftKey.at((zmax_it - m_doubleQVector.begin()) / m_width);
        double a0 = 1. / (0.25 * m_width * m_width);
        double b0 = 0.1 / (0.5 * m_width * m_height);
        double c0 = 1. / (0.25 * m_height * m_height);
        double D0 = *zmin_it;

        m_gfit2D.set_initialP(A0, x00, y00, a0, b0, c0, D0);

        /*fiting*/
        try {
            m_gfit2D.solve_system();
        }
        catch (const std::exception& e) {
            emit logging("Exception from the thread: " + QString(e.what()));
            return;
        }

        //QVector<double> fitted2D;
        //fitted2D = std::move(m_gfit2D.calcFittedGaussian());
        QVector<double> fitParaz = m_gfit2D.fittedPara();
        QVector<double> confi95z = m_gfit2D.confidence95Interval();

        const double a = fitParaz[3], b = fitParaz[4], c = fitParaz[5], Delta = sqrt(4 * b * b + (a - c) * (a - c));
        const double sigMajor = 1 / sqrt(a + c - Delta);
        const double sigMinor = 1 / sqrt(a + c + Delta);

        auto [errMajor, errMinor] = m_gfit2D.MajorMinor95();

        if (m_gfit2D.getInfo() != 1 || fitParaz[1] < m_offsetX || fitParaz[2] < m_offsetY)
        {
            emit logging(QString("muX: %1 +/- %2, sigmaX: %3 +/- %4").
                arg(fitParaz.at(1) - m_offsetX, 0, 'f', 2).arg(confi95z.at(1), 0, 'f', 2).
                arg(sigMajor, 0, 'f', 2).arg(errMajor, 0, 'f', 2) +
                ", " +
                QString("muY: %1 +/- %2, sigmaY: %3 +/- %4").
                arg(fitParaz.at(2) - m_offsetY, 0, 'f', 2).arg(confi95z.at(2), 0, 'f', 2).
                arg(sigMinor, 0, 'f', 2).arg(errMinor, 0, 'f', 2));
            return;
        }
        m_pQCP->axisRect(1)->axis(QCPAxis::atTop)->setLabel(
            QString::fromWCharArray(L"\u03bc") + QString("XY: (%1 +/- %2, %3 +/- %4)").
            arg(fitParaz.at(1) - m_offsetX, 5, 'f', 2).arg(confi95z.at(1), 5, 'f', 2).
            arg(fitParaz.at(2) - m_offsetY, 5, 'f', 2).arg(confi95z.at(2), 5, 'f', 2) + ", " +
            QString::fromWCharArray(L"\u03c3") + QString("MajMin: (%1 +/- %2, %3 +/- %4)").
            arg(sigMajor, 5, 'f', 2).arg(errMajor, 5, 'f', 2).
            arg(sigMinor, 5, 'f', 2).arg(errMinor, 5, 'f', 2)
        );

        /*parametric plot*/
        const int pointCount = 100;
        QVector<QCPCurveData> parametric(pointCount);
        for (size_t i = 0; i < pointCount; i++)
        {
            const double phi = i / (double)(pointCount - 1) * 2 * M_PI;
            /*0.135 of the peak, i.e. exp(-2)*/
            const double r = sqrt(2.) / sqrt(a * cos(phi) * cos(phi) + b * sin(2 * phi) + c * sin(phi) * sin(phi));
            parametric[i] = QCPCurveData(i, r * cos(phi) + fitParaz[1], r * sin(phi) + fitParaz[2]);
        }
        reinterpret_cast<QCPCurve*>(m_pQCP->axisRect(1)->plottables().at(2))->data()->set(parametric, true);

        /*cross hair*/
        std::array<double, 2> kMajor = { a - c - Delta, 2. * b };
        std::array<double, 2> kMinor = { a - c + Delta, 2 * b };
        {
            double norm = std::inner_product(kMajor.begin(), kMajor.end(), kMajor.begin(), 0.0);
            std::for_each(kMajor.begin(), kMajor.end(), [norm](auto& tmp) {tmp /= sqrt(norm); });
            norm = std::inner_product(kMinor.begin(), kMinor.end(), kMinor.begin(), 0.0);
            std::for_each(kMinor.begin(), kMinor.end(), [norm](auto& tmp) {tmp /= sqrt(norm); });
        }
        QVector<QCPCurveData> hair(6);
        {
            double centerx = fitParaz[1];
            double centery = fitParaz[2];
            hair[0] = (QCPCurveData(0, -m_width * kMajor[0] + centerx, -m_height * kMajor[1] + centery));
            hair[1] = (QCPCurveData(1, m_width * kMajor[0] + centerx, m_height * kMajor[1] + centery));
            hair[2] = (QCPCurveData(2, qQNaN(), qQNaN()));
            hair[3] = (QCPCurveData(3, -m_width * kMinor[0] + centerx, -m_height * kMinor[1] + centery));
            hair[4] = (QCPCurveData(4, m_width * kMinor[0] + centerx, m_height * kMinor[1] + centery));
            hair[5] = (QCPCurveData(5, qQNaN(), qQNaN()));
        }
        reinterpret_cast<QCPCurve*>(m_pQCP->axisRect(1)->plottables().at(1))->data()->set(hair, true);



        /*double check to make sure kill the label when runing, that is due to the slowness
        of the calc such that when the m_doFitting is set to false, the current fit is not yet
        finished but already in processing, when it is done, it still set the label*/
        if(!m_Stopping && !m_doFitting2D)
        {
            m_pQCP->axisRect(1)->axis(QCPAxis::atTop)->setLabel("");
        }



    }

}



void ImageCalculatingThread::run()
{
    while (!m_Stopping)
    {
        
        m_pProcessingThread->mutex().lock();
        //m_doubleQVector.clear();
        {
            m_pProcessingThread->calcWait().wait(&m_pProcessingThread->mutex(),2000);

            if (!m_Stopping)//protect it from overwrite when stop
            { 
                m_doubleQVector.swap(m_pProcessingThread->doubleVec());
                m_width = m_pProcessingThread->width();
                m_height = m_pProcessingThread->height();
                m_format = m_pProcessingThread->format();
                m_dataValid = true;
            }
        }
        m_pProcessingThread->mutex().unlock();
        

        if (m_dataValid && !m_Stopping && !m_doubleQVector.isEmpty())
        {
            //m_doubleQVector = QVector<double>(m_uint16QVector.begin(), m_uint16QVector.end());
            updateXYOffset();
            calcCrossSectionXY();
            assignValue(m_doubleQVector, m_format, m_height, m_width, m_offsetX, m_offsetY);
            fit1dGaussian();
            fit2dGaussian();
            if (m_firstStart)
            {
                setDefaultView();
                emit currentFormat(m_format);
                m_firstStart = false;
            }
            m_dataValid = false;
            emit imageReadyForPlot();
            
        }
        
        m_pProcessingThread->procWait().wakeOne();

    }
}

#pragma once
#include <QThread>
#include <qtconcurrentrun.h>
#include <VimbaCPP/Include/VimbaSystem.h>
#include "FrameObserver.h"

#include "ExternLib/qcustomplot/qcustomplot.h"
#include "Gaussian1DFit.h"
#include <utility>

class ImageCalculatingThread :
    public QThread
{
    Q_OBJECT
private:
    SP_DECL(FrameObserver)                    m_pFrameObs;
    CameraPtr                                 m_pCam;
    QSharedPointer<ImageProcessingThread>     m_pProcessingThread;
    QSharedPointer<QCustomPlot>               m_pQCP;
    QSharedPointer<QCPColorMap>               m_pQCPColormap;
    QSharedPointer<QCPGraph>                  m_pQCPbottomGraph;
    QSharedPointer<QCPGraph>                  m_pQCPleftGraph;
    QVector<ushort>                           m_uint16QVector;
    QVector<double>                           m_doubleQVector;
    QVector<double>                           m_doubleCrxX;
    QVector<double>                           m_doubleCrxY;
    QVector<double>                           m_bottomKey;
    QVector<double>                           m_leftKey;
    int                                       m_height;
    int                                       m_width;
    int                                       m_heightMax;
    int                                       m_widthMax;
    int                                       m_offsetX;
    int                                       m_offsetY;
    QString                                   m_format;
    double                                    m_exposureTime;
    double                                    m_cameraGain;

    bool                                      m_firstStart;
    bool                                      m_dataValid;

    bool                                      m_Stopping;

    bool                                      m_doFitting;

    QPoint                                    m_mousePos;

    Gaussian1DFit                             m_gfitBottom;
    Gaussian1DFit                             m_gfitLeft;
public:
    ImageCalculatingThread(const SP_DECL(FrameObserver)& ,
        const CameraPtr&,
        const QSharedPointer<QCustomPlot>&,
        const QSharedPointer<QCPColorMap>&,
        const QSharedPointer<QCPGraph>& pbot,
        const QSharedPointer<QCPGraph>& pleft);
    ~ImageCalculatingThread();
    void StartProcessing();
    void StopProcessing();
    void updateXYOffset();
    void updateExposureTime();
    void updateCameraGain();
    template <class T>
    void assignValue(const QVector<T>& vec1d, const QString& sFormat, const int& Height, const int& Width, const int& offsetX, const int& offsetY);

private:
    void calcCrossSectionXY();
    void fit1dGaussian();

public:
    virtual void run() override;

    void setDefaultView();


    const QPoint& mousePos() const { return m_mousePos; }
    std::pair<int, int> maxWidthHeight() const { return std::pair(m_widthMax, m_heightMax); }
    std::pair<int, int> WidthHeight() const { return std::pair(m_width, m_height); }
    std::pair<int, int> offsetXY() const { return std::pair(m_offsetX, m_offsetY); }
    const QString& format() const { return m_format; }
    const double exposureTime() const { return m_exposureTime; }
    const double cameraGain() const { return m_cameraGain; }
    QVector<double> rawImageDefinite();  /*used in save image*/
    QMutex& mutex() const { return m_pProcessingThread->mutex(); }

signals:
    void imageReadyForPlot();
    void logging(const QString& sMessage);
    void currentFormat(QString format);

public slots:
    
    void updateMousePos(QMouseEvent* event);
    void toggleDoFitting(bool dofit);
};


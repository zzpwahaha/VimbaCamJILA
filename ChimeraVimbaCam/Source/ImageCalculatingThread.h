#pragma once
#include <QThread>
#include <qtconcurrentrun.h>
#include <VimbaCPP/Include/VimbaSystem.h>
#include "FrameObserver.h"
#include "MakoCameraCore.h"
#include "../3rd_Party/qcustomplot/qcustomplot.h"
#include "GaussianFit.h"
#include "FrameAverager.h"
#include <deque>
#include <utility>

class ImageProcessingThread;

class ImageCalculatingThread :
    public QThread
{
    Q_OBJECT
public:
    ImageCalculatingThread(const SP_DECL(FrameObserver)&, MakoCameraCore& core,
        bool SAFEMODE, QCustomPlot* plot, QCPColorMap* cmap, QCPGraph* pbot, QCPGraph* pleft);
    ~ImageCalculatingThread();
    void StartProcessing();
    void StopProcessing();
    void updateXYOffset();
    void updateExposureTime();
    void updateCameraGain();
    template <class T>
    void assignValue(const QVector<T>& vec1d, std::string sFormat, 
        unsigned Height, unsigned Width, unsigned offsetX, unsigned offsetY);

    void setExpActive(bool active);
    void setMOTCalcActive(bool active);
    void experimentStarted();
    void experimentFinished();

    void setBackground();

private:
    void calcCrossSectionXY();
    void updateDataMaxMin();
    void fit1dGaussian();
    void fit2dGaussian();

public:
    virtual void run() override;

    void setDefaultView();


    QPoint& mousePos() { return m_mousePos; }
    std::pair<int, int> dataMaxMin() { return std::make_pair(m_datamax, m_datamin); }
    std::pair<int, int> maxWidthHeight() { return std::pair(m_widthMax, m_heightMax); }
    std::pair<int, int> WidthHeight() { return std::pair(m_width, m_height); }
    std::pair<int, int> offsetXY() { return std::pair(m_offsetX, m_offsetY); }
    std::string format() { return m_format; }
    double exposureTime() { return m_exposureTime; }
    double cameraGain() { return m_cameraGain; }
    QVector<double> rawImageDefinite();  /*used in save image*/
    QMutex& mutex() const { return m_pProcessingThread->mutex(); }
    FrameAverager& averager() { return m_avger; }
    bool doBkgSubtraction() { return m_doBkgSubtraction; }

signals:
    void imageReadyForPlot();
    void imageReadyForExp(const QVector<double>&, int w, int h);
    void currentFormat(QString format);
    void backgroundStatus(bool valid);

public slots:
    
    void updateMousePos(QMouseEvent* event);
    void toggleDoFitting(bool dofit);
    void toggleDoFitting2D(bool dofit);
    void toggleBkgSubtraction(bool doSub);


private:
    SP_DECL(FrameObserver)                    m_pFrameObs;
    MakoCameraCore&                           core;
    CameraPtr                                 m_pCam;
    QSharedPointer<ImageProcessingThread>     m_pProcessingThread;
    bool                                      expActive;
    bool                                      expRunning;

    QCustomPlot*                              m_pQCP;
    QCPColorMap*                              m_pQCPColormap;
    QCPGraph*                                 m_pQCPbottomGraph;
    QCPGraph*                                 m_pQCPleftGraph;
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
    int                                       m_datamax;
    int                                       m_datamin;
    std::string                               m_format;
    double                                    m_exposureTime;
    double                                    m_cameraGain;

    bool                                      m_firstStart;
    bool                                      m_dataValid;

    bool                                      m_Stopping;

    std::atomic<bool>                         m_doFitting;
    std::atomic<bool>                         m_doFitting2D;

    QPoint                                    m_mousePos;

    std::atomic<bool>                         m_doBkgSubtraction;
    bool                                      m_bkgValid;
    QVector<double>                           m_background;
    FrameAverager                             m_avger;
    Gaussian1DFit                             m_gfitBottom;
    Gaussian1DFit                             m_gfitLeft;
    Gaussian2DFit                             m_gfit2D;
};

